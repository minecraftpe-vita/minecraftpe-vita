#include "container.h"

#include "AppPlatform_Symbian.h"

int AppPlatform_Symbian::getScreenWidth() {
	fprintf(stdout, "get screen size of %p\n", CMcpeContainer::instance());
	return CMcpeContainer::instance()->Size().iWidth;
}
int AppPlatform_Symbian::getScreenHeight() {
	fprintf(stdout, "get screen size of %p\n", CMcpeContainer::instance());
	return CMcpeContainer::instance()->Size().iHeight;
}
