
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

#define RADIANCE_CASCADES_MAP_IMPLEMENTATION
#include "map.h"

#define RADIANCE_CASCADES_CASCADES_IMPLEMENTATION
#include "cascades.h"

void texture_render(GLuint vao, texture texture, shader_program shader);

void
init_map_pixels(map m);

#define WIDTH 800
#define HEIGHT 800

int main(void) {
    // variables
    map m = map_create(WIDTH, HEIGHT);

    radiance_cascade *cascades =
        calloc(CASCADE_NUMBER, sizeof(radiance_cascade));

    GLFWwindow *glfw_win;
    float delta_time = 0;
    uint64 fps_counter = 0;
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
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        fprintf(stderr, "[ERROR] Failed to initialize GLAD\n");
        return 1;
    }

    init_map_pixels(m);
    // ### test ###
    for(int32 cascade_index = 0;
        cascade_index < CASCADE_NUMBER;
        ++cascade_index) {

        printf("generating cascade %d\n", cascade_index);
        cascade_generate(m, &cascades[cascade_index], cascade_index);
    }
#if MERGE_CASCADES != 0
    cascades_merge(cascades, CASCADE_NUMBER);
#endif
#if APPLY_SKYBOX != 0
    cascade_apply_skybox(cascades[0], SKYBOX);
#endif
#if APPLY_CASCADE_TO_MAP != 0
    cascade_to_map(m, cascades[CASCADE_TO_APPLY_TO_MAP]);
#endif

    texture map_texture = map_generate_texture(m);
    texture cascade_texture = cascade_generate_texture(cascades[CASCADE_TO_DRAW]);
    map_setup_renderer(&vao, &vbo, &ebo);
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
#if DRAW_CASCADE_INSTEAD_OF_MAP == 0
        texture_render(vao, map_texture, map_shader);
#else
        texture_render(vao, cascade_texture, map_shader);
#endif

        // # finishing touch of the current frame
        glfwSwapBuffers(glfw_win);
        glfwPollEvents();
    }

    // free cascades
    for(int32 cascade_index = 0;
        cascade_index < CASCADE_NUMBER;
        ++cascade_index) {
        cascade_free(&cascades[cascade_index]);
    }
}

void texture_render(
        GLuint vao,
        texture texture,
        shader_program shader) {
    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    // render container
    glUseProgram(shader);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void init_map_pixels(map m) {
    // // fill the void
    for(int32 index = 0; index < m.w * m.h; ++index) {
        m.pixels[index] = VOID;
    }

    // int32 sphere_number = 4;
    // float radius_min = 10.f;
    // float radius_max = 50.f;

    // for(int32 sphere_x = 0;
    //     sphere_x < sphere_number;
    //     ++sphere_x) {
    //     for(int32 sphere_y = 0;
    //         sphere_y < sphere_number;
    //         ++sphere_y) {

    //         vec4f sphere_color = OBSTACLE;
    //         if ((sphere_x + sphere_y + 3) % 6 == 0) {
    //             sphere_color = GREEN_LIGHT;
    //         }

    //         circle c = {
    //             .center = (vec2f) {
    //                 .x = ((float)m.w / (float)sphere_number) * (sphere_x + 0.5f),
    //                 .y = ((float)m.h / (float)sphere_number) * (sphere_y + 0.5f)
    //             },
    //             .radius =
    //                 (float) radius_min +
    //                 (float) (radius_max - radius_min) *
    //                 ((float) (sphere_x + sphere_y) /
    //                  (2.f * sphere_number - 2.f)),
    //             .color = sphere_color
    //         };
    //         map_draw_circle(m, c);
    //     }
    // }

    rectangle red_light = {
        .pos = (vec2f) { 200.f, 200.f },
        .dim = (vec2f) { 400.f, 50.f },
        .color = RED_LIGHT
    };
    map_draw_rectangle(m, red_light);
    rectangle green_light = {
        .pos = (vec2f) { 200.f, 550.f },
        .dim = (vec2f) { 400.f, 50.f },
        .color = GREEN_LIGHT
    };
    map_draw_rectangle(m, green_light);

    rectangle obstacle_up = {
        .pos = (vec2f) { 400.f, 130.f },
        .dim = (vec2f) { 100.f, 20.f },
        .color = OBSTACLE
    };
    map_draw_rectangle(m, obstacle_up);
    rectangle obstacle_down = {
        .pos = (vec2f) { 300.f, 650.f },
        .dim = (vec2f) { 100.f, 20.f },
        .color = OBSTACLE
    };
    map_draw_rectangle(m, obstacle_down);

    circle c1 = {
        .center = (vec2f) {
            .x = 200.f,
            .y = 450.f
        },
        .radius = 50.f,
        .color = OBSTACLE
    };
    map_draw_circle(m, c1);
    circle c2 = {
        .center = (vec2f) {
            .x = 400.f,
            .y = 400.f
        },
        .radius = 50.f,
        .color = OBSTACLE
    };
    map_draw_circle(m, c2);
    circle c3 = {
        .center = (vec2f) {
            .x = 600.f,
            .y = 450.f
        },
        .radius = 50.f,
        .color = OBSTACLE
    };
    map_draw_circle(m, c3);
}
