
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

typedef struct radiance_cascade {
    vec4f *data;
    int32 data_length;
    vec2i probe_number;
    int32 angular_number;
    vec2f interval;
    vec2f probe_size;
} radiance_cascade;

vec4f
ray_intersect(map m, vec2f origin, vec2f direction, float t0, float t1);

texture
generate_map_texture(map m);

texture
generate_cascade_texture(radiance_cascade cascade);

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
generate_cascade(map m, radiance_cascade *cascade, int32 cascade_index);

vec4f
merge_intervals(vec4f near, vec4f far);

void apply_cascades(map m, radiance_cascade *cascades, int32 cascades_number);


#define WIDTH 800
#define HEIGHT 800

#define VOID (vec4f){ .r = 0.f, .g = 0, .b = 0, .a = 0.f }
#define OBSTACLE (vec4f){ .r = 0.f, .g = 0, .b = 0, .a = 1.f }
#define RED_LIGHT (vec4f){ .r = 1.3f, .g = 0, .b = 0, .a = 1.f }
#define GREEN_LIGHT (vec4f){ .r = 0.f, .g = 0.6f, .b = 0.f, .a = 1.f }
#define BLUE_LIGHT (vec4f){ .r = 0, .g = 0, .b = 1.f, .a = 1.f }
#define RAY_CASTED (vec4f){ .r = 1.f, .g = 1.f, .b = 1.f, .a = 1.f }
#define SKYBOX (vec4f){ .r = 0.1f, .g = 0.1f, .b = 0.1f, .a = 1.f }

#define SHOW_RAYS_ON_MAP 0

// # Cascades parameters
#define CASCADE_NUMBER 5
#define CASCADE_DRAW 0

#define CASCADE0_PROBE_NUMBER_X 400
#define CASCADE0_PROBE_NUMBER_Y 400
#define CASCADE0_ANGULAR_NUMBER 8
#define CASCADE0_INTERVAL_LENGTH 8 // in pixels
#define DIMENSION_SCALING 0.5 // for each dimension
#define ANGULAR_SCALING 3
#define INTERVAL_SCALING 4

int main(void) {
    // variables
    // TODO(gio): factor out initialization
    map m = {
        .pixels = NULL,
        .w = WIDTH,
        .h = HEIGHT
    };
    m.pixels = calloc(m.w * m.h, sizeof(vec4f));

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
        generate_cascade(m, &cascades[cascade_index], cascade_index);
    }
    apply_cascades(m, cascades, CASCADE_NUMBER);

    texture map_texture = generate_map_texture(m);
    texture cascade_texture = generate_cascade_texture(cascades[CASCADE_DRAW]);
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
        // render_map(vao, cascade_texture, map_shader, m);

        // # finishing touch of the current frame
        glfwSwapBuffers(glfw_win);
        glfwPollEvents();
    }
}

vec4f ray_intersect(map m, vec2f origin, vec2f direction, float t0, float t1) {
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

texture generate_cascade_texture(radiance_cascade cascade) {
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
            cascade.probe_number.x * cascade.angular_number,
            cascade.probe_number.y,
            0,
            GL_RGBA,
            GL_FLOAT, 
            cascade.data);
    return tex;
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
    //         draw_circle_on_map_pixels(m, c);
    //     }
    // }

    rectangle red_light = {
        .pos = (vec2f) { 200.f, 200.f },
        .dim = (vec2f) { 400.f, 50.f },
        .color = RED_LIGHT
    };
    draw_rectangle_on_map_pixels(m, red_light);
    rectangle green_light = {
        .pos = (vec2f) { 200.f, 550.f },
        .dim = (vec2f) { 400.f, 50.f },
        .color = GREEN_LIGHT
    };
    draw_rectangle_on_map_pixels(m, green_light);

    rectangle obstacle_up = {
        .pos = (vec2f) { 400.f, 130.f },
        .dim = (vec2f) { 100.f, 20.f },
        .color = OBSTACLE
    };
    draw_rectangle_on_map_pixels(m, obstacle_up);
    rectangle obstacle_down = {
        .pos = (vec2f) { 300.f, 650.f },
        .dim = (vec2f) { 100.f, 20.f },
        .color = OBSTACLE
    };
    draw_rectangle_on_map_pixels(m, obstacle_down);

    circle c1 = {
        .center = (vec2f) {
            .x = 200.f,
            .y = 450.f
        },
        .radius = 50.f,
        .color = OBSTACLE
    };
    draw_circle_on_map_pixels(m, c1);
    circle c2 = {
        .center = (vec2f) {
            .x = 400.f,
            .y = 400.f
        },
        .radius = 50.f,
        .color = OBSTACLE
    };
    draw_circle_on_map_pixels(m, c2);
    circle c3 = {
        .center = (vec2f) {
            .x = 600.f,
            .y = 450.f
        },
        .radius = 50.f,
        .color = OBSTACLE
    };
    draw_circle_on_map_pixels(m, c3);

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

void generate_cascade(map m, radiance_cascade *cascade, int32 cascade_index) {
    // ### Calculate parameters for this cascade index only if necessary
    if (cascade == NULL) return;
    if (cascade->data == NULL) {
        // probe count for each dimension
        float cascade_dimension_scaling =
            powf((float) DIMENSION_SCALING, (float) cascade_index);
        cascade->probe_number = (vec2i) {
            .x = CASCADE0_PROBE_NUMBER_X * cascade_dimension_scaling,
                .y = CASCADE0_PROBE_NUMBER_Y * cascade_dimension_scaling
        };

        // angular frequency
        float cascade_angular_scaling =
            powf((float) ANGULAR_SCALING, (float) cascade_index);
        cascade->angular_number = CASCADE0_ANGULAR_NUMBER * cascade_angular_scaling;

        // ray cast interval dimension
        float base_interval = CASCADE0_INTERVAL_LENGTH;
        float cascade_interval_scaling =
            powf((float) INTERVAL_SCALING, (float) cascade_index);
        float interval_length =
            base_interval * cascade_interval_scaling;
        float interval_start =
            (powf((float) base_interval, (float) cascade_index) -
              (float) base_interval) /
            (float) (base_interval - 1);
        float interval_end = interval_start + interval_length;
        cascade->interval = (vec2f) {
            .x = interval_start,
                .y = interval_end
        };
        cascade->probe_size = (vec2f) {
            .x = (float) m.w / (float) cascade->probe_number.x,
            .y = (float) m.h / (float) cascade->probe_number.y
        };

        // allocate cascade memory
        cascade->data_length =
            cascade->probe_number.x *
            cascade->probe_number.y *
            cascade->angular_number;
        cascade->data = calloc(
                cascade->data_length,
                sizeof(vec4f));
        printf("cascade(%d) data_length(%d)\n",
                cascade_index,
                cascade->data_length);
    }

    // ### Do the rest
    for(int32 x = 0; x < cascade->probe_number.x; ++x) {
        for(int32 y = 0; y < cascade->probe_number.y; ++y) {
            // probe center position to raycast from
            vec2f probe_center = {
                .x = (float) cascade->probe_size.x * (x + 0.5f),
                .y = (float) cascade->probe_size.y * (y + 0.5f),
            };

            for(int32 direction_index = 0;
                direction_index < cascade->angular_number;
                ++direction_index) {
                float direction_angle =
                    2.f * PI *
                    (((float) direction_index + 0.5f) /
                     (float) cascade->angular_number);

                vec2f ray_direction = vec2f_from_angle(direction_angle);

                int32 result_index =
                    (y * cascade->probe_number.x + x) *
                    cascade->angular_number + direction_index;

                vec4f result =
                    ray_intersect(
                            m,
                            probe_center,
                            ray_direction,
                            cascade->interval.x,
                            cascade->interval.y);

                cascade->data[result_index] = result;

                // printf("Intersection result: %f, %f, %f, %f\n",
                //         result.x,
                //         result.y,
                //         result.z,
                //         result.w);
            }
        }
    }
}

vec4f merge_intervals(vec4f near, vec4f far) {
    vec4f result = {};

    result.r = near.r + (far.r * near.a);
    result.g = near.g + (far.g * near.a);
    result.b = near.b + (far.b * near.a);
    result.a = near.a * far.a;

    return result;
}

void apply_cascades(map m, radiance_cascade *cascades, int32 cascades_number) {
    if (cascades_number <= 0) return;

    // merging cascades into cascade0
    for(int32 cascade_index = cascades_number - 2;
            cascade_index >= 0;
            --cascade_index) {
        radiance_cascade cascade_up = cascades[cascade_index + 1];
        radiance_cascade cascade = cascades[cascade_index];

        for(int32 probe_up_index = 0;
                probe_up_index < cascade_up.probe_number.x *
                cascade_up.probe_number.y;
                ++probe_up_index) {

            vec4f *probe_up =
                &cascade_up.data[probe_up_index * cascade_up.angular_number];

            int32 probes_per_up_probe_row =
                (int32) (1.f / (float) DIMENSION_SCALING);

            int32 probe_up_x = probe_up_index % cascade_up.probe_number.x;
            int32 probe_up_y = probe_up_index / cascade_up.probe_number.x;

            int32 probe_index_base =
                probe_up_y *
                    cascade_up.probe_number.x *
                    POW2(probes_per_up_probe_row) +
                probe_up_x * probes_per_up_probe_row;

            for(int32 probe_index_offset = 0;
                    probe_index_offset < POW2(probes_per_up_probe_row);
                    ++probe_index_offset) {

                // which row of the probe_up we are considering
                int32 probe_index_base_row =
                    probe_index_offset / probes_per_up_probe_row;

                // which probe of the row of the probe_up we are considering
                int32 probe_index_base_offset =
                    probe_index_offset % probes_per_up_probe_row;

                int32 probe_index =
                    // number of probes in the previous probe_ups
                    probe_index_base +
                    // number of rows I'm skipping
                    probe_index_base_row *
                        // number of probes in a row
                        cascade_up.probe_number.x * probes_per_up_probe_row +
                    probe_index_base_offset;

                // int32 probe_index = probe_index_base + probe_index_offset;
                vec4f *probe =
                    &cascade.data[probe_index * cascade.angular_number];

                for(int32 direction_index = 0;
                        direction_index < cascade.angular_number;
                        ++direction_index) {

                    vec4f average_radiance_up = {};
                    int32 direction_up_index_base =
                        direction_index * ANGULAR_SCALING;
                    // int32 average_alpha = 1.0f;
                    for(int32 direction_up_index_offset = 0;
                            direction_up_index_offset < ANGULAR_SCALING;
                            ++direction_up_index_offset) {

                        int32 direction_up_index =
                            direction_up_index_base + direction_up_index_offset;
                        // printf("direction_up_index(%d)\n", direction_up_index);
                        vec4f radiance_up = probe_up[direction_up_index];
                        // if (radiance_up.a == 0.f) {
                        //     average_alpha = 0.f;
                        // }
                        average_radiance_up = vec4f_sum_vec4f(
                                average_radiance_up,
                                vec4f_divide(
                                    radiance_up,
                                    (float) ANGULAR_SCALING));
                    }
                    // average_up.a = average_alpha;

                    // TODO(gio) continua da qua
                    // printf("average_radiance_up(%f, %f, %f, %f)\n");
                    // printf("average_radiance_up(%f, %f, %f, %f)\n");

                    vec4f probe_direction_radiance = probe[direction_index];
                    probe[direction_index] = merge_intervals(
                            probe_direction_radiance,
                            average_radiance_up);
                }
            }
        }
    }

    radiance_cascade cascade0 = cascades[0];
    // merge skybox into first cascade
    vec4f skybox_color = SKYBOX;
    for(int32 data_index = 0;
        data_index < cascade0.data_length;
        ++data_index) {
        cascade0.data[data_index] =
            vec4f_sum_vec4f(cascade0.data[data_index], skybox_color);
    }

    // applying cascades into the pixels
    for (int32 pixel_index = 0; pixel_index < m.w * m.h; ++pixel_index) {
        int32 x = pixel_index % m.w;
        int32 y = pixel_index / m.w;

        int32 probe_x = x / cascade0.probe_size.x;
        int32 probe_y = y / cascade0.probe_size.y;
        int32 probe_index =
            (probe_y * cascade0.probe_number.x + probe_x) *
            cascade0.angular_number;

        // printf("pixel(%d, %d) probe(%d, %d) probe_index(%d)\n",
        //         x, y, probe_x, probe_y, probe_index);

        vec4f *probe = &cascade0.data[probe_index];

        vec4f average = {};
        for(int32 direction_index = 0;
            direction_index < cascade0.angular_number;
            ++direction_index) {

            vec4f radiance = probe[direction_index];
            average = vec4f_sum_vec4f(
                    average,
                    vec4f_divide(
                        radiance,
                        cascade0.angular_number));
        }
        average.a = 1.f;

        m.pixels[pixel_index] = average;
    }
}
