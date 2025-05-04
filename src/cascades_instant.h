#ifndef _RC_CASCADES_INSTANT_H_
#define _RC_CASCADES_INSTANT_H_

#include <stdlib.h>

#include "cascades.h"

#define BILINEAR_FIX_INSTANT_CASCADES 1

#if BILINEAR_FIX_INSTANT_CASCADES != 0

typedef struct cached_row {
    vec4f *data;
    int32 y;
} cached_row;

typedef struct cached_rows_radiance_cascade {
    vec2i probe_number;
    int32 angular_number;
    vec2f interval;
    vec2f probe_size;
    cached_row rows[2]; // need 2 rows to apply bilinear fix on all levels
} cached_rows_radiance_cascade;


/*

Method 1 (top-down):
 - calculate 2 rows from upper cascade
 - take the 2 top-most row of the previous cascade inside of the 2 upper rows
 - repeat until you get to the lowest cascade
 - apply it to the pixels that use the current cascades in cache

Method 2 (kinda bottom-up):
 - iterate the pixels by row
 - for each new pixel row check if the cache has to be updated
 - to update (same as top-down):
    - calculate from the upper cascade downwards, so to apply the bilinear fix
    - kind of the same

key differences:
 - in Method 1 it would all start from the iteration of the upper cascade
    - inside each iteration it would recurse down, where there would be
        an additional iteration
    - when we get to the first cascade, only then we care about pixels
*/

void
cached_rows_cascade0_init(map m, cached_rows_radiance_cascade *cascade);

void
cached_rows_cascade_from_cascade0(cached_rows_radiance_cascade cascade0, cached_rows_radiance_cascade *cached_rows_cascade, int32 cascade_index);


#ifdef RADIANCE_CASCADES_CASCADES_INSTANT_IMPLEMENTATION

void cached_rows_cascade0_init(map m, cached_rows_radiance_cascade *cascade) {
    if (!cascade) return; // i dare you

    // probe count for each dimension
    cascade->probe_number = (vec2i) {
        .x = CASCADE0_PROBE_NUMBER_X,
        .y = CASCADE0_PROBE_NUMBER_Y
    };

    // angular frequency
    cascade->angular_number = CASCADE0_ANGULAR_NUMBER;

    // ray cast interval dimension
    cascade->interval = (vec2f) {
        .x = 0.f,
        .y = CASCADE0_INTERVAL_LENGTH
    };
    cascade->probe_size = (vec2f) {
        .x = (float) m.w / (float) cascade->probe_number.x,
        .y = (float) m.h / (float) cascade->probe_number.y
    };

    // allocate cascade memory
    int32 row_data_length =
        cascade->probe_number.x * cascade->angular_number;

    vec4f *data_start = calloc(row_data_length * 2, sizeof(vec4f));

    cascade->rows[0].data = data_start;
    cascade->rows[0].y =  -1;

    cascade->rows[1].data = data_start + row_data_length;
    cascade->rows[1].y =  -1;

    printf("allocated cascade0 rows(%d x 2)\n", row_data_length);
}

void cached_rows_cascade_from_cascade0(
        cached_rows_radiance_cascade cascade0,
        cached_rows_radiance_cascade *cached_rows_cascade,
        int32 cascade_index) {
    // fuck off?
    if (!cached_rows_cascade) return;

    // probe count for each dimension
    float current_cascade_dimension_scaling =
        powf((float) DIMENSION_SCALING, (float) cascade_index);
    cached_rows_cascade->probe_number = (vec2i) {
        .x = cascade0.probe_number.x * current_cascade_dimension_scaling,
        .y = cascade0.probe_number.y * current_cascade_dimension_scaling
    };

    // angular frequency
    float current_cascade_angular_scaling =
        powf((float) ANGULAR_SCALING, (float) cascade_index);
    cached_rows_cascade->angular_number =
        cascade0.angular_number * current_cascade_angular_scaling;

    // ray cast interval dimension
    float base_interval = cascade0.interval.y;
    float cascade_interval_scaling =
        powf((float) INTERVAL_SCALING, (float) cascade_index);
    float interval_length =
        base_interval * cascade_interval_scaling;
    float interval_start =
        ((powf((float) base_interval, (float) cascade_index + 1.f) -
          (float) base_interval) /
         (float) (base_interval - 1)) * (1.f - (float)INTERVAL_OVERLAP);
    float interval_end = interval_start + interval_length;
    cached_rows_cascade->interval = (vec2f) {
        .x = interval_start,
        .y = interval_end
    };
    cached_rows_cascade->probe_size = (vec2f) {
        .x = (float) cascade0.probe_size.x / current_cascade_dimension_scaling,
        .y = (float) cascade0.probe_size.y / current_cascade_dimension_scaling
    };

    int32 row_data_length =
        cached_rows_cascade->probe_number.x * cached_rows_cascade->angular_number;
    vec4f *data_start = calloc(row_data_length * 2, sizeof(vec4f));

    cached_rows_cascade->rows[0].data = data_start;
    cached_rows_cascade->rows[0].y =  -1;

    cached_rows_cascade->rows[1].data = data_start + row_data_length;
    cached_rows_cascade->rows[1].y =  -1;

    printf("allocated cascade(%d) rows(%d x 2)\n",
            cascade_index,
            row_data_length);
}

void calculate_cascades_and_apply_to_map(map m, int32 cascades_number) {
    cached_rows_radiance_cascade *cascades =
        calloc(cascades_number, sizeof(cached_rows_radiance_cascade));

    // fill information about first cascade
    cached_rows_cascade0_init(m, &cascades[0]);

    // fill information about other cascades
    for(int32 cascade_index = 1;
        cascade_index < cascades_number;
        ++cascade_index) {
        cached_rows_cascade_from_cascade0(
                cascades[0],
                &cascades[cascade_index],
                cascade_index);
    }

    // NOTE(gio): iterate each pixel row
    for(int32 pix_y = 0; pix_y < m.h; ++pix_y) {
        // NOTE(gio): check for each cascade if I need to calculate a new row
        for(int32 cascade_index = cascades_number - 1;
                cascade_index >= 0;
                --cascade_index) {

            cached_rows_radiance_cascade *cascade = &cascades[cascade_index];

            // loop constant: every higher cascade is already calculated
            int32 bilinear_base_y = (int32)
                floorf(((float) (pix_y + 0.5f) /
                        (float) cascade0.probe_size.y) - 0.5f);

            for(int32 row_index = 0; row_index < 2; ++row_index) {
                cached_row row = cascade->rows[row_index];

                // TODO(gio): update the cached_row
                if (row.y != bilinear_base_y + row_index) {
                }
            }
        }

        cached_rows_radiance_cascade cascade0 = cascades[0];

        vec2f base_coord = (vec2f) {
            .x = -1, // updated for each pixel
            .y = ((float) (pix_y + 0.5f) /
                  (float) cascade0.probe_size.y) - 0.5f
        };
        vec2i bilinear_base = (vec2i) {
            .x = -1, // updated for each pixel
            .y = (int32) floorf(base_coord.y)
        };
        vec2f ratio = (vec2f) {
            .x = -1, // updated for each pixel
            .y = (base_coord.y - (int32) base_coord.y) * SIGN(base_coord.y),
        };

        // here all the cache is correct for the pixel i need
        for(int32 pix_x = 0; pix_x < m.w; ++pix_x) {
            base_coord.x = ((float) (pix_x + 0.5f) /
                            (float) cascade0.probe_size.x) - 0.5f;

            bilinear_base.x = (int32) floorf(base_coord.x);

            ratio.x =
                (base_coord.x - (int32) base_coord.x) * SIGN(base_coord.x);

            vec4f weights = bilinear_weights(ratio);

            vec4f average = {};
            for(int32 direction_index = 0;
                direction_index < cascade0.angular_number;
                ++direction_index) {

                // NOTE(bilinear): get the radiance from 4 probes around
                vec4f radiance_up = {};

                // Count how many values have been used in the average
                int32 usable_probe_up_count = 0;

                for(int32 bilinear_index = 0;
                        bilinear_index < 4;
                        ++bilinear_index) {

                    // NOTE(gio): offset.y represents which cache row I need
                    //              to use currently.
                    vec2i offset = bilinear_offset(bilinear_index);

                    if (0 <= bilinear_base.x + offset.x &&
                        bilinear_base.x + offset.x < cascade0.probe_number.x &&
                        0 <= bilinear_base.y + offset.y &&
                        bilinear_base.y + offset.y < cascade0.probe_number.y) {

                        // here the value is usable for the average
                        usable_probe_up_count++;

                        // TODO(gio): check offset.y is either 0 or 1
                        cached_row bilinear_row = cascade0.rows[offset.y];
                        int32 bilinear_probe_index = bilinear_base.x + offset.x;
                        vec4f *bilinear_probe = &bilinear_row.data[
                            bilinear_probe_index * cascade0.angular_number];

                        vec4f bilinear_radiance =
                            bilinear_probe[direction_index];

                        radiance_up = vec4f_sum_vec4f(
                                radiance_up,
                                vec4f_divide(
                                    vec4f_diff_vec4f(
                                        bilinear_radiance,
                                        radiance_up),
                                    (float) usable_probe_up_count));
                    }

                }
                average = vec4f_sum_vec4f(
                        average,
                        vec4f_divide(
                            radiance_up,
                            cascade0.angular_number));
            }
            average.a = 1.f;

            int32 pixel_index = pix_y * m.w + pix_x;
            m.pixels[pixel_index] = average;
        }
    }
}


#endif // RADIANCE_CASCADES_CASCADES_INSTANT_IMPLEMENTATION

#else // BILINEAR_FIX_INSTANT_CASCADES == 0

typedef struct cached_probe {
    vec4f *data;
    int32 x;
    int32 y;
} cached_probe;

typedef struct cached_radiance_cascade {
    vec2i probe_number;
    int32 angular_number;
    vec2f interval;
    vec2f probe_size;
    cached_probe probe;
} cached_radiance_cascade;

radiance_cascade
cascade_instant_init(map m);

void
cascade_instant_generate(map m, radiance_cascade cascade0, int32 cascades_number);

void
cascade_instant_recurse_down(map m, radiance_cascade cascade0, int32 cascade_index, cached_radiance_cascade *cascades, int32 cached_cascades_length);

void
cascade_cached_from_cascade0(radiance_cascade cascade0, cached_radiance_cascade *cached_cascade, int32 cascade_index);

#ifdef RADIANCE_CASCADES_CASCADES_INSTANT_IMPLEMENTATION

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
    printf("allocated cascade0(%d)\n", cascade.data_length);

    return cascade;
}

void cascade_instant_generate(
        map m,
        radiance_cascade cascade0,
        int32 cascades_number) {
    // ### If the cascade is not initialized, return
    if (cascade0.data == NULL) return;
    
    // TODO(gio): double check this shit
    int32 cache_size =
        CASCADE0_ANGULAR_NUMBER *
        ((powf((float)ANGULAR_SCALING, cascades_number) - 1) /
            (ANGULAR_SCALING - 1));
    vec4f *cache = calloc(cache_size, sizeof(vec4f));
    printf("allocated cache(%d)\n", cache_size);
    // distribute pointers to each cascade level cached probe
    // TODO(gio): double check this shit
    int32 cached_cascades_length = cascades_number;
    cached_radiance_cascade *cascades =
        calloc(cached_cascades_length, sizeof(cached_radiance_cascade));
    printf("allocated cached_cascades(%d)\n", cached_cascades_length);
    for(int32 cached_cascade_index = 0;
            cached_cascade_index < cached_cascades_length;
            ++cached_cascade_index) {
        int32 cached_probe_offset =
            CASCADE0_ANGULAR_NUMBER *
            ((powf((float)ANGULAR_SCALING, cached_cascade_index) - 1) /
             (ANGULAR_SCALING - 1));

        // initialize the cached cascade
        cascade_cached_from_cascade0(
                cascade0,
                &cascades[cached_cascade_index],
                cached_cascade_index);
        cascades[cached_cascade_index].probe.data =
            cache + cached_probe_offset;
        // TODO(gio): you sure about this -1?
        cascades[cached_cascade_index].probe.x = -1;
        cascades[cached_cascade_index].probe.y = -1;

        printf("cached_cascade(%d) probe_number(%d, %d)\n",
                cached_cascade_index,
                cascades[cached_cascade_index].probe_number.x,
                cascades[cached_cascade_index].probe_number.y);
    }

    cached_radiance_cascade *top_cascade =
        &cascades[cached_cascades_length - 1];

    // Calculating current cascade
    for(int32 top_probe_index = 0;
            top_probe_index < top_cascade->probe_number.x *
            top_cascade->probe_number.y;
            ++top_probe_index) {

        top_cascade->probe.x = top_probe_index % top_cascade->probe_number.x;
        top_cascade->probe.y = top_probe_index / top_cascade->probe_number.x;

        // probe center position to raycast from
        vec2f probe_center = {
            .x = (float) top_cascade->probe_size.x *
                (top_cascade->probe.x + 0.5f),
            .y = (float) top_cascade->probe_size.y *
                (top_cascade->probe.y + 0.5f),
        };

        // calculate the top probe, then recurse down
        for(int32 direction_index = 0;
                direction_index < top_cascade->angular_number;
                ++direction_index) {
            float direction_angle =
                2.f * PI *
                (((float) direction_index + 0.5f) /
                 (float) top_cascade->angular_number);

            vec2f ray_direction = vec2f_from_angle(direction_angle);

            vec4f radiance =
                map_ray_intersect(
                        m,
                        probe_center,
                        ray_direction,
                        top_cascade->interval.x,
                        top_cascade->interval.y);

            top_cascade->probe.data[direction_index] = radiance;
        }

        // now recursing down within the current top probe
        cascade_instant_recurse_down(
                m,
                cascade0,
                cascades_number - 2,
                cascades,
                cached_cascades_length);
    }
}

void cascade_instant_recurse_down(
        map m,
        radiance_cascade cascade0,
        int32 cascade_index,
        cached_radiance_cascade *cascades,
        int32 cached_cascades_length) {

    // remember that in the cache the first cascade is not present, hence the -1
    cached_radiance_cascade *cascade = &cascades[cascade_index];
    cached_radiance_cascade *top_cascade = &cascades[cascade_index + 1];

    int32 probes_per_up_probe_row =
        (int32) (1.f / (float) DIMENSION_SCALING);

    int32 probe_index_base =
        top_cascade->probe.y *
        top_cascade->probe_number.x *
            POW2(probes_per_up_probe_row) +
        top_cascade->probe.x * probes_per_up_probe_row;

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
            top_cascade->probe_number.x * probes_per_up_probe_row +
            probe_index_base_offset;

        cascade->probe.x = probe_index % cascade->probe_number.x;
        cascade->probe.y = probe_index / cascade->probe_number.x;

        // probe center position to raycast from
        vec2f probe_center = {
            .x = (float) cascade->probe_size.x * (cascade->probe.x + 0.5f),
            .y = (float) cascade->probe_size.y * (cascade->probe.y + 0.5f),
        };

        for(int32 direction_index = 0;
                direction_index < cascade->angular_number;
                ++direction_index) {

            float direction_angle =
                2.f * PI *
                (((float) direction_index + 0.5f) /
                 (float) cascade->angular_number);

            vec2f ray_direction = vec2f_from_angle(direction_angle);

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
                vec4f radiance_up = top_cascade->probe.data[direction_up_index];
                // if (radiance_up.a == 0.f) {
                //     average_alpha = 0.f;
                // }
                average_radiance_up = vec4f_sum_vec4f(
                        average_radiance_up,
                        vec4f_divide(
                            radiance_up,
                            (float) ANGULAR_SCALING));
            }

            // calculate current cascade probe radiance for this direction
            vec4f probe_direction_radiance =
                map_ray_intersect(
                        m,
                        probe_center,
                        ray_direction,
                        cascade->interval.x,
                        cascade->interval.y);

            // already merge top cascade in this one, so it's ready to use
            cascade->probe.data[direction_index] = cascade_merge_intervals(
                    probe_direction_radiance,
                    average_radiance_up);

            if (cascade_index == 0) {
                int32 result_index =
                    (cascade->probe.y * cascade->probe_number.x +
                     cascade->probe.x) *
                    cascade->angular_number + direction_index;
                cascade0.data[result_index] =
                    cascade->probe.data[direction_index];
            }
        }

        // now recursing down within the current probe, if is not the last one
        if (cascade_index > 0) {
            cascade_instant_recurse_down(
                    m,
                    cascade0,
                    cascade_index - 1,
                    cascades,
                    cached_cascades_length);
        }
    }
}

void cascade_cached_from_cascade0(
        radiance_cascade cascade0,
        cached_radiance_cascade *cached_cascade,
        int32 cascade_index) {
    // fuck off?
    if (!cached_cascade) return;

    // probe count for each dimension
    float current_cascade_dimension_scaling =
        powf((float) DIMENSION_SCALING, (float) cascade_index);
    cached_cascade->probe_number = (vec2i) {
        .x = cascade0.probe_number.x * current_cascade_dimension_scaling,
        .y = cascade0.probe_number.y * current_cascade_dimension_scaling
    };

    // angular frequency
    float current_cascade_angular_scaling =
        powf((float) ANGULAR_SCALING, (float) cascade_index);
    cached_cascade->angular_number =
        cascade0.angular_number * current_cascade_angular_scaling;

    // ray cast interval dimension
    float base_interval = cascade0.interval.y;
    float cascade_interval_scaling =
        powf((float) INTERVAL_SCALING, (float) cascade_index);
    float interval_length =
        base_interval * cascade_interval_scaling;
    float interval_start =
        ((powf((float) base_interval, (float) cascade_index + 1.f) -
          (float) base_interval) /
         (float) (base_interval - 1)) * (1.f - (float)INTERVAL_OVERLAP);
    float interval_end = interval_start + interval_length;
    cached_cascade->interval = (vec2f) {
        .x = interval_start,
        .y = interval_end
    };
    cached_cascade->probe_size = (vec2f) {
        .x = (float) cascade0.probe_size.x / current_cascade_dimension_scaling,
        .y = (float) cascade0.probe_size.y / current_cascade_dimension_scaling
    };
}

#endif // RADIANCE_CASCADES_CASCADES_INSTANT_IMPLEMENTATION

#endif // BILINEAR_FIX_INSTANT_CASCADES

#endif // _RC_CASCADES_INSTANT_H_
