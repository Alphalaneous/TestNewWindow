#include "Window.hpp"
#include "WindowManager.hpp"
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int);

Window* Window::create(ZStringView title) {
    auto ret = new Window();
    if (ret->init(std::move(title))) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

Window::~Window() {
    auto wm = WindowManager::get();

    if (wm->getActiveWindow() == this) {
        wm->setActiveWindow(nullptr);
    }

    wm->removeWindow(m_window, m_handle);

    if (m_handle && m_dc) {
        ReleaseDC(m_handle, m_dc);
        m_dc = nullptr;
    }

    if (m_context) {
        wglDeleteContext(m_context);
        m_context = nullptr;
    }

    if (m_window) {
        glfwDestroyWindow(reinterpret_cast<GLFWwindow*>(m_window));
        m_window = nullptr;
    }

    wglMakeCurrent(wm->getMainDC(), wm->getMainContext());
}

void glfwErrorCallback(int error, const char* description) {
    log::info("GLFW Error {}: {}", error, description);
}

void glfwFramebufferSizeCallback(GLFWwindow* glfwWindow, int width, int height) {
    auto wm = WindowManager::get();
    auto window = wm->getWindowByGLFW(glfwWindow);

    window->m_width = width;
    window->m_height = height;
    window->m_dirty = true;

    auto size = window->convertSizeToPoints(width, height);

    window->setContentSize(size);
    window->onResize(size);
}

void glfwMouseButtonCallback(GLFWwindow* glfwWindow, int button, int action, int mods) {
    auto wm = WindowManager::get();
    auto window = wm->getWindowByGLFW(glfwWindow);

    if (button == GLFW_MOUSE_BUTTON_1) {
        auto size = CCDirector::get()->getWinSize();
        auto diff = std::max(0.f, window->m_height / window->m_scale - size.height);

        auto pos = CCPoint(window->m_mousePosX, window->m_mousePosY - diff);

        switch (action) {
            case 1: {
                wm->setActiveWindow(window);
                
                window->m_touch = new CCTouch();
                window->m_touch->autorelease();
                window->m_touch->setTouchInfo(0, pos.x, pos.y);

                window->m_touchSet = CCSet::create();
                window->m_touchSet->addObject(window->m_touch);

                CCTouchDispatcher::get()->touchesBegan(window->m_touchSet, nullptr);
                break;
            }
            case 0: {
                window->m_touch->setTouchInfo(0, pos.x, pos.y);
                CCTouchDispatcher::get()->touchesEnded(window->m_touchSet, nullptr);

                wm->setActiveWindow(nullptr);
                break;
            }
        }
    }
}

void glfwMousePositionCallback(GLFWwindow* glfwWindow, double xPos, double yPos) {
    auto wm = WindowManager::get();
    auto window = wm->getWindowByGLFW(glfwWindow);

    auto realPos = window->convertSizeToPoints(xPos, window->m_height - yPos);

    window->m_mousePosX = realPos.width;
    window->m_mousePosY = realPos.height;

    auto size = CCDirector::get()->getWinSize();
    auto diff = std::max(0.f, window->m_height / window->m_scale - size.height);

    auto pos = CCPoint(window->m_mousePosX, window->m_mousePosY - diff);

    if (window->m_touch) {
        window->m_touch->setTouchInfo(0, pos.x, pos.y);
        CCTouchDispatcher::get()->touchesMoved(window->m_touchSet, nullptr);
    }
}

bool Window::init(ZStringView title) {
    if (!CCNode::init()) return false;

    glfwSetErrorCallback(glfwErrorCallback);
    
    m_window = glfwCreateWindow(m_width, m_height, title.c_str(), nullptr, nullptr);

    if (!m_window) {
        log::info("failed create window");
        return false;
    }

    auto wm = WindowManager::get();

    m_handle = glfwGetWin32Window(m_window);
    m_dc = GetDC(m_handle);

    int pf = GetPixelFormat(wm->getMainDC());
    PIXELFORMATDESCRIPTOR pfd;
    DescribePixelFormat(wm->getMainDC(), pf, sizeof(pfd), &pfd);
    SetPixelFormat(m_dc, pf, &pfd);

    m_context = wglCreateContext(m_dc);

    wglShareLists(wm->getMainContext(), m_context);

    wglMakeCurrent(m_dc, m_context);

    auto swapInterval = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    if (swapInterval) swapInterval(0);

    glfwSetFramebufferSizeCallback(m_window, glfwFramebufferSizeCallback);
    glfwSetMouseButtonCallback(m_window, glfwMouseButtonCallback);
    glfwSetCursorPosCallback(m_window, glfwMousePositionCallback);

    static float quadVertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };
    static unsigned int quadIndices[] = {0, 1, 2, 2, 3, 0};

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glGenBuffers(1, &m_quadEBO);

    GLint oldVAO;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &oldVAO);

    glBindVertexArray(m_quadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(oldVAO);

    static const char* vsSrc = R"(
    #version 150 core
    in vec2 position;
    in vec2 texCoord;
    out vec2 vTexCoord;
    void main() {
        vTexCoord = texCoord;
        gl_Position = vec4(position, 0.0, 1.0);
    }
    )";
    static const char* fsSrc = R"(
    #version 150 core
    in vec2 vTexCoord;
    out vec4 fragColor;
    uniform sampler2D tex;
    void main() {
        fragColor = texture(tex, vTexCoord);
    }
    )";

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsSrc, nullptr);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsSrc, nullptr);
    glCompileShader(fs);

    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vs);
    glAttachShader(m_shaderProgram, fs);

    glBindAttribLocation(m_shaderProgram, 0, "position");
    glBindAttribLocation(m_shaderProgram, 1, "texCoord");
    glLinkProgram(m_shaderProgram);

    glDeleteShader(vs);
    glDeleteShader(fs);

    createFBO(m_width, m_height);

    wglMakeCurrent(wm->getMainDC(), wm->getMainContext());

    setContentSize(convertSizeToPoints(m_width, m_height));

    return true;
}

void Window::onEnter() {
    CCNode::onEnter();
    auto wm = WindowManager::get();
    wm->addWindow(this, m_window, m_handle);
}

void Window::onExit() {
    CCNode::onExit();
    auto wm = WindowManager::get();

    if (wm->getActiveWindow() == this) {
        wm->setActiveWindow(nullptr);
    }

    wm->removeWindow(m_window, m_handle);
}

void Window::visit() {}

void Window::setPosition(const CCPoint& position) {
    CCNode::setPosition({0, 0});
}

void Window::setScale(float scale) {
    CCNode::setScale(1);
}

void Window::setScaleX(float scale) {
    CCNode::setScaleX(1);
}

void Window::setScaleY(float scale) {
    CCNode::setScaleY(1);
}

const CCAffineTransform Window::nodeToParentTransform() {
    auto transform = CCNode::nodeToParentTransform();

    auto parent = getParent();
    if (!parent) return transform;
    auto nodeSpace = parent->convertToNodeSpace({0, 0});

    transform.tx = nodeSpace.x;
    transform.ty = nodeSpace.y;

    return transform;
}

const CCAffineTransform Window::parentToNodeTransform() {
    return CCAffineTransformIdentity;
}

void Window::drawFullscreenQuad(GLuint textureID) { 
    glUseProgram(m_shaderProgram); 
    glActiveTexture(GL_TEXTURE0); 
    glBindTexture(GL_TEXTURE_2D, textureID); 

    if (!m_hasTexLocation) {
        m_texLocation = glGetUniformLocation(m_shaderProgram, "tex");
        m_hasTexLocation = true;
    }

    glUniform1i(m_texLocation, 0); 
    GLint oldVAO;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &oldVAO);
    glBindVertexArray(m_quadVAO); 
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); 
    glBindVertexArray(oldVAO); 
}

void Window::createFBO(int width, int height) {
    auto view = CCDirector::get()->getOpenGLView();
    auto design = view->getDesignResolutionSize();

    m_scaleX = m_width / design.width;
    m_scaleY = m_height / design.height;
    m_scale = std::min(m_scaleX, m_scaleY);

    GLint oldFBO;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFBO);

    if (m_fbo) glDeleteFramebuffers(1, &m_fbo);
    if (m_fboTexture) glDeleteTextures(1, &m_fboTexture);
    if (m_fboRBO) glDeleteRenderbuffers(1, &m_fboRBO);
    
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glGenTextures(1, &m_fboTexture);
    glBindTexture(GL_TEXTURE_2D, m_fboTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fboTexture, 0);

    glGenRenderbuffers(1, &m_fboRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, m_fboRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_fboRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        log::info("FBO not complete!");
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, oldFBO);
}

void Window::safeSwapBuffers() {
    glfwSwapBuffers(m_window);
}

void Window::render() {
    if (m_dirty) {
        m_dirty = false;
        wglMakeCurrent(m_dc, m_context);
        createFBO(m_width, m_height);
        safeSwapBuffers();
    }

    auto wm = WindowManager::get();

    wglMakeCurrent(wm->getMainDC(), wm->getMainContext());

    GLint oldViewport[4];
    glGetIntegerv(GL_VIEWPORT, oldViewport);

    GLint oldFBO;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFBO);

    kmMat4 oldProj, oldModel;
    kmGLGetMatrix(KM_GL_PROJECTION, &oldProj);
    kmGLGetMatrix(KM_GL_MODELVIEW, &oldModel);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, m_width, m_height);

    kmMat4 proj;
    kmMat4OrthographicProjection(&proj, 0, m_width / m_scale, 0, m_height / m_scale, -1024, 1024);
    kmGLMatrixMode(KM_GL_PROJECTION);
    kmGLLoadMatrix(&proj);

    kmGLMatrixMode(KM_GL_MODELVIEW);
    kmGLLoadIdentity();

    CCNode::visit();

    kmGLMatrixMode(KM_GL_PROJECTION);
    kmGLLoadMatrix(&oldProj);

    kmGLMatrixMode(KM_GL_MODELVIEW);
    kmGLLoadMatrix(&oldModel);

    glBindFramebuffer(GL_FRAMEBUFFER, oldFBO);
    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);

    wglMakeCurrent(m_dc, m_context);

    GLint oldVAO;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &oldVAO);

    glViewport(0, 0, m_width, m_height);
    drawFullscreenQuad(m_fboTexture);

    glBindVertexArray(oldVAO);
    safeSwapBuffers();

    wglMakeCurrent(wm->getMainDC(), wm->getMainContext());
}

bool Window::isWindowMaximized() {
    if (!m_handle) return false;

    WINDOWPLACEMENT placement = {};
    placement.length = sizeof(WINDOWPLACEMENT);

    if (!GetWindowPlacement(m_handle, &placement)) {
        return false;
    }

    return placement.showCmd == SW_MAXIMIZE;
}

void Window::setMaximized(bool maximized) {
    m_maximized = maximized;
    auto window = reinterpret_cast<GLFWwindow*>(m_window);
    if (m_maximized) {
        ShowWindow(m_handle, SW_MAXIMIZE);
    }
    else {
        ShowWindow(m_handle, SW_RESTORE);
    }
}

void Window::setWindowPosition(int x, int y) {
    m_positionX = x;
    m_positionY = y;

    auto window = reinterpret_cast<GLFWwindow*>(m_window);
    glfwSetWindowPos(window, x, y);
}

CCSize Window::convertSizeToPoints(int width, int height) {
    return {width / m_scale, height / m_scale};
}

void Window::setWindowSize(int width, int height) {
    auto window = reinterpret_cast<GLFWwindow*>(m_window);

    m_width = width;
    m_height = height;

    if (m_maximized) {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        setContentSize(convertSizeToPoints(m_width, m_height));
        return;
    }

    setContentSize(convertSizeToPoints(m_width, m_height));
    glfwSetWindowSize(window, width, height);
}

void Window::onResize(const CCSize& size) {
    ResizeEvent(this).send(size);
}
