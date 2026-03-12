#include "Keyboard.h"

/*
const int KeyboardAction::KEYUP = 0;
const int KeyboardAction::KEYDOWN = 1;
*/

#ifdef __NDS__
int
    Keyboard2::_states[256] = {0};

std::vector<KeyboardAction>
    Keyboard2::_inputs;
std::vector<char>
    Keyboard2::_inputText;

int
    Keyboard2::_index = -1;

int
    Keyboard2::_textIndex = -1;
#else
int
    Keyboard::_states[256] = {0};

std::vector<KeyboardAction>
    Keyboard::_inputs;
std::vector<char>
    Keyboard::_inputText;

int
    Keyboard::_index = -1;

int
    Keyboard::_textIndex = -1;
#endif