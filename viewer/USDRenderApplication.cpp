#include "USDRenderApplication.h"

#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/camera.h>

#include <pxr/imaging/hio/image.h>

#include <pxr/imaging/hd/camera.h>
#include <pxr/imaging/hd/pluginRenderDelegateUniqueHandle.h>
#include <pxr/imaging/hd/task.h>
#include <pxr/imaging/hd/rendererPlugin.h>

#include <pxr/imaging/hdx/taskController.h>

#include <pxr/usdImaging/usdAppUtils/camera.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#include <pxr/base/tf/staticTokens.h>

#include "RenderSceneDelegate.h"
#include "RenderTask.h"

#include <string>
#include <sstream>
#include <iostream>

#if PXR_USE_NAMESPACES
using namespace pxr;
#endif

TF_DEFINE_PRIVATE_TOKENS(
    g_tokens,

    (iadCollection)(renderBufferDescriptor)((rendermode, "spyri4:rendermode")));

std::istream &operator>>(std::istream &is, std::tuple<int, int> &ints)
{
    is >> std::get<0>(ints);
    is.get();
    is >> std::get<1>(ints);
    return is;
}

#include "../extern/args/args.hxx"

ApplicationParameter::ApplicationParameter(int argc, char **argv)
{
    args::ArgumentParser parser("This is a test program.", "This goes after the options.");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

    args::Positional<std::string> usdFile(parser, "usdFile", "The foo position");
    args::Flag batch(parser, "batch", "The rendering is started in batch mode.", {'b', "batch"});
    args::ValueFlag<std::tuple<int, int>> resolution(parser, "width height", "This takes a pair of integers.", {"resolution"});
    args::ValueFlag<std::string> renderer(parser, "renderer", "This takes a pair of integers.", {'r', "renderer"});
    args::ValueFlag<std::string> camera(parser, "camera", "This takes a pair of integers.", {'c', "camera"});

    try
    {
        parser.ParseCLI(argc, argv);
    }
    // catch (const args::Completion& e)
    //{
    //     std::cout << e.what();
    //     return 0;
    // }
    catch (const args::Help &)
    {
        std::cout << parser;
        // return true;
    }
    catch (const args::ParseError &e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        // return false;
    }

    if (usdFile)
    {
        this->usdFilePath = args::get(usdFile);
    }
    else
    {
        std::cerr << "Error: USD scene file is not set";
        std::cerr << parser;
        // return false;
    }

    if (batch)
    {
        this->batch = args::get(batch);
    }

    if (renderer)
    {
        this->renderer = args::get(renderer);
    }

    if (camera)
    {
        this->camera = args::get(camera);
    }

    if (resolution)
    {
        std::tie(this->resolution[0], this->resolution[1]) = args::get(resolution);
    }
}

std::string ApplicationParameter::toString() const
{
    std::stringstream ss;
    ss << "ApplicationParameter:" << std::endl;
    ss << "\t usdFilePath = " << usdFilePath << std::endl;
    ss << "\t renderer = " << renderer << std::endl;
    ss << "\t camera = " << camera << std::endl;
    ss << "\t batch = " << (batch ? "True" : "False") << std::endl;
    ss << "\t resolution = " << "[" << resolution.x() << ", " << resolution.y() << "]" << std::endl;
    return ss.str();
}

USDRenderApplication::USDRenderApplication(const ApplicationParameter &args) : args(args)
{
    Intialize();
    LoadUSDScene(args.usdFilePath);
    resolution = args.resolution;
}

void USDRenderApplication::Intialize()
{
    pxr::HdRendererPluginRegistry &pluginRegistry = pxr::HdRendererPluginRegistry::GetInstance();
    // Get available RenderDelegates
    pxr::HfPluginDescVector pluginDescs;
    pluginRegistry.GetPluginDescs(&pluginDescs);

    if (pluginDescs.size() > 0)
    {
        for (size_t i = 0; i < pluginDescs.size(); ++i)
        {
            availableRenderDelegates[pluginDescs[i].displayName] = pluginDescs[i].id;
        }
    }

    auto renderDelegateEntry = availableRenderDelegates.find(args.renderer);
    if (renderDelegateEntry != availableRenderDelegates.end())
    {
        pxr::HdRendererPluginHandle plugin = pluginRegistry.GetOrCreateRendererPlugin(pxr::TfToken(availableRenderDelegates[args.renderer]));
    }
    else
    {
    }
}

std::vector<pxr::SdfPath> USDRenderApplication::FindAvailableCameras()
{
    // Load available Cameras

    std::vector<pxr::SdfPath> sceneCameras;

    pxr::UsdPrimRange primRange = stage->TraverseAll();
    for (auto prim = primRange.cbegin(); prim != primRange.cend(); prim++)
    {
        if (!prim->IsA<pxr::UsdGeomCamera>())
        {
            continue;
        }
        pxr::SdfPath cameraPath = prim->GetPath();
        sceneCameras.push_back(cameraPath);
        break;
    }

    return sceneCameras;
}

void USDRenderApplication::PrintAvailableRenderDelegates() const
{
    if (availableRenderDelegates.size() > 0)
    {
        std::cout << "Renderers:" << std::endl;
        int i = 0;
        for (auto it = availableRenderDelegates.begin(); it != availableRenderDelegates.end(); ++it)
        {
            std::cout << "\t renderer[" << i << "] = " << it->first << std::endl;
            i++;
        }
    }
}

void USDRenderApplication::PrintAvailableCameras() const
{
    if (cameras.size() > 0)
    {
        std::cout << "Cameras:" << std::endl;
        for (int i = 0; i < cameras.size(); i++)
        {
            std::cout << "\t camera[" << i << "] = " << cameras[i] << std::endl;
        }
    }
}

void USDRenderApplication::Prepare()
{
    // prepare camera
    pxr::SdfPath cameraPath = pxr::SdfPath(args.camera);
    pxr::UsdGeomCamera camera = pxr::UsdAppUtilsGetCameraAtPath(stage, cameraPath);
    if (camera)
    {
        std::cout << "Camera exists" << std::endl;
    }
    else
    {
        std::cout << "No Camera" << std::endl;
    }

    pxr::HdRendererPluginRegistry &pluginRegistry = pxr::HdRendererPluginRegistry::GetInstance();

    pxr::HdRendererPluginHandle plugin = pluginRegistry.GetOrCreateRendererPlugin(pxr::TfToken(availableRenderDelegates[args.renderer]));

    if (!plugin)
    {
        std::cout << "Renderer not found/supported: " << args.renderer << std::endl;
    }
    else
    {
        std::cout << "Renderer loaded: " << args.renderer << std::endl;
    }
    // prepare delegate
    std::cout << "Render Delegate" << std::endl;
    renderDelegate = plugin->CreateRenderDelegate();
    renderDelegate->SetRenderSetting(pxr::HdRenderSettingsTokens->enableInteractive, pxr::VtValue(false));

    std::cout << "Render Index" << std::endl;
    renderIndex = pxr::HdRenderIndex::New(renderDelegate, pxr::HdDriverVector());
    std::cout << "HdCamera" << std::endl;
    pxr::HdCamera *hdCamera = static_cast<pxr::HdCamera *>(renderIndex->GetSprim(pxr::HdTokens->camera, cameraPath));

    std::cout << "Scene Delegate" << std::endl;
    renderSceneDelegate = std::make_unique<RenderSceneDelegate>(renderIndex, pxr::SdfPath("/kraken"));
    sceneDelegate = std::make_unique<pxr::UsdImagingDelegate>(renderIndex, pxr::SdfPath::AbsoluteRootPath());
    sceneDelegate->Populate(stage->GetPseudoRoot());
    sceneDelegate->SetTime(0);
    sceneDelegate->SetRefineLevelFallback(4);

    //pxr::SdfPath controllerId("/controllerIs");
    //taskController = new pxr::HdxTaskController(renderIndex, controllerId, false);

    std::cout << "RenderBuffer" << std::endl;
    // Set up rendering context.
    //renderBuffer = (pxr::HdRenderBuffer *)renderDelegate->CreateFallbackBprim(pxr::HdPrimTypeTokens->renderBuffer);
    //renderBuffer->Allocate(pxr::GfVec3i(resolution.x(), resolution.y(), 1), pxr::HdFormatFloat32Vec4, false);

    pxr::HdAovDescriptor aovDesc = renderDelegate->GetDefaultAovDescriptor(pxr::HdAovTokens->color);

    auto id = pxr::SdfPath("/kraken/framebuffer/color");
    //auto id = renderBuffer->GetId();

    renderIndex->InsertBprim(HdBprimTypeTokens->renderBuffer, renderSceneDelegate.get(), id);

    pxr::HdRenderBufferDescriptor desc;
    desc.dimensions[0] = resolution.x();
    desc.dimensions[1] = resolution.y();
    desc.dimensions[2] = 1;

    desc.format = aovDesc.format;
    desc.multiSampled = false;
    renderSceneDelegate->SetParameter(id,
                                g_tokens->renderBufferDescriptor, desc);
    renderIndex->GetChangeTracker().MarkBprimDirty(id,
                                                   HdRenderBuffer::DirtyDescription);

    // std::cout << "RenderBuffer: width = " << (int) renderBuffer->GetWidth() << "\t height = " << (int) renderBuffer->GetHeight() << "\t depth = " << (int) renderBuffer->GetDepth() << std::endl;

    std::cout << "AOVs" << std::endl;
    pxr::HdRenderPassAovBindingVector aovBindings(1);
    aovBindings[0].aovName = pxr::HdAovTokens->color;
    aovBindings[0].renderBuffer = reinterpret_cast<HdRenderBuffer *>(
        renderIndex->GetBprim(HdPrimTypeTokens->renderBuffer, id));
    aovBindings[0].renderBufferId = id; //::SdfPath("/framebuffer/color");
    aovId.push_back(aovBindings[0].renderBufferId);

    pxr::CameraUtilFraming framing;
    framing.dataWindow = pxr::GfRect2i(pxr::GfVec2i(0, 0), pxr::GfVec2i(resolution.x(), resolution.y()));
    framing.displayWindow = pxr::GfRange2f(pxr::GfVec2f(0.0f, 0.0f), pxr::GfVec2f((float)resolution.x(), (float)resolution.y()));
    framing.pixelAspectRatio = (float)resolution.y() / (float)resolution.x();

    std::cout << "RenderPassState" << std::endl;
    renderPassState = std::make_shared<pxr::HdRenderPassState>();

#if PXR_VERSION <= 2311
    std::pair<bool, CameraUtilConformWindowPolicy> overrideWindowPolicy(false, CameraUtilFit);
    renderPassState->SetCameraAndFraming(camera, framing, overrideWindowPolicy);
#else
    std::optional<pxr::CameraUtilConformWindowPolicy> overrideWindowPolicy(pxr::CameraUtilFit);
    renderPassState->SetCamera(hdCamera);
    renderPassState->SetFraming(framing);
    renderPassState->SetOverrideWindowPolicy(overrideWindowPolicy);
#endif

    renderPassState->SetAovBindings(aovBindings);

    pxr::HdRprimCollection renderCollection(pxr::HdTokens->geometry, pxr::HdReprSelector(pxr::HdReprTokens->refined));
    std::cout << "RenderPass" << std::endl;
    pxr::HdRenderPassSharedPtr renderPass = renderDelegate->CreateRenderPass(renderIndex, renderCollection);

    pxr::TfTokenVector renderTags(1, pxr::HdRenderTagTokens->geometry);
    auto renderTask = std::make_shared<RenderTask>(renderPass, renderPassState, renderTags);

    // pxr::HdTaskSharedPtrVector tasks;
    tasks.push_back(renderTask);

    /*
        pxr::HdEngine engine;
        engine.Execute(renderIndex, &tasks);
        renderBuffer->Resolve();

        renderTimer.Stop();
            printf("Rendering finished (%.3fs)\n", renderTimer.GetSeconds());
        fflush(stdout);
    */
}
void USDRenderApplication::Render()
{
    //std::cout << "renderBuffer: width = " << renderBuffer->GetWidth() << " height = " << renderBuffer->GetHeight() << std::endl;
    // Perform rendering.
    pxr::TfStopwatch renderTimer;
    renderTimer.Start();
    pxr::HdEngine engine;
    engine.Execute(renderIndex, &tasks);
    //renderBuffer->Resolve();

    renderTimer.Stop();
    //printf("Rendering finished (%.3fs)\n", renderTimer.GetSeconds());
    fflush(stdout);
}

void USDRenderApplication::Run()
{
    if (args.listRenderDelegates)
    {
        PrintAvailableRenderDelegates();
    }

    if (args.listCameras)
    {
        PrintAvailableCameras();
    }

    if (args.listRenderDelegates)
    {
        Prepare();
        Render();
        StoreImage();
    }
}

void USDRenderApplication::LoadUSDScene(const std::string &usdFilePath)
{
    // Load scene
    stage = pxr::UsdStage::Open(usdFilePath);
    cameras = FindAvailableCameras();
}

void USDRenderApplication::StoreImage()
{
    float *mappedMem = (float *)renderBuffer->Map();

    // Write image to file.
    pxr::TfStopwatch writeTimer;
    writeTimer.Start();

    pxr::HioImageSharedPtr image = pxr::HioImage::OpenForWriting("out.exr");
    /*
        if (!image)
        {
        fprintf(stderr, "Unable to open output file for writing\n");
        return EXIT_FAILURE;
        }
    */
    pxr::HioImage::StorageSpec storage;
    storage.width = (int)renderBuffer->GetWidth();
    storage.height = (int)renderBuffer->GetHeight();
    storage.depth = (int)renderBuffer->GetDepth();
    storage.format = pxr::HioFormat::HioFormatFloat32Vec4;
    storage.flipped = true;
    storage.data = mappedMem;

    pxr::VtDictionary metadata;
    image->Write(storage, metadata);

    writeTimer.Stop();
    printf("Wrote image (%.3fs)\n", writeTimer.GetSeconds());
    fflush(stdout);

    renderBuffer->Unmap();
}

void USDRenderApplication::Resize(const nanogui::Vector2i size)
{
    std::cout << "USDRenderApplication::Resize = " << size << std::endl;
    resolution = size;

    pxr::GfVec4d viewport(0.f, 0.f, resolution.x(), resolution.y());
    renderPassState->SetViewport(viewport);
    // this->taskController->SetRenderViewport(viewport);
    /* */
    /* */
    if (sceneDelegate)
    {
        for (const auto &id : aovId)
        {
            pxr::HdRenderBufferDescriptor desc =
                renderSceneDelegate->GetRenderBufferDescriptor(id);
            std::cout << "id = " << id << std::endl;
            std::cout << "desc: dimension = " << desc.dimensions[0] << "\t" << desc.dimensions[1] << std::endl;
            desc.dimensions[0] = resolution.x();
            desc.dimensions[1] = resolution.y();
            // std::cout << "desc = " << desc. << std::endl;
            // sceneDelegate->SetReprFallback
            //((pxr::HdSceneDelegate*)sceneDelegate.get())->SetParameter(id,
            //     g_tokens->renderBufferDescriptor, desc);
            renderSceneDelegate->SetParameter(id,
                                        g_tokens->renderBufferDescriptor, desc);
            renderIndex->GetChangeTracker().MarkBprimDirty(id,
                                                           pxr::HdRenderBuffer::DirtyDescription);
            // GfVec4d viewport(0, 0, fbWidth, fbHeight);
            // internalSceneDelegate->SetCamera(renderPassState, viewport);
        }
    }
}