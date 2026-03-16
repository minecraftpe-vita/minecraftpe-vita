#include "app.h"
#include "document.h"

#include <e32std.h>
#include <eikstart.h>
#include <apparc.h>

CApaDocument *CMcpeApp::CreateDocumentL() {
	return CMcpeDocument::NewL(*this);
}

TUid CMcpeApp::AppDllUid() const {
	return { static_cast<TInt32>(0xE000C418) };
}

CApaApplication *NewApplication() {
	return new CMcpeApp;
}

GLDEF_C TInt E32Main() {
	User::SetFloatingPointMode(EFpModeRunFast);

	return EikStart::RunApplication(NewApplication);
}
