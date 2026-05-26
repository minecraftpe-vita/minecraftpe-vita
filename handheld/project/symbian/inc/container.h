#pragma once

#ifndef _PROJECT_SYMBIAN_CONTAINER_H_0839D35E_88DD_52B8_BAB2_3B3290752949
#define _PROJECT_SYMBIAN_CONTAINER_H_0839D35E_88DD_52B8_BAB2_3B3290752949

#include <GLES/egl.h>

#include <aknappui.h>
#include <aknwseventobserver.h>
#include <coecntrl.h>
#include <w32std.h>
#include <ES_SOCK.H>
#include <commdb.h>
#include <commdbconnpref.h>
#include <extendedconnpref.h>
#include <remconcoreapitarget.h>
#include <remconcoreapitargetobserver.h>
#include <remconinterfaceselector.h>

#include <list>
#include <queue>
#include <set>

#include "../../../src/NinecraftApp.h"

#include "../../../src/AppPlatform_Symbian.h"
#include "../../../src/App.h"

#include "../../../src/platform/input/Mouse.h"
#include "../../../src/platform/input/Multitouch.h"
#include "../../../src/platform/input/Keyboard.h"

struct SoundHandlerSymbian;

struct CMcpeAppUi;
struct CNetKeepAlive;
struct CBasicRemConObserver;

struct MBasicRemConObserver {
	virtual void HandleVolumeKeyL(TKeyCode) = 0;
};

struct CMcpeContainer : CCoeControl, MAknWsEventObserver, MBasicRemConObserver {
	friend struct CMcpeAppUi;

	friend struct SoundHandlerSymbian;

	void ConstructL(const TRect &aRect, CAknAppUi *aAppUi);

	CMcpeContainer();

	~CMcpeContainer() override;

	bool PromptTextL(std::string &out, TInt maxLength = 0);

	bool IsImeShown() const;

	void HandleWsEventL(const TWsEvent &aEvent, CCoeControl *aDestination) override;

	void HandleVolumeKeyL(TKeyCode aKey) override;

	static TInt DrawCallBack(TAny *aInstance);

	inline static CMcpeContainer *instance() { return gInstance; }

private:
	void SizeChanged() override;

	void HandleResourceChange(TInt aType) override;

	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType) override;

	inline TInt CountComponentControls() const override { return 0; }

	inline CCoeControl *ComponentControl(TInt aIndex) const override { return NULL; }

	inline void Draw(const TRect &aRect) const override {}

private:
	enum TStatus {
		ENotReady,
		EOpen,
		ESetVolume
	};

	enum { KBufferMaxFrames = 10 };

	static CMcpeContainer *gInstance;

	EGLDisplay iEglDisplay;
	EGLSurface iEglSurface;
	EGLContext iEglContext;

	CPeriodic *iPeriodic;
	CAknAppUi *iAppUi;

	std::list<TInt> iPressedKeys;
	std::set<TInt> iDepressedModifiers;
	bool iModifierUsed;

	NinecraftApp *iApp;
	AppContext iAppCxt;
	AppPlatform_Symbian iAppPlat;

#ifndef NO_NETWORK
	CNetKeepAlive *iNetKeepAlive;
#endif
	CBasicRemConObserver *iRcObserver;

	TStatus iOutputStatus;
};

#ifndef NO_NETWORK
struct CNetKeepAlive : CActive {
	static CNetKeepAlive *NewL();

	virtual ~CNetKeepAlive();

	void ConnectL();

	void Disconnect();

	enum TStatus {
		ENetIdle,
		ENetLinkBroken,
		ENetConnecting,
		ENetMonitoring,
	};

	inline bool IsArmed() const { return iStatusCode != ENetIdle; }

private:
	CNetKeepAlive();

	void ConstructL();

	void RunL() override;

	void DoCancel() override;

private:
	RSocketServ iSockServ;
	RConnection iConn;

	TStatus iStatusCode;
	TNifProgressBuf iProgress;
};
#endif

struct CBasicRemConObserver : CActive, MRemConCoreApiTargetObserver {
	static CBasicRemConObserver *NewL(MBasicRemConObserver& aObserver);

	virtual ~CBasicRemConObserver();

private:
	void RunL() override;

	void DoCancel() override;

	void FinishCommandL(TRemConCoreApiOperationId aId);

private:
	CBasicRemConObserver(MBasicRemConObserver& aObserver);
	void ConstructL();

	void MrccatoCommand(TRemConCoreApiOperationId, TRemConCoreApiButtonAction) override;

	inline void MrccatoPlay(TRemConCoreApiPlaybackSpeed, TRemConCoreApiButtonAction) override {}
	inline void MrccatoSelectAudioInputFunction(TUint8, TRemConCoreApiButtonAction) override {}
	inline void MrccatoSelectAvInputFunction(TUint8, TRemConCoreApiButtonAction) override {}
	inline void MrccatoSelectDiskFunction(TUint, TRemConCoreApiButtonAction) override {}
	inline void MrccatoTuneFunction(TBool, TUint, TUint, TRemConCoreApiButtonAction) override {}

private:
	MBasicRemConObserver& iTarget;
	CRemConInterfaceSelector *iInterfaceSelector;
	CRemConCoreApiTarget *iCoreTarget;

	std::queue<TRemConCoreApiOperationId> iQueue;
};

#endif
