#include "container.h"

#include "AppPlatform_Symbian.h"

static inline CMcpeContainer *container() { return CMcpeContainer::instance(); }


int AppPlatform_Symbian::getScreenWidth() { return container()->Size().iWidth; }

int AppPlatform_Symbian::getScreenHeight() { return container()->Size().iHeight; }

void AppPlatform_Symbian::showKeyboard() {
	if (!container()->PromptTextL(iBuffer)) { iBuffer = ""; }
}

bool AppPlatform_Symbian::isKeyboardVisible() { return container()->IsImeShown(); }
