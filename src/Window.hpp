#pragma once

#include <GLFW/glfw3.h>
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class WindowNode;

class Window : public CCNode {
public:
    class ResizeEvent : public GlobalEvent<ResizeEvent, bool(CCSize size), Window*> {
        using GlobalEvent::GlobalEvent;
    };

    ~Window();

    static Window* create(ZStringView title);

    bool init(ZStringView title);
    void setMaximized(bool maximized);
    void setWindowPosition(int x, int y);
    void setWindowSize(int width, int height);
    void render();
    void onResize(const CCSize& size);
    void createFBO(int width, int height);
    bool isWindowMaximized();
    void drawFullscreenQuad(GLuint textureID);
    void safeSwapBuffers();

    void onEnter() override;
    void onExit() override;

    void visit() override;
    void setPosition(const CCPoint& position) override;
    void setScale(float scale) override;
    void setScaleX(float scale) override;
    void setScaleY(float scale) override;

    const CCAffineTransform nodeToParentTransform() override;
    const CCAffineTransform parentToNodeTransform() override;

    CCSize convertSizeToPoints(int width, int height);

    int m_width = 400;
    int m_height = 300;

    float m_mousePosX;
    float m_mousePosY;

    bool m_dirty;
    WNDPROC m_oldWndProc;

    GLFWwindow* m_window = nullptr;
    bool m_maximized = false;

    Ref<CCTouch> m_touch;
    Ref<CCSet> m_touchSet;

    int m_positionX;
    int m_positionY;

    GLuint m_fbo;
    GLuint m_fboTexture;
    GLuint m_fboRBO;

    float m_scaleX;
    float m_scaleY;
    float m_scale;

    GLuint m_texLocation;
    bool m_hasTexLocation;
    
    GLuint m_quadVAO = 0;
    GLuint m_quadVBO = 0;
    GLuint m_quadEBO = 0;
    GLuint m_shaderProgram = 0;

    std::string m_id;

    HWND m_handle;
    HGLRC m_context;
    HDC m_dc;
};