#pragma once

#ifndef _PROJECT_SYMBIAN_APPUI_H_4B0D02C8_EB93_5C0F_BF84_A1F4912C3DE9
#define _PROJECT_SYMBIAN_APPUI_H_4B0D02C8_EB93_5C0F_BF84_A1F4912C3DE9

#include <aknappui.h>
#include <coedef.h>
#include <w32std.h>

struct CMcpeContainer;

struct CMcpeAppUi : CAknAppUi {
	friend struct CMcpeContainer;

	CMcpeAppUi();

	~CMcpeAppUi() override;

	void ConstructL() override;

private:
	void HandleCommandL(TInt aCommand) override;

private:
	CMcpeContainer *iAppContainer;
};

#endif

