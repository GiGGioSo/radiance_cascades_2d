#ifndef _RC_TESTS_H_
#define _RC_TESTS_H_

#include "map.h"

#define INIT_MAP test_double_light

void
test_double_light(map m);

void
test_spheres(map m);

void
test_penumbra(map m);

#ifdef RADIANCE_CASCADES_TESTS_IMPLEMENTATION

void test_double_light(map m) {
    // fill the void
    for(int32 index = 0; index < m.w * m.h; ++index) {
        m.pixels[index] = VOID;
    }

    // lights
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

    // rectangle obstables
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

    // circle obstacles
    circle c1 = {
        .center = (vec2f) {
            .x = 200.f,
            .y = 400.f
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
            .y = 400.f
        },
        .radius = 50.f,
        .color = OBSTACLE
    };
    map_draw_circle(m, c3);
}

void test_spheres(map m) {
    int32 sphere_number = 4;
    float radius_min = 10.f;
    float radius_max = 50.f;

    for(int32 sphere_x = 0;
        sphere_x < sphere_number;
        ++sphere_x) {
        for(int32 sphere_y = 0;
            sphere_y < sphere_number;
            ++sphere_y) {

            vec4f sphere_color = OBSTACLE;
            if ((sphere_x + sphere_y + 3) % 6 == 0) {
                sphere_color = GREEN_LIGHT;
            }

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
                .color = sphere_color
            };
            map_draw_circle(m, c);
        }
    }
}

void test_penumbra(map m) {
    rectangle light = {
        .pos = (vec2f) { 100.f, 300.f },
        .dim = (vec2f) { 50.f, 200.f },
        .color = vec4f_divide(WHITE_LIGHT, 0.75f)
    };
    map_draw_rectangle(m, light);

    rectangle obstacle = {
        .pos = (vec2f) { 450.f, 400.f },
        .dim = (vec2f) { 10.f, 350.f },
        .color = OBSTACLE
    };
    map_draw_rectangle(m, obstacle);
}

void test_slit(map m) {
    rectangle light = {
        .pos = (vec2f) { 100.f, 250.f },
        .dim = (vec2f) { 50.f, 300.f },
        .color = vec4f_divide(GREEN_LIGHT, 1.f)
    };
    map_draw_rectangle(m, light);

    rectangle slit_up = {
        .pos = (vec2f) { 200.f, 0.f },
        .dim = (vec2f) { 30.f, 375.f },
        .color = OBSTACLE
    };
    map_draw_rectangle(m, slit_up);

    rectangle slit_down = {
        .pos = (vec2f) { 200.f, 425.f },
        .dim = (vec2f) { 30.f, 375.f },
        .color = OBSTACLE
    };
    map_draw_rectangle(m, slit_down);
}

#endif // RADIANCE_CASCADES_TESTS_IMPLEMENTATION

#endif // _RC_TESTS_H_
