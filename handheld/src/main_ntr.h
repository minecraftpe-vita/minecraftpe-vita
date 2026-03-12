#ifndef MAIN_NDS_H__
#define MAIN_NDS_H__

#include <cassert>
#include <vector>
#include <cmath>

#if defined(__NDS__)
//#include <nds.h>
#include <fat.h>
#include <filesystem.h>
#include <dswifi9.h>
#endif

#include <nds/interrupts.h>
#include <nds/arm9/input.h>
#include <nds/arm9/videoGL.h>

#include "App.h"
#include "AppPlatform_ntr.h"
#include "platform/log.h"
#include "platform/input/Mouse.h"
#include "platform/input/Multitouch.h"
#include "platform/input/Keyboard.h"
#include "platform/input/Controller.h"

static bool _app_inited = false;
static bool sneaking = false;

// Инициализация 3D движка Nintendo DS
static void initGL(App* app, AppContext* state, uint32_t w, uint32_t h)
{
    // Включаем 3D на верхнем экране
    videoSetMode(MODE_0_3D);
    
    // Инициализируем 3D движок
    glInit();
    
    // Настройки движка NDS
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_ANTIALIAS);
    glEnable(GL_BLEND);
    
    // Фон неба по умолчанию
    glClearColor(100, 149, 237, 31);
    glClearPolyID(63);
    glClearDepth(0x7FFF);
    
    // Вьюпорт строго 256x192 (размер экрана DS)
    glViewport(0, 0, 255, 191);

    if (!_app_inited) {
        _app_inited = true;
        app->init(*state);
    } else {
        app->onGraphicsReset(*state);
    }
    app->setSize(256, 192);
}

// Тачскрин NDS поддерживает только ОДНО касание (стилус)
void handleTouch() {
    static int prevX = 0;
    static int prevY = 0;
    static bool wasTouched = false;

    touchPosition touch;
    touchRead(&touch);
    
    int keys = keysHeld();
    bool isTouched = (keys & KEY_TOUCH);

    if (isTouched && !wasTouched) {
        // Касание началось
        Mouse::feed(MouseAction::ACTION_LEFT, MouseAction::DATA_DOWN, touch.px, touch.py);
        Multitouch::feed(1, MouseAction::DATA_DOWN, touch.px, touch.py, 0);
    } 
    else if (isTouched && wasTouched) {
        // Движение стилуса
        if (touch.px != prevX || touch.py != prevY) {
            Mouse::feed(MouseAction::ACTION_MOVE, MouseAction::DATA_DOWN, touch.px, touch.py);
            Multitouch::feed(1, MouseAction::DATA_DOWN, touch.px, touch.py, 0);
        }
    } 
    else if (!isTouched && wasTouched) {
        // Стилус убрали
        Mouse::feed(MouseAction::ACTION_LEFT, MouseAction::DATA_UP, prevX, prevY);
        Multitouch::feed(1, MouseAction::DATA_UP, prevX, prevY, 0);
    }

    wasTouched = isTouched;
    if (isTouched) {
        prevX = touch.px;
        prevY = touch.py;
    }
}

// Контроллер NDS (D-Pad + ABXY + LR)
void handleController() {
    scanKeys();
    u32 kDown = keysDown();
    u32 kUp = keysUp();
    u32 kHeld = keysHeld();
    u32 changed = kDown | kUp;

    // На NDS нет аналоговых стиков. 
    // Назначим D-Pad на ходьбу (WASD).
    if (changed & KEY_UP)    Keyboard2::feed(Keyboard2::KEY_W, (kHeld & KEY_UP) ? 1 : 0);
    if (changed & KEY_DOWN)  Keyboard2::feed(Keyboard2::KEY_S, (kHeld & KEY_DOWN) ? 1 : 0);
    if (changed & KEY_LEFT)  Keyboard2::feed(Keyboard2::KEY_A2, (kHeld & KEY_LEFT) ? 1 : 0);
    if (changed & KEY_RIGHT) Keyboard2::feed(Keyboard2::KEY_D, (kHeld & KEY_RIGHT) ? 1 : 0);

    // Прыжок - кнопка A
    if (changed & KEY_A) Keyboard2::feed(Keyboard2::KEY_SPACE, (kHeld & KEY_A) ? 1 : 0);

    // Крафт / Инвентарь - кнопка X
    if (changed & KEY_X) Keyboard2::feed(Keyboard2::KEY_E, (kHeld & KEY_X) ? 1 : 0);

    // Выкинуть / Назад - кнопка B
    if (changed & KEY_B) Keyboard2::feed(Keyboard2::KEY_ESCAPE, (kHeld & KEY_B) ? 1 : 0);

    // Смена вида (F5) - кнопка Y
    if (changed & KEY_Y) Keyboard2::feed(Keyboard2::KEY_F5, (kHeld & KEY_Y) ? 1 : 0);

    // Пауза - Start
    if (changed & KEY_START) Keyboard2::feed(Keyboard2::KEY_P, (kHeld & KEY_START) ? 1 : 0);

    if (changed & KEY_SELECT) {
        if (kDown & KEY_SELECT) sneaking = !sneaking;
        Keyboard2::feed(Keyboard2::KEY_LSHIFT, sneaking);
    }

    if (changed & KEY_L) Mouse::feed(MouseAction::ACTION_LEFT, (kHeld & KEY_L) ? 1 : 0, 0, 0);

    if (changed & KEY_R) Mouse::feed(MouseAction::ACTION_RIGHT, (kHeld & KEY_R) ? 1 : 0, 0, 0);
}

int main(int argc, char** argv) {
    if (!fatInitDefault()) {
        printf("FAT init failed!\n");
    }

    // Инициализация файловой системы рома (аналог RomFS)
    if (!nitroFSInit(NULL)) {
        printf("NitroFS init failed!\n");
    }

    // Инициализация Wi-Fi (Для RakNet / Мультиплеера)
    Wifi_InitDefault(INIT_ONLY);

    MAIN_CLASS* app = new MAIN_CLASS();

    // Пути для NDS (корень SD-карты, папка minecraftpe)
    app->externalStoragePath = "fat:/minecraftpe";
    app->externalCacheStoragePath = "fat:/minecraftpe";

    int commandPort = 0;
    if (argc > 1) commandPort = atoi(argv[1]);
    if (commandPort != 0) app->commandPort = commandPort;

    AppContext context;
    AppPlatform_NDS platform;
    context.doRender = true;
    context.platform = &platform;

    initGL(app, &context, 256, 192);
    
    while (1) {
        handleController();
        handleTouch();
        
        // Кнопки Start + Select = Выход из игры
        if ((keysHeld() & KEY_START) && (keysHeld() & KEY_SELECT)) break;
        
        app->update();

        glFlush(0);
        swiWaitForVBlank();
    }

    return 0;
}

#endif