#pragma once

#ifndef _PROJECT_SYMBIAN_APP_H_BB260A1E_C74A_586B_ADB8_FE5909FD021E
#define _PROJECT_SYMBIAN_APP_H_BB260A1E_C74A_586B_ADB8_FE5909FD021E

#include <aknapp.h>
#include <apparc.h>

struct CMcpeApp : CAknApplication {
private:
	CApaDocument *CreateDocumentL() override;

	TUid AppDllUid() const override;
};

LOCAL_C CApaApplication *NewApplication();

GLDEF_C TInt E32Main();

#endif

