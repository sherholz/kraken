#pragma once

#include <pxr/imaging/hd/engine.h>

#include <pxr/imaging/hd/task.h>
#include <pxr/imaging/hd/renderPass.h>
#include <pxr/imaging/hd/renderPassState.h>
#include <pxr/imaging/hd/rendererPluginRegistry.h>
#include <pxr/imaging/hd/renderBuffer.h>

#include <pxr/imaging/hdx/taskController.h>

#include <pxr/usdImaging/usdImaging/delegate.h>
#include <pxr/usd/usd/stage.h>

#include "RenderSceneDelegate.h"

#include <string>
#include <sstream>

#include <nanogui/vector.h>

struct ApplicationParameter
{
    std::string usdFilePath{""};
    std::string renderer{"Kraken"};
    // std::string renderer{"HdEmbreeRendererPlugin"};
    // std::string renderer{"HdEmbree2RendererPlugin"};
    // std::string renderer{"HdStormRendererPlugin"};

    std::string camera{"/camera"};
    bool batch{false};

    bool render{true};
    bool listCameras{true};
    bool listRenderDelegates{true};

    nanogui::Vector2f resolution{1920, 1080};

    ApplicationParameter() = default;
    ApplicationParameter(int argc, char **argv);

    std::string toString() const;
};

class USDRenderApplication
{

public:
    USDRenderApplication(const ApplicationParameter &args);

    inline nanogui::Vector2i getResolution() const{
        return resolution;
    }

    void Prepare();

    void Render();

    void Run();

    void Resize(const nanogui::Vector2i size);

    void Pause();

    void PrintAvailableCameras() const;

    void PrintAvailableRenderDelegates() const;

private:
    std::vector<pxr::SdfPath> FindAvailableCameras();

    void LoadUSDScene(const std::string &usdFilePath);

    void Intialize();

    void StoreImage();

    

    pxr::HdEngine engine;

    pxr::HdRenderDelegate* renderDelegate;

    pxr::HdRenderPassStateSharedPtr renderPassState;
    pxr::HdRenderPassSharedPtr renderPass;
    pxr::HdRenderBuffer *renderBuffer;
    pxr::HdRenderIndex* renderIndex;
    pxr::HdTaskSharedPtrVector tasks;

    std::unique_ptr<RenderSceneDelegate> renderSceneDelegate;
    std::unique_ptr<pxr::UsdImagingDelegate> sceneDelegate;
    //pxr::HdxTaskController* taskController;

    pxr::UsdStageRefPtr stage;
    std::vector<pxr::SdfPath> cameras;
    std::map<std::string, std::string> availableRenderDelegates;

    nanogui::Vector2i resolution;
    const ApplicationParameter& args;

    std::vector<pxr::SdfPath> aovId;
};