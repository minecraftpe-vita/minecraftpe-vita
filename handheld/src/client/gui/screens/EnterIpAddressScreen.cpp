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

static char ILLEGAL_FILE_CHARACTERS[] = {
	'/', '\n', '\r', '\t', '\0', '\f', '`', '?', '*', '\\', '<', '>', '|', '\"', ':'
};

EnterIpAddressScreen::EnterIpAddressScreen()
:	bHeader(0, "Enter IP Address"),
	bBack(1, "Back"),
	bJoin(2, "Join"),
	bServerIp(3, "127.0.0.1:19132")
{
}


EnterIpAddressScreen::~EnterIpAddressScreen()
{
}

void EnterIpAddressScreen::init() {
	buttons.push_back(&bHeader);
	buttons.push_back(&bJoin);
	buttons.push_back(&bBack);
	buttons.push_back(&bServerIp);
	bServerIp.setFocus(minecraft);
}

void EnterIpAddressScreen::tick(){
	if(!bServerIp.focused) {
		buttonClicked(&bJoin);
	}
}

void EnterIpAddressScreen::buttonClicked(Button* button) {
	Screen::buttonClicked(button);

	if(button == &bBack) {
		minecraft->screenChooser.setScreen(SCREEN_JOINGAME);
	}
	if(button == &bJoin) {
		std::string serverIp = bServerIp.text;

		PingedCompatibleServer server;
		server.name = RakNet::RakString("TransRights");
		server.address = RakNet::SystemAddress(serverIp.c_str());
		server.pingTime = RakNet::TimeMS(1);
		server.isSpecial = false;

		minecraft->joinMultiplayer(server);
		{
			bJoin.active = false;
			bBack.active = false;
			minecraft->setScreen(new ProgressScreen());
		}

	}
}

void EnterIpAddressScreen::setupPositions() {
	int padding = 10;

	bJoin.y = 0;
	bJoin.x = width - bJoin.width;

	bBack.x = 0;
	bBack.y = 0;

	bHeader.x = bBack.width;
	bHeader.width = width - (bBack.width + bJoin.width);
	bHeader.height = bJoin.height;

	bServerIp.x = padding;
	bServerIp.y = bHeader.height + (padding*2);
	bServerIp.width = width - (padding*2);
	bServerIp.height = bHeader.height;
}

void EnterIpAddressScreen::render(int xm, int ym, float a)
{
	renderBackground();
	//renderDirtBackground(0);
	glEnable2(GL_BLEND);

	drawCenteredString(minecraft->font, "Enter the ip address of a server to connect to it:", width/2, bServerIp.y - 10, 0xffcccccc);

	Screen::render(xm, ym, a);
	glDisable2(GL_BLEND);

}
