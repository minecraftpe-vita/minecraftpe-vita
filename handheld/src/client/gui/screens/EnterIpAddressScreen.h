#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__RenameMPLevelScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__RenameMPLevelScreen_H__

#include "../Screen.h"

#include "../components/ImageButton.h"
#include "../components/Button.h"
#include "../components/TextBox.h"

class EnterIpAddressScreen: public Screen
{
public:
    EnterIpAddressScreen();
    virtual ~EnterIpAddressScreen();

    void setupPositions() override;
    void init() override;
    void tick() override;
    void render(int xm, int ym, float a) override;
    void buttonClicked(Button* button) override;
private:
    Touch::THeader bHeader;
    Touch::TButton bJoin;
    Touch::TButton bBack;
    TextBox bServerIp;

#if 0
    virtual void init();
	virtual void render(int xm, int ym, float a);
#endif

private:
    std::string _levelId;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__RenameMPLevelScreen_H__*/
