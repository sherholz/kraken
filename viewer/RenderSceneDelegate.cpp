#include "RenderSceneDelegate.h"


TF_DEFINE_PRIVATE_TOKENS(
    g_tokens,

    (iadCollection)
    (renderBufferDescriptor)
    ((rendermode, "spyri4:rendermode"))
);

RenderSceneDelegate::RenderSceneDelegate(pxr::HdRenderIndex *renderIndex,
                                         pxr::SdfPath const &delegateId /*,
                                          SdfPath const& cameraId*/
                                         )
    : pxr::HdSceneDelegate(renderIndex, delegateId) //,
//  _cameraId(cameraId)
{
}

pxr::HdRenderBufferDescriptor RenderSceneDelegate::GetRenderBufferDescriptor(pxr::SdfPath const &id)
{
    return GetParameter<pxr::HdRenderBufferDescriptor>(id,
                                                       g_tokens->renderBufferDescriptor);
}