#ifndef COBRA_MATH_H
#define COBRA_MATH_H

#include <math.h>
#include <stdbool.h>

#define COBRA_MATH_EPSILON 1e-5f
#define DEGREE 180.0f/M_PI

typedef float c_float;

typedef union {
    struct { c_float x, y, z; };
    c_float comp[3];
} cobra_vec3;

cobra_vec3 cobra_vec3_add(cobra_vec3 a, cobra_vec3 b);

cobra_vec3 cobra_vec3_sub(cobra_vec3 a, cobra_vec3 b);

c_float cobra_vec3_dot(cobra_vec3 a, cobra_vec3 b) ;

cobra_vec3 cobra_vec3_cross(cobra_vec3 a, cobra_vec3 b);

cobra_vec3 cobra_vec3_scale(cobra_vec3 a, c_float s);

c_float cobra_vec3_length(cobra_vec3 a);

cobra_vec3 cobra_vec3_normalize(cobra_vec3 a);

bool cobra_vec3_is_collinear(cobra_vec3 a, cobra_vec3 b, cobra_vec3 c);

cobra_vec3 cobra_vec3_face_normal(cobra_vec3 a, cobra_vec3 b, cobra_vec3 c);

cobra_vec3 cobra_vec3_project(cobra_vec3 point, c_float fov_factor, c_float view_width, c_float view_height);

cobra_vec3 cobra_vec3_rotate_x(cobra_vec3 v, c_float angle);

cobra_vec3 cobra_vec3_rotate_y(cobra_vec3 v, c_float angle);

cobra_vec3 cobra_vec3_rotate_z(cobra_vec3 v, c_float angle);

#endif