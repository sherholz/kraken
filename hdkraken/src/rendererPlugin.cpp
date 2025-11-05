//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "rendererPlugin.h"
#include "renderDelegate.h"

#include "pxr/imaging/hd/rendererPluginRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the plugin with the renderer plugin system.
TF_REGISTRY_FUNCTION(TfType)
{
    HdRendererPluginRegistry::Define<HdKrakenRendererPlugin>();
}

HdRenderDelegate*
HdKrakenRendererPlugin::CreateRenderDelegate()
{
    return new HdKrakenRenderDelegate();
}

HdRenderDelegate*
HdKrakenRendererPlugin::CreateRenderDelegate(
    HdRenderSettingsMap const& settingsMap)
{
    return new HdKrakenRenderDelegate(settingsMap);
}

void
HdKrakenRendererPlugin::DeleteRenderDelegate(HdRenderDelegate *renderDelegate)
{
    delete renderDelegate;
}

bool 
HdKrakenRendererPlugin::IsSupported(
    HdRendererCreateArgs const & /*rendererCreateArgs*/,
    std::string * /* reasonWhyNot */) const
{
    // Nothing more to check for now, we assume if the plugin loads correctly
    // it is supported.
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
