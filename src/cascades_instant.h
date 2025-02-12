#ifndef _RC_CASCADES_INSTANT_H_
#define _RC_CASCADES_INSTANT_H_

#include <stdlib.h>

#include "cascades.h"

radiance_cascade
cascade_instant_init(map m);

void
cascade_instant_generate(map m, radiance_cascade cascade, int32 cascade_index, int32 merge_with_existing);

radiance_cascade
cascade_from_cascade0(radiance_cascade cascade0, int32 cascade_index);

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

    return cascade;
}

void cascade_instant_generate(
        map m,
        radiance_cascade cascade0,
        int32 cascades_number,
        int32 merge_with_existing) {
// WORK IN PROGRESS but want to make it compile :)
#if 0
    // ### If the cascade is not initialized, return
    if (cascade0.data == NULL) return;
    
    // TODO(gio): need to also cache the probe center
    // NOTE(gio): the `-1` everywhere is because we do not cache cascade0 probes
    // allocate cache memory
    // TODO(gio): double check this shit
    int32 cache_size =
        CASCADE0_ANGULAR_NUMBER *
        ((powf((float)ANGULAR_SCALING, cascades_number) - 1) /
            (ANGULAR_SCALING - 1)) - 1;
    vec4f *cache = calloc(cache_size, sizeof(vec4f));
    // distribute pointers to each cascade level cached probe
    // TODO(gio): double check this shit
    vec4f **cached_probes = calloc(cascades_number-1, sizeof(vec4f *));
    for(int32 cached_probe_index = 0;
            cached_probe_index < cascades_number - 1;
            ++cached_probe_index) {
        int32 cached_probe_offset =
            CASCADE0_ANGULAR_NUMBER *
            ((powf((float)ANGULAR_SCALING, cached_probe_index) - 1) /
             (ANGULAR_SCALING - 1));
        cached_probes[cached_probe_index] = cache + cached_probe_offset;
    }

    radiance_cascade top_cascade =
        cascade_from_cascade0(cascade0, cascades_number-1);

    // Calculating current cascade
    for(int32 x = 0; x < top_cascade.probe_number.x; ++x) {
        for(int32 y = 0; y < top_cascade.probe_number.y; ++y) {
            // probe center position to raycast from
            vec2f probe_center = {
                .x = (float) top_cascade.probe_size.x * (x + 0.5f),
                .y = (float) top_cascade.probe_size.y * (y + 0.5f),
            };

            for(int32 direction_index = 0;
                direction_index < top_cascade.angular_number;
                ++direction_index) {
                float direction_angle =
                    2.f * PI *
                    (((float) direction_index + 0.5f) /
                     (float) top_cascade.angular_number);

                vec2f ray_direction = vec2f_from_angle(direction_angle);

                int32 result_index =
                    (y * top_cascade.probe_number.x + x) *
                    top_cascade.angular_number + direction_index;

                vec4f radiance =
                    map_ray_intersect(
                            m,
                            probe_center,
                            ray_direction,
                            top_cascade.interval.x,
                            top_cascade.interval.y);

                top_cascade.data[result_index] = near_radiance;
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
#endif
}

radiance_cascade cascade_from_cascade0(
        radiance_cascade cascade0,
        int32 cascade_index) {
    radiance_cascade current_cascade = {};
    // probe count for each dimension
    float current_cascade_dimension_scaling =
        powf((float) DIMENSION_SCALING, (float) cascade_index);
    current_cascade.probe_number = (vec2i) {
        .x = cascade0.probe_number.x * current_cascade_dimension_scaling,
        .y = cascade0.probe_number.y * current_cascade_dimension_scaling
    };

    // angular frequency
    float current_cascade_angular_scaling =
        powf((float) ANGULAR_SCALING, (float) cascade_index);
    current_cascade.angular_number =
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
    current_cascade.interval = (vec2f) {
        .x = interval_start,
        .y = interval_end
    };
    current_cascade.probe_size = (vec2f) {
        .x = (float) cascade0.probe_size.x / current_cascade_dimension_scaling,
        .y = (float) cascade0.probe_size.y / current_cascade_dimension_scaling
    };

    current_cascade.data_length =
        current_cascade.probe_number.x *
        current_cascade.probe_number.y *
        current_cascade.angular_number;
    // We do not allocate any data, we just need the info
    current_cascade.data = NULL;

    return current_cascade;
}

#endif // RADIANCE_CASCADES_CASCADES_INSTANT_IMPLEMENTATION

#endif // _RC_CASCADES_INSTANT_H_
