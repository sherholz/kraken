#include "RenderTask.h"
RenderTask::RenderTask(const pxr::HdRenderPassSharedPtr& renderPass,
                                   const pxr::HdRenderPassStateSharedPtr& renderPassState,
                                   const pxr::TfTokenVector& renderTags)
  : pxr::HdTask(pxr::SdfPath::EmptyPath())
  , m_renderPass(renderPass)
  , m_renderPassState(renderPassState)
  , m_renderTags(renderTags)
{
}

void RenderTask::Sync(pxr::HdSceneDelegate* sceneDelegate,
                            pxr::HdTaskContext* taskContext,
                            pxr::HdDirtyBits* dirtyBits)
{
  TF_UNUSED(sceneDelegate);
  TF_UNUSED(taskContext);
  //sceneDelegate->Sync();
  m_renderPass->Sync();

  *dirtyBits = pxr::HdChangeTracker::Clean;
}

void RenderTask::Prepare(pxr::HdTaskContext* taskContext,
                               pxr::HdRenderIndex* renderIndex)
{
  TF_UNUSED(taskContext);

  const pxr::HdResourceRegistrySharedPtr& resourceRegistry = renderIndex->GetResourceRegistry();
  m_renderPassState->Prepare(resourceRegistry);
}

void RenderTask::Execute(pxr::HdTaskContext* taskContext)
{
  TF_UNUSED(taskContext);

  m_renderPass->Execute(m_renderPassState, m_renderTags);
  while (!m_renderPass->IsConverged())
  {}
}

const pxr::TfTokenVector& RenderTask::GetRenderTags() const
{
  return m_renderTags;
}
