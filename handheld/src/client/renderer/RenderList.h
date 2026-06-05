#ifndef NET_MINECRAFT_CLIENT_RENDERER__RenderList_H__
#define NET_MINECRAFT_CLIENT_RENDERER__RenderList_H__

//package net.minecraft.client.renderer;

class RenderChunk;
class Vec3;

class RenderList
{
	static const int MAX_NUM_OBJECTS = 1024 * 3;

public:
	RenderList();
	~RenderList();

    void init(float xOff, float yOff, float zOff);

	void add(int list);
	void addR(const RenderChunk& chunk, const Vec3& pos);

	__inline void next() { ++listIndex; }

    void render();
	void renderChunks();

    void clear();


	float xOff, yOff, zOff;
	int* lists;
	RenderChunk* rlists;
	Vec3* rpos;


	int listIndex;
	bool inited;
	bool rendered;

private:
	int bufferLimit;
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER__RenderList_H__*/
