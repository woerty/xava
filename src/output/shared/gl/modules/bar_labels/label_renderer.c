#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "label_renderer.h"
#include "shared/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ATLAS_W 512
#define ATLAS_H 512
#define FIRST_CHAR 32
#define NUM_CHARS  96

struct label_renderer {
    GLuint atlas_tex;
    GLuint program;
    GLuint vao, vbo;
    GLint  u_atlas, u_color;
    stbtt_bakedchar cdata[NUM_CHARS];
    float font_size_px;

    float *verts;
    int vert_count;
    int vert_capacity;
};

static const char *VS_SRC =
    "#version 420 core\n"
    "layout(location=0) in vec2 pos;\n"
    "layout(location=1) in vec2 uv;\n"
    "out vec2 v_uv;\n"
    "void main() {\n"
    "  v_uv = uv;\n"
    "  gl_Position = vec4(pos, 0.0, 1.0);\n"
    "}\n";

static const char *FS_SRC =
    "#version 420 core\n"
    "in vec2 v_uv;\n"
    "uniform sampler2D atlas;\n"
    "uniform vec4 color;\n"
    "layout(location=0) out vec4 frag;\n"
    "void main() {\n"
    "  float a = texture(atlas, v_uv).r;\n"
    "  frag = vec4(color.rgb, color.a * a);\n"
    "}\n";

static void push_vertex(label_renderer *lr, float x, float y, float u, float v) {
    if (lr->vert_count >= lr->vert_capacity) {
        lr->vert_capacity *= 2;
        lr->verts = realloc(lr->verts, sizeof(float) * 4 * lr->vert_capacity);
    }
    float *p = lr->verts + lr->vert_count * 4;
    p[0] = x; p[1] = y; p[2] = u; p[3] = v;
    lr->vert_count++;
}

static GLuint compile_one(GLenum type, const char *src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char info[1024]; glGetShaderInfoLog(s, sizeof(info), NULL, info);
        xavaWarn("label_renderer: shader compile error (type=%d): %s", type, info);
    }
    return s;
}

static GLuint compile_shaders(void) {
    GLuint vs = compile_one(GL_VERTEX_SHADER, VS_SRC);
    GLuint fs = compile_one(GL_FRAGMENT_SHADER, FS_SRC);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs); glAttachShader(p, fs);
    glLinkProgram(p);
    GLint linked; glGetProgramiv(p, GL_LINK_STATUS, &linked);
    if (!linked) {
        char info[1024]; glGetProgramInfoLog(p, sizeof(info), NULL, info);
        xavaWarn("label_renderer: program link error: %s", info);
    }
    glDeleteShader(vs); glDeleteShader(fs);
    return p;
}

label_renderer *label_renderer_create(const char *ttf_path, float font_size_px) {
    FILE *f = fopen(ttf_path, "rb");
    if (!f) { xavaLog("label_renderer: cannot open font %s", ttf_path); return NULL; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *ttf = malloc(sz);
    fread(ttf, 1, sz, f);
    fclose(f);

    label_renderer *lr = calloc(1, sizeof(*lr));
    lr->font_size_px = font_size_px;
    lr->vert_capacity = 4096;
    lr->verts = malloc(sizeof(float) * 4 * lr->vert_capacity);

    unsigned char *bitmap = calloc(1, ATLAS_W * ATLAS_H);
    stbtt_BakeFontBitmap(ttf, 0, font_size_px, bitmap,
        ATLAS_W, ATLAS_H, FIRST_CHAR, NUM_CHARS, lr->cdata);
    free(ttf);

    glGenTextures(1, &lr->atlas_tex);
    glBindTexture(GL_TEXTURE_2D, lr->atlas_tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, ATLAS_W, ATLAS_H, 0,
                 GL_RED, GL_UNSIGNED_BYTE, bitmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    free(bitmap);

    lr->program = compile_shaders();
    lr->u_atlas = glGetUniformLocation(lr->program, "atlas");
    lr->u_color = glGetUniformLocation(lr->program, "color");

    glGenVertexArrays(1, &lr->vao);
    glGenBuffers(1, &lr->vbo);
    glBindVertexArray(lr->vao);
    glBindBuffer(GL_ARRAY_BUFFER, lr->vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, (void*)(sizeof(float)*2));
    glBindVertexArray(0);
    /* Restore the global GL_ARRAY_BUFFER binding so we don't poison other
     * modules (especially the post pass) which use client-side vertex arrays
     * via glVertexAttribPointer(..., cpu_ptr). With our VBO still bound, that
     * CPU pointer would be interpreted as a VBO offset and read garbage. */
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return lr;
}

void label_renderer_destroy(label_renderer *lr) {
    if (!lr) return;
    glDeleteTextures(1, &lr->atlas_tex);
    glDeleteProgram(lr->program);
    glDeleteVertexArrays(1, &lr->vao);
    glDeleteBuffers(1, &lr->vbo);
    free(lr->verts);
    free(lr);
}

void label_renderer_begin(label_renderer *lr) {
    if (lr) lr->vert_count = 0;
}

void label_renderer_draw_label(label_renderer *lr, const char *text, float x, float y) {
    if (!lr || !text) return;
    for (const char *c = text; *c; c++) {
        if ((unsigned char)*c < FIRST_CHAR || (unsigned char)*c >= FIRST_CHAR + NUM_CHARS) continue;
        stbtt_aligned_quad q;
        stbtt_GetBakedQuad(lr->cdata, ATLAS_W, ATLAS_H,
                           (unsigned char)*c - FIRST_CHAR, &x, &y, &q, 1);
        /* Two triangles per glyph quad. Pixel coords converted to NDC in flush. */
        push_vertex(lr, q.x0, q.y0, q.s0, q.t0);
        push_vertex(lr, q.x1, q.y0, q.s1, q.t0);
        push_vertex(lr, q.x1, q.y1, q.s1, q.t1);
        push_vertex(lr, q.x0, q.y0, q.s0, q.t0);
        push_vertex(lr, q.x1, q.y1, q.s1, q.t1);
        push_vertex(lr, q.x0, q.y1, q.s0, q.t1);
    }
}

void label_renderer_flush(label_renderer *lr, int sw, int sh,
        float r, float g, float b, float a) {
    if (!lr || lr->vert_count == 0) return;

    /* Convert pixel coords to NDC in place. Y is flipped (pixel-y down, NDC up). */
    for (int i = 0; i < lr->vert_count; i++) {
        float *p = lr->verts + i * 4;
        p[0] = (p[0] / (float)sw) * 2.0f - 1.0f;
        p[1] = 1.0f - (p[1] / (float)sh) * 2.0f;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(lr->program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, lr->atlas_tex);
    glUniform1i(lr->u_atlas, 0);
    glUniform4f(lr->u_color, r, g, b, a);

    glBindVertexArray(lr->vao);
    glBindBuffer(GL_ARRAY_BUFFER, lr->vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(float) * 4 * lr->vert_count,
                 lr->verts, GL_DYNAMIC_DRAW);

    glDrawArrays(GL_TRIANGLES, 0, lr->vert_count);

    glBindVertexArray(0);
    /* Restore the global GL_ARRAY_BUFFER binding (see comment in create()). */
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
