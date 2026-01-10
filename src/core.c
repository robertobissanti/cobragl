#include "cobragl/core.h"
#include "cobragl/math.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

bool cobra_window_create(cobra_window *win, int width, int height, const char *title)
{
  if (!win)
    return false;

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

  // Abilitiamo il VSync per sincronizzare il rendering con il refresh rate del monitor (es. 60Hz).
  // Questo elimina il "tearing" (linee spezzate) e gli artefatti visivi durante il movimento.
  SDL_SetRenderVSync(win->sdl_renderer, 1);

  // Impostiamo il colore di pulizia del renderer a Nero Opaco per sicurezza
  SDL_SetRenderDrawColor(win->sdl_renderer, 0, 0, 0, 255);

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

  // Disabilitiamo il blending della texture SDL.
  // Poiché facciamo il blending manuale nel buffer software e scriviamo pixel opachi (Alpha 255),
  // vogliamo che la texture sovrascriva completamente il contenuto della finestra (Copy, non Blend).
  SDL_SetTextureBlendMode(win->color_buffer_texture, SDL_BLENDMODE_NONE);

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
  if (!win)
    return;

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
  if (!win)
    return;

  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    if (event.type == SDL_EVENT_QUIT)
    {
      win->should_close = true;
    }
  }
}

void cobra_window_draw_line_3d(cobra_window *win, cobra_vec3 p1, cobra_vec3 p2, 
                               float fov, float thickness, uint32_t color, bool aa, bool use_ss) {
    if (!win) return;
    
    // Piano vicino (Near Plane). 
    // Aumentato a 0.5f per evitare coordinate proiettate troppo grandi che causano artefatti.
    float near_plane = 0.5f; 

    // 1. Trivial Reject: Entrambi i punti sono dietro la camera
    if (p1.z < near_plane && p2.z < near_plane) return;

    // 2. Clipping 3D: Se un punto è dietro, accorciamo la linea fino al near plane
    if (p1.z < near_plane) {
        float t = (near_plane - p1.z) / (p2.z - p1.z);
        p1.x = p1.x + (p2.x - p1.x) * t;
        p1.y = p1.y + (p2.y - p1.y) * t;
        p1.z = near_plane;
    } else if (p2.z < near_plane) {
        float t = (near_plane - p2.z) / (p1.z - p2.z);
        p2.x = p2.x + (p1.x - p2.x) * t;
        p2.y = p2.y + (p1.y - p2.y) * t;
        p2.z = near_plane;
    }

    // 3. Proiezione (ora sicura perché z >= near_plane)
    cobra_vec3 proj1 = cobra_vec3_project(p1, fov, (float)win->width, (float)win->height);
    cobra_vec3 proj2 = cobra_vec3_project(p2, fov, (float)win->width, (float)win->height);

    // 4. Disegno 2D (con clipping schermo automatico)
    if (aa) {
        cobra_window_draw_line_aa(win, proj1.x, proj1.y, proj2.x, proj2.y, thickness, color, use_ss);
    } else {
        cobra_window_draw_line(win, (int)proj1.x, (int)proj1.y, (int)proj2.x, (int)proj2.y, color);
    }
}

void cobra_window_clear(cobra_window *win, uint32_t color)
{
  if (!win)
    return;

  // Puliamo il color buffer
  for (int i = 0; i < win->width * win->height; i++)
  {
    win->color_buffer[i] = color;
    win->z_buffer[i] = 1.0f; // Inizializziamo Z a 1.0 (profondità massima)
  }
}

void cobra_window_present(cobra_window *win)
{
  if (!win)
    return;

  // Aggiorniamo la texture con i dati del buffer
  SDL_UpdateTexture(
      win->color_buffer_texture,
      NULL,
      win->color_buffer,
      (int)(win->width * sizeof(uint32_t)));

  // Puliamo il renderer SDL (Backbuffer) prima di disegnare la texture
  // Questo rimuove qualsiasi residuo del frame precedente dal buffer della GPU
  SDL_RenderClear(win->sdl_renderer);

  // Copiamo la texture sul renderer
  SDL_RenderTexture(win->sdl_renderer, win->color_buffer_texture, NULL, NULL);
  SDL_RenderPresent(win->sdl_renderer);
}

void cobra_window_draw_point(cobra_window *win, int x, int y, uint32_t color)
{
  if (!win)
    return;

  if (x >= 0 && x < win->width && y >= 0 && y < win->height)
  {
    win->color_buffer[(y * win->width) + x] = color;
  }
}

// Helper interno per il blending Alpha
// Fonde il colore 'color' con alpha 'alpha' (0.0-1.0). Esposta pubblicamente.
void cobra_window_draw_point_aa(cobra_window *win, int x, int y, uint32_t color, float alpha)
{
  if (!win || x < 0 || x >= win->width || y < 0 || y >= win->height)
    return;

  if (alpha <= 0.0f) return;
  
  // Se alpha è pieno, sovrascriviamo (più veloce)
  if (alpha >= 1.0f) {
    win->color_buffer[y * win->width + x] = color;
    return;
  }

  uint32_t bg = win->color_buffer[y * win->width + x];

  // Estrazione canali (ARGB)
  int r = (color >> 16) & 0xFF;
  int g = (color >> 8) & 0xFF;
  int b = color & 0xFF;

  int bg_r = (bg >> 16) & 0xFF;
  int bg_g = (bg >> 8) & 0xFF;
  int bg_b = bg & 0xFF;

  // Blending lineare float
  float inv_alpha = 1.0f - alpha;
  // Aggiungiamo +0.5f per arrotondamento corretto (round-to-nearest) invece di troncamento
  int out_r = (int)(r * alpha + bg_r * inv_alpha + 0.5f);
  int out_g = (int)(g * alpha + bg_g * inv_alpha + 0.5f);
  int out_b = (int)(b * alpha + bg_b * inv_alpha + 0.5f);

  // Ricomponiamo (Alpha 255 fisso per il buffer finale)
  win->color_buffer[y * win->width + x] = (0xFF << 24) | (out_r << 16) | (out_g << 8) | out_b;
}

// --- COHEN-SUTHERLAND CLIPPING ALGORITHM ---
// Definizioni dei codici di regione (Bitmask)
#define CS_INSIDE 0  // 0000
#define CS_LEFT   1  // 0001
#define CS_RIGHT  2  // 0010
#define CS_BOTTOM 4  // 0100
#define CS_TOP    8  // 1000

// Calcola il codice di regione - versione branchless ottimizzata
static inline int compute_outcode(int x, int y, int min_x, int min_y, int max_x, int max_y) {
    return ((x < min_x) << 0) |      // CS_LEFT
           ((x >= max_x) << 1) |      // CS_RIGHT
           ((y < min_y) << 2) |       // CS_BOTTOM
           ((y >= max_y) << 3);       // CS_TOP
}

// Helper per il clipping su un bordo specifico (Integer)
static inline void clip_to_edge(int *x, int *y, int x0, int y0, int x1, int y1, 
                                 int edge_bit, int edge_coord) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    
    // edge_bit: LEFT=1, RIGHT=2, BOTTOM=4, TOP=8
    // Verticale se bit 0 o 1 è settato (1 o 2)
    int is_vertical = edge_bit & 3; 
    
    if (is_vertical) {
        *x = edge_coord;
        *y = dy ? (y0 + (int)((double)dy * (edge_coord - x0) / dx)) : y0;
    } else {
        *y = edge_coord;
        *x = dx ? (x0 + (int)((double)dx * (edge_coord - y0) / dy)) : x0;
    }
}

// Algoritmo di Clipping - versione compatta (int)
static bool cohen_sutherland_clip(int *x0, int *y0, int *x1, int *y1, 
                                  int min_x, int min_y, int max_x, int max_y) {
    int outcode0 = compute_outcode(*x0, *y0, min_x, min_y, max_x, max_y);
    int outcode1 = compute_outcode(*x1, *y1, min_x, min_y, max_x, max_y);

    // Massimo 4 iterazioni (una per ogni bordo)
    for (int iter = 0; iter < 4; iter++) {
        // Trivial Accept: entrambi dentro
        if (!(outcode0 | outcode1)) {
            return true;
        } 
        // Trivial Reject: entrambi fuori dalla stessa parte
        if (outcode0 & outcode1) {
            return false;
        }

        // Seleziona il punto fuori e il bit di bordo da clippare
        int code_out = outcode0 ? outcode0 : outcode1;
        int bit = code_out & -code_out;  // Isola il bit meno significativo (LSB)
        
        // Tabella di coordinate dei bordi
        int edges[4] = {min_x, max_x - 1, min_y, max_y - 1};
        int edge_idx = __builtin_ctz(bit);  // Count Trailing Zeros (log2 del bit)
        int edge_coord = edges[edge_idx];
        
        // Calcola intersezione
        int x, y;
        clip_to_edge(&x, &y, *x0, *y0, *x1, *y1, bit, edge_coord);
        
        // Aggiorna punto e codice
        int *px = (code_out == outcode0) ? x0 : x1;
        int *py = (code_out == outcode0) ? y0 : y1;
        *px = x; *py = y;
        
        // Ricalcola codice solo per il punto modificato
        if (code_out == outcode0)
             outcode0 = compute_outcode(x, y, min_x, min_y, max_x, max_y);
        else
             outcode1 = compute_outcode(x, y, min_x, min_y, max_x, max_y);
    }
    return true;
}

void cobra_window_draw_line(cobra_window *win, int x0, int y0, int x1, int y1, uint32_t color)
{
  if (!win)
    return;

  // Applichiamo il clipping geometrico.
  // Usiamo 0,0,w,h per clipping esatto, la funzione ora supporta "Guard Bands" (es. -10, -10, w+10, h+10)
  if (!cohen_sutherland_clip(&x0, &y0, &x1, &y1, 0, 0, win->width, win->height)) {
      return; // Linea completamente fuori
  }

  // Algoritmo di Bresenham super-compatto
  // Serve a tracciare una linea rettatra due punti su 
  // una griglia di pixel (rasterizzazione).

  // Retta in forma implicita (segmento orientato)
  // f(x,y) = (x-x0)*(y1-y0) - (y-y0)*(x1-x0)
  //
  // Con le convenzioni:
  //   dx = |x1-x0| > 0
  //   dy = -|y1-y0| < 0
  // Calcoliamo le differenze assolute (delta) tra le coordinate
  int dx = abs(x1 - x0);
  int dy = -abs(y1 - y0);

  //   sx = sign(x1-x0)
  //   sy = sign(y1-y0)
  // Determiniamo la direzione del passo (+1 o -1) per ogni asse
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;

  // valgono le identità:
  //   x1-x0 = sx*dx
  //   y1-y0 = -sy*dy
  //
  // quindi:
  //   f(x,y) = - [ (x-x0)*sy*dy + (y-y0)*sx*dx ]
  //
  // Moltiplicare f per -1 NON cambia lo zero-set (f=0), ma inverte il segno.
  // Questo è utile perché possiamo scegliere una funzione equivalente e poi
  // usare disuguaglianze coerenti (>= / <=) nei test.
  // Introduciamo quindi:
  // La variabile di stato 'decision' è una trasformazione affine di f:
  //   S(x,y) = -2*sx*sy*f(x,y) + 2*dx + 2*dy
  //
  // Nel loop, 'decision' è mantenuta incrementalmente: non si ricalcola da zero.
  // È una forma affine del valore f_i = E(f_i,f_i) al pixel corrente.
  // Questa scelta non è arbitraria:
  // - preserva i segni rilevanti per i test geometrici
  // - consente aggiornamenti incrementali costanti (+2*dx, +2*dy)
  // - rende le soglie di decisione indipendenti dai quadranti
  //
  // Valore iniziale:
  //   S_0 = -2*sx*sy*f(x0,y0) + 2*dx + 2*dy = 2*dx + 2*dy

  // dx2 e dy2 sono variabili ausiliari in modo da operare meno 
  // calcoli in loop
  int dx2 = 2 * dx;
  int dy2 = 2 * dy; 
  int S = dx2 + dy2; // decision_0

  while (true)
  {
    // Disegniamo il pixel corrente
    cobra_window_draw_point(win, x0, y0, color);

    // Se abbiamo raggiunto il punto finale, usciamo dal loop
    if (x0 == x1 && y0 == y1)
      break;


    // mi sposto in X se S>=dy che corrisponde a porre sx*sy*f(W) <= 0
    // con W = (x_i + 1/2*sx, y_i + 1*sy)
    int condX = (S >= dy); 
    // mi sposto in Y se S<=dy che corrisponde a porre sx*sy*f(Q) >= 0
    // con Q = (x_i + 1*sx, y_i + 1/2*sy)
    int condY = (S <= dx); 

    x0 += condX * sx; // aggiorniamo il valore di x0 in base a condX
    y0 += condY * sy; // aggiorniamo il valore di y0 in base a condY
    S += condX * dy2 + condY * dx2; // aggiorniamo decision
  }
}

// Versione float di compute_outcode branchless
static inline int compute_outcode_f(float x, float y, float min_x, float min_y, float max_x, float max_y) {
    return ((x < min_x) << 0) |
           ((x >= max_x) << 1) |
           ((y < min_y) << 2) |
           ((y >= max_y) << 3);
}

// Helper per il clipping su un bordo specifico (Float)
static inline void clip_to_edge_f(float *x, float *y, float x0, float y0, 
                                   float x1, float y1, int edge_bit, 
                                   float edge_coord) {
    float dx = x1 - x0;
    float dy = y1 - y0;
    int is_vertical = edge_bit & 3;
    
    if (is_vertical) {
        *x = edge_coord;
        *y = (fabsf(dx) > 1e-6f) ? (y0 + dy * (edge_coord - x0) / dx) : y0;
    } else {
        *y = edge_coord;
        *x = (fabsf(dy) > 1e-6f) ? (x0 + dx * (edge_coord - y0) / dy) : x0;
    }
}

// Versione float di Cohen-Sutherland per l'Anti-Aliasing (Compact)
static bool cohen_sutherland_clip_f(float *x0, float *y0, float *x1, float *y1, 
                                    float min_x, float min_y, float max_x, float max_y) {
    int outcode0 = compute_outcode_f(*x0, *y0, min_x, min_y, max_x, max_y);
    int outcode1 = compute_outcode_f(*x1, *y1, min_x, min_y, max_x, max_y);

    for (int iter = 0; iter < 4; iter++) {
        if (!(outcode0 | outcode1)) {
            return true;
        }
        if (outcode0 & outcode1) {
            return false;
        }

        int code_out = outcode0 ? outcode0 : outcode1;
        int bit = code_out & -code_out;
        
        float edges[4] = {min_x, max_x, min_y, max_y};
        int edge_idx = __builtin_ctz(bit);
        float edge_coord = edges[edge_idx];
        
        float x, y;
        clip_to_edge_f(&x, &y, *x0, *y0, *x1, *y1, bit, edge_coord);
        
        float *px = (code_out == outcode0) ? x0 : x1;
        float *py = (code_out == outcode0) ? y0 : y1;
        *px = x; *py = y;
        
        if (code_out == outcode0)
            outcode0 = compute_outcode_f(x, y, min_x, min_y, max_x, max_y);
        else
            outcode1 = compute_outcode_f(x, y, min_x, min_y, max_x, max_y);
    }
    return true;
}

void cobra_window_draw_line_aa(cobra_window *win, float x0, float y0, float x1, float y1, float width, uint32_t color, bool use_ss)
{
  if (!win) return;

  // --- GUARD BAND CLIPPING ---
  // Usiamo una "Guard Band" (cornice di sicurezza) attorno allo schermo.
  // Questo serve a:
  // 1. Tagliare linee enormi (ottimizzazione)
  // 2. Mantenere i "Caps" (punte arrotondate) fuori dalla vista, così non si vedono artefatti di taglio.
  // Il margine deve essere almeno pari al raggio della linea + un extra.
  float gb_margin = width * 0.5f + 2.0f;
  
  if (!cohen_sutherland_clip_f(&x0, &y0, &x1, &y1, 
                               -gb_margin, -gb_margin, 
                               (float)win->width + gb_margin, (float)win->height + gb_margin)) {
      return; // Linea completamente fuori dalla Guard Band
  }

  // Ricalcoliamo i delta dopo il clipping (la geometria è cambiata)
  float fdx = x1 - x0;
  float fdy = y1 - y0;
  float len_sq = fdx*fdx + fdy*fdy;
  if (len_sq <= 0.0f) return;

  float len = sqrtf(len_sq);
  float inv_len = 1.0f / len;
  float inv_len_sq = 1.0f / len_sq;

  // Gestione linee sottili (< 1.0px)
  // Se la linea è sub-pixel, blocchiamo la geometria a 1.0px per garantire continuità
  // (evita buchi) ma riduciamo l'opacità globale per simulare lo spessore.
  float alpha_master = 1.0f;
  // Applichiamo il fix thin-line solo in modalità SDF (il SS puro dovrebbe campionare la geometria reale)
  if (!use_ss && width < 1.0f) {
      // Usiamo sqrtf(width) invece di width lineare per dare un "boost" di visibilità
      // alle linee sottili (gamma correction percettiva), altrimenti sembrerebbero troppo tenui.
      alpha_master = sqrtf(width);
      width = 1.0f;
  }

  float radius = width * 0.5f;
  
  // Estendiamo il segmento per coprire i "Round Caps"
  // Calcoliamo il versore e estendiamo di 'radius' + margine
  float ext = radius + 2.0f;
  float ux = fdx * inv_len;
  float uy = fdy * inv_len;

  // Calcoliamo i nuovi estremi interi per il loop di Bresenham (Extended Bresenham)
  int ix0 = (int)roundf(x0 - ux * ext);
  int iy0 = (int)roundf(y0 - uy * ext);
  int ix1 = (int)roundf(x1 + ux * ext);
  int iy1 = (int)roundf(y1 + uy * ext);

  // Setup Bresenham sui punti estesi
  int dx = abs(ix1 - ix0);
  int dy = -abs(iy1 - iy0);
  int sx = (ix0 < ix1) ? 1 : -1;
  int sy = (iy0 < iy1) ? 1 : -1;
  int dx2 = 2*dx;
  int dy2 = 2*dy;
  int S = dx2 + dy2;

  // Span perpendicolare: deve coprire lo spessore proiettato
  // Aumentiamo ulteriormente il margine (+3.0f) per garantire la copertura
  // anche in casi limite di pendenza e coordinate sub-pixel.
  int span_r = (int)ceilf(radius * 1.41421356f + 3.0f);

  // --- PRE-CALCOLO GRADIENTI E SOGLIE ---

  // Gradienti: quanto cambiano t (lungo la linea) e d (perpendicolare) per ogni passo di 1px in X e Y
  float dt_dx = fdx * inv_len_sq;
  float dt_dy = fdy * inv_len_sq;
  float dd_dx = -fdy * inv_len;
  float dd_dy =  fdx * inv_len;

  // Pre-calcolo gradienti orientati per il loop esterno (evita moltiplicazioni con sx/sy nel loop)
  float dt_dx_sx = dt_dx * sx;
  float dt_dy_sy = dt_dy * sy;
  float dd_dx_sx = dd_dx * sx;
  float dd_dy_sy = dd_dy * sy;

  // Soglie per AA al quadrato (per evitare sqrtf)
  // r_in: raggio interno (alpha=1), r_out: raggio esterno (alpha=0)
  float r_in = radius - 0.5f;
  float r_out = radius + 0.5f;
  
  // Gestione corretta raggio interno per linee sottili
  float r_in_eff = (r_in > 0.0f) ? r_in : 0.0f;
  float r_in_sq  = r_in_eff * r_in_eff;
  // Epsilon per evitare buchi numerici
  float eps = 1e-3f;
  float r_out_sq = (r_out + eps) * (r_out + eps);

  // Setup Span Direction
  bool is_x_major = (fabsf(fdx) >= fabsf(fdy));
  int span_sx = !is_x_major;
  int span_sy = is_x_major;

  // Gradienti specifici per lo span (loop interno)
  // Invece di moltiplicare k * gradiente ogni volta, sommiamo questo valore.
  // Pre-calcolo gradienti span per loop "veramente incrementale"
  float dt_span = span_sx * dt_dx + span_sy * dt_dy;
  float dd_span = span_sx * dd_dx + span_sy * dd_dy;

  // Stato iniziale di t e d al punto griglia (ix0, iy0)
  // Calcolato una volta sola rispetto all'origine reale (x0, y0)
  float start_dx = (ix0 + 0.5f) - x0;
  float start_dy = (iy0 + 0.5f) - y0;
  float t_current = start_dx * dt_dx + start_dy * dt_dy;
  float d_current = start_dx * dd_dx + start_dy * dd_dy;

  int step_count = 0;

  while (true) {
    // --- LOOP INTERNO INCREMENTALE ---
    
    // Calcoliamo t e d per l'inizio dello span (k = -span_r)
    // Nota: usiamo moltiplicazione qui solo per settare l'inizio riga, poi solo somme.
    float t_iter = t_current - span_r * dt_span;
    float d_iter = d_current - span_r * dd_span;

    int px = ix0 - span_r * span_sx;
    int py = iy0 - span_r * span_sy;

    for (int k = -span_r; k <= span_r; k++) {
      
      // Bounds Check anticipato: se siamo fuori schermo, saltiamo i calcoli pesanti
      // ma aggiorniamo comunque gli iteratori per non rompere il loop incrementale.
      if (px < 0 || px >= win->width || py < 0 || py >= win->height) {
          t_iter += dt_span;
          d_iter += dd_span;
          px += span_sx;
          py += span_sy;
          continue;
      }

      if (use_ss) {
        // --- SUPERSAMPLING RGSS (Rotated Grid Supersampling) ---
        // 4 campioni ottimizzati invece di 16, qualità comparabile ma molto più veloce.
        static const float rgss[4][2] = {
            {-0.375f, -0.125f}, {0.125f, -0.375f},
            {-0.125f,  0.375f}, {0.375f,  0.125f}
        };

        int hits = 0;
        for (int i = 0; i < 4; i++) {
            float ox = rgss[i][0];
            float oy = rgss[i][1];
                
            // Calcoliamo t e d per il subpixel
            float t_sub = t_iter + ox * dt_dx + oy * dt_dy;
            float d_sub = d_iter + ox * dd_dx + oy * dd_dy;
                
            float tc = t_sub;
            if (tc < 0.0f) tc = 0.0f; else if (tc > 1.0f) tc = 1.0f;
            float dtc = t_sub - tc;
            
            float dist_sq_sub = d_sub*d_sub + dtc*dtc*len_sq;
                
            if (dist_sq_sub <= radius*radius) {
                hits++;
            }
        }
        
        if (hits > 0) {
            cobra_window_draw_point_aa(win, px, py, color, hits * 0.25f); // hits / 4.0f
        }

      } else {
        // --- SDF ANALITICO (Fast & Smooth) ---

        // Calcoliamo la distanza geometrica al quadrato in un colpo solo.
        float tc = t_iter;
        if (tc < 0.0f) tc = 0.0f;
        else if (tc > 1.0f) tc = 1.0f;
        
        float dtc = t_iter - tc;
        float d2 = d_iter * d_iter;
        float dist_sq = d2 + (dtc * dtc) * len_sq;

        if (dist_sq < r_in_sq) {
          // Interno pieno (Core)
          cobra_window_draw_point_aa(win, px, py, color, 1.0f);
        } else if (dist_sq > r_out_sq) {
          // Esterno vuoto -> Skip
        } else {
          // Fascia AA
          float alpha = r_out - sqrtf(dist_sq);
          
          if (alpha <= 0.0f) alpha = 0.0f;
          else if (alpha > 1.0f) alpha = 1.0f;
          
          // Applicazione fattore di copertura per linee sottili (Thin-line mode)
          alpha *= alpha_master;

          cobra_window_draw_point_aa(win, px, py, color, alpha);
        }
      }

      // Avanzamento puramente incrementale (solo addizioni)
      t_iter += dt_span;
      d_iter += dd_span;
      px += span_sx;
      py += span_sy;
    }

    if (ix0 == ix1 && iy0 == iy1) break;

    // Bresenham Step
    int cx = (int) (S>=dy);
    int cy = (int) (S<=dx);
    
    ix0 += cx*sx;
    iy0 += cy*sy;
    S += cx*dy2+cy*dx2;

    // Aggiornamento t_current/d_current Branchless
    // Usiamo i gradienti pre-orientati: cx/cy sono 0 o 1, quindi selezionano l'incremento
    t_current += (float)cx * dt_dx_sx + (float)cy * dt_dy_sy;
    d_current += (float)cx * dd_dx_sx + (float)cy * dd_dy_sy;

    // Re-anchor ogni 64 passi per resettare errori di accumulo float
    if ((++step_count & 63) == 0) {
        float cur_dx = (ix0 + 0.5f) - x0;
        float cur_dy = (iy0 + 0.5f) - y0;
        t_current = cur_dx * dt_dx + cur_dy * dt_dy;
        d_current = cur_dx * dd_dx + cur_dy * dd_dy;
    }
  }
}
