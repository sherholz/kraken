#pragma once

#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/layout.h>

#include <nanogui/texture.h>
#include <nanogui/shader.h>
#include <nanogui/renderpass.h>

#include "USDRenderApplication.h"

using namespace nanogui;

class USDRenderApplicationWindow : public Screen
{
public:
    USDRenderApplicationWindow(const ApplicationParameter &appPar);

    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override;

    virtual void draw(NVGcontext *ctx) override;

    virtual void draw_contents() override;

    bool resize_event(const Vector2i &size) override;

private:
    // ProgressBar *m_progress;
    ref<Shader> m_shader;
    ref<RenderPass> m_render_pass;

    ref<Texture> m_frame_buffer;

    USDRenderApplication *app;
    ApplicationParameter appPar;
};