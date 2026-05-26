#include "EnterIpAddressScreen.h"
#include "ScreenChooser.h"
#include "StartMenuScreen.h"
#include "DialogDefinitions.h"
#include "ProgressScreen.h"
#include "../Gui.h"
#include "../../Minecraft.h"
#include "../../../AppPlatform.h"
#include "../../../platform/log.h"
#include "../../../world/level/storage/LevelStorageSource.h"
#include "../components/ImageButton.h"
#include "../components/Button.h"
#include "../components/TextBox.h"
#include "../../../network/RakNetInstance.h"

EnterIpAddressScreen::EnterIpAddressScreen()
:	bHeader(0),
	bBack(0),
	bJoin(0),
	bServerIp(0)
{
}


EnterIpAddressScreen::~EnterIpAddressScreen()
{
	delete bHeader;
	delete bBack;
	delete bJoin;
	delete bServerIp;
}

void EnterIpAddressScreen::init() {

	if(minecraft->useTouchscreen()) {
		bHeader = new Touch::THeader(0, "Enter IP Address");
		bBack = new Touch::TButton(1, "Back");
		bJoin = new Touch::TButton(2, "Join");
		bServerIp = new TextBox(3, "");
	}
	else {
		bHeader = new Touch::THeader(0, "Enter IP Address");
		bBack = new Button(1, "Back");
		bJoin = new Button(2, "Join");
		bServerIp = new TextBox(3, "");
	}

	buttons.push_back(bHeader);
	buttons.push_back(bJoin);
	buttons.push_back(bBack);
	buttons.push_back(bServerIp);

	tabButtons.push_back(bServerIp);
	tabButtons.push_back(bJoin);
	tabButtons.push_back(bBack);

#ifndef __EPOC32__
	bServerIp->setFocus(minecraft);
#endif
}

void EnterIpAddressScreen::tick(){
#ifndef __EPOC32__
	if(!bServerIp->focused) {
		buttonClicked(bJoin);
	}
#endif
}

void EnterIpAddressScreen::buttonClicked(Button* button) {
	Screen::buttonClicked(button);

	if(button == bBack) {
		minecraft->screenChooser.setScreen(SCREEN_JOINGAME);
	}
	if(button == bJoin) {
		std::string serverIp = bServerIp->text;

		PingedCompatibleServer server;
		server.name = RakNet::RakString("TransRights");
		server.address = RakNet::SystemAddress(serverIp.c_str());
		server.pingTime = RakNet::TimeMS(1);
		server.isSpecial = false;

		minecraft->joinMultiplayer(server);
		{
			bJoin->active = false;
			bBack->active = false;
			minecraft->setScreen(new ProgressScreen());
		}

	}
}

void EnterIpAddressScreen::setupPositions() {
	int padding = 10;

	if(!minecraft->useTouchscreen()) {
		bJoin->width = 70;
		bBack->width = 70;
	}

	bJoin->y = 0;
	bJoin->x = width - bJoin->width;

	bBack->x = 0;
	bBack->y = 0;

	bHeader->x = bBack->width;
	bHeader->width = width - (bBack->width + bJoin->width);
	bHeader->height = bJoin->height;

	bServerIp->x = padding;
	bServerIp->y = bHeader->height + (padding*2);
	bServerIp->width = width - (padding*2);
	bServerIp->height = bHeader->height;
}

void EnterIpAddressScreen::render(int xm, int ym, float a)
{
	renderBackground();
	//renderDirtBackground(0);
	glEnable2(GL_BLEND);

	drawCenteredString(minecraft->font, "Enter the ip address of a server to connect to it:", width/2, bServerIp->y - 10, 0xffcccccc);

	super::render(xm, ym, a);
	glDisable2(GL_BLEND);

}
