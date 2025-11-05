//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "renderDelegate.h"
#include "mesh.h"
#include "renderPass.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

const TfTokenVector HdKrakenRenderDelegate::SUPPORTED_RPRIM_TYPES =
{
    HdPrimTypeTokens->mesh,
};

const TfTokenVector HdKrakenRenderDelegate::SUPPORTED_SPRIM_TYPES =
{
};

const TfTokenVector HdKrakenRenderDelegate::SUPPORTED_BPRIM_TYPES =
{
};

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
    std::cout << "Creating Tiny RenderDelegate" << std::endl;
    _resourceRegistry = std::make_shared<HdResourceRegistry>();
}

HdKrakenRenderDelegate::~HdKrakenRenderDelegate()
{
    _resourceRegistry.reset();
    std::cout << "Destroying Tiny RenderDelegate" << std::endl;
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

void 
HdKrakenRenderDelegate::CommitResources(HdChangeTracker *tracker)
{
    std::cout << "=> CommitResources RenderDelegate" << std::endl;
}

HdRenderPassSharedPtr 
HdKrakenRenderDelegate::CreateRenderPass(
    HdRenderIndex *index,
    HdRprimCollection const& collection)
{
    std::cout << "Create RenderPass with Collection=" 
        << collection.GetName() << std::endl; 

    return HdRenderPassSharedPtr(new HdKrakenRenderPass(index, collection));  
}

HdRprim *
HdKrakenRenderDelegate::CreateRprim(TfToken const& typeId,
                                    SdfPath const& rprimId)
{
    std::cout << "Create Tiny Rprim type=" << typeId.GetText() 
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
    std::cout << "Destroy Tiny Rprim id=" << rPrim->GetId() << std::endl;
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
    TF_CODING_ERROR("Unknown Bprim type=%s id=%s", 
        typeId.GetText(), 
        bprimId.GetText());
    return nullptr;
}

HdBprim *
HdKrakenRenderDelegate::CreateFallbackBprim(TfToken const& typeId)
{
    TF_CODING_ERROR("Creating unknown fallback bprim type=%s", 
        typeId.GetText()); 
    return nullptr;
}

void
HdKrakenRenderDelegate::DestroyBprim(HdBprim *bPrim)
{
    TF_CODING_ERROR("Destroy Bprim not supported");
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

HdRenderParam *
HdKrakenRenderDelegate::GetRenderParam() const
{
    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE
