#ifndef _RC_MATHY_H_
#define _RC_MATHY_H_

///
////////////////////////////////////
/// TY - MATHY THE MATHEMATICIAN ///
////////////////////////////////////
///

#include <stdint.h>

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) (((a) > (b)) ? (b) : (a))
#endif
#ifndef CLAMP
#define CLAMP(x, min, max) MIN(MAX((x), (min)), (max))
#endif

#ifndef UINT8_MAX
#define UINT8_MAX 0xff
#endif

#ifndef UINT16_MAX
#define UINT16_MAX 0xffff
#endif

#ifndef UINT32_MAX
#define UINT32_MAX 0xffffffff
#endif

#ifndef UINT64_MAX
#define UINT64_MAX 0xffffffffffffffff
#endif

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef struct vec2i {
    union {
        int32 e[2];
        struct {
            int32 x, y;
        };
    };
} vec2i;

typedef struct vec3i {
    union {
        int32 e[3];
        struct {
            int32 x, y, z;
        };
        struct {
            int32 r, g, b;
        };
    };
} vec3i;

typedef struct vec4i {
    union {
        int32 e[4];
        struct {
            int32 x, y, z, w;
        };
        struct {
            int32 r, g, b, a;
        };
    };
} vec4i;

typedef struct vec2f {
    union {
        float e[2];
        struct {
            float x, y;
        };
    };
} vec2f;

typedef struct vec3f {
    union {
        float e[3];
        struct {
            float x, y, z;
        };
        struct {
            float r, g, b;
        };
    };
} vec3f;

typedef struct vec4f {
    union {
        float e[4];
        struct {
            float x, y, z, w;
        };
        struct {
            float r, g, b, a;
        };
    };
} vec4f;

typedef struct mat4f {
    union {
        float e[16];
        float m[4][4];
    };
} mat4f;

vec4f
make_vec4f(float x, float y, float z, float w);

float
vec2f_length_squared(vec2f v);

vec4f
mat4f_x_vec4f(mat4f m, vec4f v);

mat4f
mat4f_x_mat4f(mat4f m1, mat4f m2);


#ifdef RADIANCE_CASCADES_MATHY_IMPLEMENTATION

vec4f make_vec4f(float x, float y, float z, float w) {
    vec4f result;

    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;

    return result;
}

float vec2f_length_squared(vec2f v) {
    return v.x * v.x + v.y * v.y;
}

vec4f mat4f_x_vec4f(mat4f m, vec4f v) {
    vec4f result = {};

    for(int i = 0; i < 4; i++) {
        float row_x_column = 0.f;
        for(int j = 0; j < 4; j++) {
            row_x_column += m.m[i][j] * v.e[j];
        }
        result.e[i] = row_x_column;
    }

    return result;
}

mat4f mat4f_x_mat4f(mat4f m1, mat4f m2) {
    mat4f result = {};

    for(int row = 0; row < 4; row++) {
        for(int col = 0; col < 4; col++) {

            float row_x_column = 0.f;
            for(int iter = 0; iter < 4; iter++) {
                row_x_column += m1.m[row][iter] * m2.m[iter][col];
            }
            result.m[row][col] = row_x_column;
        }
    }

    return result;
}

#endif

#endif //_RC_MATHY_H_
