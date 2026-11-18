/* lighting.h - Motor de iluminación ambiental.
 *  - Dentro del aura (70-90 px) la luz cae suavemente desde el centro.
 *  - Fuera hay PENUMBRA DENSA PERO VISIBLE (25%), y más lejos 12%.
 *  - La batería de la linterna reduce el radio; la hora del día lo ajusta.
 *  - tint(): colorea el ambiente según la franja (Tarde cálida, Noche azulada). */
#ifndef LIGHTING_H
#define LIGHTING_H

#include "render.h"

class Lighting {
public:
    int base_radius;
    Lighting() : base_radius(85) {}
    void set_radius(int r) { base_radius = r; }

    int radius(int frame, int hp, int maxhp, int battery, int slot) const {
        int r = base_radius * (55 + 45 * battery / 100) / 100;      /* linterna baja = aura corta */
        if (slot == 2) r = r * 92 / 100;
        r += (int)(3.0f * sinf(frame * 0.09f));
        if ((hash2(frame / 5, 11) & 7) == 0) r -= 4;
        if (maxhp > 0 && hp * 100 / maxhp < 30) r -= 6 + (int)(3.0f * sinf(frame * 0.3f));
        return r < 36 ? 36 : r;
    }
    void apply(int cx, int cy, int R) const {
        const int r2 = R * R;
        const int c2 = (R * 45 / 100) * (R * 45 / 100);
        const int far_r = R * 5 / 2, far2 = far_r * far_r;
        for (int y = 0; y < SCR_H; y++) {
            int dy = y - cy, dy2 = dy * dy;
            pix *row = &g_fb[y * BUF_W];
            if (dy2 >= far2) { for (int x = 0; x < SCR_W; x++) row[x] = dim3(row[x]); continue; }
            for (int x = 0; x < SCR_W; x++) {
                int dx = x - cx, d2 = dx * dx + dy2;
                if (d2 >= far2) row[x] = dim3(row[x]);
                else if (d2 >= r2) row[x] = dim2(row[x]);
                else if (d2 > c2) row[x] = shade(row[x], 32 - (d2 - c2) * 24 / (r2 - c2));
            }
        }
    }
    /* mode 1 = tarde cálida, 2 = noche azulada */
    static void tint(int mode) {
        if (mode == 0) return;
        for (int y = 0; y < SCR_H; y++) {
            pix *row = &g_fb[y * BUF_W];
            for (int x = 0; x < SCR_W; x++) {
                pix c = row[x];
                int r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
                if (mode == 1) { g = g * 27 / 32; b = b * 19 / 32; }
                else { r = r * 18 / 32; g = g * 21 / 32; b = b * 28 / 32; }
                row[x] = (pix)(0x8000 | (b << 10) | (g << 5) | r);
            }
        }
    }
private:
    static inline pix dim2(pix c) { return (pix)(0x8000 | ((c >> 2) & 0x1CE7)); }
    static inline pix dim3(pix c) { return (pix)(0x8000 | ((c >> 3) & 0x0C63)); }
};

#endif
