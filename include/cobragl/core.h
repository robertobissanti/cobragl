#ifndef COBRA_CORE_H
#define COBRA_CORE_H

#include <stdbool.h> // For bool type
#include <stdint.h>
#include <SDL3/SDL.h> // For SDL_Window, SDL_Renderer, etc.
#include "cobragl/math.h"

typedef struct {
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

#endif