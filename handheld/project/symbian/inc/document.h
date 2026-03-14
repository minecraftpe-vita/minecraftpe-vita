#pragma once

#ifndef _PROJECT_SYMBIAN_DOCUMENT_H_214337C3_7C51_515E_A3F5_B39B8237C5C4
#define _PROJECT_SYMBIAN_DOCUMENT_H_214337C3_7C51_515E_A3F5_B39B8237C5C4

#include <akndoc.h>
#include <eikapp.h>
#include <eikdoc.h>

struct CMcpeDocument : CAknDocument {
	static CMcpeDocument *NewL(CEikApplication &aApp);

	~CMcpeDocument() override;

protected:
	void ConstructL();

private:
	CMcpeDocument(CEikApplication &aApp);

	CEikAppUi *CreateAppUiL() override;
};

#endif

