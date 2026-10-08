#include "CCEGLView.hpp"
#include "WindowManager.hpp"

bool MyCCEGLView::initGlew() {
    if (!CCEGLView::initGlew()) return false;

    WindowManager::get()->setupWindowManager();

    return true;
}