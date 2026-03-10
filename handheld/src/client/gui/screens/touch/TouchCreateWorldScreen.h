#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS_TOUCH__TouchCreateWorldScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS_TOUCH__TouchCreateWorldScreen_H__

#include "../ConfirmScreen.h"
#include "../../Screen.h"
#include "../../TweenData.h"
#include "../../components/ImageButton.h"
#include "../../components/Button.h"
#include "../../components/TextBox.h"
#include "../../../Minecraft.h"
#include "../../../../world/level/storage/LevelStorageSource.h"
#include "../../../../world/level/LevelSettings.h"

namespace Touch {

class CreateWorldScreen: public Screen
{
public:
	CreateWorldScreen();
	virtual ~CreateWorldScreen();

	void init() override;
	void setupPositions() override;

	void render(int xm, int ym, float a) override;

	void buttonClicked(Button* button) override;
	bool handleBackEvent(bool isDown) override;

private:
	void loadLevelSource();
	std::string getUniqueLevelName(const std::string& level);

	THeader bHeader;
	TButton bBack;
	TButton bStart;
	TextBox bLevelName;
	TextBox bSeed;
	TButton bCreative;
	TButton bSurvival;

	int gameType;
};
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS_TOUCH__TouchSelectWorldScreen_H__*/
