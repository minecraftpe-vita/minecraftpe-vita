#include "container.h"

#include "AppPlatform_Symbian.h"

#include <escapeutils.h>

static inline CMcpeContainer *container() { return CMcpeContainer::instance(); }


int AppPlatform_Symbian::getScreenWidth() { return container()->Size().iWidth; }

int AppPlatform_Symbian::getScreenHeight() { return container()->Size().iHeight; }

void AppPlatform_Symbian::showKeyboard(std::string defaultText, int maxLength) {
	iImeRequested = true;

	iBuffer = defaultText;
	if (!container()->PromptTextL(iBuffer, maxLength)) {
		iBuffer = defaultText;
	}
}

bool AppPlatform_Symbian::isKeyboardVisible() {
	if (iImeRequested && !container()->IsImeShown()) {
		iImeRequested = false;
		return true;
	}
	return container()->IsImeShown();
}
