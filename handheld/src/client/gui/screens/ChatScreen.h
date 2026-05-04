#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__ChatScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__ChatScreen_H__

#include "../Screen.h"
#include "../components/TextBox.h"
#include "../components/ImageButton.h"


class ChatScreen: public Screen
{
public:
	ChatScreen();
	virtual ~ChatScreen() {};

	void init() override;
	void render(int xm, int ym, float a) override;
	void buttonClicked(Button* button) override;
	void setupPositions() override;
	void tick() override;

private:
	Touch::TButton bSend;
	TextBox bMessage;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__ChatScreen_H__*/
