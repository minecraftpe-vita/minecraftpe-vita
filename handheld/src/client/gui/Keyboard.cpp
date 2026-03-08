#include "Keyboard.h"
#include "../Minecraft.h"
#include "../renderer/Textures.h"


static constexpr int KEY_H   = 26;
static constexpr int KEY_GAP = 2;


KeyButton::KeyButton(int x, int y, int width, int height,
                     const std::string& label, const std::string& altLabel)
:   _currentlyDown(false),
    x(x), y(y), width(width), height(height),
    label(label),
    altLabel(altLabel.empty() ? label : altLabel)
{}

void KeyButton::clicked() {
    _currentlyDown = true;
}

void KeyButton::released() {
    _currentlyDown = false;
}

bool KeyButton::isInside(int ox, int oy, int xm, int ym) const {
    int ax = ox + x, ay = oy + y;
    return xm >= ax && ym >= ay && xm < ax + width && ym < ay + height;
}

bool KeyButton::hovered(Minecraft* minecraft, int ox, int oy, int xm, int ym) const {
    if (minecraft->useTouchscreen())
        return _currentlyDown && isInside(ox, oy, xm, ym);
    return isInside(ox, oy, xm, ym);
}

void KeyButton::render(Minecraft* minecraft, int ox, int oy, int xm, int ym) {
    int ax = ox + x;
    int ay = oy + y;

    bool isHovered = hovered(minecraft, ox, oy, xm, ym);

    minecraft->textures->loadAndBindTexture("gui/touchgui.png");
    glColor4f2(1.0f, 1.0f, 1.0f, 1.0f);
    blit(ax, ay, isHovered ? 66 : 0, 0, width, height, 66, 26);

    Font* font = minecraft->font;
    int color  = isHovered ? 0xffffa0 : 0xe0e0e0;
    drawCenteredString(font, label, ax + width / 2, ay + (height - 8) / 2, color);
}

GuiKeyboard::GuiKeyboard()
:   _x(0), _y(0), _width(220), _height(0),
    _shift(false), _capsLock(false)
{
    buildKeys();
}

void GuiKeyboard::setPosition(int x, int y) {
    _x = x;
    _y = y;
}

void GuiKeyboard::buildKeys() {
    _keys.clear();

    int kw  = (_width - 9 * KEY_GAP) / 10;
    int row = 0;

    auto rowY = [&](int r) { return r * (KEY_H + KEY_GAP); };

    { // 1234567890
        const char* nums[] = {"1","2","3","4","5","6","7","8","9","0"};
        const char* syms[] = {"!","@","#","$","%","^","&","*","(",")"};;
        for (int i = 0; i < 10; ++i)
            _keys.emplace_back(i * (kw + KEY_GAP), rowY(0), kw, KEY_H, nums[i], syms[i]);
    }

    { // qwertyuiop
        const char* keys[] = {"q","w","e","r","t","y","u","i","o","p"};
        for (int i = 0; i < 10; ++i)
            _keys.emplace_back(i * (kw + KEY_GAP), rowY(1), kw, KEY_H, keys[i], "");
    }

    { // asdfghjkl
        const char* keys[] = {"a","s","d","f","g","h","j","k","l"};
        int totalW = 9 * kw + 8 * KEY_GAP;
        int startX = (_width - totalW) / 2;
        for (int i = 0; i < 9; ++i)
            _keys.emplace_back(startX + i * (kw + KEY_GAP), rowY(2), kw, KEY_H, keys[i], "");
    }

    { // shift zxcvbnm back
        int specialW = kw + kw / 2;
        int innerW   = _width - 2 * (specialW + KEY_GAP);
        int lkW      = (innerW - 6 * KEY_GAP) / 7;

        _keys.emplace_back(0, rowY(3), specialW, KEY_H, "Shift", "Shift");

        const char* keys[] = {"z","x","c","v","b","n","m"};
        int kx = specialW + KEY_GAP;
        for (int i = 0; i < 7; ++i, kx += lkW + KEY_GAP)
            _keys.emplace_back(kx, rowY(3), lkW, KEY_H, keys[i], "");

        _keys.emplace_back(_width - specialW, rowY(3), specialW, KEY_H, "BS", "BS");
    }

    { // space enter
        int enterW = kw * 2 + KEY_GAP;
        int spaceW = _width - enterW - KEY_GAP;
        _keys.emplace_back(0,               rowY(4), spaceW,  KEY_H, " ",     " ");
        _keys.emplace_back(spaceW + KEY_GAP, rowY(4), enterW, KEY_H, "Enter", "Enter");
    }

    _height = 5 * KEY_H + 4 * KEY_GAP;
}

std::string GuiKeyboard::resolveLabel(const KeyButton& key) const {
    if (key.label.size() != 1) return key.label;

    char c = key.label[0];
    bool isLower = c >= 'a' && c <= 'z';
    bool isUpper = c >= 'A' && c <= 'Z';
    if (!isLower && !isUpper) {
        return (_shift) ? key.altLabel : key.label;
    }
    bool upper = _shift ^ _capsLock; // if shift invert capslock state
    return upper ? key.altLabel : key.label;
}

void GuiKeyboard::tick(Minecraft* minecraft) {
    (void)minecraft;
}

void GuiKeyboard::render(Minecraft* minecraft, int xm, int ym) {
    glEnable2(GL_BLEND);
    glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    fill(_x - 2, _y - 2, _x + _width + 2, _y + _height + 2, 0x88000000);

    for (auto& key : _keys) {
        key.label = resolveLabel(key);
        key.render(minecraft, _x, _y, xm, ym);
    }
}

void GuiKeyboard::mouseClicked(Minecraft* minecraft, int mx, int my, int btn) {
    (void)btn;
    for (auto& key : _keys) {
        if (key.isInside(_x, _y, mx, my)) {
            key.clicked();

            std::string token = resolveLabel(key);

            if (token == "Shift") {
                _shift    = !_shift;
            } else if (token == "Caps") {
                _capsLock = !_capsLock;
            } else {
                if (_shift && token != "BS" && token != "Enter" && token != " ") {
                    _shift = false;
                }
                if (onKey) onKey(token);
            }
            return;
        }
    }
}

void GuiKeyboard::mouseReleased(Minecraft* minecraft, int mx, int my, int btn) {
    (void)minecraft;
    (void)btn;
    for (auto& key : _keys) {
        if (key._currentlyDown) {
            key.released();
        }
    }
}
