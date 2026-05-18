#include "TextBox.h"
#include "../../Minecraft.h"
#include "../../../AppPlatform.h"
#include "../../renderer/Textures.h"
#include "../Screen.h"

#include "Button.h"

TextBox::TextBox( int id, const std::string& msg )
 : Button(id, 0, 0, 0, 0, msg),
 focused(false),
 defaultText(msg),
 _minecraft(nullptr),
 text(msg){

}

TextBox::TextBox( int id, int x, int y, const std::string& msg ) 
 : Button(id, x, y, 0, 0, msg),
	focused(false),
	defaultText(msg),
	_minecraft(nullptr),
	text(msg){
}

TextBox::TextBox( int id, int x, int y, int w, int h, const std::string& msg )
 : Button(id, x, y, w, h, msg),
   focused(false),
   defaultText(msg),
   _minecraft(nullptr),
   text(msg) {
}

TextBox::~TextBox() {
	if(focused) {
		if(_minecraft != nullptr) {
			this->_minecraft->platform()->hideKeyboard();
		}
	}

}

void TextBox::setFocus(Minecraft* minecraft) {
	if(!focused) {
		if(minecraft->platform()->isKeyboardVisible()) return;
		this->_minecraft = minecraft;

		minecraft->platform()->showKeyboard(text);
		focused = true;

	}
}

bool TextBox::loseFocus(Minecraft* minecraft) {
	this->_minecraft = minecraft;
	if(focused) {
		if(minecraft->platform()->isKeyboardVisible()) minecraft->platform()->hideKeyboard();

		focused = false;
		return true;

	}
	return false;
}

void TextBox::setPressed(Minecraft* minecraft) {
	this->_minecraft = minecraft;
	this->setFocus(minecraft);
}



void TextBox::render(Minecraft* mc, int xm, int ym) {
	int prevY = this->y;

	if(focused) {
		// update textbox to contain most recent typed text;

		std::string input = mc->platform()->getKeyboardInput();

		if(!mc->platform()->isKeyboardVisible()) {
			this->loseFocus(mc);

			if(input.empty()) {
				// set default if it was left empty.
				input = defaultText;
			}
		}

		// set text to current input
		this->text = input;

		// move button onscreen if its blocked by the keyboard;

		// get the keyboard position ..
		int keyboardX = mc->platform()->getKeyboardX();
		int keyboardY = mc->platform()->getKeyboardY();
		mc->screen->toGUICoordinate(keyboardX, keyboardY);

		// set render position to be above the keyboard if the keyboard is open
		if(keyboardY > 0 && (this->y + this->height) >= keyboardY) {
			this->y = (keyboardY - this->height);
		}

	}
	super::render(mc, xm, ym);
	this->y = prevY;
}

// render non-centered
void TextBox::renderFace(Minecraft* mc, int xm, int ym) {
	Font* font = mc->font;

	int caret = focused ? mc->platform()->getKeyboardCarret() : text.length();
	if(caret < 0 || caret > text.length())
		caret = text.length();


	// find portion of text that fits within the textbox;
	int padding = 10;
	int offset = caret;
	int end = caret;

	while(font->width(text.substr(offset, end)) < (this->width - padding*2) && ( offset > 0 || end < text.length() )) {
		if(offset > 0) offset--;
		if(end < text.length()) end++;
	}

	std::string visibleInput = text.substr(offset, end);

	// calculate caret position in textbox
	int caretX = font->width(visibleInput.substr(0, caret - offset));
	int caretY = 1;

	// draw the textbox to the screen, along with text + caret
	int drawX = x + padding;
	int drawY = y + (height - 8) / 2;

	if (!active) {
		drawString(font, visibleInput, drawX, drawY, 0xffa0a0a0);
		if(focused) drawString(font, "_", drawX + caretX, drawY+caretY, 0xffa0a0a0);
	} else {
		if (hovered(mc, xm, ym) || selected) {
			drawString(font, visibleInput, drawX, drawY, 0xffffa0);
			drawString(font, "_", drawX + caretX, drawY+caretY, 0xffffa0);
		} else {
			drawString(font, visibleInput, drawX, drawY, 0xe0e0e0);
			if(focused) drawString(font, "_", drawX + caretX, drawY+caretY, 0xe0e0e0);
		}
	}
}

// use THeader sprite ..
void TextBox::renderBg( Minecraft* minecraft, int xm, int ym ) {
	minecraft->textures->loadAndBindTexture("gui/touchgui.png");

	//printf("ButtonId: %d - Hovered? %d (cause: %d, %d, %d, %d, <> %d, %d)\n", id, hovered, x, y, x+w, y+h, xm, ym);
	glColor4f2(1, 1, 1, 1);

	// Left cap
	blit(x, y, 150, 26, 2, height-1, 2, 25);
	// Middle
	blit(x+2, y, 153, 26, width-3, height-1, 8, 25);
	// Right cap
	blit(x+width-2, y, 162, 26, 2, height-1, 2, 25);
	// Shadow
	glEnable2(GL_BLEND);
	glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	blit(x, y+height-1, 153, 52, width, 3, 8, 3);
}
