#ifndef NET_MINECRAFT_CLIENT_RENDERER__RenderChunk_H__
#define NET_MINECRAFT_CLIENT_RENDERER__RenderChunk_H__

//package net.minecraft.client.renderer;

#include "gles.h"
#include "../../world/phys/Vec3.h"

class RenderChunk
{
public:
	RenderChunk();

	GLuint vboId;
	GLsizei vertexCount;
	std::vector<uint16_t> indices;
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER__RenderChunk_H__*/
