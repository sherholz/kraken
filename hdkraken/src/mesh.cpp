//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "mesh.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

HdKrakenMesh::HdKrakenMesh(SdfPath const& id)
    : HdMesh(id)
{
}

HdDirtyBits
HdKrakenMesh::GetInitialDirtyBitsMask() const
{
    return HdChangeTracker::Clean
        | HdChangeTracker::DirtyTransform;
}

HdDirtyBits
HdKrakenMesh::_PropagateDirtyBits(HdDirtyBits bits) const
{
    return bits;
}

void 
HdKrakenMesh::_InitRepr(TfToken const &reprToken, HdDirtyBits *dirtyBits)
{

}

void
HdKrakenMesh::Sync(HdSceneDelegate *sceneDelegate,
                   HdRenderParam   *renderParam,
                   HdDirtyBits     *dirtyBits,
                   TfToken const   &reprToken)
{
    std::cout << "* (multithreaded) Sync Tiny Mesh id=" << GetId() << std::endl;
}

PXR_NAMESPACE_CLOSE_SCOPE
