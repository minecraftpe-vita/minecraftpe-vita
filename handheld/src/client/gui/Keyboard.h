#pragma once

#include "GuiComponent.h"
#include <functional>
#include <vector>

class Minecraft;

class KeyButton : public GuiComponent {
public:
    std::string label;
    std::string altLabel;
    bool        _currentlyDown;
    int         x;
    int         y;
    int         width;
    int         height;

    KeyButton(int x, int y, int width, int height,
              const std::string& label, const std::string& altLabel = "");

    void render(Minecraft* minecraft, int ox, int oy, int xm, int ym);
    void clicked();
    void released();
    bool isInside(int ox, int oy, int xm, int ym) const;
    bool hovered(Minecraft* minecraft, int ox, int oy, int xm, int ym) const;
};


class GuiKeyboard : public GuiComponent {
public:
    GuiKeyboard();
    virtual ~GuiKeyboard() = default;

    std::function<void(const std::string&)> onKey;

    void setPosition(int x, int y);
    void render(Minecraft* minecraft, int xm, int ym);
    void tick(Minecraft* minecraft);
    void mouseClicked(Minecraft* minecraft, int mx, int my, int btn);
    void mouseReleased(Minecraft* minecraft, int mx, int my, int btn);
private:
    int  _x, _y, _width, _height;
    bool _shift;
    bool _capsLock;

    std::vector<KeyButton>   _keys;

    void buildKeys();
    std::string resolveLabel(const KeyButton& key) const;
};