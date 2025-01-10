
#include <stdio.h>
#include <stdlib.h>

#include <glad/glad.h>
#include <glfw3.h>

#define RADIANCE_CASCADES_RENDER_IMPLEMENTATION
#include "render.h"

#define RADIANCE_CASCADES_MATHY_IMPLEMENTATION
#include "mathy.h"

#define RADIANCE_CASCADES_SHAPES_IMPLEMENTATION
#include "shapes.h"

typedef struct map {
    vec4f *pixels;
    int32 w;
    int32 h;
} map;

vec4f
ray_intersect(map m, vec2f origin, vec2f direction, float t0, float t1);

texture
generate_map_texture(map m);

void
init_map_pixels(map m);

void
draw_circle_on_map_pixels(map m, circle c);

void
draw_rectangle_on_map_pixels(map m, rectangle r);

void
setup_map_renderer(GLuint *vao, GLuint *vbo, GLuint *ebo);

void
render_map(GLuint vao, texture map_texture, shader_program map_shader, map m);

#define WIDTH 800
#define HEIGHT 600

#define VOID (vec4f){ .r = 0.f, .g = 0, .b = 0, .a = 0.f }
#define OBSTACLE (vec4f){ .r = 0.f, .g = 0, .b = 0, .a = 1.f }
#define RED_LIGHT (vec4f){ .r = 1.f, .g = 0, .b = 0, .a = 1.f }
#define GREEN_LIGHT (vec4f){ .r = 0, .g = 1.f, .b = 0, .a = 1.f }
#define BLUE_LIGHT (vec4f){ .r = 0, .g = 0, .b = 1.f, .a = 1.f }
#define RAY_CASTED (vec4f){ .r = 1.f, .g = 1.f, .b = 1.f, .a = 1.f }

// # Cascades parameters
#define CASCADE0_PROBE_NUMBER 200
#define DIMENSION_SCALING 0.5 // for each dimension
#define ANGULAR_SCALING 2

int main(void) {
    // variables
    // TODO(gio): factor out initialization
    map m = {
        .pixels = NULL,
        .w = WIDTH,
        .h = HEIGHT
    };
    m.pixels = malloc(sizeof(vec4f) * m.w * m.h);

    GLFWwindow *glfw_win;
    float delta_time;
    uint64 fps_counter;
    GLuint vao, vbo, ebo;
    shader_program map_shader;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    glfw_win = glfwCreateWindow(WIDTH, HEIGHT, "Radiance Cascades", NULL, NULL);

    if (glfw_win == NULL) {
        fprintf(stderr, "[ERROR] Failed to create GLFW window\n");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(glfw_win);
    glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        fprintf(stderr, "[ERROR] Failed to initialize GLAD\n");
        return 1;
    }

    init_map_pixels(m);
    texture map_texture = generate_map_texture(m);
    setup_map_renderer(&vao, &vbo, &ebo);
    map_shader =
        shader_create_program("res/shaders/map.vs", "res/shaders/map.fs");
    glUseProgram(map_shader);
    shader_set_int32(map_shader, "map_texture", 0);

    float this_frame = 0.f;
    float last_frame = 0.f;
    float time_from_last_fps_update = 0.f;
    while (!glfwWindowShouldClose(glfw_win)) {
        // # fps stuff
        this_frame = (float)glfwGetTime();
        delta_time = this_frame - last_frame;
        last_frame = this_frame;
        fps_counter++;
        time_from_last_fps_update += delta_time;
        if (time_from_last_fps_update > 1.f) {
            printf("FPS: %lu\n", fps_counter);
            fps_counter = 0;
            time_from_last_fps_update -= 1.f;
        }

        // # keyboard input stuff
        if (glfwGetKey(glfw_win, GLFW_KEY_Q) == GLFW_PRESS) {
            glfwSetWindowShouldClose(glfw_win, GLFW_TRUE);
        }

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // # render map texture
        render_map(vao, map_texture, map_shader, m);

        // # finishing touch of the current frame
        glfwSwapBuffers(glfw_win);
        glfwPollEvents();
    }
}

// TODO(gio): uses DDA algorithm for now. Optimize it
vec4f ray_intersect(map m, vec2f origin, vec2f direction, float t0, float t1) {
    vec2f start = {
        .x = CLAMP(origin.x + direction.x * t0, 0.f, (float) m.w),
        .y = CLAMP(origin.y + direction.y * t0, 0.f, (float) m.h)
    };
    vec2f end = {
        .x = CLAMP(origin.x + direction.x * t1, 0.f, (float) m.w),
        .y = CLAMP(origin.y + direction.y * t1, 0.f, (float) m.h)
    };

    // TODO(gio): manage vertical lines separately (slope = inf)
    float slope = (end.y - start.y) / (end.x - start.x);

    for(int x = (int) start.x; x < end.x; ++x) {
        int y = start.y + slope * (x - start.x);

        int index = y * m.w + x;

        if (!vec4f_equals(m.pixels[index], VOID)) {
            return m.pixels[index];
        } else {
            m.pixels[index] = RAY_CASTED;
        }
    }

    return VOID;
}

texture generate_map_texture(map m) {
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
            GL_RGBA,
            m.w,
            m.h,
            0,
            GL_RGBA,
            GL_FLOAT, 
            m.pixels);
    return tex;
}

void init_map_pixels(map m) {
    // fill the void
    for(int32 index = 0; index < m.w * m.h; ++index) {
        m.pixels[index] = VOID;
    }

    // light source: middle left
    int32 light_w = 20;
    int32 light_h = m.h / 3;
    int32 light_x = (m.w / 4) - (light_w / 2);
    int32 light_y = (m.h / 2) - (light_h / 2);
    for(int32 y = light_y; y < light_y + light_h; ++y) {
        for(int32 x = light_x; x < light_x + light_w; ++x) {
            int32 index = y * m.w + x;
            m.pixels[index] = RED_LIGHT;
        }
    }

    // obstacle: center down
    int32 obstacle_w = 30;
    int32 obstacle_h = m.h / 3;
    int32 obstacle_x = (m.w / 2) - (obstacle_w / 2);
    int32 obstacle_y = (m.h / 2);
    for(int32 y = obstacle_y; y < obstacle_y + obstacle_h; ++y) {
        for(int32 x = obstacle_x; x < obstacle_x + obstacle_w; ++x) {
            int32 index = y * m.w + x;
            m.pixels[index] = OBSTACLE;
        }
    }

    circle c = {
        .center = (vec2f) { 400.f, 300.f },
        .radius = 100.f,
        .color = (vec4f) { 0.f, 0.f, 1.f, 1.f }
    };
    draw_circle_on_map_pixels(m, c);

    rectangle r = {
        .pos = (vec2f) { 440.f, 150.f },
        .dim = (vec2f) { 200.f, 150.f },
        .color = (vec4f) { 0.f, 1.f, 0.f, 1.f }
    };
    draw_rectangle_on_map_pixels(m, r);

    vec2f ray_origin = {
        .x = (0 / 4) + 15.f,
        .y = m.h / 2
    };
    vec2f ray_direction = {
        .x = 1.f,
        .y = -0.3f
    };
    vec4f intersection_result =
        ray_intersect(m, ray_origin, ray_direction, 2.f, m.w / 4);

    printf("Intersection result: %f, %f, %f, %f\n",
            intersection_result.x,
            intersection_result.y,
            intersection_result.z,
            intersection_result.w);
}

void draw_circle_on_map_pixels(map m, circle c) {
    vec2f draw_start = {
        .x = CLAMP(c.center.x - c.radius, 0.f, (float) m.w),
        .y = CLAMP(c.center.y - c.radius, 0.f, (float) m.h)
    };

    vec2f draw_end = {
        .x = CLAMP(c.center.x + c.radius, 0.f, (float) m.w),
        .y = CLAMP(c.center.y + c.radius, 0.f, (float) m.h)
    };

    int32 radius_squared = c.radius * c.radius;

    for(float x = draw_start.x; x < draw_end.x; ++x) {
        for(float y = draw_start.y; y < draw_end.y; ++y) {
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

void draw_rectangle_on_map_pixels(map m, rectangle r) {
    vec2f draw_start = {
        .x = CLAMP(r.pos.x, 0.f, (float) m.w),
        .y = CLAMP(r.pos.y, 0.f, (float) m.h)
    };

    vec2f draw_end = {
        .x = CLAMP(r.pos.x + r.dim.x, 0.f, (float) m.w),
        .y = CLAMP(r.pos.y + r.dim.y, 0.f, (float) m.h)
    };

    for(float x = draw_start.x; x < draw_end.x; ++x) {
        for(float y = draw_start.y; y < draw_end.y; ++y) {
            int32 index = (int32) (y * m.w + x);
            m.pixels[index].r = r.color.r;
            m.pixels[index].g = r.color.g;
            m.pixels[index].b = r.color.b;
            m.pixels[index].a = r.color.a;
        }
    }
}

void setup_map_renderer(GLuint *vao, GLuint *vbo, GLuint *ebo) {
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

void render_map(
        GLuint vao,
        texture map_texture,
        shader_program map_shader,
        map m) {
    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, map_texture);
    // render container
    glUseProgram(map_shader);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void generate_cascade(int32 cascade_index) {
}

