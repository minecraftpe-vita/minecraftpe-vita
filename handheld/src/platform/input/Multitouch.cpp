#include "Multitouch.h"

int Multitouch::_index = -1;
int Multitouch::_activePointerCount = 0;
int Multitouch::_activePointerList[Multitouch::MAX_POINTERS] = {-1};
int Multitouch::_activePointerThisUpdateCount = 0;
int Multitouch::_activePointerThisUpdateList[Multitouch::MAX_POINTERS] = {-1};

bool Multitouch::_wasPressed[Multitouch::MAX_POINTERS] = {false};
bool Multitouch::_wasReleased[Multitouch::MAX_POINTERS] = {false};
bool Multitouch::_wasPressedThisUpdate[Multitouch::MAX_POINTERS] = {false};
bool Multitouch::_wasReleasedThisUpdate[Multitouch::MAX_POINTERS] = {false};

TouchPointer Multitouch::_pointers[Multitouch::MAX_POINTERS];

std::vector<MouseAction> Multitouch::_inputs; 
