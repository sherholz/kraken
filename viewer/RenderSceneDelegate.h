#pragma once

#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/base/tf/diagnostic.h>
#if PXR_USE_NAMESPACES
using namespace pxr;
#endif

class RenderSceneDelegate : public pxr::HdSceneDelegate
{
public:
    RenderSceneDelegate(pxr::HdRenderIndex *renderIndex,
                  pxr::SdfPath const &delegateId/*,
                  SdfPath const &cameraId*/);

  virtual ~RenderSceneDelegate() = default;

  // HdxTaskController set/get interface
  template <typename T>
  void SetParameter(pxr::SdfPath const& id, pxr::TfToken const& key,
                    T const& value) {
      _valueCacheMap[id][key] = value;
  }

  template <typename T>
  T GetParameter(pxr::SdfPath const& id, pxr::TfToken const& key) const {
      pxr::VtValue vParams;
      _ValueCache vCache;
      TF_VERIFY(
          TfMapLookup(_valueCacheMap, id, &vCache) &&
          TfMapLookup(vCache, key, &vParams) &&
          vParams.IsHolding<T>());
      return vParams.Get<T>();
  }

  bool HasParameter(pxr::SdfPath const& id, pxr::TfToken const& key) const {
      _ValueCache vCache;
      if (TfMapLookup(_valueCacheMap, id, &vCache) &&
          vCache.count(key) > 0) {
          return true;
      }
      return false;
  }

  HdRenderBufferDescriptor GetRenderBufferDescriptor(SdfPath const& id) override;

  private:

    typedef pxr::TfHashMap<pxr::TfToken, pxr::VtValue, pxr::TfToken::HashFunctor> _ValueCache;
    typedef pxr::TfHashMap<pxr::SdfPath, _ValueCache, pxr::SdfPath::Hash> _ValueCacheMap;
    _ValueCacheMap _valueCacheMap;
};