#ifndef _RC_MAP_H_
#define _RC_MAP_H_

#include <string.h>

#include "render.h"
#include "mathy.h"
#include "shapes.h"

#define SHOW_RAYS_ON_MAP 0

#define VOID (vec4f){ .r = 0.f, .g = 0, .b = 0, .a = 0.f }
#define OBSTACLE (vec4f){ .r = 0.f, .g = 0, .b = 0, .a = 1.f }
#define RED_LIGHT (vec4f){ .r = 1.0f, .g = 0, .b = 0, .a = 1.f }
#define GREEN_LIGHT (vec4f){ .r = 0.f, .g = 0.5f, .b = 0.f, .a = 1.f }
#define BLUE_LIGHT (vec4f){ .r = 0, .g = 0, .b = 1.f, .a = 1.f }
#define LIGHT_BLUE_LIGHT (vec4f){ .r = 0.6f, .g = 0.6f, .b = 1.f, .a = 1.f }
#define WHITE_LIGHT (vec4f){ .r = 1.f, .g = 1.f, .b = 1.f, .a = 1.f }
#define RAY_CASTED (vec4f){ .r = 1.f, .g = 1.f, .b = 1.f, .a = 1.f }

#define SKYBOX (vec3f){ .r = 0.015f, .g = 0.02f, .b = 0.045f }

typedef struct map {
    vec4f *pixels;
    int32 w;
    int32 h;
} map;

map
map_create(int32 width, int32 height);

vec4f
map_ray_intersect(map m, vec2f origin, vec2f direction, float t0, float t1);

texture
map_generate_texture(map m);

void
map_draw_circle(map m, circle c);

void
map_draw_rectangle(map m, rectangle r);

void
map_setup_renderer(GLuint *vao, GLuint *vbo, GLuint *ebo);

void
map_render(GLuint vao, texture map_texture, shader_program map_shader, map m);

#ifdef RADIANCE_CASCADES_MAP_IMPLEMENTATION

map map_create(int32 width, int32 height) {
    map m = {
        .pixels = NULL,
        .w = width,
        .h = height
    };
    m.pixels = calloc(m.w * m.h, sizeof(vec4f));

    return m;
}

vec4f map_ray_intersect(map m, vec2f origin, vec2f direction, float t0, float t1) {
    vec4f result = {};
    vec2i start = {
        .x = (int32) ((origin.x + direction.x * t0) + 0.5f),
        .y = (int32) ((origin.y + direction.y * t0) + 0.5f)
    };
    vec2i end = {
        .x = (int32) ((origin.x + direction.x * t1) + 0.5f),
        .y = (int32) ((origin.y + direction.y * t1) + 0.5f)
    };
    // vec2i start = {
    //     .x = CLAMP((int32) ((origin.x + direction.x * t0) + 0.5f),
    //             0, m.w-1),
    //     .y = CLAMP((int32) ((origin.y + direction.y * t0) + 0.5f),
    //             0, m.h-1)
    // };
    // vec2i end = {
    //     .x = CLAMP((int32) ((origin.x + direction.x * t1) + 0.5f),
    //             0, m.w-1),
    //     .y = CLAMP((int32) ((origin.y + direction.y * t1) + 0.5f),
    //             0, m.h-1)
    // };

    if (start.x != end.x) {
        float slope = direction.y / direction.x;

        int32 direction_x = DIRECTION(end.x - start.x);

        for(int32 x = start.x;
            x * direction_x < end.x * direction_x;
            x += direction_x) {
            if (!(0 <= x && x < m.w)) continue;

            // printf("x(%d)\n", x);

            int32 y1 = (int32) ((float) start.y +
                    slope * ((float) x - (float) start.x));
            int32 y2 = (int32) ((float) start.y +
                    slope * ((float) x - (float) start.x + (float) direction_x));

            int32 direction_y = DIRECTION(y2 - y1);
            for(int32 y = y1;
                y * direction_y <= y2 * direction_y;
                y += direction_y) {
                // printf("y(%d)\n", y);

                if (!(0 <= y && y < m.h)) continue; // out of map

                int32 index = y * m.w + x;
#if SHOW_RAYS_ON_MAP == 0
                if (!vec4f_equals(m.pixels[index], VOID)) {
                    result = (vec4f) {
                        .r = m.pixels[index].r,
                        .g = m.pixels[index].g,
                        .b = m.pixels[index].b,
                        .a = 0.f // alpha 0 means it hit something
                    };
                    return result;
                }
#else
                if (!vec4f_equals(m.pixels[index], VOID) &&
                    !vec4f_equals(m.pixels[index], RAY_CASTED)) {
                    result = (vec4f) {
                        .r = m.pixels[index].r,
                        .g = m.pixels[index].g,
                        .b = m.pixels[index].b,
                        .a = 0.f // alpha 0 means it hit something
                    };
                    return result;
                } else {
                    m.pixels[index] = RAY_CASTED;
                }
#endif
            }
        }
    } else {
        int32 direction_y = DIRECTION(end.y - start.y);
        for(int32 y = start.y;
            y * direction_y <= end.y * direction_y;
            y += direction_y) {
            if (!(0 <= y && y < m.h)) continue; // out of map
            int32 index = (int32) (y * m.w + start.x);

#if SHOW_RAYS_ON_MAP != 1
            if (!vec4f_equals(m.pixels[index], VOID)) {
                result = (vec4f) {
                    .r = m.pixels[index].r,
                    .g = m.pixels[index].g,
                    .b = m.pixels[index].b,
                    .a = 0.f // alpha 0 means it hit something
                };
                return result;
            }
#else
            if (!vec4f_equals(m.pixels[index], VOID) &&
                !vec4f_equals(m.pixels[index], RAY_CASTED)) {
                result = (vec4f) {
                    .r = m.pixels[index].r,
                    .g = m.pixels[index].g,
                    .b = m.pixels[index].b,
                    .a = 0.f // alpha 0 means it hit something
                };
                return result;
            } else {
                m.pixels[index] = RAY_CASTED;
            }
#endif
        }
    }

    // alpha 1 means it hit nothing
    result = (vec4f) {
        .r = 0.f,
        .g = 0.f,
        .b = 0.f,
        .a = 1.f
    };
    return result;
}

void map_setup_renderer(GLuint *vao, GLuint *vbo, GLuint *ebo) {
    float vertices[] = {
        -1.f, -1.f, 0.f, 1.f, // up left
        -1.f,  1.f, 0.f, 0.f, // down left
         1.f,  1.f, 1.f, 0.f, // down right
         1.f, -1.f, 1.f, 1.f  // up right
    };
    int32 indices[] = {
        0, 1, 3,
        1, 2, 3
    };
    glGenVertexArrays(1, vao);
    glGenBuffers(1, vbo);
    glGenBuffers(1, ebo);

    glBindVertexArray(*vao);

    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(vertices),
            vertices,
            GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ebo);
    glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            sizeof(indices),
            indices,
            GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(
            0, 2, GL_FLOAT, GL_FALSE,
            4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord
    glVertexAttribPointer(
            1, 2, GL_FLOAT, GL_FALSE,
            4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

texture map_generate_texture(map m) {
    texture tex;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA32F,
            m.w,
            m.h,
            0,
            GL_RGBA,
            GL_FLOAT, 
            m.pixels);
    return tex;
}

void map_draw_circle(map m, circle c) {
    vec2i draw_start = {
        .x = CLAMP((int32) (c.center.x - c.radius), 0.f, m.w-1),
        .y = CLAMP((int32) (c.center.y - c.radius), 0.f, m.h-1)
    };

    vec2i draw_end = {
        .x = CLAMP((int32) (c.center.x + c.radius), 0.f, m.w-1),
        .y = CLAMP((int32) (c.center.y + c.radius), 0.f, m.h-1)
    };

    int32 radius_squared = c.radius * c.radius;

    for(int32 x = draw_start.x; x <= draw_end.x; ++x) {
        for(int32 y = draw_start.y; y <= draw_end.y; ++y) {
            vec2f relative_pos = {
                .x = x - c.center.x,
                .y = y - c.center.y,
            };
            if (vec2f_length_squared(relative_pos) <= radius_squared) {
                int32 index = (int32) (y * m.w + x);
                m.pixels[index].r = c.color.r;
                m.pixels[index].g = c.color.g;
                m.pixels[index].b = c.color.b;
                m.pixels[index].a = c.color.a;
            }
        }
    }
}

void map_draw_rectangle(map m, rectangle r) {
    vec2i draw_start = {
        .x = CLAMP((int32) r.pos.x, 0.f, m.w-1),
        .y = CLAMP((int32) r.pos.y, 0.f, m.h-1)
    };

    vec2i draw_end = {
        .x = CLAMP((int32) (r.pos.x + r.dim.x), 0.f, m.w-1),
        .y = CLAMP((int32) (r.pos.y + r.dim.y), 0.f, m.h-1)
    };

    for(float x = draw_start.x; x <= draw_end.x; ++x) {
        for(float y = draw_start.y; y <= draw_end.y; ++y) {
            int32 index = (int32) (y * m.w + x);
            m.pixels[index].r = r.color.r;
            m.pixels[index].g = r.color.g;
            m.pixels[index].b = r.color.b;
            m.pixels[index].a = r.color.a;
        }
    }
}

#endif // RADIANCE_CASCADES_MAP_IMPLEMENTATION

#endif // _RC_MAP_H_
