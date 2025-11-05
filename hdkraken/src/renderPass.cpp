//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "renderPass.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

HdKrakenRenderPass::HdKrakenRenderPass(
    HdRenderIndex *index,
    HdRprimCollection const &collection)
    : HdRenderPass(index, collection)
{
}

HdKrakenRenderPass::~HdKrakenRenderPass()
{
    std::cout << "Destroying renderPass" << std::endl;
}

void
HdKrakenRenderPass::_Execute(
    HdRenderPassStateSharedPtr const& renderPassState,
    TfTokenVector const &renderTags)
{
    std::cout << "=> Execute RenderPass" << std::endl;
}

PXR_NAMESPACE_CLOSE_SCOPE
