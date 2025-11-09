#pragma once

#include <pxr/imaging/hd/engine.h>

#include <pxr/imaging/hd/task.h>
#include <pxr/imaging/hd/renderPass.h>
#include <pxr/imaging/hd/renderPassState.h>
#include <pxr/imaging/hd/rendererPluginRegistry.h>
#include <pxr/imaging/hd/renderBuffer.h>

#include <pxr/usd/usd/stage.h>

#include <string>
#include <sstream>

#include <nanogui/vector.h>

struct ApplicationParameter
{
    std::string usdFilePath{""};
    std::string renderer{"HdKrakenRendererPlugin"};
    // std::string renderer{"HdEmbreeRendererPlugin"};
    // std::string renderer{"HdEmbree2RendererPlugin"};
    // std::string renderer{"HdStormRendererPlugin"};

    std::string camera{"/camera"};
    bool batch{false};
    nanogui::Vector2f resolution{1920, 1080};

    ApplicationParameter() = default;
    ApplicationParameter(int argc, char **argv);

    std::string toString() const;
};

class USDRenderApplication
{

public:
    USDRenderApplication(const ApplicationParameter &args);

    void Resize(const nanogui::Vector2i size);

    void Pause();

private:
    std::vector<pxr::SdfPath> FindAvailableCameras();

    void LoadUSDScene(const std::string &usdFilePath);

    void Intialize();

    void StoreImage();

    pxr::HdEngine engine;
    pxr::HdRenderPassStateSharedPtr renderPassState;
    pxr::HdRenderPassSharedPtr renderPass;
    pxr::HdRenderBuffer *renderBuffer;
    pxr::HdTaskSharedPtrVector tasks;

    pxr::UsdStageRefPtr stage;
    std::vector<pxr::SdfPath> cameras;
};