/*
 * gl2citro3d.h - OpenGL ES 1.1 -> Citro3D Translation Layer
 *
 * Implements a subset of OpenGL ES 1.1 (fixed-function pipeline) on top of
 * the Nintendo 3DS Citro3D library and PICA200 GPU hardware.
 *
 * Designed for use with Minecraft PE (minecraftpe-nx) but usable by any
 * application that uses GL ES 1.1 with vertex arrays and VBOs.
 *
 * Usage:
 *   #include "gl2citro3d.h"
 *   // replaces <GLES/gl.h> - provides all gl* function declarations
 *
 * Initialization:
 *   gl2c3d_init();    // call once at startup after gfxInitDefault()
 *   gl2c3d_fini();    // call at shutdown
 *   gl2c3d_frame_begin(); // call at start of each frame
 *   gl2c3d_frame_end();   // call at end of each frame
 */

#ifndef GL2CITRO3D_H
#define GL2CITRO3D_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/* ========================================================================= */
/* GL Type Definitions                                                       */
/* ========================================================================= */

typedef unsigned int    GLenum;
typedef unsigned char   GLboolean;
typedef unsigned int    GLbitfield;
typedef void            GLvoid;
typedef int8_t          GLbyte;
typedef uint8_t         GLubyte;
typedef int16_t         GLshort;
typedef uint16_t        GLushort;
typedef int             GLint;
typedef unsigned int    GLuint;
typedef int             GLsizei;
typedef float           GLfloat;
typedef float           GLclampf;
typedef double          GLclampd;
typedef int             GLfixed;
typedef ptrdiff_t       GLintptr;
typedef ptrdiff_t       GLsizeiptr;

/* ========================================================================= */
/* GL Constants                                                              */
/* ========================================================================= */

/* Boolean */
#define GL_FALSE                    0
#define GL_TRUE                     1

/* Errors */
#define GL_NO_ERROR                 0
#define GL_INVALID_ENUM             0x0500
#define GL_INVALID_VALUE            0x0501
#define GL_INVALID_OPERATION        0x0502
#define GL_OUT_OF_MEMORY            0x0505

/* Data types */
#define GL_BYTE                     0x1400
#define GL_UNSIGNED_BYTE            0x1401
#define GL_SHORT                    0x1402
#define GL_UNSIGNED_SHORT           0x1403
#define GL_INT                      0x1404
#define GL_UNSIGNED_INT             0x1405
#define GL_FLOAT                    0x1406

/* Primitives */
#define GL_POINTS                   0x0000
#define GL_LINES                    0x0001
#define GL_LINE_LOOP                0x0002
#define GL_LINE_STRIP               0x0003
#define GL_TRIANGLES                0x0004
#define GL_TRIANGLE_STRIP           0x0005
#define GL_TRIANGLE_FAN             0x0006
#define GL_QUADS                    0x0007

/* Matrix mode */
#define GL_MODELVIEW                0x1700
#define GL_PROJECTION               0x1701
#define GL_TEXTURE                  0x1702

/* Client state arrays */
#define GL_VERTEX_ARRAY             0x8074
#define GL_NORMAL_ARRAY             0x8075
#define GL_COLOR_ARRAY              0x8076
#define GL_TEXTURE_COORD_ARRAY      0x8078

/* Enable caps */
#define GL_FOG                      0x0B60
#define GL_LIGHTING                 0x0B50
#define GL_TEXTURE_2D               0x0DE1
#define GL_CULL_FACE                0x0B44
#define GL_ALPHA_TEST               0x0BC0
#define GL_BLEND                    0x0BE2
#define GL_COLOR_LOGIC_OP           0x0BF2
#define GL_DITHER                   0x0BD0
#define GL_STENCIL_TEST             0x0B90
#define GL_DEPTH_TEST               0x0B71
#define GL_SCISSOR_TEST             0x0C11
#define GL_POLYGON_OFFSET_FILL      0x8037
#define GL_SAMPLE_ALPHA_TO_COVERAGE 0x809E
#define GL_SAMPLE_COVERAGE          0x80A0
#define GL_MULTISAMPLE              0x809D
#define GL_RESCALE_NORMAL           0x803A
#define GL_NORMALIZE                0x0BA1
#define GL_COLOR_MATERIAL           0x0B57

/* Fog params */
#define GL_FOG_MODE                 0x0B65
#define GL_FOG_DENSITY              0x0B62
#define GL_FOG_START                0x0B63
#define GL_FOG_END                  0x0B64
#define GL_FOG_COLOR                0x0B66
#define GL_EXP                      0x0800
#define GL_EXP2                     0x0801
#define GL_LINEAR                   0x2601

/* Shade model */
#define GL_FLAT                     0x1D00
#define GL_SMOOTH                   0x1D01

/* Depth func / alpha func */
#define GL_NEVER                    0x0200
#define GL_LESS                     0x0201
#define GL_EQUAL                    0x0202
#define GL_LEQUAL                   0x0203
#define GL_GREATER                  0x0204
#define GL_NOTEQUAL                 0x0205
#define GL_GEQUAL                   0x0206
#define GL_ALWAYS                   0x0207

/* Blend factors */
#define GL_ZERO                     0
#define GL_ONE                      1
#define GL_SRC_COLOR                0x0300
#define GL_ONE_MINUS_SRC_COLOR      0x0301
#define GL_SRC_ALPHA                0x0302
#define GL_ONE_MINUS_SRC_ALPHA      0x0303
#define GL_DST_ALPHA                0x0304
#define GL_ONE_MINUS_DST_ALPHA      0x0305
#define GL_DST_COLOR                0x0306
#define GL_ONE_MINUS_DST_COLOR      0x0307
#define GL_SRC_ALPHA_SATURATE       0x0308

/* Texture target */
#define GL_TEXTURE_2D               0x0DE1

/* Texture parameters */
#define GL_TEXTURE_WRAP_S           0x2802
#define GL_TEXTURE_WRAP_T           0x2803
#define GL_TEXTURE_MAG_FILTER       0x2800
#define GL_TEXTURE_MIN_FILTER       0x2801
#define GL_NEAREST                  0x2600
/* GL_LINEAR defined above as 0x2601 */
#define GL_NEAREST_MIPMAP_NEAREST   0x2700
#define GL_LINEAR_MIPMAP_NEAREST    0x2701
#define GL_NEAREST_MIPMAP_LINEAR    0x2702
#define GL_LINEAR_MIPMAP_LINEAR     0x2703
#define GL_REPEAT                   0x2901
#define GL_CLAMP_TO_EDGE            0x812F
#define GL_MIRRORED_REPEAT          0x8370

/* Texture formats */
#define GL_ALPHA                    0x1906
#define GL_RGB                      0x1907
#define GL_RGBA                     0x1908
#define GL_LUMINANCE                0x1909
#define GL_LUMINANCE_ALPHA          0x190A
#define GL_RGBA8_OES                0x8058
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG  0x8C02
#define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG   0x8C00

/* Pixel types */
#define GL_UNSIGNED_SHORT_4_4_4_4   0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1   0x8034
#define GL_UNSIGNED_SHORT_5_6_5     0x8363

/* Buffer objects */
#define GL_ARRAY_BUFFER             0x8892
#define GL_ELEMENT_ARRAY_BUFFER     0x8893
#define GL_STATIC_DRAW              0x88E4
#define GL_DYNAMIC_DRAW             0x88E8
#define GL_STREAM_DRAW              0x88E0

/* Clear bits */
#define GL_DEPTH_BUFFER_BIT         0x00000100
#define GL_STENCIL_BUFFER_BIT       0x00000400
#define GL_COLOR_BUFFER_BIT         0x00004000

/* Cull face */
#define GL_FRONT                    0x0404
#define GL_BACK                     0x0405
#define GL_FRONT_AND_BACK           0x0408
#define GL_CW                       0x0900
#define GL_CCW                      0x0901

/* Get pnames */
#define GL_MODELVIEW_MATRIX         0x0BA6
#define GL_PROJECTION_MATRIX        0x0BA7
#define GL_TEXTURE_MATRIX           0x0BA8
#define GL_VIEWPORT                 0x0BA2
#define GL_MAX_TEXTURE_SIZE         0x0D33

/* Hints */
#define GL_PERSPECTIVE_CORRECTION_HINT 0x0C50
#define GL_FASTEST                  0x1101
#define GL_NICEST                   0x1102
#define GL_DONT_CARE                0x1100

/* Framebuffer */
#define GL_FRAMEBUFFER              0x8D40
#define GL_RENDERBUFFER             0x8D41
#define GL_COLOR_ATTACHMENT0        0x8CE0
#define GL_DEPTH_ATTACHMENT         0x8D00
#define GL_DEPTH24_STENCIL8_OES     0x88F0

/* Polygon mode (stubbed) */
#define GL_FILL                     0x1B02
#define GL_LINE                     0x1B01
#define GL_POINT                    0x1B00

/* glGetString */
#define GL_VENDOR                   0x1F00
#define GL_RENDERER                 0x1F01
#define GL_VERSION                  0x1F02
#define GL_EXTENSIONS               0x1F03

/* Display list (compatibility stubs) */
#define GL_COMPILE                  0x1300
#define GL_COMPILE_AND_EXECUTE      0x1301

/* ========================================================================= */
/* Library Init/Shutdown                                                     */
/* ========================================================================= */

void gl2c3d_init(void);
void gl2c3d_fini(void);
void gl2c3d_frame_begin(void);
void gl2c3d_frame_end(void);

/* Set render target for left/right eye (for stereoscopic 3D) */
void gl2c3d_set_render_target(int is_right_eye);

/* ========================================================================= */
/* GL Function Declarations                                                  */
/* ========================================================================= */

/* Error */
GLenum glGetError(void);

/* Clear */
void glClear(GLbitfield mask);
void glClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a);
void glClearDepthf(GLclampf depth);

/* Viewport & Scissor */
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);

/* Enable/Disable */
void glEnable(GLenum cap);
void glDisable(GLenum cap);

/* Depth */
void glDepthFunc(GLenum func);
void glDepthMask(GLboolean flag);
void glDepthRangef(GLclampf near_val, GLclampf far_val);

/* Blend */
void glBlendFunc(GLenum sfactor, GLenum dfactor);

/* Alpha test */
void glAlphaFunc(GLenum func, GLclampf ref);

/* Cull face */
void glCullFace(GLenum mode);
void glFrontFace(GLenum mode);

/* Color */
void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void glColor3f(GLfloat r, GLfloat g, GLfloat b);
void glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a);

/* Shade model */
void glShadeModel(GLenum mode);

/* Matrix ops */
void glMatrixMode(GLenum mode);
void glLoadIdentity(void);
void glPushMatrix(void);
void glPopMatrix(void);
void glTranslatef(GLfloat x, GLfloat y, GLfloat z);
void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void glScalef(GLfloat x, GLfloat y, GLfloat z);
void glMultMatrixf(const GLfloat *m);
void glLoadMatrixf(const GLfloat *m);
void glOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top,
              GLfloat near_val, GLfloat far_val);

/* Texture */
void glGenTextures(GLsizei n, GLuint *textures);
void glDeleteTextures(GLsizei n, const GLuint *textures);
void glBindTexture(GLenum target, GLuint texture);
void glTexImage2D(GLenum target, GLint level, GLint internalformat,
                  GLsizei width, GLsizei height, GLint border,
                  GLenum format, GLenum type, const GLvoid *pixels);
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                     GLsizei width, GLsizei height, GLenum format,
                     GLenum type, const GLvoid *pixels);
void glTexParameteri(GLenum target, GLenum pname, GLint param);
void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat,
                            GLsizei width, GLsizei height, GLint border,
                            GLsizei imageSize, const GLvoid *data);

/* VBO */
void glGenBuffers(GLsizei n, GLuint *buffers);
void glDeleteBuffers(GLsizei n, const GLuint *buffers);
void glBindBuffer(GLenum target, GLuint buffer);
void glBufferData(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);

/* Vertex arrays */
void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer);
void glEnableClientState(GLenum cap);
void glDisableClientState(GLenum cap);

/* Draw */
void glDrawArrays(GLenum mode, GLint first, GLsizei count);
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);

/* Fog */
void glFogf(GLenum pname, GLfloat param);
void glFogfv(GLenum pname, const GLfloat *params);
void glFogx(GLenum pname, GLfixed param);

/* Lighting (stubs for compatibility) */
void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz);

/* Query */
void glGetFloatv(GLenum pname, GLfloat *params);
void glGetIntegerv(GLenum pname, GLint *params);
const GLubyte* glGetString(GLenum name);

/* Misc */
void glHint(GLenum target, GLenum mode);
void glPolygonOffset(GLfloat factor, GLfloat units);
void glLineWidth(GLfloat width);
void glPolygonMode(GLenum face, GLenum mode);
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                  GLenum format, GLenum type, GLvoid *pixels);
void glFlush(void);
void glFinish(void);
void glPixelStorei(GLenum pname, GLint param);

/* Framebuffer objects (passthrough stubs - real FBO handled by main_ctr) */
void glGenFramebuffers(GLsizei n, GLuint *ids);
void glDeleteFramebuffers(GLsizei n, const GLuint *ids);
void glBindFramebuffer(GLenum target, GLuint framebuffer);
void glGenRenderbuffers(GLsizei n, GLuint *ids);
void glDeleteRenderbuffers(GLsizei n, const GLuint *ids);
void glBindRenderbuffer(GLenum target, GLuint renderbuffer);
void glRenderbufferStorage(GLenum target, GLenum internalformat,
                           GLsizei width, GLsizei height);
void glFramebufferRenderbuffer(GLenum target, GLenum attachment,
                               GLenum renderbuffertarget, GLuint renderbuffer);

/* Display lists (compatibility stubs - MCPE uses these for font rendering) */
GLuint glGenLists(GLsizei range);
void glNewList(GLuint list, GLenum mode);
void glEndList(void);
void glCallList(GLuint list);
void glCallLists(GLsizei n, GLenum type, const GLvoid *lists);
void glDeleteLists(GLuint list, GLsizei range);

#ifdef __cplusplus
}
#endif

#endif /* GL2CITRO3D_H */
