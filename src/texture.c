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

#include <string.h>

#include <GL/gl.h>
#include <GL/gx2gl_ext.h>

#include "texture.h"
#include "context.h"

GLAPI void APIENTRY glBindTexture (GLenum target, GLuint texture) {
    // TODO: this
}

GLAPI void APIENTRY glTexParameteri( GLenum target, GLenum pname, GLint param ) {
    // TODO: this
}

GLAPI void APIENTRY glTexImage2D (GLenum target, GLint level, GLint internalformat,
                                  GLsizei width, GLsizei height, GLint border,
                                  GLenum format, GLenum type, const GLvoid *pixels) {
    // TODO: this
}

static unsigned gx2glAllocTexture(void) {
    unsigned idx;
    for (idx = 1; idx < GX2GL_MAX_TEXTURES; idx++)
        if (!cur_ctx->textures[idx].in_use) {
            cur_ctx->textures[idx].in_use = true;
            return idx;
        }

    return 0; // TODO: signal error
}

__attribute__((unused))
static void gx2glFreeTexture(unsigned idx) {
    cur_ctx->textures[idx].in_use = false;
}

GLAPI void APIENTRY glGenTextures (GLsizei n, GLuint *textures) {
    GLsizei n_processed;
    for (n_processed = 0; n_processed < n; n_processed++)
        textures[n_processed] = gx2glAllocTexture();
}
