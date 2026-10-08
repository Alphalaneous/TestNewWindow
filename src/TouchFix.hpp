#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/CCTouchDispatcher.hpp>

using namespace geode::prelude;

class $modify(MyCCTouchDispatcher, CCTouchDispatcher) {

    CCArray* filterHandlers();
    void restoreHandlers(CCArray* handlers);
    void removeDelegate(CCTouchDelegate* delegate);
    void addTargetedDelegate(CCTouchDelegate* delegate, int priority, bool swallowsTouches);
    void touches(CCSet *pTouches, CCEvent *pEvent, unsigned int uIndex);
};