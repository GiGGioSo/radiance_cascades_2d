#ifndef _RC_CASCADES_H_
#define _RC_CASCADES_H_

#include <stdlib.h>

// ### CASCADES PARAMETERS ###
#define CASCADE_NUMBER 8

#define MERGE_CASCADES 1

#define APPLY_SKYBOX 0

#define APPLY_CASCADE_TO_MAP 1
#define CASCADE_TO_APPLY_TO_MAP 0

#define DRAW_CASCADE_INSTEAD_OF_MAP 0
#define CASCADE_TO_DRAW 0

#define CASCADE0_PROBE_NUMBER_X 768
#define CASCADE0_PROBE_NUMBER_Y 768
#define CASCADE0_ANGULAR_NUMBER 32
#define CASCADE0_INTERVAL_LENGTH 18 // in pixels
#define DIMENSION_SCALING 0.5 // for each dimension
#define ANGULAR_SCALING 2
#define INTERVAL_SCALING 4
#define INTERVAL_OVERLAP 0.f // from 0 (no overlap) to 1 (full overlap)
// ###########################

typedef struct radiance_cascade {
    vec4f *data;
    int32 data_length;
    vec2i probe_number;
    int32 angular_number;
    vec2f interval;
    vec2f probe_size;
} radiance_cascade;

texture
cascade_generate_texture(radiance_cascade cascade);

void
cascade_generate(map m, radiance_cascade *cascade, int32 cascade_index);

void
cascade_free(radiance_cascade *cascade);

vec4f
cascade_merge_intervals(vec4f near, vec4f far);

vec4f
merge_intervals(vec4f near, vec4f far);

void
cascades_merge(radiance_cascade *cascades, int32 cascades_number);

vec4f
bilinear_weights(vec2f ratio);

vec2i
bilinear_offset(int32 index);

void
cascade_apply_skybox(radiance_cascade cascade, vec3f skybox);

void
cascade_to_map(map m, radiance_cascade cascade);

#ifdef RADIANCE_CASCADES_CASCADES_IMPLEMENTATION

texture cascade_generate_texture(radiance_cascade cascade) {
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

void cascade_generate(
        map m,
        radiance_cascade *cascade,
        int32 cascade_index) {
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

        // ray cast interval dimension - method 1
        //float base_interval = CASCADE0_INTERVAL_LENGTH;
        //float cascade_interval_scaling =
        //    powf((float) INTERVAL_SCALING, (float) cascade_index);
        //float interval_length =
        //    base_interval * cascade_interval_scaling;
        //float interval_start =
        //    ((powf((float) base_interval, (float) cascade_index + 1.f) -
        //      (float) base_interval) /
        //    (float) (base_interval - 1)) * (1.f - (float)INTERVAL_OVERLAP);
        //float interval_end = interval_start + interval_length;

        // method 2
        float base_interval = CASCADE0_INTERVAL_LENGTH;
        float interval_start_multiplier = 0;
        if (cascade_index > 0) {
            interval_start_multiplier =
                powf((float) 2, (float) cascade_index-1.f);
        }
        float interval_end_multiplier = powf((float) 2, (float) cascade_index);
        float interval_start = base_interval * interval_start_multiplier;
        float interval_end = base_interval * interval_end_multiplier;

        cascade->interval = (vec2f) {
            .x = interval_start,
            .y = interval_end
        };
        printf("cascade(%d) interval(%f, %f)\n",
                cascade_index,
                cascade->interval.x,
                cascade->interval.y);
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
        printf("cascade(%d) data_length(%d) probe_number(%d, %d) directions(%d)\n",
                cascade_index,
                cascade->data_length,
                cascade->probe_number.x,
                cascade->probe_number.y,
                cascade->angular_number);
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
                    map_ray_intersect(
                            m,
                            probe_center,
                            ray_direction,
                            cascade->interval.x,
                            cascade->interval.y);

                cascade->data[result_index] = result;
            }
        }
    }
}

void cascade_free(radiance_cascade *cascade) {
    if (cascade == NULL) return;
    if (cascade->data) {
        free(cascade->data);
        cascade->data = NULL;
    }
}

vec4f cascade_merge_intervals(vec4f near, vec4f far) {
    vec4f result = {};

    result.r = near.r + (far.r * near.a);
    result.g = near.g + (far.g * near.a);
    result.b = near.b + (far.b * near.a);
    result.a = near.a * far.a;

    return result;
}

void cascades_merge(
        radiance_cascade *cascades,
        int32 cascades_number) {
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

            // vec4f *probe_up =
            //     &cascade_up.data[probe_up_index * cascade_up.angular_number];

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

                int32 probe_x = probe_index % cascade.probe_number.x;
                int32 probe_y = probe_index / cascade.probe_number.x;

                // NOTE(bilinear): for finding top-left bilinear probe (of cascade_up obv)
                vec2f base_coord = vec2f_sum_vec2f(
                    (vec2f) {
                        .x = (float) ((probe_x + 0.5f) * cascade.probe_size.x) /
                                (float) cascade_up.probe_size.x,
                        .y = (float) ((probe_y + 0.5f) * cascade.probe_size.y) /
                                (float) cascade_up.probe_size.y
                    },
                    (vec2f) { .x = -0.5f, .y = -0.5f }
                );
                vec2i bilinear_base = (vec2i) {
                    .x = (int32) floorf(base_coord.x),
                    .y = (int32) floorf(base_coord.y)
                };
                vec2f ratio = (vec2f) {
                    .x = (base_coord.x - (int32) base_coord.x) *
                            SIGN(base_coord.x),
                    .y = (base_coord.y - (int32) base_coord.y) *
                            SIGN(base_coord.y)
                };
                vec4f weights = bilinear_weights(ratio);

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

                        // NOTE(bilinear): get the radiance from 4 probes around
                        //                  only valid probes are used
                        //                  (e.g. on the corners, some positions
                        //                          might be invalid, hence
                        //                          will not be used)
                        vec4f radiance_up = {};

                        // Count how many values have been used in the average
                        int32 usable_probe_up_count = 0;

                        for(int32 bilinear_index = 0;
                            bilinear_index < 4;
                            ++bilinear_index) {

                            vec2i offset = bilinear_offset(bilinear_index);
                            
                            vec4f bilinear_radiance_up = (vec4f) { 0, 0, 0, 0 };

                            if (0 <= bilinear_base.x + offset.x &&
                                bilinear_base.x + offset.x <
                                    cascade_up.probe_number.x &&
                                0 <= bilinear_base.y + offset.y &&
                                bilinear_base.y + offset.y <
                                    cascade_up.probe_number.y) {

                                // here the value is usable for the average
                                usable_probe_up_count++;

                                int32 bilinear_probe_up_index =
                                    (bilinear_base.y + offset.y) *
                                    cascade_up.probe_number.x +
                                    (bilinear_base.x + offset.x);

                                vec4f *bilinear_probe_up =
                                    &cascade_up.data[bilinear_probe_up_index *
                                    cascade_up.angular_number];
                                bilinear_radiance_up =
                                    bilinear_probe_up[direction_up_index];

                                radiance_up = vec4f_sum_vec4f(
                                        radiance_up,
                                        vec4f_divide(
                                            vec4f_diff_vec4f(
                                                bilinear_radiance_up,
                                                radiance_up),
                                            (float) usable_probe_up_count));
                            }

                            // radiance_up = vec4f_sum_vec4f(
                            //         radiance_up,
                            //         vec4f_mult(
                            //             bilinear_radiance_up,
                            //             weights.e[bilinear_index]));
                        }

                        average_radiance_up = vec4f_sum_vec4f(
                                average_radiance_up,
                                vec4f_divide(
                                    radiance_up,
                                    (float) ANGULAR_SCALING));
                    }

                    vec4f probe_direction_radiance = probe[direction_index];
                    probe[direction_index] = cascade_merge_intervals(
                            probe_direction_radiance,
                            average_radiance_up);
                }
            }
        }
    }
}

vec4f bilinear_weights(vec2f ratio) {
    return (vec4f){
        .x = (1.f - ratio.x) * (1.f - ratio.y),
        .y = ratio.x * (1.f - ratio.y),
        .z = (1.f - ratio.x) * ratio.y,
        .w = ratio.x * ratio.y
    };
}

vec2i bilinear_offset(int32 index) {
    vec2i offsets[] = {
        (vec2i) { .x = 0, .y = 0},
        (vec2i) { .x = 1, .y = 0},
        (vec2i) { .x = 0, .y = 1},
        (vec2i) { .x = 1, .y = 1},
    };
    if (0 > index || index >= ARR_LEN(offsets)) {
        return offsets[0];
    }
    return offsets[index];
}

// TODO(gio): do this better, result sucks rn
//              merge the skybox to the last cascade instead of this
void cascade_apply_skybox(
        radiance_cascade cascade,
        vec3f skybox) {

    vec4f skybox_hit = {
        .r = skybox.r,
        .g = skybox.g,
        .b = skybox.b,
        .a = 1.f // because it "hit" the skybox
    };

    // merge skybox into cascade
    for(int32 data_index = 0;
        data_index < cascade.data_length;
        ++data_index) {

        cascade.data[data_index] =
            // cascade_merge_intervals(cascade.data[data_index], skybox_hit);
            cascade_merge_intervals(skybox_hit, cascade.data[data_index]);
    }
}

void cascade_to_map(map m, radiance_cascade cascade) {
    // applying cascades into the pixels
    for (int32 pixel_index = 0; pixel_index < m.w * m.h; ++pixel_index) {
        int32 x = pixel_index % m.w;
        int32 y = pixel_index / m.w;

        // int32 probe_x = x / cascade.probe_size.x;
        // int32 probe_y = y / cascade.probe_size.y;
        // int32 probe_index =
        //     (probe_y * cascade.probe_number.x + probe_x) *
        //     cascade.angular_number;

        // printf("pixel(%d, %d) probe(%d, %d) probe_index(%d)\n",
        //         x, y, probe_x, probe_y, probe_index);

        // vec4f *probe = &cascade.data[probe_index];

        // NOTE(bilinear): for finding top-left bilinear probe (of cascade_up obv)
        vec2f base_coord = vec2f_sum_vec2f(
            (vec2f) {
                .x = (float) (x + 0.5f) / (float) cascade.probe_size.x,
                .y = (float) (y + 0.5f) / (float) cascade.probe_size.y
            },
            (vec2f) { .x = -0.5f, .y = -0.5f }
        );
        vec2i bilinear_base = (vec2i) {
            .x = (int32) floorf(base_coord.x),
            .y = (int32) floorf(base_coord.y)
        };
        vec2f ratio = (vec2f) {
            .x = (base_coord.x - (int32) base_coord.x) * SIGN(base_coord.x),
            .y = (base_coord.y - (int32) base_coord.y) * SIGN(base_coord.y),
        };
        vec4f weights = bilinear_weights(ratio);

        vec4f average = {};
        for(int32 direction_index = 0;
            direction_index < cascade.angular_number;
            ++direction_index) {

            // vec4f radiance = probe[direction_index];
            // NOTE(bilinear): get the radiance from 4 probes around
            vec4f radiance_up = {};

            // Count how many values have been used in the average
            int32 usable_probe_up_count = 0;

            for(int32 bilinear_index = 0;
                    bilinear_index < 4;
                    ++bilinear_index) {

                vec2i offset = bilinear_offset(bilinear_index);

                vec4f bilinear_radiance = (vec4f) { 0, 0, 0, 0 };

                if (0 <= bilinear_base.x + offset.x &&
                    bilinear_base.x + offset.x < cascade.probe_number.x &&
                    0 <= bilinear_base.y + offset.y &&
                    bilinear_base.y + offset.y < cascade.probe_number.y) {

                    // here the value is usable for the average
                    usable_probe_up_count++;

                    int32 bilinear_probe_index =
                        (bilinear_base.y + offset.y) * cascade.probe_number.x +
                        (bilinear_base.x + offset.x);

                    vec4f *bilinear_probe =
                        &cascade.data[bilinear_probe_index *
                        cascade.angular_number];
                    bilinear_radiance =
                        bilinear_probe[direction_index];

                    radiance_up = vec4f_sum_vec4f(
                            radiance_up,
                            vec4f_divide(
                                vec4f_diff_vec4f(
                                    bilinear_radiance,
                                    radiance_up),
                                (float) usable_probe_up_count));
                }


                // radiance_up = vec4f_sum_vec4f(
                //         radiance_up,
                //         vec4f_mult(
                //             bilinear_radiance,
                //             weights.e[bilinear_index]));
            }
            average = vec4f_sum_vec4f(
                    average,
                    vec4f_divide(
                        radiance_up,
                        cascade.angular_number));
        }
        average.a = 1.f;

        m.pixels[pixel_index] = average;
    }
}

#endif // RADIANCE_CASCADES_CASCADES_IMPLEMENTATION

#endif // _RC_CASCADES_H_
