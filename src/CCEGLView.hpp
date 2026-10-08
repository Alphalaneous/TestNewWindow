#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/CCEGLView.hpp>

using namespace geode::prelude;

class $modify(MyCCEGLView, CCEGLView) {

    bool initGlew();
};