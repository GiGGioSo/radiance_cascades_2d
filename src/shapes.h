#ifndef _RC_SHAPES_H_
#define _RC_SHAPES_H_

#include "mathy.h"

typedef struct {
    vec2f center;
    float radius;
    vec4f color;
} circle;

typedef struct {
    vec2f pos;
    vec2f dim;
    vec4f color;
} rectangle;


#ifdef RADIANCE_CASCADES_SHAPES_IMPLEMENTATION
#endif

#endif // _RC_SHAPES_H_
