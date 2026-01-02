#include "cobragl/math.h"


cobra_vec3 cobra_vec3_add(cobra_vec3 a, cobra_vec3 b){
     return (cobra_vec3){{a.x + b.x, a.y + b.y, a.z + b.z}};
}

cobra_vec3 cobra_vec3_sub(cobra_vec3 a, cobra_vec3 b){
     return (cobra_vec3){{a.x - b.x, a.y - b.y, a.z - b.z}};
}

c_float cobra_vec3_dot(cobra_vec3 a, cobra_vec3 b){
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

cobra_vec3 cobra_vec3_cross(cobra_vec3 a, cobra_vec3 b){
    c_float x = a.y * b.z - a.z * b.y;
    c_float y = a.z * b.x - a.x * b.z;
    c_float z = a.x * b.y - a.y * b.x;
    return (cobra_vec3){{x, y, z}};
}

cobra_vec3 cobra_vec3_scale(cobra_vec3 a, c_float s){
  return (cobra_vec3){{a.x * s, a.y * s, a.z * s}};
}

c_float cobra_vec3_length(cobra_vec3 a){
    return sqrtf(cobra_vec3_dot(a, a));
}

cobra_vec3 cobra_vec3_normalize(cobra_vec3 a){
    c_float len = cobra_vec3_length(a);
    if(len == 0.0f) return (cobra_vec3){{0,0,0}};
    return cobra_vec3_scale(a, 1.0f / len);
}

bool cobra_vec3_is_collinear(cobra_vec3 a, cobra_vec3 b, cobra_vec3 c){
    cobra_vec3 ab = cobra_vec3_sub(b, a);
    cobra_vec3 ac = cobra_vec3_sub(c, a);
    cobra_vec3 cross = cobra_vec3_cross(ab, ac);
    return cobra_vec3_length(cross) < COBRA_MATH_EPSILON;
}

cobra_vec3 cobra_vec3_face_normal(cobra_vec3 a, cobra_vec3 b, cobra_vec3 c){
    cobra_vec3 ab = cobra_vec3_sub(b, a);
    cobra_vec3 ac = cobra_vec3_sub(c, a);
    return cobra_vec3_normalize(cobra_vec3_cross(ab, ac));
}

cobra_vec3 cobra_vec3_project(cobra_vec3 point, c_float fov_factor, c_float view_width, c_float view_height) {
    cobra_vec3 projected;
    // Evitiamo divisioni per zero se il punto è esattamente sulla camera
    c_float z = (point.z != 0.0f) ? point.z : 0.001f;
    
    // Perspective Divide: x' = x / z
    // Moltiplichiamo per fov_factor per scalare in base al campo visivo
    // Aggiungiamo metà larghezza/altezza per centrare (0,0) al centro dello schermo
    projected.x = (point.x * fov_factor) / z + (view_width * 0.5f);
    
    // Invertiamo la Y ( -point.y ) perché a schermo la Y va verso il basso
    projected.y = (-point.y * fov_factor) / z + (view_height * 0.5f);
    
    projected.z = point.z; // Manteniamo la Z originale per eventuale Z-buffer
    return projected;
}

cobra_vec3 cobra_vec3_rotate_x(cobra_vec3 v, c_float angle) {
    return (cobra_vec3){{
        v.x,
        v.y * cosf(angle) - v.z * sinf(angle),
        v.y * sinf(angle) + v.z * cosf(angle)
    }};
}

cobra_vec3 cobra_vec3_rotate_y(cobra_vec3 v, c_float angle) {
    return (cobra_vec3){{
        v.x * cosf(angle) - v.z * sinf(angle),
        v.y,
        v.x * sinf(angle) + v.z * cosf(angle)
    }};
}

cobra_vec3 cobra_vec3_rotate_z(cobra_vec3 v, c_float angle) {
    return (cobra_vec3){{
        v.x * cosf(angle) - v.y * sinf(angle),
        v.x * sinf(angle) + v.y * cosf(angle),
        v.z
    }};
}