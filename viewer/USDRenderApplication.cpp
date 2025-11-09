#include "USDRenderApplication.h"

#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/camera.h>

#include <pxr/imaging/hio/image.h>

#include <string>
#include <sstream>
#include <iostream>

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

    std::cout << "read config" << std::endl;

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

USDRenderApplication::USDRenderApplication(const ApplicationParameter &args)
{
    Intialize();
    LoadUSDScene(args.usdFilePath);
}

void USDRenderApplication::Intialize()
{
    pxr::HdRendererPluginRegistry &pluginRegistry = pxr::HdRendererPluginRegistry::GetInstance();
    // Get available RenderDelegates
    pxr::HfPluginDescVector pluginDescs;
    pluginRegistry.GetPluginDescs(&pluginDescs);

    if (pluginDescs.size() > 0)
    {
        std::cout << "Renderers:" << std::endl;
        for (size_t i = 0; i < pluginDescs.size(); ++i)
        {
            // if (pluginDescs[i].displayName == defaultRendererDisplayName
            //||  pluginDescs[i].id == defaultRendererDisplayName) {
            //     return pluginDescs[i].id;
            // }
            std::cout << "\t renderer[" << i << "]: displayName = " << pluginDescs[i].displayName << "\t id = " << pluginDescs[i].id << std::endl;
        }
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

    if (sceneCameras.size() > 0)
    {
        std::cout << "Scene cameras:" << std::endl;
        for (int i = 0; i < sceneCameras.size(); i++)
        {
            std::cout << "\t camera[" << i << "] = " << sceneCameras[i] << std::endl;
        }
    }

    return sceneCameras;
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
    std::cout << "USDRenderApplication::Resize" << std::endl;
}