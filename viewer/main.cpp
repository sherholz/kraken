/*
    src/example1.cpp -- C++ version of an example application that shows
    how to use the various widget classes. For a Python implementation, see
    '../python/example1.py'.

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/
#include "USDRenderApplication.h"
#include "USDRenderApplicationWindow.h"

#include <iostream>
#include <memory>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#if defined(_MSC_VER)
#pragma warning(disable : 4505) // don't warn about dead code in stb_image.h
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

int main(int argc, char **argv)
{
    ApplicationParameter appPar(argc, argv);
    USDRenderApplication* app = new USDRenderApplication(appPar);
    if (!appPar.batch){
    try
    {
        nanogui::init();
        /* scoped variables */ {
            ref<USDRenderApplicationWindow> appWindow = new USDRenderApplicationWindow(app);
            appWindow->dec_ref();
            appWindow->set_visible(true);
            nanogui::run(RunMode::VSync);
        }

        nanogui::shutdown();
    }
    catch (const std::exception &e)
    {
        std::string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
#if defined(_WIN32)
        MessageBoxA(nullptr, error_msg.c_str(), NULL, MB_ICONERROR | MB_OK);
#else
        std::cerr << error_msg << std::endl;
#endif
        return -1;
    }
    catch (...)
    {
        std::cerr << "Caught an unknown error!" << std::endl;
    }
    }
    else 
    {
        app->Run();
        //app->Render();
        //app->StoreImage();
    }
    return 0;
}
