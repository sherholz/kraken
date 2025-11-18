//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "renderBuffer.h"
#include "renderDelegate.h"
#include "mesh.h"
#include "renderPass.h"
#include "config.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdKrakenRenderSettingsTokens, HDKRAKEN_RENDER_SETTINGS_TOKENS);

const TfTokenVector HdKrakenRenderDelegate::SUPPORTED_RPRIM_TYPES =
{
    HdPrimTypeTokens->mesh,
};

const TfTokenVector HdKrakenRenderDelegate::SUPPORTED_SPRIM_TYPES =
{
};

const TfTokenVector HdKrakenRenderDelegate::SUPPORTED_BPRIM_TYPES =
{
    HdPrimTypeTokens->renderBuffer,
};

static void _RenderCallback(HdKrakenRenderer *renderer,
                            HdRenderThread *renderThread)
{
    renderer->Clear();
    renderer->Render(renderThread);
}

HdKrakenRenderDelegate::HdKrakenRenderDelegate()
    : HdRenderDelegate()
{
    _Initialize();
}

HdKrakenRenderDelegate::HdKrakenRenderDelegate(
    HdRenderSettingsMap const& settingsMap)
    : HdRenderDelegate(settingsMap)
{
    _Initialize();
}

void
HdKrakenRenderDelegate::_Initialize()
{
    // Initialize the settings and settings descriptors.
    _settingDescriptors.resize(6);
    _settingDescriptors[0] = { "Enable Scene Colors",
        HdKrakenRenderSettingsTokens->enableSceneColors,
        VtValue(HdKrakenConfig::GetInstance().useFaceColors) };
    _settingDescriptors[1] = { "Enable Ambient Occlusion",
        HdKrakenRenderSettingsTokens->enableAmbientOcclusion,
        VtValue(HdKrakenConfig::GetInstance().ambientOcclusionSamples > 0) };
    _settingDescriptors[2] = { "Enable Scene Lighting",
        HdKrakenRenderSettingsTokens->enableLighting,
        VtValue(HdKrakenConfig::GetInstance().useLighting) };
    _settingDescriptors[3] = { "Ambient Occlusion Samples",
        HdKrakenRenderSettingsTokens->ambientOcclusionSamples,
        VtValue(int(HdKrakenConfig::GetInstance().ambientOcclusionSamples)) };
    _settingDescriptors[4] = { "Samples To Convergence",
        HdRenderSettingsTokens->convergedSamplesPerPixel,
        VtValue(int(HdKrakenConfig::GetInstance().samplesToConvergence)) };
    _settingDescriptors[5] = { "Random Number Seed",
        HdKrakenRenderSettingsTokens->randomNumberSeed,
        VtValue(HdKrakenConfig::GetInstance().randomNumberSeed) };
    _PopulateDefaultSettings(_settingDescriptors);

    
    std::cout << "Creating Kraken RenderDelegate" << std::endl;
    _resourceRegistry = std::make_shared<HdResourceRegistry>();

    // Set the background render thread's rendering entrypoint to
    // HdEmbreeRenderer::Render.
    _renderThread.SetRenderCallback(
        std::bind(_RenderCallback, &_renderer, &_renderThread));
    // Start the background render thread.
    _renderThread.StartThread();
}

HdKrakenRenderDelegate::~HdKrakenRenderDelegate()
{
    _resourceRegistry.reset();
    std::cout << "Destroying Kraken RenderDelegate" << std::endl;
}

TfTokenVector const&
HdKrakenRenderDelegate::GetSupportedRprimTypes() const
{
    return SUPPORTED_RPRIM_TYPES;
}

TfTokenVector const&
HdKrakenRenderDelegate::GetSupportedSprimTypes() const
{
    return SUPPORTED_SPRIM_TYPES;
}

TfTokenVector const&
HdKrakenRenderDelegate::GetSupportedBprimTypes() const
{
    return SUPPORTED_BPRIM_TYPES;
}

HdResourceRegistrySharedPtr
HdKrakenRenderDelegate::GetResourceRegistry() const
{
    return _resourceRegistry;
}

HdAovDescriptor
HdKrakenRenderDelegate::GetDefaultAovDescriptor(TfToken const& name) const
{
    if (name == HdAovTokens->color) {
        return HdAovDescriptor(HdFormatFloat32Vec4, true,
                               VtValue(GfVec4f(0.0f)));
    }
    /* else if (name == HdAovTokens->normal || name == HdAovTokens->Neye) {
        return HdAovDescriptor(HdFormatFloat32Vec3, false,
                               VtValue(GfVec3f(-1.0f)));
    } else if (name == HdAovTokens->depth) {
        return HdAovDescriptor(HdFormatFloat32, false, VtValue(1.0f));
    } else if (name == HdAovTokens->cameraDepth) {
        return HdAovDescriptor(HdFormatFloat32, false, VtValue(0.0f));
    } else if (name == HdAovTokens->primId ||
               name == HdAovTokens->instanceId ||
               name == HdAovTokens->elementId) {
        return HdAovDescriptor(HdFormatInt32, false, VtValue(-1));
    } else {
        HdParsedAovToken aovId(name);
        if (aovId.isPrimvar) {
            return HdAovDescriptor(HdFormatFloat32Vec3, false,
                                   VtValue(GfVec3f(0.0f)));
        }
    }*/

    return HdAovDescriptor();
}

VtDictionary 
HdKrakenRenderDelegate::GetRenderStats() const
{
    VtDictionary stats;
    //stats[HdPerfTokens->numCompletedSamples.GetString()] = 
    //    _renderer.GetCompletedSamples();
    return stats;
}

bool
HdKrakenRenderDelegate::IsPauseSupported() const
{
    return true;
}

bool
HdKrakenRenderDelegate::Pause()
{
    _renderThread.PauseRender();
    return true;
}

bool
HdKrakenRenderDelegate::Resume()
{
    _renderThread.ResumeRender();
    return true;
}

void 
HdKrakenRenderDelegate::CommitResources(HdChangeTracker *tracker)
{
    //std::cout << "=> CommitResources RenderDelegate" << std::endl;
}

HdRenderPassSharedPtr 
HdKrakenRenderDelegate::CreateRenderPass(
    HdRenderIndex *index,
    HdRprimCollection const& collection)
{
    std::cout << "Create RenderPass with Collection=" 
        << collection.GetName() << std::endl; 

    return HdRenderPassSharedPtr(new HdKrakenRenderPass(
        index, collection, &_renderThread, &_renderer, &_sceneVersion));
}

HdRprim *
HdKrakenRenderDelegate::CreateRprim(TfToken const& typeId,
                                    SdfPath const& rprimId)
{
    std::cout << "Create Kraken Rprim type=" << typeId.GetText() 
        << " id=" << rprimId 
        << std::endl;

    if (typeId == HdPrimTypeTokens->mesh) {
        return new HdKrakenMesh(rprimId);
    } else {
        TF_CODING_ERROR("Unknown Rprim type=%s id=%s", 
            typeId.GetText(), 
            rprimId.GetText());
    }
    return nullptr;
}

void
HdKrakenRenderDelegate::DestroyRprim(HdRprim *rPrim)
{
    std::cout << "Destroy Kraken Rprim id=" << rPrim->GetId() << std::endl;
    delete rPrim;
}

HdSprim *
HdKrakenRenderDelegate::CreateSprim(TfToken const& typeId,
                                    SdfPath const& sprimId)
{
    TF_CODING_ERROR("Unknown Sprim type=%s id=%s", 
        typeId.GetText(), 
        sprimId.GetText());
    return nullptr;
}

HdSprim *
HdKrakenRenderDelegate::CreateFallbackSprim(TfToken const& typeId)
{
    TF_CODING_ERROR("Creating unknown fallback sprim type=%s", 
        typeId.GetText()); 
    return nullptr;
}

void
HdKrakenRenderDelegate::DestroySprim(HdSprim *sPrim)
{
    TF_CODING_ERROR("Destroy Sprim not supported");
}

HdBprim *
HdKrakenRenderDelegate::CreateBprim(TfToken const& typeId, SdfPath const& bprimId)
{
    if (typeId == HdPrimTypeTokens->renderBuffer) {
        return new HdKrakenRenderBuffer(bprimId);
    } else {
        TF_CODING_ERROR("Unknown Bprim Type %s", typeId.GetText());
    }
    return nullptr;
}

HdBprim *
HdKrakenRenderDelegate::CreateFallbackBprim(TfToken const& typeId)
{
    if (typeId == HdPrimTypeTokens->renderBuffer) {
        return new HdKrakenRenderBuffer(SdfPath("/kraken/framebuffer/renderbuffer"));
    } else {
        TF_CODING_ERROR("Unknown Bprim Type %s", typeId.GetText());
    }
    return nullptr;
}

void
HdKrakenRenderDelegate::DestroyBprim(HdBprim *bPrim)
{
    delete bPrim;
}

HdInstancer *
HdKrakenRenderDelegate::CreateInstancer(
    HdSceneDelegate *delegate,
    SdfPath const& id)
{
    TF_CODING_ERROR("Creating Instancer not supported id=%s", 
        id.GetText());
    return nullptr;
}

void 
HdKrakenRenderDelegate::DestroyInstancer(HdInstancer *instancer)
{
    TF_CODING_ERROR("Destroy instancer not supported");
}

HdRenderSettingDescriptorList
HdKrakenRenderDelegate::GetRenderSettingDescriptors() const
{
    return _settingDescriptors;
}

HdRenderParam *
HdKrakenRenderDelegate::GetRenderParam() const
{
    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE
