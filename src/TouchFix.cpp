#include "TouchFix.hpp"
#include "Window.hpp"
#include "WindowManager.hpp"

static std::set<CCTouchDelegate*> s_removedDelegates;

void MyCCTouchDispatcher::removeDelegate(CCTouchDelegate* delegate) {
    s_removedDelegates.insert(delegate);
    CCTouchDispatcher::removeDelegate(delegate);
}

void MyCCTouchDispatcher::addTargetedDelegate(CCTouchDelegate* delegate, int priority, bool swallowsTouches) {
    s_removedDelegates.erase(delegate);
    CCTouchDispatcher::addTargetedDelegate(delegate, priority, swallowsTouches);
}

CCArray* MyCCTouchDispatcher::filterHandlers() {
    auto removed = CCArray::create();
    auto wm = WindowManager::get();

    for (auto handler : CCArrayExt<CCTargetedTouchHandler, false>(m_pTargetedHandlers)) {
        auto node = typeinfo_cast<CCNode*>(handler->getDelegate());
        if (!node) continue;

        auto activeWindow = wm->getActiveWindow();
        auto window = node->getParentByType<Window>();
        if (window == activeWindow) {
            continue;
        }
        removed->addObject(handler);
    }

    m_pTargetedHandlers->removeObjectsInArray(removed);
    return removed;
}

void MyCCTouchDispatcher::restoreHandlers(CCArray* handlers) {
    for (auto handler : CCArrayExt<CCTargetedTouchHandler, false>(handlers)) {
        if (s_removedDelegates.contains(handler->getDelegate())) continue;

        bool exists = m_pTargetedHandlers->containsObject(handler);
        if (exists) continue;

        m_pTargetedHandlers->addObject(handler);
    }
}

void MyCCTouchDispatcher::touches(CCSet *pTouches, CCEvent *pEvent, unsigned int uIndex) {
    Ref<CCArray> handlersCopy = m_pTargetedHandlers->shallowCopy();
    auto touchesCopy = pTouches->mutableCopy();

    auto removed = filterHandlers();
    CCTouchDispatcher::touches(touchesCopy, pEvent, uIndex);
    restoreHandlers(removed);

    s_removedDelegates.clear();
}