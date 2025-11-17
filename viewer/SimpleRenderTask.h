
#include <pxr/imaging/hd/task.h>
#include <pxr/imaging/hd/renderPass.h>
#include <pxr/imaging/hd/renderPassState.h>

class SimpleRenderTask final : public pxr::HdTask
{
public:
    SimpleRenderTask(const pxr::HdRenderPassSharedPtr &renderPass,
                     const pxr::HdRenderPassStateSharedPtr &renderPassState,
                     const pxr::TfTokenVector &renderTags);

    void Sync(pxr::HdSceneDelegate *sceneDelegate,
              pxr::HdTaskContext *taskContext,
              pxr::HdDirtyBits *dirtyBits);

    void Prepare(pxr::HdTaskContext *taskContext,
                 pxr::HdRenderIndex *renderIndex);

    void Execute(pxr::HdTaskContext *taskContext);

    const pxr::TfTokenVector &GetRenderTags() const;

private:
    pxr::HdRenderPassSharedPtr m_renderPass;
    pxr::HdRenderPassStateSharedPtr m_renderPassState;
    pxr::TfTokenVector m_renderTags;
};