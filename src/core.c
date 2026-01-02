#include "cobragl/core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool cobra_window_create(cobra_window *win, int width, int height, const char *title)
{
  if (!win) return false;

  // Inizializziamo i puntatori a NULL per garantire una pulizia sicura in caso di errore
  win->sdl_window = NULL;
  win->sdl_renderer = NULL;
  win->color_buffer_texture = NULL;
  win->color_buffer = NULL;
  win->z_buffer = NULL;

  if (!SDL_Init(SDL_INIT_VIDEO))
  {
    fprintf(stderr, "Errore inizializzazione SDL: %s\n", SDL_GetError());
    return false;
  }

  // Creiamo finestra e renderer insieme
  if (!SDL_CreateWindowAndRenderer(title, width, height, 0, &win->sdl_window, &win->sdl_renderer))
  {
    fprintf(stderr, "Errore creazione finestra/renderer: %s\n", SDL_GetError());
    cobra_window_destroy(win); // Pulisce SDL (Quit)
    return false;
  }

  // Creiamo una texture in streaming per l'accesso diretto ai pixel
  win->color_buffer_texture = SDL_CreateTexture(
      win->sdl_renderer,
      SDL_PIXELFORMAT_ARGB8888,
      SDL_TEXTUREACCESS_STREAMING,
      width,
      height);

  if (!win->color_buffer_texture)
  {
    fprintf(stderr, "Errore creazione texture: %s\n", SDL_GetError());
    cobra_window_destroy(win); // Pulisce finestra e renderer
    return false;
  }

  // Abilitiamo il blending: SDL userà il canale Alpha per la trasparenza
  SDL_SetTextureBlendMode(win->color_buffer_texture, SDL_BLENDMODE_BLEND);

  // Allocazione buffer
  win->color_buffer = (uint32_t *)malloc(sizeof(uint32_t) * width * height);
  win->z_buffer = (float *)malloc(sizeof(float) * width * height);

  if (!win->color_buffer || !win->z_buffer)
  {
    fprintf(stderr, "Errore allocazione memoria buffer.\n");
    cobra_window_destroy(win); // Pulisce texture, renderer, finestra e buffer parziali
    return false;
  }

  win->width = width;
  win->height = height;
  win->should_close = false;

  return true;
}

void cobra_window_destroy(cobra_window *win)
{
  if (!win) return;

  if (win->color_buffer)
    free(win->color_buffer);
  if (win->z_buffer)
    free(win->z_buffer);
  if (win->color_buffer_texture)
    SDL_DestroyTexture(win->color_buffer_texture);
  if (win->sdl_renderer)
  {
    SDL_DestroyRenderer(win->sdl_renderer);
  }
  if (win->sdl_window)
  {
    SDL_DestroyWindow(win->sdl_window);
  }
  SDL_Quit();
}

void cobra_window_poll_events(cobra_window *win)
{
  if (!win) return;

  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    if (event.type == SDL_EVENT_QUIT)
    {
      win->should_close = true;
    }
  }
}

void cobra_window_clear(cobra_window *win, uint32_t color)
{
  if (!win) return;

  // Puliamo il color buffer
  for (int i = 0; i < win->width * win->height; i++)
  {
    win->color_buffer[i] = color;
    win->z_buffer[i] = 1.0f; // Inizializziamo Z a 1.0 (profondità massima)
  }
}

void cobra_window_present(cobra_window *win)
{
  if (!win) return;

  // Aggiorniamo la texture con i dati del buffer
  SDL_UpdateTexture(
      win->color_buffer_texture,
      NULL,
      win->color_buffer,
      (int)(win->width * sizeof(uint32_t)));

  // Copiamo la texture sul renderer
  SDL_RenderTexture(win->sdl_renderer, win->color_buffer_texture, NULL, NULL);
  SDL_RenderPresent(win->sdl_renderer);
}

void cobra_window_put_pixel(cobra_window *win, int x, int y, uint32_t color)
{
  if (!win) return;

  if (x >= 0 && x < win->width && y >= 0 && y < win->height)
  {
    win->color_buffer[(y * win->width) + x] = color;
  }
}

void cobra_window_draw_line(cobra_window *win, int x0, int y0, int x1, int y1, uint32_t color)
{
  // Algoritmo di Bresenham serve a tracciare una linea retta 
  // tra due punti su una griglia di pixel (rasterizzazione).
  // Calcoliamo le differenze assolute (delta) tra le coordinate
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);

  // Determiniamo la direzione del passo (+1 o -1) per ogni asse
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;

  // Variabile di decisione scalata per 2 (2 * (dx - dy))
  // Scaliamo tutto per 2 all'inizio per evitare la moltiplicazione dentro il loop
  int dy2 = 2 * dy;
  int dx2 = 2 * dx;
  int decision = dx2 - dy2; //2 * (dx - dy);


  while (true)
  {
    // Disegniamo il pixel corrente
    cobra_window_put_pixel(win, x0, y0, color);

    // Se abbiamo raggiunto il punto finale, usciamo dal loop
    if (x0 == x1 && y0 == y1) break;

    // Salviamo il valore corrente della decisione (snapshot)
    // Non serve più moltiplicare per 2 perché 'decision' è già scalato
    int decision_curr = decision;

    // Se la decisione accumulata suggerisce un movimento orizzontale
    if (decision_curr > -dy) {
      decision -= dy2;
      x0 += sx;
    }

    // Se la decisione accumulata suggerisce un movimento verticale
    if (decision_curr < dx) {
      decision += dx2;
      y0 += sy;
    }
  }
}