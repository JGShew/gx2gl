/*******************************************************************************
 *
 * Copyright (c) 2019, snickerbockers <snickerbockers@washemu.org>
 * All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are
 *    met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <gx2/draw.h>
#include <gx2r/draw.h>
#include <gx2r/buffer.h>
#include <whb/gfx.h>
#include <coreinit/debug.h>
#include <gx2/mem.h>
#include <coreinit/memheap.h>
#include <coreinit/memunitheap.h>
#include <coreinit/memblockheap.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memexpheap.h>
#include <coreinit/memfrmheap.h>
#include <gx2/state.h>
#include <gx2r/mem.h>
#include <gx2/context.h>
#include <gx2/display.h>
#include <gx2/registers.h>
#include <gx2/swap.h>
#include <gx2/clear.h>
#include <gx2/event.h>

#include <GL/gl.h>
#include <GL/gx2gl.h>

#include "context.h"
#include "glff_shader.h"

#define MAX_CONTEXTS 4

#define VERT_LEN 4

#define CMDBUF_POOL_SIZE 4194304
#define CMDBUF_POOL_ALIGN 64

static void *cmdbuf_pool;

static struct gx2gl_context gx2gl_ctx_arr[MAX_CONTEXTS];
struct gx2gl_context *cur_ctx;
static MEMHeapHandle heap_mem1, heap_fg, heap_mem2;
static MEMBlockHeapTracking tracking;

static void *
my_alloc_fn(GX2RResourceFlags flags, uint32_t size, uint32_t align);
static void
my_free_fn(GX2RResourceFlags flags, void *block);

#define BLOCK_HEAP_SIZE 33554356

struct game_screen {
    void *buf;
    GX2ContextState *ctx_state;
    GX2ColorBuffer col_buf;
    GX2DepthBuffer depth_buf;

    int width, height;

    GX2ScanTarget const scan_tgt;

    uint32_t sz;
};

#define RENDER_MODE GX2_DRC_RENDER_MODE_SINGLE
#define SURF_FMT    GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8

struct game_screen drc;

static void init_gamepad_screen(struct game_screen *screen) {
    memset(screen, 0, sizeof(*screen));

    screen->width = 854;
    screen->height = 480;

    uint32_t wtf;
    GX2CalcDRCSize(RENDER_MODE, SURF_FMT, GX2_BUFFERING_MODE_DOUBLE,
                   &screen->sz, &wtf);
    OSReport("game_screen buffer size is 0x%08x\n", (unsigned)screen->sz);

    MEMHeapHandle heap_mem2 = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM2);
    screen->buf = MEMAllocFromExpHeapEx(heap_mem2, screen->sz, 4096);
    OSReport("game_screen buffer allocated 0x%p\n", screen->buf);
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU, screen->buf, screen->sz);
    GX2SetDRCBuffer(screen->buf, screen->sz, RENDER_MODE,
                    SURF_FMT, GX2_BUFFERING_MODE_DOUBLE);


    GX2ColorBuffer *col_buf = &screen->col_buf;
    col_buf->surface.dim = GX2_SURFACE_DIM_TEXTURE_2D;
    col_buf->surface.width = 854;
    col_buf->surface.height = 480;
    col_buf->surface.depth = 1;
    col_buf->surface.mipLevels = 1;
    col_buf->surface.format = SURF_FMT;
    col_buf->surface.aa = GX2_AA_MODE1X;
    col_buf->surface.use = GX2_SURFACE_USE_TEXTURE_COLOR_BUFFER_TV;
    col_buf->viewNumSlices = 1;

    GX2CalcSurfaceSizeAndAlignment(&col_buf->surface);
    GX2InitColorBufferRegs(col_buf);

    OSReport("game_screen color buffer imageSize is 0x%08x",
             col_buf->surface.imageSize);
    OSReport("game_screen color buffer alignment is 0x%08x",
             col_buf->surface.alignment);

    col_buf->surface.image =
        MEMAllocFromFrmHeapEx(MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1),
                                col_buf->surface.imageSize,
                                col_buf->surface.alignment);

    OSReport("game_screen color buffer has been initialized");

    GX2DepthBuffer *depth_buf = &screen->depth_buf;

    depth_buf->surface.dim = GX2_SURFACE_DIM_TEXTURE_2D;
    depth_buf->surface.width = 854;
    depth_buf->surface.height = 480;
    depth_buf->surface.depth = 1;
    depth_buf->surface.mipLevels = 1;
    depth_buf->surface.format = GX2_SURFACE_FORMAT_FLOAT_R32;
    depth_buf->surface.aa = GX2_AA_MODE1X;
    depth_buf->surface.use = GX2_SURFACE_USE_DEPTH_BUFFER |
        GX2_SURFACE_USE_TEXTURE;
    depth_buf->surface.tileMode = GX2_TILE_MODE_DEFAULT;
    depth_buf->depthClear = 1.0f;
    depth_buf->viewNumSlices = 1;

    GX2CalcSurfaceSizeAndAlignment(&depth_buf->surface);
    GX2InitDepthBufferRegs(depth_buf);

    OSReport("depth_buf.surface.imageSize is 0x%08x",
             depth_buf->surface.imageSize);
    OSReport("depth_buf.surface.alignment is 0x%08x",
             depth_buf->surface.alignment);
    depth_buf->surface.image =
        MEMAllocFromFrmHeapEx(MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1),
                                depth_buf->surface.imageSize,
                                depth_buf->surface.alignment);
    OSReport("depth_buf.surface.image is %p", depth_buf->surface.image);
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU,
                  depth_buf->surface.image,
                  depth_buf->surface.imageSize);



    GX2ContextState *ctx_state = (GX2ContextState *)MEMAllocFromExpHeapEx(heap_mem2, sizeof(GX2ContextState), GX2_CONTEXT_STATE_ALIGNMENT);
    screen->ctx_state = ctx_state;

    GX2SetupContextStateEx(ctx_state, TRUE);

    GX2SetColorControl(GX2_LOGIC_OP_COPY, 1, 0, 1);
    GX2SetBlendControl(GX2_RENDER_TARGET_0, GX2_BLEND_MODE_SRC_ALPHA,
                       GX2_BLEND_MODE_INV_SRC_ALPHA, GX2_BLEND_COMBINE_MODE_ADD,
                       TRUE, GX2_BLEND_MODE_SRC_ALPHA,
                       GX2_BLEND_MODE_INV_SRC_ALPHA, GX2_BLEND_COMBINE_MODE_ADD);
    GX2SetSwapInterval(1);
}

void gx2glInit(void) {
    heap_mem1 = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1);
    heap_mem2 = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM2);
    heap_fg = MEMGetBaseHeapHandle(MEM_BASE_HEAP_FG);

    OSReport("heaps initialized");

    cmdbuf_pool = MEMAllocFromExpHeapEx(heap_mem2, CMDBUF_POOL_SIZE, CMDBUF_POOL_ALIGN);

    OSReport("cmdbuf_pool has been allocated; pointer is %p", cmdbuf_pool);

    OSReport("About to call GX2Init");
    uint32_t init_attribs[] = {
        GX2_INIT_CMD_BUF_BASE, (uintptr_t)cmdbuf_pool,
        GX2_INIT_CMD_BUF_POOL_SIZE, CMDBUF_POOL_SIZE,
        GX2_INIT_END
    };
    GX2Init(init_attribs);

    OSReport("setting allocator functions");
    GX2RSetAllocator(my_alloc_fn, my_free_fn);

    init_gamepad_screen(&drc);
}

static void *
my_alloc_fn(GX2RResourceFlags flags, uint32_t size, uint32_t align) {
    OSReport("ERROR: %s called.", __func__);
    return MEMAllocFromExpHeapEx(heap_mem2, size, align);
}

static void
my_free_fn(GX2RResourceFlags flags, void *block) {
    // TODO: this
    OSReport("ERROR: %s called.", __func__);
}

static GX2PrimitiveMode gx2glGetGx2PrimitiveMode(GLenum mode) {
    switch (mode) {
    case GL_QUADS:
        return GX2_PRIMITIVE_MODE_QUADS;
    case GL_TRIANGLES:
    default:
        return GX2_PRIMITIVE_MODE_TRIANGLES;
    }
}

gx2glContext gx2glCreateContext(void) {
    gx2glContext handle;
    for (handle = 0; handle < MAX_CONTEXTS; handle++)
        if (gx2gl_ctx_arr[handle].valid == 0)
            break;
    if (handle == MAX_CONTEXTS)
        return -1;

    struct gx2gl_context *ctx = gx2gl_ctx_arr + handle;

    if (!WHBGfxLoadGFDShaderGroup(&ctx->shaderGroup, 0, shader_bytecode)) {
        OSReport("failure to load gfd shader group");
        exit(1);
    }
    WHBGfxInitShaderAttribute(&ctx->shaderGroup, "vert_pos", 0, 0,
                              GX2_ATTRIB_FORMAT_FLOAT_32_32_32_32);
    WHBGfxInitShaderAttribute(&ctx->shaderGroup, "mvp_row0", 1, 0,
                              GX2_ATTRIB_FORMAT_FLOAT_32_32_32_32);
    WHBGfxInitShaderAttribute(&ctx->shaderGroup, "mvp_row1", 2, 0,
                              GX2_ATTRIB_FORMAT_FLOAT_32_32_32_32);
    WHBGfxInitShaderAttribute(&ctx->shaderGroup, "mvp_row2", 3, 0,
                              GX2_ATTRIB_FORMAT_FLOAT_32_32_32_32);
    WHBGfxInitShaderAttribute(&ctx->shaderGroup, "mvp_row3", 4, 0,
                              GX2_ATTRIB_FORMAT_FLOAT_32_32_32_32);
    WHBGfxInitFetchShader(&ctx->shaderGroup);

    ctx->maxVerts = 1024;
    ctx->immedBuf = MEMAllocFromExpHeapEx(heap_mem2, ctx->maxVerts * sizeof(float) * VERT_LEN, GX2_VERTEX_BUFFER_ALIGNMENT);

    /* WHBGfxClearColor(1.0f, 1.0f, 1.0f, 1.0f); */

    ctx->valid = 1;

    return handle;
}

void gx2glDestroyContext(gx2glContext handle) {
    if (handle < 0)
        return;
    struct gx2gl_context *ctx = gx2gl_ctx_arr + handle;
    if (!ctx->valid)
        return;
    if (cur_ctx == ctx)
        cur_ctx = NULL;

    ctx->valid = 0;

    // TODO: free immedBuf
    /* free(ctx->immedBuf); */

    memset(ctx, 0, sizeof(*ctx));
}

void gx2glMakeCurrent(gx2glContext ctx) {
    if (ctx < MAX_CONTEXTS && gx2gl_ctx_arr[ctx].valid)
        cur_ctx = gx2gl_ctx_arr + ctx;
}

void gx2glBeginRender(void) {
    GX2SetContextState(drc.ctx_state);
    // TODO: GX2Invaldiate some stuff?
    GX2SetViewport(0, 0, drc.width, drc.height, 0, 1);
    GX2SetScissor(0, 0, 854, 480);
    GX2ClearColor(&drc.col_buf, 0.0f, 0.0f, 0.0f, 0.0f);
    GX2ClearDepthStencilEx(&drc.depth_buf, 1.0f, 0, GX2_CLEAR_FLAGS_BOTH);

    GX2SetColorBuffer(&drc.col_buf, GX2_RENDER_TARGET_0);
    GX2SetDepthBuffer(&drc.depth_buf);

    GX2SetFetchShader(&cur_ctx->shaderGroup.fetchShader);
    GX2SetVertexShader(cur_ctx->shaderGroup.vertexShader);
    GX2SetPixelShader(cur_ctx->shaderGroup.pixelShader);
}

void gx2glEndRender(void) {
    GX2SetContextState(drc.ctx_state);
    GX2CopyColorBufferToScanBuffer(&drc.col_buf, GX2_SCAN_TARGET_DRC);
    GX2SwapScanBuffers();

    GX2Flush();
    GX2SetDRCEnable(1);
    GX2SetTVEnable(0);
    GX2WaitForVsync();
    GX2DrawDone();
}

GLAPI void APIENTRY glShadeModel(GLenum mode) {
}

GLAPI void APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
}

GLAPI void APIENTRY glClear(GLbitfield mask) {
}

GLAPI void APIENTRY glBegin(GLenum mode) {
    cur_ctx->nVerts = 0;
    cur_ctx->polyMode = mode;
    cur_ctx->immedMode = GL_TRUE;
}

GLAPI void APIENTRY glEnd(void) {
    cur_ctx->immedMode = GL_FALSE;

    if (cur_ctx->nVerts) {
        float mvp[16];
        gx2gl_get_mvp(mvp);
        float row0[4] = { mvp[0], mvp[4], mvp[8], mvp[12] };
        float row1[4] = { mvp[1], mvp[5], mvp[9], mvp[13] };
        float row2[4] = { mvp[2], mvp[6], mvp[10], mvp[14] };
        float row3[4] = { mvp[4], mvp[7], mvp[11], mvp[15] };
        uint32_t row0i[4], row1i[4], row2i[4], row3i[4];
        memcpy(row0i, row0, sizeof(row0i));
        memcpy(row1i, row1, sizeof(row1i));
        memcpy(row2i, row2, sizeof(row2i));
        memcpy(row3i, row3, sizeof(row3i));

        GX2SetVertexUniformReg(0, 4, row0i);
        GX2SetVertexUniformReg(4, 4, row1i);
        GX2SetVertexUniformReg(8, 4, row2i);
        GX2SetVertexUniformReg(12, 4, row3i);

        GX2SetAttribBuffer(0, VERT_LEN * sizeof(float) * cur_ctx->nVerts, VERT_LEN * sizeof(float), cur_ctx->immedBuf);
        GX2DrawEx(gx2glGetGx2PrimitiveMode(cur_ctx->polyMode), cur_ctx->nVerts, 0, 1);
        GX2DrawDone();
    }
}

GLAPI void APIENTRY glVertex3f(GLfloat x, GLfloat y, GLfloat z) {
    if (cur_ctx->immedMode == GL_TRUE &&
        (cur_ctx->polyMode == GL_TRIANGLES || cur_ctx->polyMode == GL_QUADS)) {
        float *vout = cur_ctx->immedBuf + VERT_LEN * cur_ctx->nVerts++;
        vout[0] = x;
        vout[1] = y;
        vout[2] = z;
        vout[3] = 1.0f;
    }
}

GLAPI void APIENTRY glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
    GLfloat mat[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        x, y, z, 1.0f
    };
    glMultMatrixf(mat);
}

GLAPI void APIENTRY glClearDepth(double depth) {
}

GLAPI void APIENTRY glClearColor(GLclampf red, GLclampf green,
                                 GLclampf blue, GLclampf alpha) {
    /* WHBGfxClearColor(red, green, blue, alpha); */
}

GLAPI void APIENTRY glHint(GLenum target, GLenum mode) {
}

GLAPI void APIENTRY glDepthFunc(GLenum func) {
}

GLAPI void APIENTRY glEnable(GLenum cap) {
}
