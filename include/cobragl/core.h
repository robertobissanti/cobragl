#ifndef COBRAGL_CORE_H
#define COBRAGL_CORE_H

#include <stdbool.h>
#include <stdint.h>
#include <SDL3/SDL.h>
#include "cobragl/math.h"

typedef struct cobra_window {
  SDL_Window *sdl_window;
  SDL_Renderer *sdl_renderer;
  SDL_Texture *color_buffer_texture;
  uint32_t *color_buffer;
  float *z_buffer;
  int width;
  int height;
  bool should_close;
} cobra_window;

bool cobra_window_create(cobra_window *win, int width, int height, const char *title);
void cobra_window_destroy(cobra_window *win);
void cobra_window_poll_events(cobra_window *win);
void cobra_window_clear(cobra_window *win, uint32_t color);
void cobra_window_present(cobra_window *win);

void cobra_window_draw_point(cobra_window *win, int x, int y, uint32_t color);
void cobra_window_draw_point_aa(cobra_window *win, int x, int y, uint32_t color, float alpha);
void cobra_window_draw_line(cobra_window *win, int x0, int y0, int x1, int y1, uint32_t color);
void cobra_window_draw_line_aa(cobra_window *win, float x0, float y0, float x1, float y1, float width, uint32_t color, bool use_ss);
// Disegna una linea 3D gestendo proiezione e clipping (Near Plane)
void cobra_window_draw_line_3d(cobra_window *win, cobra_vec3 p1, cobra_vec3 p2, float fov, float thickness, uint32_t color, bool aa, bool use_ss);

#endif // COBRAGL_CORE_H