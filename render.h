/* render.h - Render por software sobre framebuffer de 16 bits (RGBA 5551)
 * Resolucion PSP: 480x272, stride 512. Todo se dibuja en RAM (g_fb) y
 * la capa de plataforma lo copia a la VRAM cada cuadro. */
#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>
#include <string.h>
#include <math.h>

#define SCR_W 480
#define SCR_H 272
#define BUF_W 512

typedef uint16_t pix;

/* Color constante (5551 PSP: bits 0-4 R, 5-9 G, 10-14 B, 15 alfa) */
#define RGB5(r,g,b) ((pix)(0x8000 | (((b)>>3)<<10) | (((g)>>3)<<5) | ((r)>>3)))
#define BLACK 0x8000

static pix g_fb[BUF_W * SCR_H] __attribute__((aligned(64)));

/* Color en tiempo de ejecucion con recorte 0..255 */
static inline pix rgb(int r, int g, int b) {
    if (r < 0) r = 0; else if (r > 255) r = 255;
    if (g < 0) g = 0; else if (g > 255) g = 255;
    if (b < 0) b = 0; else if (b > 255) b = 255;
    return (pix)(0x8000 | ((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3));
}

/* ---------- Aleatorio rapido (xorshift) y hash ---------- */
static uint32_t g_rng = 0x1234567u;
static inline uint32_t rnd(void) {
    g_rng ^= g_rng << 13; g_rng ^= g_rng >> 17; g_rng ^= g_rng << 5;
    return g_rng;
}
static inline int rndr(int n) { return (int)(rnd() % (uint32_t)n); }
static inline uint32_t hash2(int x, int y) {
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

/* ---------- Primitivas ---------- */
static inline void gfx_px(int x, int y, pix c) {
    if ((unsigned)x < SCR_W && (unsigned)y < SCR_H) g_fb[y * BUF_W + x] = c;
}
static void gfx_clear(pix c) {
    for (int i = 0; i < BUF_W * SCR_H; i++) g_fb[i] = c;
}
static void gfx_rect(int x, int y, int w, int h, pix c) {
    int x1 = x + w, y1 = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 > SCR_W) x1 = SCR_W;
    if (y1 > SCR_H) y1 = SCR_H;
    for (int j = y; j < y1; j++) {
        pix *p = &g_fb[j * BUF_W + x];
        for (int i = x; i < x1; i++) *p++ = c;
    }
}
static void gfx_frame(int x, int y, int w, int h, pix c) {
    gfx_rect(x, y, w, 1, c); gfx_rect(x, y + h - 1, w, 1, c);
    gfx_rect(x, y, 1, h, c); gfx_rect(x + w - 1, y, 1, h, c);
}
static void gfx_ellipse(int cx, int cy, int rx, int ry, pix c) {
    if (rx <= 0 || ry <= 0) return;
    for (int dy = -ry; dy <= ry; dy++) {
        float k = (float)dy / (float)ry;
        int hw = (int)(rx * sqrtf(1.0f - k * k));
        gfx_rect(cx - hw, cy + dy, hw * 2 + 1, 1, c);
    }
}
static void gfx_disc(int cx, int cy, int r, pix c) { gfx_ellipse(cx, cy, r, r, c); }
static void gfx_ring(int cx, int cy, int r, pix c) {
    int steps = r * 7 + 8;
    for (int i = 0; i < steps; i++) {
        float a = i * 6.2831853f / (float)steps;
        gfx_px(cx + (int)floorf(cosf(a) * r + 0.5f), cy + (int)floorf(sinf(a) * r + 0.5f), c);
    }
}

/* ---------- Efectos de color ---------- */
/* Oscurece un pixel: f = 0..32 (32 = sin cambio) */
static inline pix shade(pix c, int f) {
    int r = (c & 31) * f >> 5, g = ((c >> 5) & 31) * f >> 5, b = ((c >> 10) & 31) * f >> 5;
    return (pix)(0x8000 | (b << 10) | (g << 5) | r);
}
static void gfx_fade(int f) {            /* oscurece toda la pantalla */
    if (f >= 32) return;
    for (int y = 0; y < SCR_H; y++)
        for (int x = 0; x < SCR_W; x++) {
            pix *p = &g_fb[y * BUF_W + x]; *p = shade(*p, f < 0 ? 0 : f);
        }
}
static void gfx_add(int a) {             /* destello (trueno): suma luz */
    if (a <= 0) return;
    for (int y = 0; y < SCR_H; y++)
        for (int x = 0; x < SCR_W; x++) {
            pix *p = &g_fb[y * BUF_W + x];
            int r = (*p & 31) + a, g = ((*p >> 5) & 31) + a, b = ((*p >> 10) & 31) + a;
            if (r > 31) r = 31;
            if (g > 31) g = 31;
            if (b > 31) b = 31;
            *p = (pix)(0x8000 | (b << 10) | (g << 5) | r);
        }
}

/* Aura de luz: todo fuera del radio R queda negro absoluto; dentro se
 * atenua con la distancia. Solo recorre la caja (R+B) alrededor del jugador. */
static void gfx_light(int cx, int cy, int R, int B) {
    int r2 = R * R;
    for (int y = cy - B; y <= cy + B; y++) {
        if (y < 0 || y >= SCR_H) continue;
        for (int x = cx - B; x <= cx + B; x++) {
            if (x < 0 || x >= SCR_W) continue;
            int dx = x - cx, dy = y - cy, d2 = dx * dx + dy * dy;
            pix *p = &g_fb[y * BUF_W + x];
            if (d2 >= r2) { *p = BLACK; continue; }
            int f = (r2 - d2) * 40 / r2;
            if (f > 32) f = 32;
            *p = shade(*p, f);
        }
    }
}

/* ---------- Texto bitmap (fuente 8x8 MSX incluida en libpspdebug) ---------- */
extern unsigned char msx[];

static void gfx_char(int x, int y, int ch, pix c, int s) {
    const unsigned char *g = &msx[(ch & 255) * 8];
    for (int j = 0; j < 8; j++)
        for (int i = 0; i < 8; i++)
            if (g[j] & (0x80 >> i)) gfx_rect(x + i * s, y + j * s, s, s, c);
}
static void gfx_text_n(int x, int y, const char *s, int n, pix c, int sc) {
    int cx = x, cy = y;
    for (; *s && n > 0; s++, n--) {
        if (*s == '\n') { cx = x; cy += 10 * sc; continue; }
        gfx_char(cx, cy, (unsigned char)*s, c, sc);
        cx += 8 * sc;
    }
}
static void gfx_text(int x, int y, const char *s, pix c, int sc) { gfx_text_n(x, y, s, 1 << 30, c, sc); }
static int text_w(const char *s) {       /* ancho de la linea mas larga a escala 1 */
    int best = 0, cur = 0;
    for (; *s; s++) {
        if (*s == '\n') { if (cur > best) best = cur; cur = 0; } else cur++;
    }
    if (cur > best) best = cur;
    return best * 8;
}
static void gfx_text_c(int y, const char *s, pix c, int sc) {
    gfx_text((SCR_W - text_w(s) * sc) / 2, y, s, c, sc);
}
static void gfx_text_sh(int x, int y, const char *s, pix c, int sc) {
    gfx_text(x + sc, y + sc, s, BLACK, sc);
    gfx_text(x, y, s, c, sc);
}

/* ---------- Sprites 8x8: filas de texto; '1'..'9' = indice de paleta ---------- */
static void gfx_sprite(const char *const *rows, const pix *pal, int x, int y, int s, int flip) {
    for (int j = 0; j < 8; j++)
        for (int i = 0; i < 8; i++) {
            char ch = rows[j][flip ? 7 - i : i];
            if (ch >= '1' && ch <= '9') gfx_rect(x + i * s, y + j * s, s, s, pal[ch - '1']);
        }
}

#endif
