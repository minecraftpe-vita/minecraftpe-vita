#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__ProgressScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__ProgressScreen_H__

#include "../Screen.h"

class ProgressScreen: public Screen
{
public:
	ProgressScreen();

	void render(int xm, int ym, float a);
	bool isInGameScreen();
#if defined(__VITA__) || defined(WIN32) || defined(RPI)
	void keyPressed(int eventKey);
#endif

	void tick();
private:
	int ticks;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__ProgressScreen_H__*/
