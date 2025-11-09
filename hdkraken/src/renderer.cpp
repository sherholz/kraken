//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "renderer.h"

#include "config.h"
#include "mesh.h"
#include "renderBuffer.h"

#include "pxr/imaging/hd/perfLog.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/threadLimits.h"

#include "pxr/base/tf/hash.h"

#include <embree4/rtcore_common.h>
#include <embree4/rtcore_geometry.h>
#include <embree4/rtcore_scene.h>

#include <algorithm>
#include <chrono>
#include <limits>
#include <stdint.h>
#include <thread>
#include <iostream>

// -------------------------------------------------------------------------
// Old TBB workaround - we plan to remove this once OpenUSD adopts
// oneTBB as a min spec. This applies the "Work" thread limit to the
// render thread if "Work" is using old TBB, but won't affect other "Work"
// implementations.  Note that it may affect Embree TBB usage as well.
//
// Note: The TBB version macro is located in different headers in legacy TBB.
// -------------------------------------------------------------------------
#if __has_include(<tbb/tbb_stddef.h>)
#include <tbb/tbb_stddef.h>
#elif __has_include(<tbb/version.h>)
#include <tbb/version.h>
#endif

#ifndef TBB_INTERFACE_VERSION_MAJOR
#error "TBB version macro TBB_INTERFACE_VERSION_MAJOR not found"
#endif

#if TBB_INTERFACE_VERSION_MAJOR < 12

#include <optional>
#include <tbb/task_scheduler_init.h>

namespace {

PXR_NAMESPACE_USING_DIRECTIVE

// Make the calling context respect PXR_WORK_THREAD_LIMIT, if run from a thread
// other than the main thread (ie, the renderThread)
class _ScopedThreadScheduler {
public:
    _ScopedThreadScheduler() {
        auto limit = WorkGetConcurrencyLimitSetting();
        if (limit != 0) {
            _tbbTaskSchedInit.emplace(limit);
        }
    }

    std::optional<tbb::task_scheduler_init> _tbbTaskSchedInit;
};

} // anonymous namespace

#endif  // TBB_INTERFACE_VERSION_MAJOR < 12


namespace {

PXR_NAMESPACE_USING_DIRECTIVE

// -------------------------------------------------------------------------
// Constants
// -------------------------------------------------------------------------

template <typename T>
constexpr T _pi = static_cast<T>(M_PI);

constexpr float _rayHitContinueBias = 0.001f;

constexpr float _minLuminanceCutoff = 1e-9f;

constexpr GfVec3f _invalidColor = GfVec3f(-std::numeric_limits<float>::infinity());

// -------------------------------------------------------------------------
// General Ray Utilities
// -------------------------------------------------------------------------

// Dot product, but set to 0 if less than 0 - ie, 0 for backward-facing rays
inline float
_DotZeroClip(GfVec3f const& a, GfVec3f const& b)
{
    return std::max(0.0f, GfDot(a, b));
}

}  // anonymous namespace

PXR_NAMESPACE_OPEN_SCOPE

HdKrakenRenderer::HdKrakenRenderer()
    : _aovBindings()
    , _aovNames()
    , _aovBindingsNeedValidation(false)
    , _aovBindingsValid(false)
    , _width(0)
    , _height(0)
    , _viewMatrix(1.0f) // == identity
    , _projMatrix(1.0f) // == identity
    , _inverseViewMatrix(1.0f) // == identity
    , _inverseProjMatrix(1.0f) // == identity
    //, _scene(nullptr)
    , _samplesToConvergence(1)
    //, _ambientOcclusionSamples(0)
    //, _enableSceneColors(false)
    //, _enableLighting(false)
    , _completedSamples(0)
{
}

HdKrakenRenderer::~HdKrakenRenderer() = default;

//void
//HdKrakenRenderer::SetScene(RTCScene scene)
//{
//    _scene = scene;
//}

void
HdKrakenRenderer::SetSamplesToConvergence(int samplesToConvergence)
{
    _samplesToConvergence = samplesToConvergence;
}

void
HdKrakenRenderer::SetDataWindow(const GfRect2i &dataWindow)
{
    std::cout << "HdKrakenRenderer::SetDataWindow" << std::endl;
    _dataWindow = dataWindow;

    // Here for clients that do not use camera framing but the
    // viewport.
    //
    // Re-validate the attachments, since attachment viewport and
    // render viewport need to match.
    _aovBindingsNeedValidation = true;
}

void
HdKrakenRenderer::SetCamera(const GfMatrix4d& viewMatrix,
                            const GfMatrix4d& projMatrix)
{
    _viewMatrix = viewMatrix;
    _projMatrix = projMatrix;
    _inverseViewMatrix = viewMatrix.GetInverse();
    _inverseProjMatrix = projMatrix.GetInverse();
}

void
HdKrakenRenderer::SetAovBindings(
    HdRenderPassAovBindingVector const &aovBindings)
{
    _aovBindings = aovBindings;
    _aovNames.resize(_aovBindings.size());
    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        _aovNames[i] = HdParsedAovToken(_aovBindings[i].aovName);
    }

    // Re-validate the attachments.
    _aovBindingsNeedValidation = true;
}

bool
HdKrakenRenderer::_ValidateAovBindings()
{
    if (!_aovBindingsNeedValidation) {
        return _aovBindingsValid;
    }

    _aovBindingsNeedValidation = false;
    _aovBindingsValid = true;

    for (size_t i = 0; i < _aovBindings.size(); ++i) {

        // By the time the attachment gets here, there should be a bound
        // output buffer.
        if (_aovBindings[i].renderBuffer == nullptr) {
            TF_WARN("Aov '%s' doesn't have any renderbuffer bound",
                    _aovNames[i].name.GetText());
            _aovBindingsValid = false;
            continue;
        }

        if (_aovNames[i].name != HdAovTokens->color &&
            _aovNames[i].name != HdAovTokens->cameraDepth &&
            _aovNames[i].name != HdAovTokens->depth &&
            _aovNames[i].name != HdAovTokens->primId &&
            _aovNames[i].name != HdAovTokens->instanceId &&
            _aovNames[i].name != HdAovTokens->elementId &&
            _aovNames[i].name != HdAovTokens->Neye &&
            _aovNames[i].name != HdAovTokens->normal &&
            !_aovNames[i].isPrimvar) {
            TF_WARN("Unsupported attachment with Aov '%s' won't be rendered to",
                    _aovNames[i].name.GetText());
        }

        HdFormat format = _aovBindings[i].renderBuffer->GetFormat();

        // depth is only supported for float32 attachments
        if ((_aovNames[i].name == HdAovTokens->cameraDepth ||
             _aovNames[i].name == HdAovTokens->depth) &&
            format != HdFormatFloat32) {
            TF_WARN("Aov '%s' has unsupported format '%s'",
                    _aovNames[i].name.GetText(),
                    TfEnum::GetName(format).c_str());
            _aovBindingsValid = false;
        }

        // ids are only supported for int32 attachments
        if ((_aovNames[i].name == HdAovTokens->primId ||
             _aovNames[i].name == HdAovTokens->instanceId ||
             _aovNames[i].name == HdAovTokens->elementId) &&
            format != HdFormatInt32) {
            TF_WARN("Aov '%s' has unsupported format '%s'",
                    _aovNames[i].name.GetText(),
                    TfEnum::GetName(format).c_str());
            _aovBindingsValid = false;
        }

        // Normal is only supported for vec3 attachments of float.
        if ((_aovNames[i].name == HdAovTokens->Neye ||
             _aovNames[i].name == HdAovTokens->normal) &&
            format != HdFormatFloat32Vec3) {
            TF_WARN("Aov '%s' has unsupported format '%s'",
                    _aovNames[i].name.GetText(),
                    TfEnum::GetName(format).c_str());
            _aovBindingsValid = false;
        }

        // Primvars support vec3 output (though some channels may not be used).
        if (_aovNames[i].isPrimvar &&
            format != HdFormatFloat32Vec3) {
            TF_WARN("Aov 'primvars:%s' has unsupported format '%s'",
                    _aovNames[i].name.GetText(),
                    TfEnum::GetName(format).c_str());
            _aovBindingsValid = false;
        }

        // color is only supported for vec3/vec4 attachments of float,
        // unorm, or snorm.
        if (_aovNames[i].name == HdAovTokens->color) {
            switch(format) {
                case HdFormatUNorm8Vec4:
                case HdFormatUNorm8Vec3:
                case HdFormatSNorm8Vec4:
                case HdFormatSNorm8Vec3:
                case HdFormatFloat32Vec4:
                case HdFormatFloat32Vec3:
                    break;
                default:
                    TF_WARN("Aov '%s' has unsupported format '%s'",
                        _aovNames[i].name.GetText(),
                        TfEnum::GetName(format).c_str());
                    _aovBindingsValid = false;
                    break;
            }
        }

        // make sure the clear value is reasonable for the format of the
        // attached buffer.
        if (!_aovBindings[i].clearValue.IsEmpty()) {
            HdTupleType clearType =
                HdGetValueTupleType(_aovBindings[i].clearValue);

            // array-valued clear types aren't supported.
            if (clearType.count != 1) {
                TF_WARN("Aov '%s' clear value type '%s' is an array",
                        _aovNames[i].name.GetText(),
                        _aovBindings[i].clearValue.GetTypeName().c_str());
                _aovBindingsValid = false;
            }

            // color only supports float/double vec3/4
            if (_aovNames[i].name == HdAovTokens->color &&
                clearType.type != HdTypeFloatVec3 &&
                clearType.type != HdTypeFloatVec4 &&
                clearType.type != HdTypeDoubleVec3 &&
                clearType.type != HdTypeDoubleVec4) {
                TF_WARN("Aov '%s' clear value type '%s' isn't compatible",
                        _aovNames[i].name.GetText(),
                        _aovBindings[i].clearValue.GetTypeName().c_str());
                _aovBindingsValid = false;
            }

            // only clear float formats with float, int with int, float3 with
            // float3.
            if ((format == HdFormatFloat32 && clearType.type != HdTypeFloat) ||
                (format == HdFormatInt32 && clearType.type != HdTypeInt32) ||
                (format == HdFormatFloat32Vec3 &&
                 clearType.type != HdTypeFloatVec3)) {
                TF_WARN("Aov '%s' clear value type '%s' isn't compatible with"
                        " format %s",
                        _aovNames[i].name.GetText(),
                        _aovBindings[i].clearValue.GetTypeName().c_str(),
                        TfEnum::GetName(format).c_str());
                _aovBindingsValid = false;
            }
        }
    }

    return _aovBindingsValid;
}

/* static */
GfVec4f
HdKrakenRenderer::_GetClearColor(VtValue const& clearValue)
{
    HdTupleType type = HdGetValueTupleType(clearValue);
    if (type.count != 1) {
        return GfVec4f(0.0f, 0.0f, 0.0f, 1.0f);
    }

    switch(type.type) {
        case HdTypeFloatVec3:
        {
            GfVec3f f =
                *(static_cast<const GfVec3f*>(HdGetValueData(clearValue)));
            return GfVec4f(f[0], f[1], f[2], 1.0f);
        }
        case HdTypeFloatVec4:
        {
            GfVec4f f =
                *(static_cast<const GfVec4f*>(HdGetValueData(clearValue)));
            return f;
        }
        case HdTypeDoubleVec3:
        {
            GfVec3d f =
                *(static_cast<const GfVec3d*>(HdGetValueData(clearValue)));
            return GfVec4f(f[0], f[1], f[2], 1.0f);
        }
        case HdTypeDoubleVec4:
        {
            GfVec4d f =
                *(static_cast<const GfVec4d*>(HdGetValueData(clearValue)));
            return GfVec4f(f);
        }
        default:
            return GfVec4f(0.0f, 0.0f, 0.0f, 1.0f);
    }
}

void
HdKrakenRenderer::Clear()
{
    std::cout << "HdKrakenRenderer::Clear" << std::endl;
    if (!_ValidateAovBindings()) {
        return;
    }

    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        if (_aovBindings[i].clearValue.IsEmpty()) {
            continue;
        }

        HdKrakenRenderBuffer *rb = 
            static_cast<HdKrakenRenderBuffer*>(_aovBindings[i].renderBuffer);

        rb->Map();
        if (_aovNames[i].name == HdAovTokens->color) {
            GfVec4f clearColor = _GetClearColor(_aovBindings[i].clearValue);
            rb->Clear(4, clearColor.data());
        } else if (rb->GetFormat() == HdFormatInt32) {
            int32_t clearValue = _aovBindings[i].clearValue.Get<int32_t>();
            rb->Clear(1, &clearValue);
        } else if (rb->GetFormat() == HdFormatFloat32) {
            float clearValue = _aovBindings[i].clearValue.Get<float>();
            rb->Clear(1, &clearValue);
        } else if (rb->GetFormat() == HdFormatFloat32Vec3) {
            GfVec3f clearValue = _aovBindings[i].clearValue.Get<GfVec3f>();
            rb->Clear(3, clearValue.data());
        } // else, _ValidateAovBindings would have already warned.

        rb->Unmap();
        rb->SetConverged(false);
    }
}

void
HdKrakenRenderer::MarkAovBuffersUnconverged()
{
    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        HdKrakenRenderBuffer *rb =
            static_cast<HdKrakenRenderBuffer*>(_aovBindings[i].renderBuffer);
        rb->SetConverged(false);
    }
}

int
HdKrakenRenderer::GetCompletedSamples() const
{
    return _completedSamples.load();
}

static
bool
_IsContained(const GfRect2i &rect, int width, int height)
{
    return
        rect.GetMinX() >= 0 && rect.GetMaxX() < width &&
        rect.GetMinY() >= 0 && rect.GetMaxY() < height;
}

void
HdKrakenRenderer::_PreRenderSetup()
{
    _completedSamples.store(0);

    // Commit any pending changes to the scene.
    //rtcCommitScene(_scene);

    if (!_ValidateAovBindings()) {
        // We aren't going to render anything. Just mark all AOVs as converged
        // so that we will stop rendering.
        for (size_t i = 0; i < _aovBindings.size(); ++i) {
            HdKrakenRenderBuffer *rb = static_cast<HdKrakenRenderBuffer*>(
                _aovBindings[i].renderBuffer);
            rb->SetConverged(true);
        }
        // XXX:validation
        TF_WARN("Could not validate Aovs. Render will not complete");
        return;
    }

    _width  = 0;
    _height = 0;

    // Map all of the attachments.
    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        //
        // XXX
        //
        // A scene delegate might specify the path to a
        // render buffer instead of a pointer to the
        // render buffer.
        //
        static_cast<HdKrakenRenderBuffer*>(
            _aovBindings[i].renderBuffer)->Map();

        if (i == 0) {
            _width  = _aovBindings[i].renderBuffer->GetWidth();
            _height = _aovBindings[i].renderBuffer->GetHeight();
        } else {
            if ( _width  != _aovBindings[i].renderBuffer->GetWidth() ||
                 _height != _aovBindings[i].renderBuffer->GetHeight()) {
                TF_CODING_ERROR(
                    "Embree render buffers have inconsistent sizes");
            }
        }
    }

    if (_width > 0 || _height > 0) {
        if (!_IsContained(_dataWindow, _width, _height)) {
            TF_CODING_ERROR(
                "dataWindow is larger than render buffer");
        }
    }
}

void
HdKrakenRenderer::Render(HdRenderThread *renderThread)
{
    std::cout << "HdKrakenRenderer::Render" << std::endl;
#if TBB_INTERFACE_VERSION_MAJOR < 12
    _ScopedThreadScheduler scheduler;
#endif

    _PreRenderSetup();

    // Render the image. Each pass through the loop adds a sample per pixel
    // (with jittered ray direction); the longer the loop runs, the less noisy
    // the image becomes. We add a cancellation point once per loop.
    //
    // We consider the image converged after N samples, which is a convenient
    // and simple heuristic.
    std::cout << "_samplesToConvergence = " << _samplesToConvergence << std::endl;
    for (int i = 0; i < _samplesToConvergence; ++i) {
        // Pause point.
        while (renderThread->IsPauseRequested()) {
            if (renderThread->IsStopRequested()) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        // Cancellation point.
        if (renderThread->IsStopRequested()) {
            break;
        }
    
        std::cout << "No Stop Request = " << std::endl;

        const unsigned int tileSize = HdKrakenConfig::GetInstance().tileSize;
        const unsigned int numTilesX =
            (_dataWindow.GetWidth() + tileSize-1) / tileSize;
        const unsigned int numTilesY =
            (_dataWindow.GetHeight() + tileSize-1) / tileSize;

        std::cout << "_dataWindows: width = " << _dataWindow.GetWidth() << " height = "<< _dataWindow.GetHeight() << std::endl; 
        std::cout << "tileSize = " << tileSize << " numTilesX = " << numTilesX << "numTilesY = " << numTilesY << std::endl;


        // Render by scheduling square tiles of the sample buffer in a parallel
        // for loop.
        // Always pass the renderThread to _RenderTiles to allow the first frame
        // to be interrupted.
        WorkParallelForN(numTilesX*numTilesY,
            std::bind(&HdKrakenRenderer::_RenderTiles, this,
                renderThread, i, std::placeholders::_1, std::placeholders::_2));

        // After the first pass, mark the single-sampled attachments as
        // converged and unmap them. If there are no multisampled attachments,
        // we are done.
        if (i == 0) {
            bool moreWork = false;
            for (size_t i = 0; i < _aovBindings.size(); ++i) {
                HdKrakenRenderBuffer *rb = static_cast<HdKrakenRenderBuffer*>(
                    _aovBindings[i].renderBuffer);
                if (rb->IsMultiSampled()) {
                    moreWork = true;
                }
            }
            if (!moreWork) {
                _completedSamples.store(i+1);
                break;
            }
        }

        // Track the number of completed samples for external consumption.
        _completedSamples.store(i+1);

        // Cancellation point.
        if (renderThread->IsStopRequested()) {
            break;
        }
    }

    // Mark the multisampled attachments as converged and unmap all buffers.
    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        HdKrakenRenderBuffer *rb = static_cast<HdKrakenRenderBuffer*>(
            _aovBindings[i].renderBuffer);
        rb->Unmap();
        rb->SetConverged(true);
    }
}

void
HdKrakenRenderer::_RenderTiles(HdRenderThread *renderThread, int sampleNum,
                               size_t tileStart, size_t tileEnd)
{
    std::cout << "HdKrakenRenderer::_RenderTiles" << std::endl; 
    const unsigned int minX = _dataWindow.GetMinX();
    unsigned int minY = _dataWindow.GetMinY();
    const unsigned int maxX = _dataWindow.GetMaxX() + 1;
    unsigned int maxY = _dataWindow.GetMaxY() + 1;

    // If a client does not use AOVs and we have no render buffers,
    // _height is 0 and we shouldn't use it to flip the data window.
    if (_height > 0) {
        // The data window is y-Down but the image line order
        // is from bottom to top, so we need to flip it.
        std::swap(minY, maxY);
        minY = _height - minY;
        maxY = _height - maxY;
    }

    const unsigned int tileSize =
        HdKrakenConfig::GetInstance().tileSize;
    const unsigned int numTilesX =
        (_dataWindow.GetWidth() + tileSize-1) / tileSize;
/*
    // Initialize the RNG for this tile (each tile creates one as
    // a lazy way to do thread-local RNGs).
    size_t seed;
    if (_randomNumberSeed == -1) {
        seed = std::chrono::system_clock::now().time_since_epoch().count();
    } else {
        seed = static_cast<size_t>(_randomNumberSeed);
    }
    seed = TfHash::Combine(seed, tileStart);
    seed = TfHash::Combine(seed, sampleNum);
    std::default_random_engine random(seed);

    // Create a uniform distribution for jitter calculations.
    std::uniform_real_distribution<float> uniform_dist(0.0f, 1.0f);
    auto uniform_float = [&random, &uniform_dist]() {
        return uniform_dist(random);
    };
*/
    // _RenderTiles gets a range of tiles; iterate through them.
    for (unsigned int tile = tileStart; tile < tileEnd; ++tile) {

        // Cancellation point.
        if (renderThread && renderThread->IsStopRequested()) {
            break;
        }

        // Compute the pixel location of tile boundaries.
        const unsigned int tileY = tile / numTilesX;
        const unsigned int tileX = tile - tileY * numTilesX; 
        // (Above is equivalent to: tileX = tile % numTilesX)
        const unsigned int x0 = tileX * tileSize + minX;
        const unsigned int y0 = tileY * tileSize + minY;
        // Clamp to data window, in case tileSize doesn't
        // neatly divide its with and height.
        const unsigned int x1 = std::min(x0+tileSize, maxX);
        const unsigned int y1 = std::min(y0+tileSize, maxY);

        // Loop over pixels casting rays.
        for (unsigned int y = y0; y < y1; ++y) {
            for (unsigned int x = x0; x < x1; ++x) {
                // Jitter the camera ray direction.
                GfVec2f jitter(0.0f, 0.0f);
                //if (HdKrakenConfig::GetInstance().jitterCamera) {
                //    jitter = GfVec2f(uniform_float(), uniform_float());
                //}

                // Un-transform the pixel's NDC coordinates through the
                // projection matrix to get the trace of the camera ray in the
                // near plane.
                const float w(_dataWindow.GetWidth());
                const float h(_dataWindow.GetHeight());

                const GfVec3f ndc(
                    2.0f * ((x + jitter[0] - minX) / w) - 1.0f,
                    2.0f * ((y + jitter[1] - minY) / h) - 1.0f,
                    -1.0f);
                const GfVec3f nearPlaneTrace(_inverseProjMatrix.Transform(ndc));

                GfVec3f origin;
                GfVec3f dir;

                const bool isOrthographic = round(_projMatrix[3][3]) == 1.0;
                if (isOrthographic) {
                    // During orthographic projection: trace parallel rays
                    // from the near plane trace.
                    origin = nearPlaneTrace;
                    dir = GfVec3f(0.0f, 0.0f, -1.0f);
                } else {
                    // Otherwise, assume this is a perspective projection;
                    // project from the camera origin through the
                    // near plane trace.
                    origin = GfVec3f(0.0f, 0.0f, 0.0f);
                    dir = nearPlaneTrace;
                }
                // Transform camera rays to world space.
                origin = GfVec3f(_inverseViewMatrix.Transform(origin));
                dir = GfVec3f(
                    _inverseViewMatrix.TransformDir(dir)).GetNormalized();

                // Trace the ray.
                //_TraceRay(x, y, origin, dir, random);
                HdKrakenRenderBuffer *renderBuffer =
                    static_cast<HdKrakenRenderBuffer*>(_aovBindings[0].renderBuffer);

                if (renderBuffer->IsConverged()) {
                    continue;
                }
                //std::cout << "x = " << x << " y = " << y<< " width = " << renderBuffer->GetWidth() << " height = " << renderBuffer->GetHeight()<< std::endl;
                if (x < renderBuffer->GetWidth() && y < renderBuffer->GetHeight())
                {
                    if (_aovNames[0].name == HdAovTokens->color) {
                        GfVec4f clearColor = _GetClearColor(_aovBindings[0].clearValue);
                        GfVec4f sample = GfVec4f((float)x/(float)renderBuffer->GetWidth(),(float)y/(float)renderBuffer->GetHeight(),0,1);
                        renderBuffer->Write(GfVec3i(x,y,1), 4, sample.data());
                    }
                }
            }
        }
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
