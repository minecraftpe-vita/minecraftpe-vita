#ifndef NET_MINECRAFT_CLIENT__MouseHandler_H__
#define NET_MINECRAFT_CLIENT__MouseHandler_H__

//package net.minecraft.client;

class ITurnInput;
class Minecraft;

class MouseHandler
{
public:
	MouseHandler(Minecraft* minecraft, ITurnInput* turnInput);
	MouseHandler();
	~MouseHandler();

	void setTurnInput(ITurnInput* turnInput);

    void grab();
    void release();

    void poll();

    float xd, yd;
private:
	int toSkip;
	ITurnInput* _turnInput;
	Minecraft* minecraft;
};

#endif /*NET_MINECRAFT_CLIENT__MouseHandler_H__*/
