#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include "Window.hpp"

using namespace geode::prelude;

class $modify(MyMenuLayer, MenuLayer) {

	bool init() {
		if (!MenuLayer::init()) return false;

		for (int i = 0; i < 3; i++) {
			auto window = Window::create("Test");
			window->setID(fmt::format("window-{}", i + 1));

			auto btn = geode::Button::createWithLabel(fmt::format("Button {}", i + 1), "bigFont.fnt", [i] (auto self) {
				createQuickPopup("WOW!", fmt::format("Clicked button {} in another window", i + 1), "OK", nullptr, nullptr);
			});
			btn->setScale(2);
			btn->setPosition(window->getContentSize() / 2.f);
			
			window->addEventListener(Window::ResizeEvent(window), [btn] (CCSize size) {
				btn->setPosition(size / 2);
			});

			window->addChild(btn);

			addChild(window);
		}

		return true;
	}
};