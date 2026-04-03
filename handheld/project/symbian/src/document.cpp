#include "document.h"
#include "appui.h"

#include <akndoc.h>
#include <e32base.h>
#include <e32const.h>
#include <eikapp.h>
#include <eikdoc.h>

CMcpeDocument::~CMcpeDocument() {}

CMcpeDocument::CMcpeDocument(CEikApplication &aApp) : CAknDocument{aApp} {}

void CMcpeDocument::ConstructL() {}

CMcpeDocument *CMcpeDocument::NewL(CEikApplication &aApp) {
	auto self = new (ELeave) CMcpeDocument(aApp);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();

	return self;
}

CEikAppUi *CMcpeDocument::CreateAppUiL() {
	return new (ELeave) CMcpeAppUi;
}
