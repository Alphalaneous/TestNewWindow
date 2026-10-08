#pragma once

#include "GLFW/glfw3.h"
#include "Window.hpp"

class WindowManager {
public:
    static WindowManager* get();

    HGLRC getMainContext();
    HDC getMainDC();

    void setupWindowManager();

    bool isCheckingWindow();
    void shouldCheckWindow(bool check);

    void setActiveWindow(Window* window);
    Window* getActiveWindow();

    void addWindow(Window* window, GLFWwindow* glfwWindow, HWND handle);
    void removeWindow(GLFWwindow* glfwWindow, HWND handle);

    Window* getWindowByGLFW(GLFWwindow* glfwWindow);
    Window* getWindowByHandle(HWND handle);

    HGLRC m_mainContext = nullptr;
    HDC m_mainDC;

    std::unordered_map<GLFWwindow*, Window*> m_windowsByGLFW;
    std::unordered_map<HWND, Window*> m_windowsByHWND;
    Window* m_activeWindow;

    bool m_checkWindow;
};