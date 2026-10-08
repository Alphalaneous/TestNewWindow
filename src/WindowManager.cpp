#include "WindowManager.hpp"
#include <Geode/modify/CCEGLView.hpp>

class $modify(CCEGLView) {
    virtual void swapBuffers() {
        CCEGLView::swapBuffers();
        for (const auto& [k, v] : WindowManager::get()->m_windowsByGLFW) {
            v->render();
        }
    }
};

WindowManager* WindowManager::get() {
    static WindowManager instance;
    return &instance;
}

HGLRC WindowManager::getMainContext() {
    return m_mainContext;
}

HDC WindowManager::getMainDC() {
    return m_mainDC;
}

void WindowManager::setupWindowManager() {
    if (!glfwInit()) {
        log::info("Failed to init glfw :(");
    }
    m_mainDC = wglGetCurrentDC();
    m_mainContext = wglCreateContext(m_mainDC);

    wglMakeCurrent(m_mainDC, m_mainContext);
}

bool WindowManager::isCheckingWindow() {
    return m_checkWindow;
}

void WindowManager::shouldCheckWindow(bool check) {
    m_checkWindow = check;
}

void WindowManager::setActiveWindow(Window* window) {
    m_activeWindow = window;
}

Window* WindowManager::getActiveWindow() {
    return m_activeWindow;
}

void WindowManager::addWindow(Window* window, GLFWwindow* glfwWindow, HWND handle) {
    m_windowsByGLFW[glfwWindow] = window;
    m_windowsByHWND[handle] = window;
}

void WindowManager::removeWindow(GLFWwindow* glfwWindow, HWND handle) {
    m_windowsByGLFW.erase(glfwWindow);
    m_windowsByHWND.erase(handle);
}

Window* WindowManager::getWindowByGLFW(GLFWwindow* glfwWindow) {
    return m_windowsByGLFW[glfwWindow];
}

Window* WindowManager::getWindowByHandle(HWND handle) {
    return m_windowsByHWND[handle];
}