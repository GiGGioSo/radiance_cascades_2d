
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

#define RADIANCE_CASCADES_TESTS_IMPLEMENTATION
#include "tests.h"

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

    INIT_MAP(m);
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
