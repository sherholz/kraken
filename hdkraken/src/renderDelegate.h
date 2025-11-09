//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXTRAS_IMAGING_EXAMPLES_HD_KRAKEN_RENDER_DELEGATE_H
#define EXTRAS_IMAGING_EXAMPLES_HD_KRAKEN_RENDER_DELEGATE_H

#include <pxr/pxr.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/renderThread.h>
#include <pxr/imaging/hd/resourceRegistry.h>
#include <pxr/base/tf/staticTokens.h>

#include "renderer.h"
#include "renderParam.h"

PXR_NAMESPACE_OPEN_SCOPE

#define HDKRAKEN_RENDER_SETTINGS_TOKENS \
    (enableAmbientOcclusion)            \
    (enableLighting)                    \
    (enableSceneColors)                 \
    (ambientOcclusionSamples)           \
    (randomNumberSeed)

TF_DECLARE_PUBLIC_TOKENS(HdKrakenRenderSettingsTokens, HDKRAKEN_RENDER_SETTINGS_TOKENS);


///
/// \class HdKrakenRenderDelegate
///
/// Render delegates provide renderer-specific functionality to the render
/// index, the main hydra state management structure. The render index uses
/// the render delegate to create and delete scene primitives, which include
/// geometry and also non-drawable objects. The render delegate is also
/// responsible for creating renderpasses, which know how to draw this
/// renderer's scene primitives.
///
class HdKrakenRenderDelegate final : public HdRenderDelegate 
{
public:
    /// Render delegate constructor. 
    HdKrakenRenderDelegate();
    /// Render delegate constructor. 
    HdKrakenRenderDelegate(HdRenderSettingsMap const& settingsMap);
    /// Render delegate destructor.
    virtual ~HdKrakenRenderDelegate();

    /// Return this delegate's render param.
    ///   \return A shared instance of HdEmbreeRenderParam.
    HdRenderParam *GetRenderParam() const override;

    /// Return a list of which Rprim types can be created by this class's
    /// CreateRprim.
    const TfTokenVector &GetSupportedRprimTypes() const override;
    /// Return a list of which Sprim types can be created by this class's
    /// CreateSprim.
    const TfTokenVector &GetSupportedSprimTypes() const override;
    /// Return a list of which Bprim types can be created by this class's
    /// CreateBprim.
    const TfTokenVector &GetSupportedBprimTypes() const override;

    /// Returns the HdResourceRegistry instance used by this render delegate.
    HdResourceRegistrySharedPtr GetResourceRegistry() const override;

    /// Returns a list of user-configurable render settings.
    /// This is a reflection API for the render settings dictionary; it need
    /// not be exhaustive, but can be used for populating application settings
    /// UI.
    HdRenderSettingDescriptorList
        GetRenderSettingDescriptors() const override;

    /// Return true to indicate that pausing and resuming are supported.
    bool IsPauseSupported() const override;

    /// Pause background rendering threads.
    bool Pause() override;

    /// Resume background rendering threads.
    bool Resume() override;

    /// Create a renderpass. Hydra renderpasses are responsible for drawing
    /// a subset of the scene (specified by the "collection" parameter) to the
    /// current framebuffer. This class creates objects of type
    /// HdKrakenRenderPass, which draw using the kraken render engine.
    ///   \param index The render index this renderpass will be bound to.
    ///   \param collection A specifier for which parts of the scene should
    ///                     be drawn.
    ///   \return An embree renderpass object.
    HdRenderPassSharedPtr CreateRenderPass(
        HdRenderIndex *index,
        HdRprimCollection const& collection) override;

    /// Create an instancer. Hydra instancers store data needed for an
    /// instanced object to draw itself multiple times.
    ///   \param delegate The scene delegate providing data for this
    ///                   instancer.
    ///   \param id The scene graph ID of this instancer, used when pulling
    ///             data from a scene delegate.
    ///   \return An embree instancer object.
    HdInstancer *CreateInstancer(HdSceneDelegate *delegate,
                                         SdfPath const& id) override;
    /// Destroy an instancer created with CreateInstancer.
    ///   \param instancer The instancer to be destroyed.
    void DestroyInstancer(HdInstancer *instancer) override;

    /// Create a hydra Rprim, representing scene geometry. This class creates
    /// kraken-specialized geometry containers like HdKrakenMesh which map
    /// scene data to embree scene graph objects.
    ///   \param typeId The rprim type to create. This must be one of the types
    ///                 from GetSupportedRprimTypes().
    ///   \param rprimId The scene graph ID of this rprim, used when pulling
    ///                  data from a scene delegate.
    ///   \return An kraken rprim object.
    HdRprim *CreateRprim(TfToken const& typeId,
                                 SdfPath const& rprimId) override;
    
    /// Destroy an Rprim created with CreateRprim.
    ///   \param rPrim The rprim to be destroyed.
    void DestroyRprim(HdRprim *rPrim) override;

    /// Create a hydra Sprim, representing scene or viewport state like cameras
    /// or lights.
    ///   \param typeId The sprim type to create. This must be one of the types
    ///                 from GetSupportedSprimTypes().
    ///   \param sprimId The scene graph ID of this sprim, used when pulling
    ///                  data from a scene delegate.
    ///   \return An kraken sprim object.
    HdSprim *CreateSprim(TfToken const& typeId,
                         SdfPath const& sprimId) override;
   
    /// Create a hydra Sprim using default values, and with no scene graph
    /// binding.
    ///   \param typeId The sprim type to create. This must be one of the types
    ///                 from GetSupportedSprimTypes().
    ///   \return An embree fallback sprim object.
    HdSprim *CreateFallbackSprim(TfToken const& typeId) override;

    /// Destroy an Sprim created with CreateSprim or CreateFallbackSprim.
    ///   \param sPrim The sprim to be destroyed.
    void DestroySprim(HdSprim *sprim) override;

    /// Create a hydra Bprim, representing data buffers such as textures.
    ///   \param typeId The bprim type to create. This must be one of the types
    ///                 from GetSupportedBprimTypes().
    ///   \param bprimId The scene graph ID of this bprim, used when pulling
    ///                  data from a scene delegate.
    ///   \return An embree bprim object.
    HdBprim *CreateBprim(TfToken const& typeId,
                                 SdfPath const& bprimId) override;

    /// Create a hydra Bprim using default values, and with no scene graph
    /// binding.
    ///   \param typeId The bprim type to create. This must be one of the types
    ///                 from GetSupportedBprimTypes().
    ///   \return An embree fallback bprim object.
    HdBprim *CreateFallbackBprim(TfToken const& typeId) override;

    /// Destroy a Bprim created with CreateBprim or CreateFallbackBprim.
    ///   \param bPrim The bprim to be destroyed.
    void DestroyBprim(HdBprim *bprim) override;

    /// This function is called after new scene data is pulled during prim
    /// Sync(), but before any tasks (such as draw tasks) are run, and gives the
    /// render delegate a chance to transfer any invalidated resources to the
    /// rendering kernel.
    ///   \param tracker The change tracker passed to prim Sync().
    void CommitResources(HdChangeTracker *tracker) override;

    /// This function returns the default AOV descriptor for a given named AOV.
    /// This mechanism lets the renderer decide things like what format
    /// a given AOV will be written as.
    ///   \param name The name of the AOV whose descriptor we want.
    ///   \return A descriptor specifying things like what format the AOV
    ///           output buffer should be.
    HdAovDescriptor
    GetDefaultAovDescriptor(TfToken const& name) const override;

    /// This function allows the renderer to report back some useful statistics
    /// that the application can display to the user.
    VtDictionary GetRenderStats() const override;

private:
    static const TfTokenVector SUPPORTED_RPRIM_TYPES;
    static const TfTokenVector SUPPORTED_SPRIM_TYPES;
    static const TfTokenVector SUPPORTED_BPRIM_TYPES;

    void _Initialize();

    HdResourceRegistrySharedPtr _resourceRegistry;

    // This class does not support copying.
    HdKrakenRenderDelegate(const HdKrakenRenderDelegate &) = delete;
    HdKrakenRenderDelegate &operator =(const HdKrakenRenderDelegate &) = delete;
private:
    // A version counter for edits to _scene.
    std::atomic<int> _sceneVersion;

    // A shared HdKrakenRenderParam object that stores top-level embree state;
    // passed to prims during Sync().
    std::shared_ptr<HdKrakenRenderParam> _renderParam;

    // A background render thread for running the actual renders in. The
    // render thread object manages synchronization between the scene data
    // and the background-threaded renderer.
    HdRenderThread _renderThread;

    // An kraken renderer object, to perform the actual raytracing.
    HdKrakenRenderer _renderer;

    // A list of render setting exports.
    HdRenderSettingDescriptorList _settingDescriptors;

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXTRAS_IMAGING_EXAMPLES_HD_KRAKEN_RENDER_DELEGATE_H
