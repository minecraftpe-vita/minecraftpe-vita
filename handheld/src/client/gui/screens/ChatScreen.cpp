#include "ChatScreen.h"
#include "DialogDefinitions.h"
#include "../Gui.h"
#include "../../Minecraft.h"
#include "../../../AppPlatform.h"
#include "../../../platform/log.h"
#include "../../../network/RakNetInstance.h"
#include "../../../network/packet/ChatPacket.h"
#include "../../../network/packet/MessagePacket.h"

ChatScreen::ChatScreen()
:	bSend(0, "Send"),
	bMessage(1, "")
{
}

void ChatScreen::init() {
#if defined(__APPLE__) || defined(ANDROID)
	minecraft->platform()->createUserInput(DialogDefinitions::DIALOG_NEW_CHAT_MESSAGE);
#else
	buttons.push_back(&bSend);
	buttons.push_back(&bMessage);
	bMessage.setFocus(minecraft);
#endif
}

void ChatScreen::tick(){
	if(!bMessage.focused && !bMessage.text.empty()) {
		buttonClicked(&bSend);
	}

	if(minecraft->platform()->isKeyboardVisible()) {
		// get keyboard position of send button
		int keyboardX = 0;
		int keyboardY = minecraft->platform()->getKeyboardY();
		minecraft->screen->toGUICoordinate(keyboardX, keyboardY);

		// move it if nessecary
		if((bSend.y + bSend.height) >= keyboardY) {
			bSend.y = (keyboardY - bSend.height);
		}
	}
	else {
		bSend.y = bMessage.y;
	}
}

void ChatScreen::buttonClicked(Button* button) {
	if(button == &bSend) {

		if(!bMessage.text.empty()){
			// construct chat message: "<username> message"
			std::string msgFormatted = std::string("<") + minecraft->options.username + "> " + bMessage.text;

#ifndef NO_NETWORK
			if (minecraft->netCallback && minecraft->raknetInstance->isServer()) {
				// If we are hosting, then send MsgPacket ..
				MessagePacket msgPacket(msgFormatted.c_str());
				minecraft->raknetInstance->send(msgPacket);

				// display the message locally too ..
				minecraft->gui.addMessage(msgFormatted);

			} else if (minecraft->netCallback) {
				// Otherwise, sent ChatPacket
				ChatPacket chatPacket(bMessage.text, false);
				minecraft->raknetInstance->send(chatPacket);
			}
#else
		// display local message,
		minecraft->gui.addMessage(msgFormatted);
#endif


		}

		minecraft->setScreen(NULL);
		return;
	}
}

void ChatScreen::setupPositions() {

	bMessage.x = 0;
	bMessage.height = bSend.height;
	bMessage.y = height - bMessage.height;
	bMessage.width = width - bSend.width;

	bSend.x = bMessage.width;
	bSend.y = bMessage.y;
}

void ChatScreen::render(int xm, int ym, float a)
{
	#if defined(__APPLE__) || defined(ANDROID)
	int status = minecraft->platform()->getUserInputStatus();
	if (status > -1) {
		if (status == 1) {
			std::vector<std::string> v = minecraft->platform()->getUserInput();
			if (v.size() && v[0].length() > 0)
				minecraft->gui.addMessage(v[0]);
		}

		minecraft->setScreen(NULL);
	}
	#else
	renderBackground();


	glEnable2(GL_BLEND);
	Screen::render(xm, ym, a);
	glDisable2(GL_BLEND);
	#endif
}
