#include "USDRenderApplicationWindow.h"
#include <nanogui/opengl.h>
#include <string>
#include <iostream>

#include "USDRenderApplication.h"

std::string vertex_shader{
#if defined(NANOGUI_USE_OPENGL)
    R"(/* Vertex shader */
            #version 330

            in vec3 position;
            out vec2 uv;

            void main() {
                vec4 p = vec4(position, 1.0);
                gl_Position = p;
                uv = vec2((position.x+1.f)/2.f,(position.y+1.f)/2.f);
            })"
#elif defined(NANOGUI_USE_GLES)
    R"(/* Vertex shader */
            precision highp float;

            attribute vec3 position;
            varying vec2 uv;

            void main() {
                vec4 p = vec4(position, 1.0);
                gl_Position = p;
                uv = vec2((position.x+1.f)/2.f,(position.y+1.f)/2.f);
            })"
#elif defined(NANOGUI_USE_METAL)
    R"(
            #include <metal_stdlib>

            using namespace metal;

            struct VertexOut {
                float4 position [[position]];
                float2 uv;
            };

            vertex VertexOut vertex_main(const device packed_float3 *position,
                                        uint id [[vertex_id]]) {
                float4 p = float4(position[id], 1.f);
                VertexOut vert;
                vert.position = p;
                vert.uv = float2((p.x+1.f)/2.f,(p.y+1.f)/2.f);
                return vert;
            })"
#endif
};

std::string fragment_shader{
#if defined(NANOGUI_USE_OPENGL)
    R"(/* Fragment shader */
            #version 330

            in vec2 uv;
            out vec4 frag_color;
            uniform sampler2D image;

            void main() {
                vec4 value = texture(image, uv);
                frag_color = value;
            })"
#elif defined(NANOGUI_USE_GLES)
    R"(/* Fragment shader */
            precision highp float;

            varying vec2 uv;
            uniform sampler2D image;
            void main() {
                vec4 value = texture2D(image, uv);
                gl_FragColor = value;
            })"
#elif defined(NANOGUI_USE_METAL)
    /* Fragment shader */
    R"(#include <metal_stdlib>

            using namespace metal;

            struct VertexOut {
                float4 position [[position]];
                float2 uv;
            };

            fragment float4 fragment_main(VertexOut vert [[stage_in]],
                                        texture2d<float, access::sample> image,
                                        sampler image_sampler
                                        ) {
                float4 value = image.sample(image_sampler, vert.uv);       
                //return float4(vert.uv.x,vert.uv.y, 0.f, 1.f);
                return value;
            })"
#endif
};

USDRenderApplicationWindow::USDRenderApplicationWindow(USDRenderApplication* app) : Screen(app->getResolution() / 2, "NanoGUI Test")
{
    inc_ref();
    Vector2i viewport_size = app->getResolution();
    this->m_frame_buffer = new Texture(
        Texture::PixelFormat::RGBA,
        Texture::ComponentFormat::Float32,
        viewport_size,
        Texture::InterpolationMode::Nearest,
        Texture::InterpolationMode::Nearest);

    float *img_data = new float[viewport_size.x() * viewport_size.y() * 4];
    for (int x = 0; x < viewport_size.x(); x++)
    {
        for (int y = 0; y < viewport_size.y(); y++)
        {
            int idx = (y * viewport_size.x() + x) * 4;
            img_data[idx + 0] = (float)x / (float)viewport_size.x();
            img_data[idx + 1] = (float)y / (float)viewport_size.y();
            img_data[idx + 2] = 1.f;
            img_data[idx + 3] = 1.f;
        }
    }
    this->m_frame_buffer->upload((const uint8_t *)img_data);
    delete[] img_data;

    m_render_pass = new RenderPass({this});
    m_render_pass->set_clear_color(0, Color(0.3f, 0.3f, 0.32f, 1.f));

    m_shader = new Shader(
        m_render_pass,
        "framebuffer_shader",
        vertex_shader,
        fragment_shader);

    const uint32_t indices[3 * 2] = {
        0, 1, 2,
        2, 3, 0};

    const float positions[3 * 4] = {
        -0.5f, -0.5f, 0.f,
        0.5f, -0.5f, 0.f,
        0.5f, 0.5f, 0.f,
        -0.5f, 0.5f, 0.f};

    m_shader->set_buffer("indices", VariableType::UInt32, {3 * 2}, indices);
    m_shader->set_buffer("position", VariableType::Float32, {4, 3}, positions);

    this->app = app;
    app->Prepare();
}

bool USDRenderApplicationWindow::keyboard_event(int key, int scancode, int action, int modifiers)
{
    if (Screen::keyboard_event(key, scancode, action, modifiers))
        return true;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        set_visible(false);
        return true;
    }
    return false;
}

void USDRenderApplicationWindow::draw(NVGcontext *ctx)
{
    /* Draw the user interface */
    Screen::draw(ctx);
}

void USDRenderApplicationWindow::draw_contents()
{

    std::cout << "framebuffer_size" << framebuffer_size() << std::endl;
    app->Render();

    m_shader->set_texture("image", m_frame_buffer);
    m_render_pass->resize(framebuffer_size());
    m_render_pass->begin();

    m_shader->begin();
    m_shader->draw_array(Shader::PrimitiveType::Triangle, 0, 6, true);
    m_shader->end();

    m_render_pass->end();

    if (m_frame_index % 60 == 59)
    {
        char caption[128];
        snprintf(caption, 128, "USD Render View (%.2f FPS)", 1.f / m_frame_timer.value());
        set_caption(caption);
    }
}

bool USDRenderApplicationWindow::resize_event(const Vector2i &size)
{
    std::cout << "resize: " << size << std::endl;
    std::cout << "framebuffer_size: " << framebuffer_size() << std::endl;

    Vector2i real_size = size * 2;
    this->m_frame_buffer->resize(real_size);

    float *img_data = new float[real_size.x() * real_size.y() * 4];
    for (int x = 0; x < real_size.x(); x++)
    {
        for (int y = 0; y < real_size.y(); y++)
        {
            int idx = (y * real_size.x() + x) * 4;
            img_data[idx + 0] = (float)x / (float)real_size.x();
            img_data[idx + 1] = (float)y / (float)real_size.y();
            img_data[idx + 2] = 1.f;
            img_data[idx + 3] = 1.f;
        }
    }
    this->m_frame_buffer->upload((const uint8_t *)img_data);
    delete[] img_data;
    /*
    this->m_frame_buffer = new Texture(
            Texture::PixelFormat::RGBA,
            Texture::ComponentFormat::Float32,
            size,
            Texture::InterpolationMode::Nearest,
            Texture::InterpolationMode::Nearest);
    */
    app->Resize(real_size);
    return true;
}