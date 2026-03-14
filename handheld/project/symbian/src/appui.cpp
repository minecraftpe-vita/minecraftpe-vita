#include "appui.h"
#include "container.h"

#include <e32const.h>

CMcpeAppUi::CMcpeAppUi() : iAppContainer(NULL) {}

CMcpeAppUi::~CMcpeAppUi() {
	if (iAppContainer) {
		RemoveFromStack(iAppContainer);
		delete iAppContainer;
	}
}

void CMcpeAppUi::HandleEventL(TWsEvent &aWsEvent) {
	HandleWsEventL(aWsEvent, iAppContainer);
}

void CMcpeAppUi::ConstructL() {
	BaseConstructL();

	iAppContainer = new (ELeave) CMcpeContainer;
	iAppContainer->SetMopParent(this);
	iAppContainer->ConstructL(ClientRect(), this);

	AddToStackL(iAppContainer);

	SetOrientationL(EAppUiOrientationLandscape);
}

void CMcpeAppUi::HandleCommandL(TInt aCommand) {
	switch (aCommand) {
	case EAknSoftkeyBack:
	case EEikCmdExit:
		Exit();
		break;
	}
}

TKeyResponse CMcpeAppUi::HandleKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType) {
	return EKeyWasConsumed;
}
