#ifndef _RC_CASCADES_INSTANT_H_
#define _RC_CASCADES_INSTANT_H_

#include <stdlib.h>
#include <assert.h>

#include "cascades.h"

#define BILINEAR_FIX_INSTANT_CASCADES 1

#if BILINEAR_FIX_INSTANT_CASCADES != 0

typedef struct cached_row {
    int32 data_length;
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

Method:
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

void
calculate_cascades_and_apply_to_map(map m, int32 cascades_number);


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

    cascade->rows[0].data_length = row_data_length;
    cascade->rows[0].data = data_start;
    cascade->rows[0].y =  -100;

    cascade->rows[1].data_length = row_data_length;
    cascade->rows[1].data = data_start + row_data_length;
    cascade->rows[1].y =  -100;

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

    cached_rows_cascade->rows[0].data_length = row_data_length;
    cached_rows_cascade->rows[0].data = data_start;
    cached_rows_cascade->rows[0].y =  -100;

    cached_rows_cascade->rows[1].data_length = row_data_length;
    cached_rows_cascade->rows[1].data = data_start + row_data_length;
    cached_rows_cascade->rows[1].y =  -100;

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
        int32 cascade0_bilinear_base_y = (int32)
            floorf(((float) (pix_y) /
                        (float) cascades[0].probe_size.y) - 0.5f);
        // NOTE(gio): check for each cascade if I need to calculate a new row
        for(int32 cascade_index = cascades_number - 1;
                cascade_index >= 0;
                --cascade_index) {

            cached_rows_radiance_cascade *cascade = &cascades[cascade_index];
            // will need this later for the merging

            // loop constant: every higher cascade is already calculated

            int32 bilinear_base_y = cascade0_bilinear_base_y;
            for(int32 i = 1; i <= cascade_index-1; i++) {
                cached_rows_radiance_cascade *current = &cascades[i];
                cached_rows_radiance_cascade *current_up = &cascades[i+1];

                bilinear_base_y = floorf(((bilinear_base_y + 0.5f) *
                                    current->probe_size.y /
                                    current_up->probe_size.y) - 0.5f);
            }

            for(int32 row_index = 0; row_index < 2; ++row_index) {
                cached_row *row = &cascade->rows[row_index];

                // this `if` is for when the first row will replace the second row
                if (row_index == 0 &&
                    row->y != bilinear_base_y &&
                    cascade->rows[1].y == bilinear_base_y) {
                    // just need to copy the row content
                    memcpy(row->data,
                           cascade->rows[1].data,
                           row->data_length * sizeof(vec4f));
                    printf("cascade(%d) row(old: %d, new: %d) just copying\n",
                            cascade_index,
                            row->y,
                            bilinear_base_y + row_index);
                    row->y = bilinear_base_y;

                // this is for the generic case, where the data is outdated
                } else if (row->y != bilinear_base_y + row_index) {
                    printf("cascade(%d) row(old: %d, new: %d)\n",
                            cascade_index,
                            row->y,
                            bilinear_base_y + row_index);
                    row->y = bilinear_base_y + row_index;
                    // calculate cascade row
                    for(int32 x = 0; x < cascade->probe_number.x; ++x) {
                        int32 y = row->y;

                        // probe center position to raycast from
                        vec2f probe_center = {
                            .x = (float) cascade->probe_size.x * (x + 0.5f),
                            .y = (float) cascade->probe_size.y * (y + 0.5f),
                        };
                        // printf("angular_number(%d)\n",
                        //         cascade->angular_number);
                        for(int32 direction_index = 0;
                            direction_index < cascade->angular_number;
                            ++direction_index) {
                            float direction_angle =
                                2.f * PI *
                                (((float) direction_index + 0.5f) /
                                 (float) cascade->angular_number);

                            vec2f ray_direction =
                                vec2f_from_angle(direction_angle);

                            int32 result_index =
                                x * cascade->angular_number + direction_index;

                            vec4f result =
                                map_ray_intersect(
                                        m,
                                        probe_center,
                                        ray_direction,
                                        cascade->interval.x,
                                        cascade->interval.y);

                            row->data[result_index] = result;
                        }
                    }

                    // no need to merge if it's the upper most cascade
                    if (cascade_index == cascades_number - 1) {
                        continue;
                    }

                    cached_rows_radiance_cascade *cascade_up =
                        &cascades[cascade_index + 1];
                    float angular_scaling =
                        cascade_up->angular_number / cascade->angular_number;
                    // [START] merge with the cascade above if necessary

                    int32 probe_y = row->y;

                    // NOTE(bilinear): for finding top-left bilinear probe (of cascade_up obv)
                    vec2f base_coord = vec2f_sum_vec2f(
                        (vec2f) {
                            .x = 0.f,
                            .y = (float)
                                ((probe_y + 0.5f) * cascade->probe_size.y) /
                                    (float) cascade_up->probe_size.y
                        }, (vec2f) { .x = 0.f, .y = -0.5f });
                    vec2i bilinear_base = (vec2i) {
                        .x = 0,
                        .y = (int32) floorf(base_coord.y)
                    };
                    vec2f ratio = (vec2f) {
                        .x = 0.f,
                        .y = (base_coord.y - (int32) base_coord.y) *
                                SIGN(base_coord.y)
                    };

                    for(int32 probe_x = 0;
                        probe_x < cascade->probe_number.x;
                        ++probe_x) {

                        base_coord.x = (float)
                            (((probe_x + 0.5f) * cascade->probe_size.x) /
                             (float) cascade_up->probe_size.x) - 0.5f;
                        bilinear_base.x = (int32) floorf(base_coord.x);
                        ratio.x = (base_coord.x - (int32) base_coord.x) *
                            SIGN(base_coord.x);
                        vec4f weights = bilinear_weights(ratio);

                        vec4f *probe =
                            &row->data[probe_x * cascade->angular_number];

                        for(int32 direction_index = 0;
                                direction_index < cascade->angular_number;
                                ++direction_index) {

                            vec4f average_radiance_up = {};
                            int32 direction_up_index_base =
                                direction_index * ANGULAR_SCALING;
                            for(int32 direction_up_index_offset = 0;
                                    direction_up_index_offset < ANGULAR_SCALING;
                                    ++direction_up_index_offset) {

                                int32 direction_up_index =
                                    direction_up_index_base + direction_up_index_offset;

                                // NOTE(bilinear): get the radiance from
                                //                  4 probes around only
                                //                  valid probes are used
                                //                  (e.g. on the corners,
                                //                      some positions might
                                //                      be invalid, hence will
                                //                      not be used)
                                vec4f radiance_up = {};

                                // Count how many values
                                // have been used in the average
                                int32 usable_probe_up_count = 0;

                                for(int32 bilinear_index = 0;
                                        bilinear_index < 4;
                                        ++bilinear_index) {

                                    vec2i offset = bilinear_offset(bilinear_index);

                                    printf("cascade_up(%d) row0(%d, %d) row1(%d, %d) pix_y(%d)\n",
                                            cascade_index + 1,
                                            cascade_up->rows[0].y,
                                            bilinear_base.y,
                                            cascade_up->rows[1].y,
                                            bilinear_base.y+1,
                                            pix_y);
                                    printf("cascade_up(probe_size(%f, %f))\n",
                                            cascade_up->probe_size.x,
                                            cascade_up->probe_size.y);
                                    printf("cascade(probe_size(%f, %f))\n",
                                            cascade->probe_size.x,
                                            cascade->probe_size.y);
                                    assert(cascade_up->rows[0].y == bilinear_base.y);
                                    assert(cascade_up->rows[1].y == bilinear_base.y+1);

                                    if (0 <= bilinear_base.x + offset.x &&
                                            bilinear_base.x + offset.x <
                                            cascade_up->probe_number.x &&
                                        0 <= bilinear_base.y + offset.y &&
                                            bilinear_base.y + offset.y <
                                            cascade_up->probe_number.y) {

                                        // here the value is usable for the average
                                        usable_probe_up_count++;

                                        cached_row bilinear_row_up = cascade_up->rows[offset.y];
                                        int32 bilinear_probe_up_index = bilinear_base.x + offset.x;

                                        vec4f *bilinear_probe_up = &bilinear_row_up.data[
                                            bilinear_probe_up_index * cascade_up->angular_number];

                                        vec4f bilinear_radiance_up =
                                            bilinear_probe_up[direction_up_index];

                                        radiance_up = vec4f_sum_vec4f(
                                                radiance_up,
                                                vec4f_divide(
                                                    vec4f_diff_vec4f(
                                                        bilinear_radiance_up,
                                                        radiance_up),
                                                    (float) usable_probe_up_count));
                                    }
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
                    // [END]
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
