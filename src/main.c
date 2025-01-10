
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

void
generate_cascade(map m, int32 cascade_index);

#define WIDTH 2000
#define HEIGHT 2000

#define VOID (vec4f){ .r = 0.f, .g = 0, .b = 0, .a = 0.f }
#define OBSTACLE (vec4f){ .r = 0.f, .g = 0, .b = 0, .a = 1.f }
#define RED_LIGHT (vec4f){ .r = 1.f, .g = 0, .b = 0, .a = 1.f }
#define GREEN_LIGHT (vec4f){ .r = 0, .g = 1.f, .b = 0, .a = 1.f }
#define BLUE_LIGHT (vec4f){ .r = 0, .g = 0, .b = 1.f, .a = 1.f }
#define RAY_CASTED (vec4f){ .r = 1.f, .g = 1.f, .b = 1.f, .a = 1.f }

// # Cascades parameters
#define CASCADE_NUMBER 7

#define CASCADE0_PROBE_NUMBER_X 96
#define CASCADE0_PROBE_NUMBER_Y 96
#define CASCADE0_ANGULAR_NUMBER 4
#define CASCADE0_INTERVAL_LENGTH 3 // in pixels
#define DIMENSION_SCALING 0.5 // for each dimension
#define ANGULAR_SCALING 2
#define INTERVAL_SCALING 2

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
    glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        fprintf(stderr, "[ERROR] Failed to initialize GLAD\n");
        return 1;
    }

    init_map_pixels(m);
    // ### test ###
    for(int32 cascade_index = 0;
        cascade_index < CASCADE_NUMBER;
        ++cascade_index) {

        generate_cascade(m, cascade_index);
    }
    // generate_cascade(m, 2);
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

vec4f ray_intersect(map m, vec2f origin, vec2f direction, float t0, float t1) {
    vec2i start = {
        .x = CLAMP((int32) ((origin.x + direction.x * t0) + 0.5f),
                0, m.w),
        .y = CLAMP((int32) ((origin.y + direction.y * t0) + 0.5f),
                0, m.h)
    };
    vec2i end = {
        .x = CLAMP((int32) ((origin.x + direction.x * t1) + 0.5f),
                0.f, (float) m.w),
        .y = CLAMP((int32) ((origin.y + direction.y * t1) + 0.5f),
                0, m.h)
    };

    if (start.x != end.x) {
        float slope = direction.y / direction.x;

        for(int32 x = MIN(start.x, end.x); x < MAX(start.x, end.x); ++x) {
            int32 y1 = (int32) (start.y + slope * ((float) x - start.x));
            int32 y2 = (int32) (start.y + slope * ((float) x - start.x + 1));

            // if (x == MAX(start.x, end.x)) {
            //     y2 = y1 + 1;
            // }
            for(int32 y = MIN(y1, y2); y <= MAX(y1, y2); ++y) {
                if (!(0 <= y && y < m.h)) continue; // out of map

                int32 index = (int32) (y * m.w + x);
                if (!vec4f_equals(m.pixels[index], VOID) &&
                    !vec4f_equals(m.pixels[index], RAY_CASTED)) {
                    // printf("ciao ");
                    return m.pixels[index];
                } else {
                    m.pixels[index] = RAY_CASTED;
                }
            }
        }
    } else {
        for(int32 y = MIN(start.y, end.y); y <= MAX(start.y, end.y); ++y) {
            int32 index = (int32) (y * m.w + start.x);

            if (!vec4f_equals(m.pixels[index], VOID) &&
                !vec4f_equals(m.pixels[index], RAY_CASTED)) {
                // printf("ciao ");
                return m.pixels[index];
            } else {
                m.pixels[index] = RAY_CASTED;
            }
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
    // // fill the void
    for(int32 index = 0; index < m.w * m.h; ++index) {
        m.pixels[index] = VOID;
    }

    // // light source: middle left
    // int32 light_w = 20;
    // int32 light_h = m.h / 3;
    // int32 light_x = (m.w / 4) - (light_w / 2);
    // int32 light_y = (m.h / 2) - (light_h / 2);
    // for(int32 y = light_y; y < light_y + light_h; ++y) {
    //     for(int32 x = light_x; x < light_x + light_w; ++x) {
    //         int32 index = y * m.w + x;
    //         m.pixels[index] = RED_LIGHT;
    //     }
    // }

    // // obstacle: center down
    // int32 obstacle_w = 30;
    // int32 obstacle_h = m.h / 3;
    // int32 obstacle_x = (m.w / 2) - (obstacle_w / 2);
    // int32 obstacle_y = (m.h / 2);
    // for(int32 y = obstacle_y; y < obstacle_y + obstacle_h; ++y) {
    //     for(int32 x = obstacle_x; x < obstacle_x + obstacle_w; ++x) {
    //         int32 index = y * m.w + x;
    //         m.pixels[index] = OBSTACLE;
    //     }
    // }

    int32 sphere_number = 10;
    float radius_min = 5.f;
    float radius_max = 30.f;

    for(int32 sphere_x = 0;
        sphere_x < sphere_number;
        ++sphere_x) {
        for(int32 sphere_y = 0;
            sphere_y < sphere_number;
            ++sphere_y) {

             circle c = {
                 .center = (vec2f) {
                     .x = ((float)m.w / (float)sphere_number) * (sphere_x + 0.5f),
                     .y = ((float)m.h / (float)sphere_number) * (sphere_y + 0.5f)
                 },
                 .radius =
                     (float) radius_min +
                     (float) (radius_max - radius_min) *
                     ((float) (sphere_x + sphere_y) /
                        (2.f * sphere_number - 2.f)),
                 .color = RED_LIGHT
             };
             // draw_circle_on_map_pixels(m, c);
        }
    }

    // rectangle r = {
    //     .pos = (vec2f) { 440.f, 150.f },
    //     .dim = (vec2f) { 200.f, 150.f },
    //     .color = (vec4f) { 0.f, 1.f, 0.f, 1.f }
    // };
    // draw_rectangle_on_map_pixels(m, r);

    // vec2f ray_origin = {
    //     .x = (0 / 2) + 15.f,
    //     .y = m.h / 2
    // };
    // vec2f ray_direction = {
    //     .x = 1.f,
    //     .y = -0.3f
    // };
 //    vec4f intersection_result =
 //        ray_intersect(m, ray_origin, ray_direction, 2.f, m.w / 4);

 //    printf("Intersection result: %f, %f, %f, %f\n",
 //            intersection_result.x,
 //            intersection_result.y,
 //            intersection_result.z,
 //            intersection_result.w);
}

void draw_circle_on_map_pixels(map m, circle c) {
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

void draw_rectangle_on_map_pixels(map m, rectangle r) {
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

void generate_cascade(map m, int32 cascade_index) {
    // ### Calculate parameters for this cascade index
    // probe count for each dimension
    float cascade_dimension_scaling =
        powf(DIMENSION_SCALING, cascade_index);
    vec2f probe_number = {
        .x = CASCADE0_PROBE_NUMBER_X * cascade_dimension_scaling,
        .y = CASCADE0_PROBE_NUMBER_Y * cascade_dimension_scaling
    };

    // angular frequency
    float cascade_angular_scaling =
        powf(ANGULAR_SCALING, cascade_index);
    float angular_number = CASCADE0_ANGULAR_NUMBER * cascade_angular_scaling;

    // ray cast interval dimension
    float base_interval = CASCADE0_INTERVAL_LENGTH;
    float cascade_interval_scaling =
        powf(INTERVAL_SCALING, cascade_index);
    float interval_length =
        base_interval * cascade_interval_scaling;
    float interval_start =
        (powf(base_interval, cascade_index + 1) - base_interval) /
        (base_interval - 1);
    float interval_end = interval_start + interval_length;

    // ### Do the rest
    vec2f probe_size = {
        .x = (float) m.w / probe_number.x,
        .y = (float) m.h / probe_number.y
    };

    for(int32 x = 0; x < probe_number.x; ++x) {
        for(int32 y = 0; y < probe_number.y; ++y) {
            // probe center position to raycast from
            vec2f probe_center = {
                .x = probe_size.x * (x + 0.5f),
                .y = probe_size.y * (y + 0.5f),
            };

            for(int32 direction_index = 0;
                direction_index < angular_number;
                ++direction_index) {
                float direction_angle =
                    2 * PI *
                    (((float) direction_index + 0.5f) /
                     (float) angular_number);

                vec2f ray_direction = vec2f_from_angle(direction_angle);

                vec4f result =
                    ray_intersect(
                            m,
                            probe_center,
                            ray_direction,
                            interval_start,
                            interval_end);

                // printf("Intersection result: %f, %f, %f, %f\n",
                //         result.x,
                //         result.y,
                //         result.z,
                //         result.w);
            }
        }
    }

    /*
     * per ogni probe:
     *  - casta in tutte le direzioni
     *  - popola la sezione di texture con il risultato
     *
     */
}

