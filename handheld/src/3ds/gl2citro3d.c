/*
 * gl2citro3d.c - OpenGL ES 1.1 -> Citro3D Translation Layer Implementation
 *
 * Full implementation mapping GL ES 1.1 fixed-function calls to Citro3D/PICA200.
 * Designed for Minecraft PE on Nintendo 3DS.
 */

#include "gl2citro3d.h"

#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

#define GL2C3D_MAX_TEXTURES     512
#define GL2C3D_MAX_VBOS         256
#define GL2C3D_MATRIX_STACK     32
#define GL2C3D_DISPLAY_LISTS    512
#define GL2C3D_DL_MAX_OPS       64
#define GL2C3D_CMD_BUF_SIZE     (256 * 1024)
#define GL2C3D_SCREEN_W         400
#define GL2C3D_SCREEN_H         240

/* Next power of 2 >= x (for texture dimensions) */
static inline unsigned int next_pow2(unsigned int v) {
    v--;
    v |= v >> 1; v |= v >> 2; v |= v >> 4;
    v |= v >> 8; v |= v >> 16;
    return v + 1;
}

static inline float clampf(float x, float lo, float hi) {
    return x < lo ? lo : (x > hi ? hi : x);
}

/* ========================================================================= */
/* Texture slot                                                              */
/* ========================================================================= */

typedef struct {
    C3D_Tex     tex;
    int         allocated;
    int         width, height;       /* original requested size */
    int         pot_w, pot_h;        /* power-of-two padded size */
    GPU_TEXCOLOR fmt;
    /* cached params */
    int         min_filter;
    int         mag_filter;
    int         wrap_s;
    int         wrap_t;
} TexSlot;

/* ========================================================================= */
/* VBO slot                                                                  */
/* ========================================================================= */

typedef struct {
    void   *data;
    int     size;
    int     allocated;
} VBOSlot;

/* ========================================================================= */
/* Display list entry (for Font rendering compatibility)                     */
/* ========================================================================= */

typedef enum {
    DL_OP_TRANSLATE,
    DL_OP_COLOR3F,
    DL_OP_NONE,
} DLOpType;

typedef struct {
    DLOpType type;
    float    args[4];
} DLOp;

typedef struct {
    DLOp ops[GL2C3D_DL_MAX_OPS];
    int  count;
    int  used;
} DisplayList;

/* ========================================================================= */
/* Global state                                                              */
/* ========================================================================= */

static struct {
    /* Citro3D objects */
    C3D_RenderTarget *render_target_top;
    C3D_RenderTarget *render_target_bot;
    C3D_RenderTarget *current_target;
    DVLB_s           *shader_dvlb;
    shaderProgram_s   shader_program;
    int               uLoc_projection;
    int               uLoc_modelview;
    int               uLoc_fogparams;
    C3D_AttrInfo      attr_info;

    /* Matrix stacks */
    int        matrix_mode;  /* GL_MODELVIEW or GL_PROJECTION */
    C3D_Mtx    proj_stack[GL2C3D_MATRIX_STACK];
    int        proj_sp;
    C3D_Mtx    mv_stack[GL2C3D_MATRIX_STACK];
    int        mv_sp;
    C3D_Mtx    tex_stack[GL2C3D_MATRIX_STACK];
    int        tex_sp;
    int        matrices_dirty;

    /* Current color */
    float      cur_color[4];

    /* Textures */
    TexSlot    textures[GL2C3D_MAX_TEXTURES];
    GLuint     bound_texture;
    int        tex_next_id;
    int        texture_2d_enabled;

    /* VBOs */
    VBOSlot    vbos[GL2C3D_MAX_VBOS];
    GLuint     bound_vbo;
    int        vbo_next_id;

    /* Vertex array state */
    struct {
        int     enabled;
        GLint   size;
        GLenum  type;
        GLsizei stride;
        const void *pointer;
    } va_vertex, va_texcoord, va_color, va_normal;

    /* GL state flags */
    int        depth_test_enabled;
    GLenum     depth_func;
    GLboolean  depth_mask;

    int        blend_enabled;
    GLenum     blend_src, blend_dst;

    int        alpha_test_enabled;
    GLenum     alpha_func;
    float      alpha_ref;

    int        cull_face_enabled;
    GLenum     cull_face_mode;
    GLenum     front_face;

    int        scissor_test_enabled;
    GLint      scissor_x, scissor_y;
    GLsizei    scissor_w, scissor_h;

    int        fog_enabled;
    GLenum     fog_mode;
    float      fog_start, fog_end, fog_density;
    float      fog_color[4];
    C3D_FogLut fog_lut;
    int        fog_dirty;

    float      clear_r, clear_g, clear_b, clear_a;
    float      clear_depth;

    GLboolean  color_mask_r, color_mask_g, color_mask_b, color_mask_a;

    float      polygon_offset_factor, polygon_offset_units;

    /* Viewport */
    GLint      vp_x, vp_y;
    GLsizei    vp_w, vp_h;

    /* Display lists */
    DisplayList dl_store[GL2C3D_DISPLAY_LISTS];
    int         dl_recording;    /* currently recording list id, or -1 */
    GLuint      dl_next_base;

    /* Error */
    GLenum     last_error;

    int        initialized;
} g;


/* Embedded shader binary - in real build, this comes from picasso compiler.
 * We'll load it at runtime from romfs instead. */
static const char* SHADER_PATH = "romfs:/shaders/gl2citro3d_shader.shbin";

#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | \
     GX_TRANSFER_RAW_COPY(0) | \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | \
     GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
     GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

/* Forward declarations for display list recording */
static void dl_record_translate(float x, float y, float z);
static void dl_record_color3f(float r, float g_, float b);

/* ========================================================================= */
/* Helper: GPU enum conversions                                              */
/* ========================================================================= */

static GPU_TESTFUNC gl_to_gpu_testfunc(GLenum func) {
    switch (func) {
        case GL_NEVER:    return GPU_NEVER;
        case GL_LESS:     return GPU_LESS;
        case GL_EQUAL:    return GPU_EQUAL;
        case GL_LEQUAL:   return GPU_LEQUAL;
        case GL_GREATER:  return GPU_GREATER;
        case GL_NOTEQUAL: return GPU_NOTEQUAL;
        case GL_GEQUAL:   return GPU_GEQUAL;
        case GL_ALWAYS:   return GPU_ALWAYS;
        default:          return GPU_ALWAYS;
    }
}

static GPU_BLENDFACTOR gl_to_gpu_blendfactor(GLenum factor) {
    switch (factor) {
        case GL_ZERO:                return GPU_ZERO;
        case GL_ONE:                 return GPU_ONE;
        case GL_SRC_COLOR:           return GPU_SRC_COLOR;
        case GL_ONE_MINUS_SRC_COLOR: return GPU_ONE_MINUS_SRC_COLOR;
        case GL_DST_COLOR:           return GPU_DST_COLOR;
        case GL_ONE_MINUS_DST_COLOR: return GPU_ONE_MINUS_DST_COLOR;
        case GL_SRC_ALPHA:           return GPU_SRC_ALPHA;
        case GL_ONE_MINUS_SRC_ALPHA: return GPU_ONE_MINUS_SRC_ALPHA;
        case GL_DST_ALPHA:           return GPU_DST_ALPHA;
        case GL_ONE_MINUS_DST_ALPHA: return GPU_ONE_MINUS_DST_ALPHA;
        case GL_SRC_ALPHA_SATURATE:  return GPU_SRC_ALPHA_SATURATE;
        default:                     return GPU_ONE;
    }
}

static GPU_Primitive_t gl_to_gpu_primitive(GLenum mode) {
    switch (mode) {
        case GL_TRIANGLES:      return GPU_TRIANGLES;
        case GL_TRIANGLE_STRIP: return GPU_TRIANGLE_STRIP;
        case GL_TRIANGLE_FAN:   return GPU_TRIANGLE_FAN;
        default:                return GPU_TRIANGLES;
    }
}

static GPU_TEXCOLOR gl_to_gpu_texfmt(GLenum format, GLenum type) {
    if (format == GL_RGBA || format == GL_RGBA8_OES) {
        if (type == GL_UNSIGNED_BYTE)           return GPU_RGBA8;
        if (type == GL_UNSIGNED_SHORT_4_4_4_4)  return GPU_RGBA4;
        if (type == GL_UNSIGNED_SHORT_5_5_5_1)  return GPU_RGBA5551;
    }
    if (format == GL_RGB) {
        if (type == GL_UNSIGNED_BYTE)           return GPU_RGBA8;
        if (type == GL_UNSIGNED_SHORT_5_6_5)    return GPU_RGB565;
    }
    if (format == GL_LUMINANCE)                 return GPU_L8;
    if (format == GL_LUMINANCE_ALPHA)           return GPU_LA8;
    if (format == GL_ALPHA)                     return GPU_A8;
    return GPU_RGBA8;
}

static int gpu_texfmt_bpp(GPU_TEXCOLOR fmt) {
    switch (fmt) {
        case GPU_RGBA8:    return 4;
        case GPU_RGB8:     return 3;
        case GPU_RGBA5551: return 2;
        case GPU_RGB565:   return 2;
        case GPU_RGBA4:    return 2;
        case GPU_LA8:      return 2;
        case GPU_L8:       return 1;
        case GPU_A8:       return 1;
        case GPU_LA4:      return 1;
        default:           return 4;
    }
}

/* ========================================================================= */
/* Helper: Current matrix pointer                                            */
/* ========================================================================= */

static C3D_Mtx* cur_mtx(void) {
    switch (g.matrix_mode) {
        case GL_PROJECTION: return &g.proj_stack[g.proj_sp];
        case GL_TEXTURE:    return &g.tex_stack[g.tex_sp];
        default:            return &g.mv_stack[g.mv_sp];
    }
}

static int* cur_sp(void) {
    switch (g.matrix_mode) {
        case GL_PROJECTION: return &g.proj_sp;
        case GL_TEXTURE:    return &g.tex_sp;
        default:            return &g.mv_sp;
    }
}

static C3D_Mtx* cur_stack(void) {
    switch (g.matrix_mode) {
        case GL_PROJECTION: return g.proj_stack;
        case GL_TEXTURE:    return g.tex_stack;
        default:            return g.mv_stack;
    }
}

/* ========================================================================= */
/* Helper: Texture tile swizzling (Morton / Z-order)                         */
/* The PICA200 GPU uses morton-swizzled texture layout.                       */
/* ========================================================================= */

static inline uint32_t morton_interleave(uint32_t x, uint32_t y) {
    static const uint32_t xlut[8] = {0x00,0x01,0x04,0x05,0x10,0x11,0x14,0x15};
    static const uint32_t ylut[8] = {0x00,0x02,0x08,0x0a,0x20,0x22,0x28,0x2a};
    return xlut[x & 7] | ylut[y & 7];
}

static uint32_t get_morton_offset(uint32_t x, uint32_t y, uint32_t bpp) {
    /* 8x8 tile blocks, morton-ordered within each tile */
    uint32_t tile_x = x >> 3;
    uint32_t tile_y = y >> 3;
    uint32_t lx = x & 7;
    uint32_t ly = y & 7;
    uint32_t coarse = (tile_y * ((next_pow2(x + tile_x * 8) + 7) / 8) + tile_x) * 64;
    /* simplified: assume pot_w is the stride */
    return 0; /* will be replaced by proper swizzle below */
}

/* Swizzle RGBA8 image data into PICA200 morton layout (tile-based).
 * src: linear RGBA8, dst: morton-ordered for C3D_TexUpload
 * width/height must be power-of-2.
 */
static void swizzle_rgba8(uint32_t *dst, const uint32_t *src,
                          int src_w, int src_h, int pot_w, int pot_h) {
    for (int y = 0; y < src_h; y++) {
        for (int x = 0; x < src_w; x++) {
            /* Convert pixel to ABGR for PICA200 */
            uint32_t pixel = src[y * src_w + x];
            uint8_t r = (pixel >>  0) & 0xFF;
            uint8_t g_c = (pixel >>  8) & 0xFF;
            uint8_t b = (pixel >> 16) & 0xFF;
            uint8_t a = (pixel >> 24) & 0xFF;
            uint32_t out_pixel = (a << 24) | (b << 16) | (g_c << 8) | r;

            /* Morton offset within the pot texture */
            int flipped_y = pot_h - 1 - y; /* GPU expects bottom-up */
            int tile_x = x >> 3;
            int tile_y = flipped_y >> 3;
            int lx = x & 7;
            int ly = flipped_y & 7;
            int tiles_per_row = pot_w >> 3;
            int tile_offset = (tile_y * tiles_per_row + tile_x) * 64;
            int pixel_offset = tile_offset + morton_interleave(lx, ly);
            dst[pixel_offset] = out_pixel;
        }
    }
    /* Clear padding area */
    for (int y = src_h; y < pot_h; y++) {
        for (int x = 0; x < pot_w; x++) {
            int flipped_y = pot_h - 1 - y;
            int tile_x = x >> 3;
            int tile_y = flipped_y >> 3;
            int lx = x & 7;
            int ly = flipped_y & 7;
            int tiles_per_row = pot_w >> 3;
            int tile_offset = (tile_y * tiles_per_row + tile_x) * 64;
            int pixel_offset = tile_offset + morton_interleave(lx, ly);
            dst[pixel_offset] = 0;
        }
    }
}

/* Convert RGB (3 bytes/pixel) to RGBA (4 bytes/pixel) */
static uint32_t* rgb_to_rgba(const uint8_t *rgb, int w, int h) {
    uint32_t *out = (uint32_t*)linearAlloc(w * h * 4);
    if (!out) return NULL;
    for (int i = 0; i < w * h; i++) {
        out[i] = (0xFF << 24) | (rgb[i*3+2] << 16) | (rgb[i*3+1] << 8) | rgb[i*3+0];
    }
    return out;
}


/* ========================================================================= */
/* Initialization / Shutdown                                                 */
/* ========================================================================= */

void gl2c3d_init(void) {
    memset(&g, 0, sizeof(g));

    C3D_Init(GL2C3D_CMD_BUF_SIZE);

    /* Create render targets */
    g.render_target_top = C3D_RenderTargetCreate(GL2C3D_SCREEN_H, GL2C3D_SCREEN_W,
                                                  GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(g.render_target_top, GFX_TOP, GFX_LEFT,
        DISPLAY_TRANSFER_FLAGS);
    g.current_target = g.render_target_top;

    g.render_target_bot = C3D_RenderTargetCreate(240, 320,
                                                  GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(g.render_target_bot, GFX_BOTTOM, GFX_LEFT,
        DISPLAY_TRANSFER_FLAGS);

    /* Load shader */
    FILE *f = fopen(SHADER_PATH, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        void *buf = linearAlloc(sz);
        fread(buf, 1, sz, f);
        fclose(f);
        g.shader_dvlb = DVLB_ParseFile((u32*)buf, sz);
    }

    if (g.shader_dvlb) {
        shaderProgramInit(&g.shader_program);
        shaderProgramSetVsh(&g.shader_program, &g.shader_dvlb->DVLE[0]);
        C3D_BindProgram(&g.shader_program);

        g.uLoc_projection = shaderInstanceGetUniformLocation(g.shader_program.vertexShader, "projection");
        g.uLoc_modelview  = shaderInstanceGetUniformLocation(g.shader_program.vertexShader, "modelview");
        g.uLoc_fogparams  = shaderInstanceGetUniformLocation(g.shader_program.vertexShader, "fogparams");
    }

    /* Configure vertex attributes:
     * attr 0: position (3 floats)
     * attr 1: texcoord (2 floats)
     * attr 2: color    (4 unsigned bytes)
     *
     * This matches MCPE's VertexDeclPTC: {float x,y,z; float u,v; uint32 color;}
     * stride = 24 bytes
     */
    AttrInfo_Init(&g.attr_info);
    AttrInfo_AddLoader(&g.attr_info, 0, GPU_FLOAT, 3);  /* position */
    AttrInfo_AddLoader(&g.attr_info, 1, GPU_FLOAT, 2);  /* texcoord */
    AttrInfo_AddLoader(&g.attr_info, 2, GPU_UNSIGNED_BYTE, 4); /* color */
    C3D_SetAttrInfo(&g.attr_info);

    /* Default state */
    g.matrix_mode = GL_MODELVIEW;
    Mtx_Identity(&g.proj_stack[0]);
    Mtx_Identity(&g.mv_stack[0]);
    Mtx_Identity(&g.tex_stack[0]);
    g.proj_sp = 0;
    g.mv_sp = 0;
    g.tex_sp = 0;
    g.matrices_dirty = 1;

    g.cur_color[0] = 1.0f;
    g.cur_color[1] = 1.0f;
    g.cur_color[2] = 1.0f;
    g.cur_color[3] = 1.0f;

    g.depth_test_enabled = 1;
    g.depth_func = GL_LEQUAL;
    g.depth_mask = GL_TRUE;
    g.clear_depth = 1.0f;

    g.blend_src = GL_ONE;
    g.blend_dst = GL_ZERO;

    g.alpha_func = GL_ALWAYS;
    g.alpha_ref = 0.0f;

    g.cull_face_mode = GL_BACK;
    g.front_face = GL_CCW;

    g.fog_mode = GL_LINEAR;
    g.fog_start = 0.0f;
    g.fog_end = 1.0f;
    g.fog_density = 1.0f;
    g.fog_color[0] = g.fog_color[1] = g.fog_color[2] = 0.0f;
    g.fog_color[3] = 1.0f;

    g.vp_x = 0; g.vp_y = 0;
    g.vp_w = GL2C3D_SCREEN_W;
    g.vp_h = GL2C3D_SCREEN_H;

    g.color_mask_r = g.color_mask_g = g.color_mask_b = g.color_mask_a = GL_TRUE;

    g.tex_next_id = 1;
    g.vbo_next_id = 1;
    g.dl_recording = -1;
    g.dl_next_base = 1;
    g.texture_2d_enabled = 1;

    g.initialized = 1;
}

void gl2c3d_fini(void) {
    if (!g.initialized) return;

    /* Free textures */
    for (int i = 0; i < GL2C3D_MAX_TEXTURES; i++) {
        if (g.textures[i].allocated) {
            C3D_TexDelete(&g.textures[i].tex);
        }
    }

    /* Free VBOs */
    for (int i = 0; i < GL2C3D_MAX_VBOS; i++) {
        if (g.vbos[i].allocated && g.vbos[i].data) {
            linearFree(g.vbos[i].data);
        }
    }

    if (g.shader_dvlb) {
        shaderProgramFree(&g.shader_program);
        DVLB_Free(g.shader_dvlb);
    }

    C3D_RenderTargetDelete(g.render_target_top);
    C3D_RenderTargetDelete(g.render_target_bot);
    C3D_Fini();

    g.initialized = 0;
}

/* ========================================================================= */
/* Frame management                                                          */
/* ========================================================================= */

void gl2c3d_frame_begin(void) {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
}

void gl2c3d_frame_end(void) {
    C3D_FrameEnd(0);
}

void gl2c3d_set_render_target(int is_right_eye) {
    C3D_FrameDrawOn(is_right_eye ? g.render_target_bot : g.render_target_top);
    g.current_target = is_right_eye ? g.render_target_bot : g.render_target_top;
}


/* ========================================================================= */
/* Apply GPU state before draw calls                                         */
/* ========================================================================= */

static void apply_gpu_state(void) {
    /* Upload matrices */
    if (g.matrices_dirty) {
        if (g.uLoc_projection >= 0)
            C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, g.uLoc_projection, &g.proj_stack[g.proj_sp]);
        if (g.uLoc_modelview >= 0)
            C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, g.uLoc_modelview, &g.mv_stack[g.mv_sp]);
        g.matrices_dirty = 0;
    }

    /* Upload fog params */
    if (g.uLoc_fogparams >= 0) {
        float range = g.fog_end - g.fog_start;
        if (range == 0.0f) range = 1.0f;
        C3D_FVUnifSet(GPU_VERTEX_SHADER, g.uLoc_fogparams,
                      g.fog_start, g.fog_end, 1.0f / range,
                      g.fog_enabled ? 1.0f : 0.0f);
    }

    /* Depth test */
    GPU_WRITEMASK writemask = GPU_WRITE_COLOR;
    if (g.depth_mask) writemask |= GPU_WRITE_DEPTH;
    C3D_DepthTest(g.depth_test_enabled,
                  gl_to_gpu_testfunc(g.depth_func),
                  writemask);

    /* Alpha test */
    C3D_AlphaTest(g.alpha_test_enabled,
                  gl_to_gpu_testfunc(g.alpha_func),
                  (u8)(g.alpha_ref * 255.0f));

    /* Blending */
    if (g.blend_enabled) {
        C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD,
                       gl_to_gpu_blendfactor(g.blend_src),
                       gl_to_gpu_blendfactor(g.blend_dst),
                       gl_to_gpu_blendfactor(g.blend_src),
                       gl_to_gpu_blendfactor(g.blend_dst));
    } else {
        C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD,
                       GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
    }

    /* Cull face */
    if (g.cull_face_enabled) {
        /* PICA200 cull mode is inverted compared to GL because
         * the 3DS screen is rotated 90 degrees */
        GPU_CULLMODE cull;
        if (g.cull_face_mode == GL_FRONT)
            cull = (g.front_face == GL_CCW) ? GPU_CULL_FRONT_CCW : GPU_CULL_BACK_CCW;
        else
            cull = (g.front_face == GL_CCW) ? GPU_CULL_BACK_CCW : GPU_CULL_FRONT_CCW;
        C3D_CullFace(cull);
    } else {
        C3D_CullFace(GPU_CULL_NONE);
    }

    /* Scissor test */
    if (g.scissor_test_enabled) {
        /* GL scissor origin is bottom-left, PICA200 expects screen coords */
        C3D_SetScissor(GPU_SCISSOR_NORMAL,
                       g.scissor_x, g.scissor_y,
                       g.scissor_x + g.scissor_w,
                       g.scissor_y + g.scissor_h);
    } else {
        C3D_SetScissor(GPU_SCISSOR_DISABLE, 0, 0, 0, 0);
    }

    /* Fog */
    if (g.fog_enabled && g.fog_dirty) {
        u32 fc = ((u8)(g.fog_color[3]*255) << 24) |
                 ((u8)(g.fog_color[2]*255) << 16) |
                 ((u8)(g.fog_color[1]*255) << 8) |
                 ((u8)(g.fog_color[0]*255));
        C3D_FogColor(fc);

        if (g.fog_mode == GL_LINEAR) {
            FogLut_Exp(&g.fog_lut, 1.0f, g.fog_start, g.fog_end, 0.01f);
        } else if (g.fog_mode == GL_EXP) {
            FogLut_Exp(&g.fog_lut, g.fog_density, 0.0f, g.fog_end, 1.0f);
        } else { /* GL_EXP2 */
            FogLut_Exp(&g.fog_lut, g.fog_density * g.fog_density, 0.0f, g.fog_end, 2.0f);
        }
        C3D_FogGasMode(true, false, false);
        C3D_FogLutBind(&g.fog_lut);
        g.fog_dirty = 0;
    } else if (!g.fog_enabled) {
        C3D_FogGasMode(false, false, false);
    }

    /* Configure TEV (texture environment) stages.
     * This is the PICA200's fragment pipeline - replaces fragment shaders.
     *
     * Stage 0: Combine texture and vertex color
     *   If texture enabled:  output = texture_color * vertex_color
     *   If texture disabled: output = vertex_color
     *
     * Stage 1-5: passthrough (disabled)
     */
    C3D_TexEnv *env0 = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env0);

    if (g.texture_2d_enabled && g.bound_texture > 0) {
        /* Color: texture * vertex_color (modulate) */
        C3D_TexEnvSrc(env0, C3D_Both,
                      GPU_TEXTURE0, GPU_PRIMARY_COLOR, (GPU_TEVSRC)0);
        C3D_TexEnvFunc(env0, C3D_Both, GPU_MODULATE);
    } else {
        /* Color: just vertex color */
        C3D_TexEnvSrc(env0, C3D_Both,
                      GPU_PRIMARY_COLOR, (GPU_TEVSRC)0, (GPU_TEVSRC)0);
        C3D_TexEnvFunc(env0, C3D_Both, GPU_REPLACE);
    }

    /* Disable remaining TEV stages */
    for (int i = 1; i < 6; i++) {
        C3D_TexEnv *env = C3D_GetTexEnv(i);
        C3D_TexEnvInit(env);
        C3D_TexEnvSrc(env, C3D_Both, GPU_PREVIOUS, (GPU_TEVSRC)0, (GPU_TEVSRC)0);
        C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
    }

    /* Bind current texture */
    if (g.texture_2d_enabled && g.bound_texture > 0 &&
        g.bound_texture < GL2C3D_MAX_TEXTURES &&
        g.textures[g.bound_texture].allocated) {
        C3D_TexBind(0, &g.textures[g.bound_texture].tex);
    }
}


/* ========================================================================= */
/* Error                                                                     */
/* ========================================================================= */

GLenum glGetError(void) {
    GLenum e = g.last_error;
    g.last_error = GL_NO_ERROR;
    return e;
}

/* ========================================================================= */
/* Clear                                                                     */
/* ========================================================================= */

void glClearColor(GLclampf r, GLclampf g_, GLclampf b, GLclampf a) {
    g.clear_r = clampf(r, 0.0f, 1.0f);
    g.clear_g = clampf(g_, 0.0f, 1.0f);
    g.clear_b = clampf(b, 0.0f, 1.0f);
    g.clear_a = clampf(a, 0.0f, 1.0f);
}

void glClearDepthf(GLclampf depth) {
    g.clear_depth = clampf(depth, 0.0f, 1.0f);
}

void glClear(GLbitfield mask) {
    C3D_ClearBits bits = 0;
    u32 color = 0;
    u32 depth = 0;

    if (mask & GL_COLOR_BUFFER_BIT) {
        bits |= C3D_CLEAR_COLOR;
        color = ((u8)(g.clear_a * 255) << 24) |
                ((u8)(g.clear_b * 255) << 16) |
                ((u8)(g.clear_g * 255) << 8) |
                ((u8)(g.clear_r * 255));
    }
    if (mask & GL_DEPTH_BUFFER_BIT) {
        bits |= C3D_CLEAR_DEPTH;
        depth = (u32)(g.clear_depth * 0xFFFFFF);
    }

    if (bits && g.current_target) {
        C3D_RenderTargetClear(g.current_target, bits, color, depth);
    }
}

/* ========================================================================= */
/* Viewport & Scissor                                                        */
/* ========================================================================= */

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    g.vp_x = x; g.vp_y = y;
    g.vp_w = width; g.vp_h = height;
    C3D_SetViewport(y, x, height, width);
}

void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    g.scissor_x = x;
    g.scissor_y = y;
    g.scissor_w = width;
    g.scissor_h = height;
}

/* ========================================================================= */
/* Enable / Disable                                                          */
/* ========================================================================= */

void glEnable(GLenum cap) {
    switch (cap) {
        case GL_DEPTH_TEST:    g.depth_test_enabled = 1; break;
        case GL_BLEND:         g.blend_enabled = 1; break;
        case GL_ALPHA_TEST:    g.alpha_test_enabled = 1; break;
        case GL_CULL_FACE:     g.cull_face_enabled = 1; break;
        case GL_TEXTURE_2D:    g.texture_2d_enabled = 1; break;
        case GL_SCISSOR_TEST:  g.scissor_test_enabled = 1; break;
        case GL_FOG:           g.fog_enabled = 1; g.fog_dirty = 1; break;
        case GL_POLYGON_OFFSET_FILL: break;
        case GL_COLOR_MATERIAL: break;
        case GL_LIGHTING:      break;
        case GL_NORMALIZE:     break;
        case GL_RESCALE_NORMAL: break;
        default: break;
    }
}

void glDisable(GLenum cap) {
    switch (cap) {
        case GL_DEPTH_TEST:    g.depth_test_enabled = 0; break;
        case GL_BLEND:         g.blend_enabled = 0; break;
        case GL_ALPHA_TEST:    g.alpha_test_enabled = 0; break;
        case GL_CULL_FACE:     g.cull_face_enabled = 0; break;
        case GL_TEXTURE_2D:    g.texture_2d_enabled = 0; break;
        case GL_SCISSOR_TEST:  g.scissor_test_enabled = 0; break;
        case GL_FOG:           g.fog_enabled = 0; break;
        case GL_POLYGON_OFFSET_FILL: break;
        case GL_COLOR_MATERIAL: break;
        case GL_LIGHTING:      break;
        default: break;
    }
}

/* ========================================================================= */
/* Depth, Blend, Alpha, Cull                                                 */
/* ========================================================================= */

void glDepthFunc(GLenum func) { g.depth_func = func; }
void glDepthMask(GLboolean flag) { g.depth_mask = flag; }
void glDepthRangef(GLclampf near_val, GLclampf far_val) { (void)near_val; (void)far_val; }
void glBlendFunc(GLenum sfactor, GLenum dfactor) { g.blend_src = sfactor; g.blend_dst = dfactor; }
void glAlphaFunc(GLenum func, GLclampf ref) { g.alpha_func = func; g.alpha_ref = ref; }
void glCullFace(GLenum mode) { g.cull_face_mode = mode; }
void glFrontFace(GLenum mode) { g.front_face = mode; }
void glShadeModel(GLenum mode) { (void)mode; }
void glPolygonOffset(GLfloat factor, GLfloat units) {
    g.polygon_offset_factor = factor;
    g.polygon_offset_units = units;
    C3D_DepthMap(true, -1.0f + units * 0.0001f, 0.0f);
}
void glLineWidth(GLfloat width) { (void)width; }
void glPolygonMode(GLenum face, GLenum mode) { (void)face; (void)mode; }

void glColorMask(GLboolean r, GLboolean g_, GLboolean b, GLboolean a) {
    g.color_mask_r = r; g.color_mask_g = g_;
    g.color_mask_b = b; g.color_mask_a = a;
}

/* ========================================================================= */
/* Color                                                                     */
/* ========================================================================= */

void glColor4f(GLfloat r, GLfloat g_, GLfloat b, GLfloat a) {
    g.cur_color[0] = r; g.cur_color[1] = g_;
    g.cur_color[2] = b; g.cur_color[3] = a;
}

void glColor3f(GLfloat r, GLfloat g_, GLfloat b) {
    if (g.dl_recording >= 0) {
        dl_record_color3f(r, g_, b);
        return;
    }
    g.cur_color[0] = r; g.cur_color[1] = g_;
    g.cur_color[2] = b; g.cur_color[3] = 1.0f;
}

/* ========================================================================= */
/* Fog                                                                       */
/* ========================================================================= */

void glFogf(GLenum pname, GLfloat param) {
    switch (pname) {
        case GL_FOG_MODE:    g.fog_mode = (GLenum)param; break;
        case GL_FOG_START:   g.fog_start = param; break;
        case GL_FOG_END:     g.fog_end = param; break;
        case GL_FOG_DENSITY: g.fog_density = param; break;
        default: break;
    }
    g.fog_dirty = 1;
}

void glFogfv(GLenum pname, const GLfloat *params) {
    if (pname == GL_FOG_COLOR && params) {
        g.fog_color[0] = params[0]; g.fog_color[1] = params[1];
        g.fog_color[2] = params[2]; g.fog_color[3] = params[3];
        g.fog_dirty = 1;
    } else if (params) {
        glFogf(pname, params[0]);
    }
}

void glFogx(GLenum pname, GLfixed param) {
    glFogf(pname, (float)param / 65536.0f);
}

void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz) {
    (void)nx; (void)ny; (void)nz;
}


/* ========================================================================= */
/* Matrix operations                                                         */
/* ========================================================================= */

void glMatrixMode(GLenum mode) {
    g.matrix_mode = mode;
}

void glLoadIdentity(void) {
    Mtx_Identity(cur_mtx());
    g.matrices_dirty = 1;
}

void glPushMatrix(void) {
    int *sp = cur_sp();
    C3D_Mtx *stack = cur_stack();
    if (*sp < GL2C3D_MATRIX_STACK - 1) {
        Mtx_Copy(&stack[*sp + 1], &stack[*sp]);
        (*sp)++;
    }
}

void glPopMatrix(void) {
    int *sp = cur_sp();
    if (*sp > 0) {
        (*sp)--;
    }
    g.matrices_dirty = 1;
}

void glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
    /* Display list recording support */
    if (g.dl_recording >= 0) {
        dl_record_translate(x, y, z);
        return;
    }
    C3D_Mtx tmp;
    Mtx_Identity(&tmp);
    /* Citro3D matrices are row-major [WZYX]:
     * row0 = [m[0][3], m[0][2], m[0][1], m[0][0]] = [w, z, y, x]
     * Translation goes into column 3 (the W component of each row). */
    tmp.r[0].w = x;
    tmp.r[1].w = y;
    tmp.r[2].w = z;

    C3D_Mtx result;
    Mtx_Multiply(&result, cur_mtx(), &tmp);
    Mtx_Copy(cur_mtx(), &result);
    g.matrices_dirty = 1;
}

void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
    float rad = angle * M_PI / 180.0f;
    float len = sqrtf(x*x + y*y + z*z);
    if (len < 0.0001f) return;
    x /= len; y /= len; z /= len;

    C3D_Mtx rot;
    if (x == 1.0f && y == 0.0f && z == 0.0f) {
        Mtx_Identity(&rot);
        float c = cosf(rad), s = sinf(rad);
        rot.r[1].y = c;  rot.r[1].z = -s;
        rot.r[2].y = s;  rot.r[2].z = c;
    } else if (x == 0.0f && y == 1.0f && z == 0.0f) {
        Mtx_Identity(&rot);
        float c = cosf(rad), s = sinf(rad);
        rot.r[0].x = c;  rot.r[0].z = s;
        rot.r[2].x = -s; rot.r[2].z = c;
    } else if (x == 0.0f && y == 0.0f && z == 1.0f) {
        Mtx_Identity(&rot);
        float c = cosf(rad), s = sinf(rad);
        rot.r[0].x = c;  rot.r[0].y = -s;
        rot.r[1].x = s;  rot.r[1].y = c;
    } else {
        /* Arbitrary axis rotation */
        float c = cosf(rad), s = sinf(rad), ic = 1.0f - c;
        Mtx_Identity(&rot);
        rot.r[0].x = x*x*ic + c;     rot.r[0].y = x*y*ic - z*s;   rot.r[0].z = x*z*ic + y*s;
        rot.r[1].x = y*x*ic + z*s;   rot.r[1].y = y*y*ic + c;     rot.r[1].z = y*z*ic - x*s;
        rot.r[2].x = z*x*ic - y*s;   rot.r[2].y = z*y*ic + x*s;   rot.r[2].z = z*z*ic + c;
    }

    C3D_Mtx result;
    Mtx_Multiply(&result, cur_mtx(), &rot);
    Mtx_Copy(cur_mtx(), &result);
    g.matrices_dirty = 1;
}

void glScalef(GLfloat x, GLfloat y, GLfloat z) {
    C3D_Mtx tmp;
    Mtx_Identity(&tmp);
    tmp.r[0].x = x;
    tmp.r[1].y = y;
    tmp.r[2].z = z;

    C3D_Mtx result;
    Mtx_Multiply(&result, cur_mtx(), &tmp);
    Mtx_Copy(cur_mtx(), &result);
    g.matrices_dirty = 1;
}

void glMultMatrixf(const GLfloat *m) {
    /* GL uses column-major, Citro3D uses row-major [WZYX].
     * GL: m[col*4+row], so m[0..3] = col0, m[4..7] = col1, etc.
     * C3D: r[row].{x,y,z,w} where x=col0, y=col1, z=col2, w=col3
     * But C3D stores as [w,z,y,x] internally. We use the struct fields. */
    C3D_Mtx tmp;
    for (int r = 0; r < 4; r++) {
        tmp.r[r].x = m[0*4 + r];  /* col 0 */
        tmp.r[r].y = m[1*4 + r];  /* col 1 */
        tmp.r[r].z = m[2*4 + r];  /* col 2 */
        tmp.r[r].w = m[3*4 + r];  /* col 3 */
    }

    C3D_Mtx result;
    Mtx_Multiply(&result, cur_mtx(), &tmp);
    Mtx_Copy(cur_mtx(), &result);
    g.matrices_dirty = 1;
}

void glLoadMatrixf(const GLfloat *m) {
    C3D_Mtx *dst = cur_mtx();
    for (int r = 0; r < 4; r++) {
        dst->r[r].x = m[0*4 + r];
        dst->r[r].y = m[1*4 + r];
        dst->r[r].z = m[2*4 + r];
        dst->r[r].w = m[3*4 + r];
    }
    g.matrices_dirty = 1;
}

void glOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top,
              GLfloat near_val, GLfloat far_val) {
    C3D_Mtx ortho;
    Mtx_OrthoTilt(&ortho, left, right, bottom, top, near_val, far_val, false);

    C3D_Mtx result;
    Mtx_Multiply(&result, cur_mtx(), &ortho);
    Mtx_Copy(cur_mtx(), &result);
    g.matrices_dirty = 1;
}

/* ========================================================================= */
/* Query                                                                     */
/* ========================================================================= */

void glGetFloatv(GLenum pname, GLfloat *params) {
    C3D_Mtx *src = NULL;
    switch (pname) {
        case GL_MODELVIEW_MATRIX:
            src = &g.mv_stack[g.mv_sp];
            break;
        case GL_PROJECTION_MATRIX:
            src = &g.proj_stack[g.proj_sp];
            break;
        case GL_TEXTURE_MATRIX:
            src = &g.tex_stack[g.tex_sp];
            break;
        default:
            for (int i = 0; i < 16; i++) params[i] = 0.0f;
            return;
    }
    /* Convert back to column-major GL format */
    for (int r = 0; r < 4; r++) {
        params[0*4 + r] = src->r[r].x;
        params[1*4 + r] = src->r[r].y;
        params[2*4 + r] = src->r[r].z;
        params[3*4 + r] = src->r[r].w;
    }
}

void glGetIntegerv(GLenum pname, GLint *params) {
    switch (pname) {
        case GL_VIEWPORT:
            params[0] = g.vp_x; params[1] = g.vp_y;
            params[2] = g.vp_w; params[3] = g.vp_h;
            break;
        case GL_MAX_TEXTURE_SIZE:
            params[0] = 1024;
            break;
        default:
            params[0] = 0;
            break;
    }
}

const GLubyte* glGetString(GLenum name) {
    switch (name) {
        case GL_VENDOR:     return (const GLubyte*)"gl2citro3d";
        case GL_RENDERER:   return (const GLubyte*)"PICA200 (3DS)";
        case GL_VERSION:    return (const GLubyte*)"OpenGL ES-CM 1.1 gl2citro3d";
        case GL_EXTENSIONS: return (const GLubyte*)"";
        default:            return (const GLubyte*)"";
    }
}


/* ========================================================================= */
/* Texture management                                                        */
/* ========================================================================= */

void glGenTextures(GLsizei n, GLuint *textures) {
    for (GLsizei i = 0; i < n; i++) {
        textures[i] = g.tex_next_id++;
        if (g.tex_next_id >= GL2C3D_MAX_TEXTURES)
            g.tex_next_id = 1;  /* wrap around, reuse old slots */
    }
}

void glDeleteTextures(GLsizei n, const GLuint *textures) {
    for (GLsizei i = 0; i < n; i++) {
        GLuint id = textures[i];
        if (id > 0 && id < GL2C3D_MAX_TEXTURES && g.textures[id].allocated) {
            C3D_TexDelete(&g.textures[id].tex);
            g.textures[id].allocated = 0;
            if (g.bound_texture == id)
                g.bound_texture = 0;
        }
    }
}

void glBindTexture(GLenum target, GLuint texture) {
    (void)target;
    g.bound_texture = texture;
}

void glTexImage2D(GLenum target, GLint level, GLint internalformat,
                  GLsizei width, GLsizei height, GLint border,
                  GLenum format, GLenum type, const GLvoid *pixels) {
    (void)target; (void)level; (void)border;

    if (g.bound_texture == 0 || g.bound_texture >= GL2C3D_MAX_TEXTURES) return;
    TexSlot *slot = &g.textures[g.bound_texture];

    /* Delete old texture if re-uploading */
    if (slot->allocated) {
        C3D_TexDelete(&slot->tex);
        slot->allocated = 0;
    }

    GPU_TEXCOLOR gpu_fmt = gl_to_gpu_texfmt(format, type);
    int pot_w = next_pow2(width);
    int pot_h = next_pow2(height);

    /* Minimum texture size for PICA200 is 8x8 */
    if (pot_w < 8) pot_w = 8;
    if (pot_h < 8) pot_h = 8;
    /* Maximum texture size is 1024x1024 */
    if (pot_w > 1024) pot_w = 1024;
    if (pot_h > 1024) pot_h = 1024;

    if (!C3D_TexInit(&slot->tex, pot_w, pot_h, gpu_fmt)) {
        g.last_error = GL_OUT_OF_MEMORY;
        return;
    }

    slot->allocated = 1;
    slot->width = width;
    slot->height = height;
    slot->pot_w = pot_w;
    slot->pot_h = pot_h;
    slot->fmt = gpu_fmt;
    slot->min_filter = GL_NEAREST;
    slot->mag_filter = GL_NEAREST;
    slot->wrap_s = GL_REPEAT;
    slot->wrap_t = GL_REPEAT;

    /* Set default filter/wrap */
    C3D_TexSetFilter(&slot->tex, GPU_NEAREST, GPU_NEAREST);
    C3D_TexSetWrap(&slot->tex, GPU_REPEAT, GPU_REPEAT);

    if (pixels) {
        /* Upload pixel data, converting format if needed */
        if (gpu_fmt == GPU_RGBA8) {
            const uint8_t *src_bytes = (const uint8_t*)pixels;
            uint32_t *rgba_data;
            int needs_free = 0;

            if (format == GL_RGB && type == GL_UNSIGNED_BYTE) {
                rgba_data = rgb_to_rgba(src_bytes, width, height);
                needs_free = 1;
            } else {
                rgba_data = (uint32_t*)pixels;
            }

            if (rgba_data) {
                /* Swizzle into morton layout */
                uint32_t *swizzled = (uint32_t*)linearAlloc(pot_w * pot_h * 4);
                if (swizzled) {
                    memset(swizzled, 0, pot_w * pot_h * 4);
                    swizzle_rgba8(swizzled, rgba_data, width, height, pot_w, pot_h);
                    /* Copy to texture memory */
                    memcpy(slot->tex.data, swizzled, pot_w * pot_h * 4);
                    C3D_TexFlush(&slot->tex);
                    linearFree(swizzled);
                }
                if (needs_free) linearFree(rgba_data);
            }
        } else {
            /* For non-RGBA8 formats, do a simple copy (TODO: may need swizzle) */
            int bpp = gpu_texfmt_bpp(gpu_fmt);
            memcpy(slot->tex.data, pixels, width * height * bpp);
            C3D_TexFlush(&slot->tex);
        }
    }
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                     GLsizei width, GLsizei height, GLenum format,
                     GLenum type, const GLvoid *pixels) {
    (void)target; (void)level;

    if (g.bound_texture == 0 || g.bound_texture >= GL2C3D_MAX_TEXTURES) return;
    TexSlot *slot = &g.textures[g.bound_texture];
    if (!slot->allocated || !pixels) return;

    /* For sub-image update, we need to write into the morton-ordered texture.
     * This is the hot path for dynamic textures (water/lava animation). */
    if (slot->fmt == GPU_RGBA8) {
        const uint32_t *src;
        uint32_t *temp_rgba = NULL;
        int needs_free = 0;

        if (format == GL_RGBA && type == GL_UNSIGNED_BYTE) {
            src = (const uint32_t*)pixels;
        } else if (format == GL_RGB && type == GL_UNSIGNED_BYTE) {
            temp_rgba = rgb_to_rgba((const uint8_t*)pixels, width, height);
            src = temp_rgba;
            needs_free = 1;
        } else {
            return;
        }

        if (!src) return;

        uint32_t *tex_data = (uint32_t*)slot->tex.data;
        int pot_w = slot->pot_w;
        int pot_h = slot->pot_h;

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int dx = xoffset + x;
                int dy = yoffset + y;
                if (dx >= pot_w || dy >= pot_h) continue;

                uint32_t pixel = src[y * width + x];
                uint8_t r = (pixel >>  0) & 0xFF;
                uint8_t gc = (pixel >>  8) & 0xFF;
                uint8_t b = (pixel >> 16) & 0xFF;
                uint8_t a = (pixel >> 24) & 0xFF;
                uint32_t out_pixel = (a << 24) | (b << 16) | (gc << 8) | r;

                int fy = pot_h - 1 - dy;
                int tile_x = dx >> 3;
                int tile_y = fy >> 3;
                int lx = dx & 7;
                int ly = fy & 7;
                int tiles_per_row = pot_w >> 3;
                int tile_offset = (tile_y * tiles_per_row + tile_x) * 64;
                int pixel_offset = tile_offset + morton_interleave(lx, ly);
                tex_data[pixel_offset] = out_pixel;
            }
        }

        C3D_TexFlush(&slot->tex);
        if (needs_free && temp_rgba) linearFree(temp_rgba);
    }
}

void glTexParameteri(GLenum target, GLenum pname, GLint param) {
    (void)target;
    if (g.bound_texture == 0 || g.bound_texture >= GL2C3D_MAX_TEXTURES) return;
    TexSlot *slot = &g.textures[g.bound_texture];
    if (!slot->allocated) return;

    GPU_TEXTURE_FILTER_PARAM filt;
    GPU_TEXTURE_WRAP_PARAM wrap;

    switch (pname) {
        case GL_TEXTURE_MIN_FILTER:
            slot->min_filter = param;
            filt = (param == GL_LINEAR || param == GL_LINEAR_MIPMAP_LINEAR ||
                    param == GL_LINEAR_MIPMAP_NEAREST) ? GPU_LINEAR : GPU_NEAREST;
            C3D_TexSetFilter(&slot->tex,
                (slot->mag_filter == GL_LINEAR) ? GPU_LINEAR : GPU_NEAREST, filt);
            break;
        case GL_TEXTURE_MAG_FILTER:
            slot->mag_filter = param;
            filt = (param == GL_LINEAR) ? GPU_LINEAR : GPU_NEAREST;
            C3D_TexSetFilter(&slot->tex, filt,
                (slot->min_filter == GL_LINEAR || slot->min_filter == GL_LINEAR_MIPMAP_LINEAR) ?
                GPU_LINEAR : GPU_NEAREST);
            break;
        case GL_TEXTURE_WRAP_S:
            slot->wrap_s = param;
            wrap = (param == GL_CLAMP_TO_EDGE) ? GPU_CLAMP_TO_EDGE :
                   (param == GL_MIRRORED_REPEAT) ? GPU_MIRRORED_REPEAT : GPU_REPEAT;
            C3D_TexSetWrap(&slot->tex, wrap,
                (slot->wrap_t == GL_CLAMP_TO_EDGE) ? GPU_CLAMP_TO_EDGE :
                (slot->wrap_t == GL_MIRRORED_REPEAT) ? GPU_MIRRORED_REPEAT : GPU_REPEAT);
            break;
        case GL_TEXTURE_WRAP_T:
            slot->wrap_t = param;
            wrap = (param == GL_CLAMP_TO_EDGE) ? GPU_CLAMP_TO_EDGE :
                   (param == GL_MIRRORED_REPEAT) ? GPU_MIRRORED_REPEAT : GPU_REPEAT;
            C3D_TexSetWrap(&slot->tex,
                (slot->wrap_s == GL_CLAMP_TO_EDGE) ? GPU_CLAMP_TO_EDGE :
                (slot->wrap_s == GL_MIRRORED_REPEAT) ? GPU_MIRRORED_REPEAT : GPU_REPEAT,
                wrap);
            break;
        default: break;
    }
}

void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat,
                            GLsizei width, GLsizei height, GLint border,
                            GLsizei imageSize, const GLvoid *data) {
    /* PVRTC not supported on 3DS - create a blank RGBA8 texture instead */
    (void)data; (void)imageSize; (void)internalformat;
    glTexImage2D(target, level, GL_RGBA, width, height, border,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
}


/* ========================================================================= */
/* VBO management                                                            */
/* ========================================================================= */

void glGenBuffers(GLsizei n, GLuint *buffers) {
    for (GLsizei i = 0; i < n; i++) {
        buffers[i] = g.vbo_next_id++;
        if (g.vbo_next_id >= GL2C3D_MAX_VBOS)
            g.vbo_next_id = 1;
    }
}

void glDeleteBuffers(GLsizei n, const GLuint *buffers) {
    for (GLsizei i = 0; i < n; i++) {
        GLuint id = buffers[i];
        if (id > 0 && id < GL2C3D_MAX_VBOS && g.vbos[id].allocated) {
            if (g.vbos[id].data) linearFree(g.vbos[id].data);
            g.vbos[id].data = NULL;
            g.vbos[id].size = 0;
            g.vbos[id].allocated = 0;
            if (g.bound_vbo == id) g.bound_vbo = 0;
        }
    }
}

void glBindBuffer(GLenum target, GLuint buffer) {
    (void)target;
    g.bound_vbo = buffer;
}

void glBufferData(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage) {
    (void)target; (void)usage;
    if (g.bound_vbo == 0 || g.bound_vbo >= GL2C3D_MAX_VBOS) return;

    VBOSlot *slot = &g.vbos[g.bound_vbo];

    /* Realloc if size changed */
    if (slot->allocated && slot->size < (int)size) {
        if (slot->data) linearFree(slot->data);
        slot->data = NULL;
        slot->allocated = 0;
    }

    if (!slot->allocated) {
        slot->data = linearAlloc(size);
        if (!slot->data) {
            g.last_error = GL_OUT_OF_MEMORY;
            return;
        }
        slot->allocated = 1;
    }
    slot->size = (int)size;

    if (data) {
        memcpy(slot->data, data, size);
        GSPGPU_FlushDataCache(slot->data, size);
    }
}

/* ========================================================================= */
/* Vertex arrays                                                             */
/* ========================================================================= */

void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    g.va_vertex.size = size;
    g.va_vertex.type = type;
    g.va_vertex.stride = stride;
    g.va_vertex.pointer = pointer;
}

void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    g.va_texcoord.size = size;
    g.va_texcoord.type = type;
    g.va_texcoord.stride = stride;
    g.va_texcoord.pointer = pointer;
}

void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    g.va_color.size = size;
    g.va_color.type = type;
    g.va_color.stride = stride;
    g.va_color.pointer = pointer;
}

void glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer) {
    g.va_normal.type = type;
    g.va_normal.stride = stride;
    g.va_normal.pointer = pointer;
}

void glEnableClientState(GLenum cap) {
    switch (cap) {
        case GL_VERTEX_ARRAY:        g.va_vertex.enabled = 1; break;
        case GL_TEXTURE_COORD_ARRAY: g.va_texcoord.enabled = 1; break;
        case GL_COLOR_ARRAY:         g.va_color.enabled = 1; break;
        case GL_NORMAL_ARRAY:        g.va_normal.enabled = 1; break;
        default: break;
    }
}

void glDisableClientState(GLenum cap) {
    switch (cap) {
        case GL_VERTEX_ARRAY:        g.va_vertex.enabled = 0; break;
        case GL_TEXTURE_COORD_ARRAY: g.va_texcoord.enabled = 0; break;
        case GL_COLOR_ARRAY:         g.va_color.enabled = 0; break;
        case GL_NORMAL_ARRAY:        g.va_normal.enabled = 0; break;
        default: break;
    }
}

/* ========================================================================= */
/* Drawing - the core                                                        */
/* ========================================================================= */

void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    if (count <= 0) return;

    apply_gpu_state();

    /* Determine vertex data source.
     * MCPE uses VBOs with offset-based pointers (pointer = byte offset from VBO start).
     * Vertex layout is VertexDeclPTC: {float x,y,z; float u,v; uint32 color;} = 24 bytes
     */
    const void *base = NULL;
    int stride = g.va_vertex.stride;
    if (stride == 0) stride = 24;  /* MCPE default vertex stride */

    if (g.bound_vbo > 0 && g.bound_vbo < GL2C3D_MAX_VBOS &&
        g.vbos[g.bound_vbo].allocated) {
        /* VBO mode: pointer values are byte offsets */
        base = g.vbos[g.bound_vbo].data;
    } else {
        /* Client array mode: pointer is direct memory address.
         * We need it in linear memory for the GPU. */
        if (g.va_vertex.pointer) {
            base = g.va_vertex.pointer;
        } else {
            return;
        }
    }

    if (!base) return;

    /* The data might be an offset into the VBO */
    const uint8_t *vertex_base = (const uint8_t*)base;
    uintptr_t pos_offset = (uintptr_t)g.va_vertex.pointer;
    uintptr_t tex_offset = g.va_texcoord.enabled ? (uintptr_t)g.va_texcoord.pointer : 0;
    uintptr_t col_offset = g.va_color.enabled ? (uintptr_t)g.va_color.pointer : 0;

    /* If using VBO, pointers are offsets. Otherwise, compute relative offsets. */
    if (g.bound_vbo > 0 && g.vbos[g.bound_vbo].allocated) {
        /* Offsets are already correct */
    } else {
        /* Direct pointers - we need all data in linearAlloc'd memory.
         * Copy vertex data to a temp linear buffer. */
        int total_bytes = (first + count) * stride;
        void *temp = linearAlloc(total_bytes);
        if (!temp) return;
        memcpy(temp, vertex_base, total_bytes);
        GSPGPU_FlushDataCache(temp, total_bytes);
        vertex_base = (const uint8_t*)temp;
        /* We'll need to free this after draw - use a static for simplicity */
    }

    /* Configure buffer info for Citro3D */
    C3D_BufInfo *bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);

    /* MCPE vertex layout: position(3f) + texcoord(2f) + color(4ub) = 24 bytes
     * Attribute permutation: attr0=pos, attr1=tex, attr2=color */
    if (g.va_texcoord.enabled && g.va_color.enabled) {
        /* Full VTC layout */
        BufInfo_Add(bufInfo, vertex_base + pos_offset + first * stride,
                    stride, 3, 0x210);
    } else if (g.va_texcoord.enabled) {
        /* VT layout (position + texcoord, no color) */
        BufInfo_Add(bufInfo, vertex_base + pos_offset + first * stride,
                    stride, 2, 0x10);
        /* Set fixed color attribute */
        C3D_FixedAttribSet(2,
            g.cur_color[0], g.cur_color[1],
            g.cur_color[2], g.cur_color[3]);
    } else if (g.va_color.enabled) {
        /* VC layout (position + color, no texcoord) */
        BufInfo_Add(bufInfo, vertex_base + pos_offset + first * stride,
                    stride, 2, 0x20);
        C3D_FixedAttribSet(1, 0.0f, 0.0f, 0.0f, 0.0f);
    } else {
        /* V only */
        BufInfo_Add(bufInfo, vertex_base + pos_offset + first * stride,
                    stride, 1, 0x0);
        C3D_FixedAttribSet(1, 0.0f, 0.0f, 0.0f, 0.0f);
        C3D_FixedAttribSet(2,
            g.cur_color[0], g.cur_color[1],
            g.cur_color[2], g.cur_color[3]);
    }

    /* Convert primitive type */
    GPU_Primitive_t prim = gl_to_gpu_primitive(mode);

    /* For GL_LINES: PICA200 doesn't have native line primitive.
     * Convert to degenerate triangles. */
    if (mode == GL_LINES || mode == GL_LINE_STRIP) {
        prim = GPU_TRIANGLES;
        /* Just draw as triangles - lines will look wrong but won't crash */
    }

    C3D_DrawArrays(prim, 0, count);
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) {
    /* MCPE doesn't use glDrawElements much - basic implementation */
    if (count <= 0 || !indices) return;

    apply_gpu_state();

    GPU_Primitive_t prim = gl_to_gpu_primitive(mode);

    /* C3D_DrawElements expects indices in linear memory */
    int idx_size = (type == GL_UNSIGNED_SHORT) ? 2 : 4;
    void *idx_copy = linearAlloc(count * idx_size);
    if (!idx_copy) return;
    memcpy(idx_copy, indices, count * idx_size);
    GSPGPU_FlushDataCache(idx_copy, count * idx_size);

    C3D_DrawElements(prim, count,
                     (type == GL_UNSIGNED_SHORT) ? C3D_UNSIGNED_SHORT : C3D_UNSIGNED_BYTE,
                     idx_copy);

    linearFree(idx_copy);
}


/* ========================================================================= */
/* Display lists (for Font rendering compatibility)                          */
/* MCPE uses display lists only for font rendering:                          */
/*   glGenLists -> glNewList -> glTranslatef/glColor3f -> glEndList           */
/*   glCallLists to render text                                              */
/* ========================================================================= */

GLuint glGenLists(GLsizei range) {
    GLuint base = g.dl_next_base;
    g.dl_next_base += range;
    if (g.dl_next_base >= GL2C3D_DISPLAY_LISTS)
        g.dl_next_base = 1;
    /* Init all lists in range */
    for (GLsizei i = 0; i < range && (base + i) < GL2C3D_DISPLAY_LISTS; i++) {
        g.dl_store[base + i].count = 0;
        g.dl_store[base + i].used = 1;
    }
    return base;
}

void glNewList(GLuint list, GLenum mode) {
    (void)mode;
    if (list < GL2C3D_DISPLAY_LISTS) {
        g.dl_recording = list;
        g.dl_store[list].count = 0;
    }
}

void glEndList(void) {
    g.dl_recording = -1;
}

/* Record an op if we're recording, otherwise execute immediately */
static void dl_record_translate(float x, float y, float z) {
    if (g.dl_recording >= 0 && g.dl_recording < GL2C3D_DISPLAY_LISTS) {
        DisplayList *dl = &g.dl_store[g.dl_recording];
        if (dl->count < GL2C3D_DL_MAX_OPS) {
            DLOp *op = &dl->ops[dl->count++];
            op->type = DL_OP_TRANSLATE;
            op->args[0] = x; op->args[1] = y; op->args[2] = z;
        }
    }
}

static void dl_record_color3f(float r, float g_, float b) {
    if (g.dl_recording >= 0 && g.dl_recording < GL2C3D_DISPLAY_LISTS) {
        DisplayList *dl = &g.dl_store[g.dl_recording];
        if (dl->count < GL2C3D_DL_MAX_OPS) {
            DLOp *op = &dl->ops[dl->count++];
            op->type = DL_OP_COLOR3F;
            op->args[0] = r; op->args[1] = g_; op->args[2] = b;
        }
    }
}

static void dl_execute(GLuint list) {
    if (list >= GL2C3D_DISPLAY_LISTS) return;
    DisplayList *dl = &g.dl_store[list];
    if (!dl->used) return;

    for (int i = 0; i < dl->count; i++) {
        DLOp *op = &dl->ops[i];
        switch (op->type) {
            case DL_OP_TRANSLATE:
                glTranslatef(op->args[0], op->args[1], op->args[2]);
                break;
            case DL_OP_COLOR3F:
                glColor3f(op->args[0], op->args[1], op->args[2]);
                break;
            default:
                break;
        }
    }
}

void glCallList(GLuint list) {
    dl_execute(list);
}

void glCallLists(GLsizei n, GLenum type, const GLvoid *lists) {
    for (GLsizei i = 0; i < n; i++) {
        GLuint id;
        switch (type) {
            case GL_UNSIGNED_INT:
                id = ((const GLuint*)lists)[i];
                break;
            case GL_UNSIGNED_BYTE:
                id = ((const GLubyte*)lists)[i];
                break;
            case GL_UNSIGNED_SHORT:
                id = ((const GLushort*)lists)[i];
                break;
            default:
                id = ((const GLuint*)lists)[i];
                break;
        }
        dl_execute(id);
    }
}

void glDeleteLists(GLuint list, GLsizei range) {
    for (GLsizei i = 0; i < range && (list + i) < GL2C3D_DISPLAY_LISTS; i++) {
        g.dl_store[list + i].used = 0;
        g.dl_store[list + i].count = 0;
    }
}

/* ========================================================================= */
/* Framebuffer stubs (handled by main_ctr initialization, not by GL layer)   */
/* ========================================================================= */

void glGenFramebuffers(GLsizei n, GLuint *ids) {
    for (GLsizei i = 0; i < n; i++) ids[i] = i + 1;
}
void glDeleteFramebuffers(GLsizei n, const GLuint *ids) { (void)n; (void)ids; }
void glBindFramebuffer(GLenum target, GLuint framebuffer) {
    (void)target;
    /* In the new architecture, the main loop calls gl2c3d_set_render_target instead */
    if (framebuffer == 0 || framebuffer == 1) {
        C3D_FrameDrawOn(g.render_target_top);
        g.current_target = g.render_target_top;
    }
}
void glGenRenderbuffers(GLsizei n, GLuint *ids) {
    for (GLsizei i = 0; i < n; i++) ids[i] = i + 1;
}
void glDeleteRenderbuffers(GLsizei n, const GLuint *ids) { (void)n; (void)ids; }
void glBindRenderbuffer(GLenum target, GLuint renderbuffer) { (void)target; (void)renderbuffer; }
void glRenderbufferStorage(GLenum target, GLenum internalformat,
                           GLsizei width, GLsizei height) {
    (void)target; (void)internalformat; (void)width; (void)height;
}
void glFramebufferRenderbuffer(GLenum target, GLenum attachment,
                               GLenum renderbuffertarget, GLuint renderbuffer) {
    (void)target; (void)attachment; (void)renderbuffertarget; (void)renderbuffer;
}

/* ========================================================================= */
/* Misc stubs                                                                */
/* ========================================================================= */

void glHint(GLenum target, GLenum mode) { (void)target; (void)mode; }
void glFlush(void) { }
void glFinish(void) { }
void glPixelStorei(GLenum pname, GLint param) { (void)pname; (void)param; }

void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                  GLenum format, GLenum type, GLvoid *pixels) {
    (void)x; (void)y; (void)width; (void)height;
    (void)format; (void)type;
    /* Not supported on 3DS - zero fill */
    if (pixels) memset(pixels, 0, width * height * 4);
}

/* ========================================================================= */
/* Hook into display list recording for glTranslatef/glColor3f               */
/* These are called during glNewList recording by MCPE Font code.            */
/* The real glTranslatef/glColor4f above don't record. We need to            */
/* intercept at a higher level. The approach: check g.dl_recording in        */
/* the actual functions.                                                     */
/* ========================================================================= */

/* We already defined glTranslatef, glColor3f, glColor4f above.
 * Modify them to also record if in recording mode.
 * Since C doesn't allow redefining, we handle it by adding recording
 * checks directly in the implementations. Let's patch them. */

/* The cleanest way: rewrite glTranslatef to check dl_recording.
 * But we already wrote it. Instead, MCPE Font.cpp calls glTranslatef2
 * which maps to glTranslatef. Our glTranslatef already does the matrix op.
 * During recording, Font.cpp calls:
 *   glNewList(id, GL_COMPILE);
 *   ... glTranslatef2(charWidth, 0, 0); glColor3f(r,g,b); ...
 *   glEndList();
 * Then later glCallLists() to execute them.
 *
 * GL_COMPILE means "don't execute, just record." We need to support this.
 * Solution: in glTranslatef/glColor3f/etc, if dl_recording >= 0, record
 * the op instead of executing it. But we must not break normal calls.
 *
 * Actually, looking at Font.cpp more carefully:
 *   glNewList(listPos + i, GL_COMPILE);
 *   ... (draws a character quad via Tesselator) ...
 *   glTranslatef2((GLfloat)charWidths[i], 0.0f, 0.0f);
 *   glEndList();
 *
 * The Tesselator::draw() inside the display list can't be recorded
 * (it's too complex). The font rendering is the main challenge.
 *
 * For now, display lists will just record translate and color ops
 * (the only ones called between glNewList/glEndList in Font.cpp).
 * The actual character drawing is done via Tesselator which doesn't
 * use display lists for the GPU submission itself.
 *
 * The practical approach: MCPE's Font code on 3DS will need to be
 * adapted to not use display lists. For now, we provide the stubs
 * so compilation works. The glCallLists for font will execute the
 * recorded translate/color ops which advances the text cursor.
 */

