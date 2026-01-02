#include "cobragl/utils.h"

int cobra_vec3_print(cobra_vec3 v){
   printf("x: %f, y: %f, z: %f\n", v.comp[0], v.comp[1], v.comp[2]);
   return 0;
}