#include "TextBox.h"
#include "../../Minecraft.h"
#include "../../../AppPlatform.h"
TextBox::TextBox( int id, const std::string& msg )
 : Button(id, 0, 0, 0, 0, msg), focused(false) {

}

TextBox::TextBox( int id, int x, int y, const std::string& msg ) 
 : Button(id, x, y, 0, 0, msg) {
}

TextBox::TextBox( int id, int x, int y, int w, int h, const std::string& msg )
 : Button(id, x, y, w, h, msg), focused(false) {
}

TextBox::~TextBox() {}

void TextBox::setFocus(Minecraft* minecraft) {
	if(!focused) {
		minecraft->platform()->showKeyboard();
		focused = true;
	}
}

bool TextBox::loseFocus(Minecraft* minecraft) {
	if(focused) {
		minecraft->platform()->showKeyboard();
		focused = false;
		return true;
	}
	return false;
}

void TextBox::setPressed(Minecraft* minecraft) {
	this->setFocus(minecraft);
}

void TextBox::render( Minecraft* minecraft, int xm, int ym ) {
	if(focused) {
		std::string input = minecraft->platform()->getKeyboardInput();
		if(!minecraft->platform()->isKeyboardVisible()) {
			focused = false;
		}
		this->msg = input;
	}

	if(visible) {
		Button::render(minecraft, xm, ym);
	}
}
