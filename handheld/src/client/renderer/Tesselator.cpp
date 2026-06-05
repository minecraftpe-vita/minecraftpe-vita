#include "Tesselator.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

#ifdef WITH_MESHOPT
#include <meshoptimizer.h>
#endif

Tesselator Tesselator::instance(sizeof(GLfloat) * MAX_FLOATS); // max size in bytes

Tesselator::Tesselator( int size )
:	size(size),
	vertices(0),
	u(0), v(0),
	_color(0),
	hasColor(false),
	hasTexture(false),
	hasNormal(false),
	p(0),
	count(0),
	_noColor(false),
	mode(0),
	xo(0), yo(0), zo(0),
	_normal(0),
	_sx(1), _sy(1),

	tesselating(false),
	vboId(-1),
	vboCounts(128),
	totalSize(0),
	accessMode(ACCESS_STATIC),
	maxVertices(size / sizeof(VERTEX)),
	_voidBeginEnd(false)
{
	vboIds = new GLuint[vboCounts];
	_varray = new VERTEX[maxVertices];
}

Tesselator::~Tesselator()
{
	delete[] vboIds;
	delete[] _varray;
}

void Tesselator::init()
{
#ifndef STANDALONE_SERVER
	glGenBuffers2(vboCounts, vboIds);
#endif
}

void Tesselator::clear()
{
	accessMode = ACCESS_STATIC;
	vertices = 0;
	count = 0;
	p = 0;
	_voidBeginEnd = false;
}

int Tesselator::getVboCount() {
	return vboCounts;
}

void Tesselator::end( RenderChunk& rc )
{
#ifndef STANDALONE_SERVER
	//if (!tesselating) throw /*new*/ IllegalStateException("Not tesselating!");
	if (!tesselating)
		LOGI("not tesselating!\n");

	if (!tesselating || _voidBeginEnd) {
		rc = RenderChunk();
		return;
	}

	tesselating = false;

	if(vertices == 0) {
		clear();
		rc = RenderChunk();
		return;
	}

	size_t vertex_count;
	VERTEX* vertex_data;
	bool delete_vertex_data = false;
	std::vector<uint16_t> indices;
#ifdef WITH_MESHOPT
	size_t index_count = vertices;
	size_t unindexed_vertex_count = vertices;
	uint32_t* remap = new uint32_t[unindexed_vertex_count];
	vertex_count = meshopt_generateVertexRemap(remap, NULL, index_count, _varray, unindexed_vertex_count, sizeof(VERTEX));
	vertex_count = vertex_count > 100000 ? 100000 : vertex_count;

	VERTEX* remapped_vertices = new VERTEX[vertex_count];
	indices.resize(index_count);

	meshopt_remapIndexBuffer<uint16_t>(indices.data(), NULL, index_count, remap);
	meshopt_remapVertexBuffer(remapped_vertices, _varray, unindexed_vertex_count, sizeof(VERTEX), remap);
	delete[] remap;
	
	VERTEX* optimized_vertices = new VERTEX[vertex_count];
	meshopt_optimizeVertexFetch(optimized_vertices, indices.data(), index_count, remapped_vertices, vertex_count, sizeof(VERTEX));
	delete[] remapped_vertices;

	vertex_data = optimized_vertices;
	delete_vertex_data = true;

	printf("index_count=%d vertex_count=%d\n", index_count, vertex_count);
#else
	vertex_count = vertices;
	vertex_data = _varray;
	delete_vertex_data = false;
#endif

	GLuint bufferId = rc.vboId;
	if(bufferId == 0) {
		glGenBuffers2(1, &bufferId);
	}

	size_t vertices_size = vertex_count * sizeof(VERTEX);
	glBindBuffer2(GL_ARRAY_BUFFER, bufferId);
	glBufferData2(GL_ARRAY_BUFFER, vertices_size, vertex_data, GL_STATIC_DRAW);
	totalSize += vertices_size;
	if(delete_vertex_data) { delete[] vertex_data; }

#ifndef USE_VBO
	// 0 1 2 3 4 5 6 7
	// x y z u v c
	if (hasTexture) {
		glTexCoordPointer2(2, GL_FLOAT, sizeof(VERTEX), (GLvoid*) (3 * 4));
		glEnableClientState2(GL_TEXTURE_COORD_ARRAY);
	}
	if (hasColor) {
		glColorPointer2(4, GL_UNSIGNED_BYTE, sizeof(VERTEX), (GLvoid*) (5 * 4));
		glEnableClientState2(GL_COLOR_ARRAY);
	}
	if (hasNormal) {
		glNormalPointer(GL_BYTE, sizeof(VERTEX), (GLvoid*) (6 * 4));
		glEnableClientState2(GL_NORMAL_ARRAY);
	}
	glVertexPointer2(3, GL_FLOAT, sizeof(VERTEX), 0);
	glEnableClientState2(GL_VERTEX_ARRAY);

	mode = mode == GL_QUADS ? GL_TRIANGLES : mode;
	if(indices.size() == 0) {
		glDrawArrays2(mode, 0, vertex_count);
	} else {
		glDrawElements(mode, indices.size(), GL_UNSIGNED_SHORT, indices.data());
	}
	//printf("drawing %d tris, size %d (%d,%d,%d)\n", vertex_count, p, hasTexture, hasColor, hasNormal);
	glDisableClientState2(GL_VERTEX_ARRAY);
	if (hasTexture) glDisableClientState2(GL_TEXTURE_COORD_ARRAY);
	if (hasColor) glDisableClientState2(GL_COLOR_ARRAY);
	if (hasNormal) glDisableClientState2(GL_NORMAL_ARRAY);
#endif /*!USE_VBO*/

	clear();
	rc.vboId = bufferId;
	rc.vertexCount = vertex_count;
	rc.indices = std::move(indices);
#endif
}

void Tesselator::begin( int mode )
{
	if (tesselating || _voidBeginEnd) {
		if (tesselating && !_voidBeginEnd)
			LOGI("already tesselating!\n");
		return;
	}
	//if (tesselating) {
	//    throw /*new*/ IllegalStateException("Already tesselating!");
	//}
	tesselating = true;

	clear();
	this->mode = mode;
	hasNormal = false;
	hasColor = false;
	hasTexture = false;
	_noColor = false;
}

void Tesselator::begin()
{
	begin(GL_QUADS);
}

void Tesselator::tex( float u, float v )
{
	hasTexture = true;
	this->u = u;
	this->v = v;
}

int Tesselator::getColor() {
	return _color;
}

void Tesselator::color( float r, float g, float b )
{
	color((int) (r * 255), (int) (g * 255), (int) (b * 255));
}

void Tesselator::color( float r, float g, float b, float a )
{
	color((int) (r * 255), (int) (g * 255), (int) (b * 255), (int) (a * 255));
}

void Tesselator::color( int r, int g, int b )
{
	color(r, g, b, 255);
}

void Tesselator::color( int r, int g, int b, int a )
{
	if (_noColor) return;

	if (r > 255) r = 255;
	if (g > 255) g = 255;
	if (b > 255) b = 255;
	if (a > 255) a = 255;
	if (r < 0) r = 0;
	if (g < 0) g = 0;
	if (b < 0) b = 0;
	if (a < 0) a = 0;

	hasColor = true;
	//if (ByteOrder.nativeOrder() == ByteOrder.LITTLE_ENDIAN) {
	if (true) {
		_color = (a << 24) | (b << 16) | (g << 8) | (r);
	} else {
		_color = (r << 24) | (g << 16) | (b << 8) | (a);
	}
}

void Tesselator::color( char r, char g, char b )
{
	color(r & 0xff, g & 0xff, b & 0xff);
}

void Tesselator::color( int c )
{
	int r = ((c >> 16) & 255);
	int g = ((c >> 8) & 255);
	int b = ((c) & 255);
	color(r, g, b);
}

//@note: doesn't care about endianess
void Tesselator::colorABGR( int c )
{
	if (_noColor) return;
	hasColor = true;
	_color = c;
}

void Tesselator::color( int c, int alpha )
{
	int r = ((c >> 16) & 255);
	int g = ((c >> 8) & 255);
	int b = ((c) & 255);
	color(r, g, b, alpha);
}

void Tesselator::vertexUV( float x, float y, float z, float u, float v )
{
	tex(u, v);
	vertex(x, y, z);
}

void Tesselator::scale2d(float sx, float sy) {
	_sx *= sx;
	_sy *= sy;
}

void Tesselator::resetScale() {
	_sx = _sy = 1;
}

void Tesselator::vertex( float x, float y, float z )
{
#ifndef STANDALONE_SERVER
	count++;

	if (mode == GL_QUADS && (count & 3) == 0) {
		for (int i = 0; i < 2; i++) {

			const int offs = 3 - i;
			VERTEX& src = _varray[p - offs];
			VERTEX& dst = _varray[p];

			if (hasTexture) {
				dst.u = src.u;
				dst.v = src.v;
			}
			if (hasColor) {
				dst.color = src.color;
			}
			//if (hasNormal) {
			//	dst.normal = src.normal;
			//}

			dst.x = src.x;
			dst.y = src.y;
			dst.z = src.z;

			++vertices;
			++p;
		}
	}

	VERTEX& vertex = _varray[p];

	if (hasTexture) {
		vertex.u = u;
		vertex.v = v;
	}
	if (hasColor) {
		vertex.color = _color;
	}
	//if (hasNormal) {
	//	vertex.normal = _normal;
	//}

	vertex.x = _sx * (x + xo);
	vertex.y = _sy * (y + yo);
	vertex.z = z + zo;

	++p;
	++vertices;

	if ((vertices & 3) == 0 && p >= maxVertices-1) {
		for (int i = 0; i < 3; ++i)
			printf("Overwriting the vertex buffer! This chunk/entity won't show up\n");
		clear();
	}
#endif
}

void Tesselator::noColor()
{
	_noColor = true;
}

void Tesselator::setAccessMode(int mode)
{
	accessMode = mode;
}

void Tesselator::normal( float x, float y, float z )
{
	static int _warn_t = 0;
	if ((++_warn_t & 32767) == 1)
		LOGI("WARNING: Can't use normals (Tesselator::normal)\n");
	return;

	if (!tesselating) printf("But..");
	hasNormal = true;
	char xx = (char) (x * 128);
	char yy = (char) (y * 127);
	char zz = (char) (z * 127);

	_normal = xx | (yy << 8) | (zz << 16);
}

void Tesselator::offset( float xo, float yo, float zo ) {
	this->xo = xo;
	this->yo = yo;
	this->zo = zo;
}

void Tesselator::addOffset( float x, float y, float z ) {
	xo += x;
	yo += y;
	zo += z;
}

void Tesselator::offset( const Vec3& v ) {
	xo = v.x;
	yo = v.y;
	zo = v.z;
}

void Tesselator::addOffset( const Vec3& v ) {
	xo += v.x;
	yo += v.y;
	zo += v.z;
}

void Tesselator::draw()
{
#ifndef STANDALONE_SERVER
	if (!tesselating)
		LOGI("not (draw) tesselating!\n");

	if (!tesselating || _voidBeginEnd)
		return;

	tesselating = false;

	if (vertices > 0) {
		if (++vboId >= vboCounts)
			vboId = 0;

		int bufferId = vboIds[vboId];
		
		int access = GL_DYNAMIC_DRAW;//(accessMode==ACCESS_DYNAMIC) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
		int bytes = p * sizeof(VERTEX);
		glBindBuffer2(GL_ARRAY_BUFFER, bufferId);
		glBufferData2(GL_ARRAY_BUFFER, bytes, _varray, access); // GL_STREAM_DRAW

		if (hasTexture) {
			glTexCoordPointer2(2, GL_FLOAT, sizeof(VERTEX), (GLvoid*) (3 * 4));
			//glTexCoordPointer2(2, GL_FLOAT, sizeof(VERTEX), (GLvoid*) &_varray->u);
			glEnableClientState2(GL_TEXTURE_COORD_ARRAY);
		}
		if (hasColor) {
			glColorPointer2(4, GL_UNSIGNED_BYTE, sizeof(VERTEX), (GLvoid*) (5 * 4));
			//glColorPointer2(4, GL_UNSIGNED_BYTE, sizeof(VERTEX), (GLvoid*) &_varray->color);
			glEnableClientState2(GL_COLOR_ARRAY);
		}
		//if (hasNormal) {
		//	glNormalPointer(GL_BYTE, sizeof(VERTEX), (GLvoid*) (6 * 4));
		//	glEnableClientState2(GL_NORMAL_ARRAY);
		//}
		//glVertexPointer2(3, GL_FLOAT, sizeof(VERTEX), (GLvoid*)&_varray);
		glVertexPointer2(3, GL_FLOAT, sizeof(VERTEX), 0);
		glEnableClientState2(GL_VERTEX_ARRAY);

		if (mode == GL_QUADS) {
			glDrawArrays2(GL_TRIANGLES, 0, vertices);
		} else {
			glDrawArrays2(mode, 0, vertices);
		}

		glDisableClientState2(GL_VERTEX_ARRAY);
		if (hasTexture) glDisableClientState2(GL_TEXTURE_COORD_ARRAY);
		if (hasColor) glDisableClientState2(GL_COLOR_ARRAY);
		//if (hasNormal) glDisableClientState2(GL_NORMAL_ARRAY);
	}

	clear();
#endif
}

void Tesselator::voidBeginAndEndCalls(bool doVoid) {
	_voidBeginEnd = doVoid;
}

void Tesselator::enableColor() {
	_noColor = false;
}
