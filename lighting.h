/* lighting.h - Motor de iluminacion ambiental.
 *  - Dentro del aura (70-90 px) la luz cae suavemente desde el centro.
 *  - Fuera hay PENUMBRA DENSA PERO VISIBLE (25%), y mas lejos 12%:
 *    se intuyen los pasillos grotescos y las siluetas de los monstruos.
 * Todo el calculo es entero para ir fluido a 60 FPS en PSP. */
#ifndef LIGHTING_H
#define LIGHTING_H

#include "render.h"

class Lighting {
public:
    int base_radius;          /* 70..90 segun dificultad */

    Lighting() : base_radius(85) {}
    void set_radius(int r) { base_radius = r; }

    /* Radio efectivo: parpadeo lento + tirones aleatorios + miedo por poca vida */
    int radius(int frame, int hp, int maxhp) const {
        int r = base_radius + (int)(3.0f * sinf(frame * 0.09f));
        if ((hash2(frame / 5, 11) & 7) == 0) r -= 4;
        if (maxhp > 0 && hp * 100 / maxhp < 30) r -= 6 + (int)(3.0f * sinf(frame * 0.3f));
        return r < 40 ? 40 : r;
    }

    /* Aplica la luz sobre el cuadro ya dibujado, centrada en (cx,cy) */
    void apply(int cx, int cy, int R) const {
        const int r2 = R * R;
        const int c2 = (R * 45 / 100) * (R * 45 / 100);   /* nucleo a pleno brillo */
        const int far_r = R * 5 / 2, far2 = far_r * far_r;
        for (int y = 0; y < SCR_H; y++) {
            int dy = y - cy, dy2 = dy * dy;
            pix *row = &g_fb[y * BUF_W];
            if (dy2 >= far2) {                             /* fila lejana: 12% */
                for (int x = 0; x < SCR_W; x++) row[x] = dim3(row[x]);
                continue;
            }
            for (int x = 0; x < SCR_W; x++) {
                int dx = x - cx, d2 = dx * dx + dy2;
                if (d2 >= far2) row[x] = dim3(row[x]);
                else if (d2 >= r2) row[x] = dim2(row[x]);  /* penumbra 25% */
                else if (d2 > c2) row[x] = shade(row[x], 32 - (d2 - c2) * 24 / (r2 - c2));
            }
        }
    }

private:
    static inline pix dim2(pix c) { return (pix)(0x8000 | ((c >> 2) & 0x1CE7)); }   /* x 1/4 */
    static inline pix dim3(pix c) { return (pix)(0x8000 | ((c >> 3) & 0x0C63)); }   /* x 1/8 */
};

#endif
