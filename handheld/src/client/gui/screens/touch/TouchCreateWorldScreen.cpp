#include "TouchCreateWorldScreen.h"
#include "../../../Minecraft.h"

namespace Touch {

CreateWorldScreen::CreateWorldScreen()
:   bHeader(0, "Create World"),
    bBack(0, "Back"),
    bStart(1, "Start!"),
    bLevelName(0, "Level Name"),
    bSeed(1, "Seed"),
    bCreative(2, "Creative"),
    bSurvival(3, "Survival")
{
}

CreateWorldScreen::~CreateWorldScreen()
{
}

void CreateWorldScreen::init() {
    buttons.push_back(&bHeader);
    buttons.push_back(&bBack);
    buttons.push_back(&bStart);
    buttons.push_back(&bLevelName);
    buttons.push_back(&bSeed);
    buttons.push_back(&bCreative);
    buttons.push_back(&bSurvival);
}

void CreateWorldScreen::setupPositions() {
    bCreative.width = bSurvival.width = bBack.width = 120;
	bCreative.x = (width - bCreative.width) / 2;
	bCreative.y = height/3 - 40;
	bSurvival.x = (width - bSurvival.width) / 2;
	bSurvival.y = 2*height/3 - 40;
	bBack.x = bSurvival.x + bSurvival.width - bBack.width;
	bBack.y = height - 40;
}

void CreateWorldScreen::render( int xm, int ym, float a )
{
    renderDirtBackground(0);
    glEnable2(GL_BLEND);

	drawCenteredString(minecraft->font, "Mobs, health and gather resources", width/2, bSurvival.y + bSurvival.height + 4, 0xffcccccc);
	drawCenteredString(minecraft->font, "Unlimited resources and flying", width/2, bCreative.y + bCreative.height + 4, 0xffcccccc);

    Screen::render(xm, ym, a);
    glDisable2(GL_BLEND);
}

void CreateWorldScreen::buttonClicked( Button* button )
{
    if(button == &bBack) {
        minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
        return;
    }

    if(button == &bCreative) {
        this->gameType = GameType::Creative;
        return;
    }

    if(button == &bSurvival) {
        this->gameType = GameType::Survival;
        return;
    }
}

bool CreateWorldScreen::handleBackEvent(bool isDown)
{
    minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
    return true;
}

};
