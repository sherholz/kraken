//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include <pxr/imaging/hd/renderPassState.h>
#include "renderDelegate.h"
#include "renderPass.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

HdKrakenRenderPass::HdKrakenRenderPass(HdRenderIndex *index,
                                       HdRprimCollection const &collection,
                                       HdRenderThread *renderThread,
                                       HdKrakenRenderer *renderer,
                                       std::atomic<int> *sceneVersion)
    : HdRenderPass(index, collection)
    , _renderThread(renderThread)
    , _renderer(renderer)
    , _sceneVersion(sceneVersion)
    , _lastSceneVersion(0)
    , _lastSettingsVersion(0)
    , _viewMatrix(1.0f) // == identity
    , _projMatrix(1.0f) // == identity
    , _aovBindings()
    , _converged(false)
{
}

HdKrakenRenderPass::~HdKrakenRenderPass()
{
    std::cout << "Destroying renderPass" << std::endl;
}

bool
HdKrakenRenderPass::IsConverged() const
{
    // If the aov binding array is empty, the render thread is rendering into
    // _colorBuffer and _depthBuffer.  _converged is set to their convergence
    // state just before blit, so use that as our answer.
    if (_aovBindings.size() == 0) {
        return _converged;
    }

    // Otherwise, check the convergence of all attachments.
    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        if (_aovBindings[i].renderBuffer &&
            !_aovBindings[i].renderBuffer->IsConverged()) {
            return false;
        }
    }
    return true;
}

static
GfRect2i
_GetDataWindow(HdRenderPassStateSharedPtr const& renderPassState)
{
    const CameraUtilFraming &framing = renderPassState->GetFraming();
    if (framing.IsValid()) {
        return framing.dataWindow;
    } else {
        // For applications that use the old viewport API instead of
        // the new camera framing API.
        const GfVec4f vp = renderPassState->GetViewport();
        return GfRect2i(GfVec2i(0), int(vp[2]), int(vp[3]));        
    }
}

void HdKrakenRenderPass::_Execute(
    HdRenderPassStateSharedPtr const &renderPassState,
    TfTokenVector const &renderTags)
{
    bool needReStartRender = false;

    // Determine whether we need to update the renderer camera.
    const GfMatrix4d view = renderPassState->GetWorldToViewMatrix();
    const GfMatrix4d proj = renderPassState->GetProjectionMatrix();
    
    if (_viewMatrix != view || _projMatrix != proj) {
        _viewMatrix = view;
        _projMatrix = proj;

        _renderThread->StopRender();
        //_renderer->SetCamera(_viewMatrix, _projMatrix);
        needReStartRender = true;
    }
    

    const GfRect2i dataWindow = _GetDataWindow(renderPassState);
    //std::cout << dataWindow << std::endl;
    if (_dataWindow != dataWindow)
    {
        _dataWindow = dataWindow;

        _renderThread->StopRender();
        _renderer->SetDataWindow(dataWindow);
        
        if (!renderPassState->GetFraming().IsValid())
        {
            std::cout << "ERROR Framing is invalid" << std::endl;
        }
        
        needReStartRender = true;
    }
    // Determine whether we need to update the renderer AOV bindings.
    //
    // It's possible for the passed in bindings to be empty, but that's
    // never a legal state for the renderer, so if that's the case we add
    // a color and depth aov.
    //
    // If the renderer AOV bindings are empty, force a bindings update so that
    // we always get a chance to add color/depth on the first time through.
    HdRenderPassAovBindingVector aovBindings =
        renderPassState->GetAovBindings();
    if (_aovBindings != aovBindings || _renderer->GetAovBindings().empty())
    {
        _aovBindings = aovBindings;

        _renderThread->StopRender();
        if (aovBindings.empty())
        {
            std::cout << "ERROR empty AOV Bindings" << std::endl;
        }
        _renderer->SetAovBindings(aovBindings);
        // In general, the render thread clears aov bindings, but make sure
        // they are cleared initially on this thread.
        _renderer->Clear();
        needReStartRender = true;
    }

    TF_VERIFY(!_aovBindings.empty(), "No aov bindings to render into");
    //std::cout << "needReStartRender = " << needReStartRender << std::endl;
    // Only start a new render if something in the scene has changed.
    if (needReStartRender)
    {
        _converged = false;
        _renderer->MarkAovBuffersUnconverged();
        std::cout << "=> Start Render " << std::endl; 
        _renderThread->StartRender();
        //_renderThread->
    }
    //std::cout << "=> Execute RenderPass" << std::endl;
}

PXR_NAMESPACE_CLOSE_SCOPE
