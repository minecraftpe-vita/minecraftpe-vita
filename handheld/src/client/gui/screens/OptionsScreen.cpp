#include "OptionsScreen.h"

#include "StartMenuScreen.h"
#include "DialogDefinitions.h"
#include "../../Minecraft.h"
#include "../../../AppPlatform.h"
#include "../../../platform/input/Mouse.h"

#include "../components/OptionsPane.h"
#include "../components/ImageButton.h"
#include "../components/OptionsGroup.h"
#include "../components/TextBox.h"


OptionsScreen::OptionsScreen()
: bClose(NULL),
  bHeader(NULL),
  bNextPage(NULL),
  bPrevPage(NULL),
  optionPane(NULL) {
	currentPage = 0;
	maxPages = 3;
}

OptionsScreen::~OptionsScreen() {
	if(bClose != NULL) {
		delete bClose;
		bClose = NULL;
	}
	if(bHeader != NULL) {
		delete bHeader;
		bHeader = NULL;
	}
	if(bNextPage != NULL) {
		delete bNextPage;
		bNextPage = NULL;
	}
	if(bPrevPage != NULL) {
		delete bPrevPage;
		bPrevPage = NULL;
	}
	if(optionPane != NULL) {
		delete optionPane;
		optionPane = NULL;
	}
}

void OptionsScreen::init() {
	bHeader = new Touch::THeader(0, "Options");
	if(minecraft->useTouchscreen()) {
		bPrevPage = new Touch::TButton(201, "<");
		bNextPage = new Touch::TButton(202, ">");

		// set imagebutton on bClose ..
		bClose= new ImageButton(200, "");

		ImageDef def;
		def.name = "gui/touchgui.png";
		def.width = 34;
		def.height = 26;

		def.setSrc(IntRectangle(150, 0, (int)def.width, (int)def.height));
		((ImageButton*)bClose)->setImageDef(def, true);
	}
	else {
		bPrevPage = new Button(201, "<");
		bNextPage = new Button(202, ">");
		bClose = new Button(200, "Close");
	}

	buttons.push_back(bHeader);
	buttons.push_back(bPrevPage);
	buttons.push_back(bNextPage);
	buttons.push_back(bClose);

	tabButtons.push_back(bClose);
	tabButtons.push_back(bPrevPage);
	tabButtons.push_back(bNextPage);

	generateOptionScreens();
}

void OptionsScreen::setupPositions() {
	if(!minecraft->useTouchscreen()) {
		bClose->width = 70;
	}

	bClose->x = width - bClose->width;
	bClose->y = 0;

	bPrevPage->width = 40;
	bPrevPage->height = bClose->height;
	bPrevPage->x = 20;
	bPrevPage->y = height - bPrevPage->height - 10;

	bNextPage->width = 40;
	bNextPage->height = bClose->height;
	bNextPage->x = width - bNextPage->width - 20;
	bNextPage->y = height - bNextPage->height - 10;

	bHeader->x = 0;
	bHeader->y = 0;
	bHeader->width = width - bClose->width;
	bHeader->height = bClose->height;

	if (optionPane != NULL) {
		int paneWidth = (width > 400) ? 360 : 260; 
		if (paneWidth > width - 40) paneWidth = width - 40;
		optionPane->width = paneWidth;
		optionPane->x = (width - paneWidth) / 2; 
		optionPane->y = bHeader->height;
		optionPane->height = height - bHeader->height - bPrevPage->height - 15;
		optionPane->setupPositions();
	}
}

void OptionsScreen::render( int xm, int ym, float a ) {
	renderBackground();
	int xmm = xm * width / minecraft->width;
	int ymm = ym * height / minecraft->height - 1;
	if(optionPane != NULL)
		optionPane->render(minecraft, xmm, ymm);
	super::render(xm, ym, a);
}

void OptionsScreen::removed()
{
}

void OptionsScreen::buttonClicked( Button* button ) {
	if(button == bClose) {
		// we should really .. only save when closing the menu?
		minecraft->options.save();

		minecraft->reloadOptions();
		minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
	} else if (button == bPrevPage) {
		if (currentPage > 0) {
			currentPage--;
		} else {
			currentPage = maxPages - 1;
		}
		generateOptionScreens();
	} else if (button == bNextPage) {
		if (currentPage < maxPages - 1) {
			currentPage++;
		} else {
			currentPage = 0;
		}
		generateOptionScreens();
	}
}

void OptionsScreen::generateOptionScreens() {
	if (optionPane != NULL) {
		delete optionPane;
	}
	optionPane = new OptionsPane();
	char buf[32];
	sprintf(buf, "Options (%d/%d)", currentPage + 1, maxPages);
	if (bHeader) bHeader->msg = buf;
	
	if (currentPage == 0) {
		optionPane->createOptionsGroup("options.group.video")
			.addOptionItem(&Options::Option::GRAPHICS, minecraft)
			.addOptionItem(&Options::Option::RENDER_DISTANCE, minecraft)
			.addOptionItem(&Options::Option::AMBIENT_OCCLUSION, minecraft)
			.addOptionItem(&Options::Option::VIEW_BOBBING, minecraft)
			.addOptionItem(&Options::Option::ANAGLYPH, minecraft)
			.addOptionItem(&Options::Option::RENDER_DEBUG, minecraft)
			.addOptionItem(&Options::Option::LIMIT_FRAMERATE, minecraft)
			.addOptionItem(&Options::Option::GUI_SCALE, minecraft);
	} else if (currentPage == 1) {
		optionPane->createOptionsGroup("options.group.game")
			.addOptionItem(&Options::Option::DIFFICULTY, minecraft)
			.addOptionItem(&Options::Option::THIRD_PERSON, minecraft)
			.addOptionItem(&Options::Option::HIDE_GUI, minecraft);

		optionPane->createOptionsGroup("options.group.multiplayer")
			.addOptionItem(&Options::Option::SERVER_VISIBLE, minecraft)
			.addOptionItem(&Options::Option::USERNAME, minecraft);
	} else if (currentPage == 2) {
		optionPane->createOptionsGroup("options.group.control")
			.addOptionItem(&Options::Option::SENSITIVITY, minecraft)
			.addOptionItem(&Options::Option::INVERT_MOUSE, minecraft)
			.addOptionItem(&Options::Option::LEFT_HANDED, minecraft)
			.addOptionItem(&Options::Option::USE_TOUCHSCREEN, minecraft)
			.addOptionItem(&Options::Option::USE_TOUCH_JOYPAD, minecraft)
			.addOptionItem(&Options::Option::DESTROY_VIBRATION, minecraft)
			.addOptionItem(&Options::Option::AUTO_JUMP, minecraft);

		optionPane->createOptionsGroup("options.group.audio")
			.addOptionItem(&Options::Option::SOUND, minecraft);
	}
	
	if (optionPane != NULL) {
		this->setupPositions();
	}
}

void OptionsScreen::mouseClicked( int x, int y, int buttonNum ) {
	if(optionPane != NULL)
		optionPane->mouseClicked(minecraft, x, y, buttonNum);
	super::mouseClicked(x, y, buttonNum);
	}

void OptionsScreen::mouseReleased( int x, int y, int buttonNum ) {
	if(optionPane != NULL)
		optionPane->mouseReleased(minecraft, x, y, buttonNum);
	super::mouseReleased(x, y, buttonNum);
}

void OptionsScreen::tick() {
	if(optionPane != NULL)
		optionPane->tick(minecraft);
	super::tick();
}

void OptionsScreen::keyPressed(int key) {
	if(key == Keyboard::KEY_ESCAPE) {
		buttonClicked(bClose);
	}

	super::keyPressed(key);
}

void OptionsScreen::keyboardNewChar(char c) {
	super::keyboardNewChar(c);
}
