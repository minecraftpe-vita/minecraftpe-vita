// public domain or 0BSD

#include "app.h"
#include "document.h"

#include <eikstart.h>
#include <apparc.h>

CApaDocument *CMcpeApp::CreateDocumentL() {
	return CMcpeDocument::NewL(*this);
}

TUid CMcpeApp::AppDllUid() const {
	return { static_cast<TInt32>(0xE0000666) };
}

CApaApplication *NewApplication() {
	return new CMcpeApp;
}

GLDEF_C TInt E32Main() {
	return EikStart::RunApplication(NewApplication);
}
