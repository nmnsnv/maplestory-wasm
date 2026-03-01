//////////////////////////////////////////////////////////////////////////////
// This file is part of the Journey MMORPG client                           //
// Copyright © 2015-2016 Daniel Allendorf                                   //
//                                                                          //
// This program is free software: you can redistribute it and/or modify     //
// it under the terms of the GNU Affero General Public License as           //
// published by the Free Software Foundation, either version 3 of the       //
// License, or (at your option) any later version.                          //
//                                                                          //
// This program is distributed in the hope that it will be useful,          //
// but WITHOUT ANY WARRANTY; without even the implied warranty of           //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            //
// GNU Affero General Public License for more details.                      //
//                                                                          //
// You should have received a copy of the GNU Affero General Public License //
// along with this program.  If not, see <http://www.gnu.org/licenses/>.    //
//////////////////////////////////////////////////////////////////////////////
#include "Window.h"

#ifdef MS_PLATFORM_WASM
#include <emscripten.h>
#endif

#include "UI.h"

#include "../Console.h"
#include "../Constants.h"
#include "../Configuration.h"
#include "../Graphics/GraphicsGL.h"

namespace jrc
{
    Window::Window()
    {
        context = nullptr;
        glwnd = nullptr;
        opacity = 1.0f;
        opcstep = 0.0f;
        f11_down = false;
    }

    Window::~Window()
    {
        glfwTerminate();
    }

    void error_callback(int no, const char* description)
    {
        Console::get()
            .print("glfw error: " + std::string(description) + " (" + std::to_string(no) + ")");
    }

    void key_callback(GLFWwindow*, int key, int, int action, int)
    {
        UI::get().send_key(key, action != GLFW_RELEASE);
    }

    void mousekey_callback(GLFWwindow*, int button, int action, int)
    {
        switch (button)
        {
        case GLFW_MOUSE_BUTTON_LEFT:
            switch (action)
            {
            case GLFW_PRESS:
                UI::get().send_cursor(true);
                break;
            case GLFW_RELEASE:
                UI::get().send_cursor(false);
                break;
            default:
                break;
            }
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            switch (action)
            {
            case GLFW_PRESS:
                UI::get().rightclick();
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
    }

    void cursor_callback(GLFWwindow*, double xpos, double ypos)
    {
        auto x = static_cast<int16_t>(xpos);
        auto y = static_cast<int16_t>(ypos);
        Point<int16_t> pos = Point<int16_t>(x, y);
        UI::get().send_cursor(pos);
    }

    void scroll_callback(GLFWwindow*, double, double yoffset)
    {
        UI::get().send_scroll(yoffset);
    }

    void focus_callback(GLFWwindow*, int focused)
    {
        UI::get().send_focus(focused);
    }

    void close_callback(GLFWwindow* window)
    {
        UI::get().send_close();
        glfwSetWindowShouldClose(window, GL_FALSE);
    }

    void framebuffer_size_callback(GLFWwindow*, int width, int height)
    {
        if (width > 0 && height > 0)
        {
            Constants::set_viewsize(width, height);
            glViewport(0, 0, width, height);
            GraphicsGL::get().set_screensize(
                Constants::viewwidth(),
                Constants::viewheight()
            );
        }
    }

    Error Window::init()
    {
        fullscreen = Setting<Fullscreen>::get().load();

        if (!glfwInit())
        {
            return Error::GLFW;
        }

#ifdef MS_PLATFORM_WASM
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
        glwnd = glfwCreateWindow(
            Constants::VIEWWIDTH,
            Constants::VIEWHEIGHT,
            "Journey",
            fullscreen ? glfwGetPrimaryMonitor() : nullptr,
            nullptr
        );
        if (!glwnd) {
            return Error::WINDOW;
        }
        glfwMakeContextCurrent(glwnd);
        glfwSetErrorCallback(error_callback);
#else
        glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
        context = glfwCreateWindow(1, 1, "", nullptr, nullptr);
        glfwMakeContextCurrent(context);
        glfwSetErrorCallback(error_callback);
        glfwWindowHint(GLFW_VISIBLE, GL_TRUE);
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
#endif

        if (Error error = GraphicsGL::get().init())
        {
            return error;
        }

        return initwindow();
    }

    Error Window::initwindow()
    {
#ifndef MS_PLATFORM_WASM
        if (glwnd)
        {
            glfwDestroyWindow(glwnd);
        }

        glwnd = glfwCreateWindow(
            Constants::VIEWWIDTH,
            Constants::VIEWHEIGHT,
            "Journey",
            fullscreen ? glfwGetPrimaryMonitor() : nullptr,
            context
        );

        if (!glwnd)
        {
            return Error::WINDOW;
        }

        glfwMakeContextCurrent(glwnd);
#endif



        bool vsync = Setting<VSync>::get().load();
#ifndef MS_PLATFORM_WASM
        glfwSwapInterval(vsync ? 1 : 0);
#endif

        glViewport(0, 0, Constants::VIEWWIDTH, Constants::VIEWHEIGHT);
#ifndef MS_PLATFORM_WASM
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        glfwSetInputMode(glwnd, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        glfwSetInputMode(glwnd, GLFW_STICKY_KEYS, 1);
#endif
        glfwSetKeyCallback(glwnd, key_callback);
        glfwSetMouseButtonCallback(glwnd, mousekey_callback);
        glfwSetCursorPosCallback(glwnd, cursor_callback);
        glfwSetWindowFocusCallback(glwnd, focus_callback);
        glfwSetScrollCallback(glwnd, scroll_callback);
        glfwSetWindowCloseCallback(glwnd, close_callback);
        glfwSetFramebufferSizeCallback(glwnd, framebuffer_size_callback);

        int32_t framebuffer_width = 0;
        int32_t framebuffer_height = 0;
        glfwGetFramebufferSize(glwnd, &framebuffer_width, &framebuffer_height);
        if (framebuffer_width > 0 && framebuffer_height > 0)
        {
            framebuffer_size_callback(glwnd, framebuffer_width, framebuffer_height);
        }

        GraphicsGL::get().reinit();

#ifdef MS_PLATFORM_WASM
        // Prevent default browser behavior for Tab and Arrow keys
        EM_ASM({
            window.addEventListener("keydown", function(e) {
                // Tab (9), Left (37), Up (38), Right (39), Down (40), F11 (122)
                if ([9, 37, 38, 39, 40, 122].indexOf(e.keyCode) > -1) {
                    e.preventDefault();
                }
            }, false);
        });

        // Ask JS side to sync canvas size with the current viewport/fullscreen state.
        EM_ASM({
            if (window.MapleWasmUI && window.MapleWasmUI.applyScale) {
                window.MapleWasmUI.applyScale();
            }
        });
#endif

        return Error::NONE;
    }

    bool Window::not_closed() const
    {
        return glfwWindowShouldClose(glwnd) == 0;
    }

    void Window::update()
    {
        updateopc();
    }

    void Window::updateopc()
    {
        if (opcstep != 0.0f)
        {
            opacity += opcstep;

            if (opacity >= 1.0f)
            {
                opacity = 1.0f;
                opcstep = 0.0f;
            }
            else if (opacity <= 0.0f)
            {
                opacity = 0.0f;
                opcstep = -opcstep;

                fadeprocedure();
            }
        }
    }

    void Window::check_events()
    {
#ifdef MS_PLATFORM_WASM
        int32_t f11state = glfwGetKey(glwnd, GLFW_KEY_F11);
        if (f11state == GLFW_PRESS && !f11_down)
        {
            fullscreen = !fullscreen;
            EM_ASM({
                const canvas = Module && Module.canvas ? Module.canvas : null;
                if (!canvas) {
                    return;
                }

                const isCanvasFullscreen = document.fullscreenElement === canvas;
                const next = !isCanvasFullscreen;

                if (next) {
                    canvas.requestFullscreen({ navigationUI: "hide" }).catch(() => {});
                } else if (document.fullscreenElement) {
                    document.exitFullscreen().catch(() => {});
                }

                if (window.MapleWasmUI && window.MapleWasmUI.applyScale) {
                    setTimeout(window.MapleWasmUI.applyScale, 0);
                }
            });
        }
        f11_down = (f11state == GLFW_PRESS);
#else
        int32_t f11state = glfwGetKey(glwnd, GLFW_KEY_F11);
        if (f11state == GLFW_PRESS && !f11_down)
        {
            fullscreen = !fullscreen;
            initwindow();
        }
        f11_down = (f11state == GLFW_PRESS);
#endif
        glfwPollEvents();
    }

    void Window::begin() const
    {
        GraphicsGL::get().clearscene();
    }

    void Window::end() const
    {
        GraphicsGL::get().flush(opacity);
        glfwSwapBuffers(glwnd);
    }

    void Window::fadeout(float step, std::function<void()> fadeproc)
    {
        opcstep = -step;
        fadeprocedure = std::move(fadeproc);
    }

    void Window::setclipboard(const std::string& text) const
    {
        glfwSetClipboardString(glwnd, text.c_str());
    }

    std::string Window::getclipboard() const
    {
        const char* text = glfwGetClipboardString(glwnd);
        return text ? text : "";
    }
}
