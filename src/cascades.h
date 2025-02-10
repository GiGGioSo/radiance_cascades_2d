#ifndef _RC_CASCADES_H_
#define _RC_CASCADES_H_

#include <stdlib.h>

// ### CASCADES PARAMETERS ###
#define CASCADE_NUMBER 5

#define MERGE_CASCADES 1

#define APPLY_SKYBOX 0

#define APPLY_CASCADE_TO_MAP 1
#define CASCADE_TO_APPLY_TO_MAP 0

#define DRAW_CASCADE_INSTEAD_OF_MAP 0
#define CASCADE_TO_DRAW 0

#define CASCADE0_PROBE_NUMBER_X 400
#define CASCADE0_PROBE_NUMBER_Y 400
#define CASCADE0_ANGULAR_NUMBER 8
#define CASCADE0_INTERVAL_LENGTH 4 // in pixels
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

radiance_cascade
cascade_instant_init(map m);

void
cascade_instant_generate(map m, radiance_cascade cascade, int32 cascade_index, int32 merge_with_existing);

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

void
cascade_apply_skybox(radiance_cascade cascade, vec4f skybox_color);

void
cascade_to_map(map m, radiance_cascade cascade);

#ifdef RADIANCE_CASCADES_CASCADES_IMPLEMENTATION

radiance_cascade cascade_instant_init(map m) {
    radiance_cascade cascade = {};
    // probe count for each dimension
    cascade.probe_number = (vec2i) {
        .x = CASCADE0_PROBE_NUMBER_X,
        .y = CASCADE0_PROBE_NUMBER_Y
    };

    // angular frequency
    cascade.angular_number = CASCADE0_ANGULAR_NUMBER;

    // ray cast interval dimension
    cascade.interval = (vec2f) {
        .x = 0.f,
        .y = CASCADE0_INTERVAL_LENGTH
    };
    cascade.probe_size = (vec2f) {
        .x = (float) m.w / (float) cascade.probe_number.x,
        .y = (float) m.h / (float) cascade.probe_number.y
    };

    // allocate cascade memory
    cascade.data_length =
        cascade.probe_number.x *
        cascade.probe_number.y *
        cascade.angular_number;
    cascade.data = calloc(
            cascade.data_length,
            sizeof(vec4f));

    return cascade;
}

void cascade_instant_generate(
        map m,
        radiance_cascade cascade,
        int32 cascade_index,
        int32 merge_with_existing) {
    // ### If the cascade is not initialized, return
    if (cascade.data == NULL) return;

    radiance_cascade current_cascade = {};
    // probe count for each dimension
    float current_cascade_dimension_scaling =
        powf((float) DIMENSION_SCALING, (float) cascade_index);
    current_cascade.probe_number = (vec2i) {
        .x = cascade.probe_number.x * current_cascade_dimension_scaling,
        .y = cascade.probe_number.y * current_cascade_dimension_scaling
    };

    // angular frequency
    float current_cascade_angular_scaling =
        powf((float) ANGULAR_SCALING, (float) cascade_index);
    current_cascade.angular_number =
        cascade.angular_number * current_cascade_angular_scaling;

    // ray cast interval dimension
    float base_interval = cascade.interval.y;
    float cascade_interval_scaling =
        powf((float) INTERVAL_SCALING, (float) cascade_index);
    float interval_length =
        base_interval * cascade_interval_scaling;
    float interval_start =
        ((powf((float) base_interval, (float) cascade_index + 1.f) -
          (float) base_interval) /
         (float) (base_interval - 1)) * (1.f - (float)INTERVAL_OVERLAP);
    float interval_end = interval_start + interval_length;
    current_cascade.interval = (vec2f) {
        .x = interval_start,
        .y = interval_end
    };
    current_cascade.probe_size = (vec2f) {
        .x = (float) m.w / (float) current_cascade.probe_number.x,
        .y = (float) m.h / (float) current_cascade.probe_number.y
    };

    current_cascade.data_length =
        current_cascade.probe_number.x *
        current_cascade.probe_number.y *
        current_cascade.angular_number;
    // We allocate momentarily, then we merge, then we deallocate
    current_cascade.data = calloc(
            current_cascade.data_length,
            sizeof(vec4f));

    // Calculating current cascade
    for(int32 x = 0; x < current_cascade.probe_number.x; ++x) {
        for(int32 y = 0; y < current_cascade.probe_number.y; ++y) {
            // probe center position to raycast from
            vec2f probe_center = {
                .x = (float) current_cascade.probe_size.x * (x + 0.5f),
                .y = (float) current_cascade.probe_size.y * (y + 0.5f),
            };

            for(int32 direction_index = 0;
                direction_index < current_cascade.angular_number;
                ++direction_index) {
                float direction_angle =
                    2.f * PI *
                    (((float) direction_index + 0.5f) /
                     (float) current_cascade.angular_number);

                vec2f ray_direction = vec2f_from_angle(direction_angle);

                int32 result_index =
                    (y * current_cascade.probe_number.x + x) *
                    current_cascade.angular_number + direction_index;

                vec4f result =
                    map_ray_intersect(
                            m,
                            probe_center,
                            ray_direction,
                            current_cascade.interval.x,
                            current_cascade.interval.y);

                current_cascade.data[result_index] = result;
            }
        }
    }

    int32 probes_per_current_probe =
        (int32) (1.f / current_cascade_dimension_scaling);

    // Apply to existent data
    for(int32 probe_index = 0;
            probe_index <
            cascade.probe_number.x *
            cascade.probe_number.y;
            ++probe_index) {
        // Loop through base cascade probes (cascade_index 0)
        vec4f *probe =
            &cascade.data[probe_index * cascade.angular_number];

        int32 probe_x = probe_index % cascade.probe_number.x;
        int32 probe_y = probe_index / cascade.probe_number.x;

        int32 current_probe_x = probe_x / probes_per_current_probe;
        int32 current_probe_y = probe_y / probes_per_current_probe;

        int32 current_probe_index =
            current_probe_y * current_cascade.probe_number.x +
            current_probe_x;
        vec4f *current_probe = &current_cascade.data[
            current_probe_index * current_cascade.angular_number];

        for(int32 direction_index = 0;
                direction_index < cascade.angular_number;
                ++direction_index) {
            // For each direction of the base cascade probe

            vec4f current_average_radiance = {};
            int32 current_direction_index_base =
                direction_index * current_cascade_angular_scaling;
            for(int32 current_direction_index_offset = 0;
                    current_direction_index_offset <
                    current_cascade_angular_scaling;
                    ++current_direction_index_offset) {

                int32 current_direction_index =
                    current_direction_index_base +
                    current_direction_index_offset;
                vec4f current_radiance =
                    current_probe[current_direction_index];

                current_average_radiance = vec4f_sum_vec4f(
                        current_average_radiance,
                        vec4f_divide(
                            current_radiance,
                            (float) current_cascade_angular_scaling));
            }
            if (merge_with_existing) {
                // preexistant radiance
                vec4f probe_direction_radiance = probe[direction_index];
                // the current_average_radiance is the radiance of
                // lower level, so we prioritize it in the merging
                probe[direction_index] = cascade_merge_intervals(
                        current_average_radiance,
                        probe_direction_radiance);
            } else {
                // if we don't merge then we just apply the value.
                // this is usually done when we first calculate
                //  the highest level
                probe[direction_index] = current_average_radiance;
            }
        }
    }

    cascade_free(&current_cascade);
}

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

        // ray cast interval dimension
        float base_interval = CASCADE0_INTERVAL_LENGTH;
        float cascade_interval_scaling =
            powf((float) INTERVAL_SCALING, (float) cascade_index);
        float interval_length =
            base_interval * cascade_interval_scaling;
        float interval_start =
            ((powf((float) base_interval, (float) cascade_index + 1.f) -
              (float) base_interval) /
            (float) (base_interval - 1)) * (1.f - (float)INTERVAL_OVERLAP);
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
                    map_ray_intersect(
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
                    probe[direction_index] = cascade_merge_intervals(
                            probe_direction_radiance,
                            average_radiance_up);
                }
            }
        }
    }
}

// TODO(gio): do this better, result sucks rn
void cascade_apply_skybox(
        radiance_cascade cascade,
        vec4f skybox_color) {
    // merge skybox into cascade
    for(int32 data_index = 0;
        data_index < cascade.data_length;
        ++data_index) {
        cascade.data[data_index] =
            vec4f_sum_vec4f(cascade.data[data_index], skybox_color);
    }
}

void cascade_to_map(map m, radiance_cascade cascade) {
    // applying cascades into the pixels
    for (int32 pixel_index = 0; pixel_index < m.w * m.h; ++pixel_index) {
        int32 x = pixel_index % m.w;
        int32 y = pixel_index / m.w;

        int32 probe_x = x / cascade.probe_size.x;
        int32 probe_y = y / cascade.probe_size.y;
        int32 probe_index =
            (probe_y * cascade.probe_number.x + probe_x) *
            cascade.angular_number;

        // printf("pixel(%d, %d) probe(%d, %d) probe_index(%d)\n",
        //         x, y, probe_x, probe_y, probe_index);

        vec4f *probe = &cascade.data[probe_index];

        vec4f average = {};
        for(int32 direction_index = 0;
            direction_index < cascade.angular_number;
            ++direction_index) {

            vec4f radiance = probe[direction_index];
            average = vec4f_sum_vec4f(
                    average,
                    vec4f_divide(
                        radiance,
                        cascade.angular_number));
        }
        average.a = 1.f;

        m.pixels[pixel_index] = average;
    }
}

#endif // RADIANCE_CASCADES_CASCADES_IMPLEMENTATION

#endif // _RC_CASCADES_H_
