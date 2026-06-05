#include "MouseHandler.h"
#include "player/input/ITurnInput.h"
#include "Minecraft.h"

#ifdef RPI
#include <SDL/SDL.h>
#endif

MouseHandler::MouseHandler( Minecraft* minecraft, ITurnInput* turnInput )
:	_turnInput(turnInput),
	minecraft(minecraft)
{}

MouseHandler::MouseHandler()
:	_turnInput(0),
	minecraft(nullptr)
{}

MouseHandler::~MouseHandler() {
}

void MouseHandler::setTurnInput( ITurnInput* turnInput ) {
	_turnInput = turnInput;
}

void MouseHandler::grab() {
	xd = 0;
	yd = 0;

#if defined(RPI)
	//LOGI("Grabbing input!\n");
	SDL_WM_GrabInput(SDL_GRAB_ON);
	SDL_ShowCursor(0);
#endif

	minecraft->platform()->mouseGrab(true);
}

void MouseHandler::release() {
#if defined(RPI)
	//LOGI("Releasing input!\n");
	SDL_WM_GrabInput(SDL_GRAB_OFF);
	SDL_ShowCursor(1);
#endif

	minecraft->platform()->mouseGrab(false);
}

void MouseHandler::poll() {
	if (_turnInput != 0) {
		TurnDelta td = _turnInput->getTurnDelta();
		xd = td.x;
		yd = td.y;
	}
}
