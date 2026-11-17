/* assets.h - Sprites pixel art y arte procedural (dormitorio de Leo) */
#ifndef ASSETS_H
#define ASSETS_H

#include "render.h"

/* ---------- Leo (8x8) ---------- */
static const char *const SPR_LEO[8] = {
    "..2222..", ".222222.", ".211112.", "..1111..", ".333333.", "13333331", ".344443.", ".44..44." };
static const pix PAL_LEO[4] = { RGB5(200,150,120), RGB5(70,44,30), RGB5(60,70,110), RGB5(40,40,60) };

/* ---------- Resorte oxidado = Punto de Reflexion (8x8) ---------- */
static const char *const SPR_POINT[8] = {
    "...33...", "..3333..", ".333333.", "..3223..", ".1.22.1.", "..1111..", ".1....1.", "1......1" };
static const pix PAL_POINT[3] = { RGB5(110,80,60), RGB5(90,50,20), RGB5(255,150,40) };

/* ---------- Monstruos 16x16 (en mapa a escala 1, en combate a escala 4) ---------- */
static const char *const SPR_E0[16] = {   /* Boleta Reprobada */
    "...1111111111...", "..111111111111..", "..155555555551..", "..111111111111..",
    "..113311113311..", "..113311113311..", "..111111111111..", "..114444444411..",
    "..142424242421..", "..114444444411..", "..111111111111..", "..151515151515..",
    "..111111111111..", "..11.111111.11..", "..1..1.11.1..1..", "..2..2....2..2.." };
static const char *const SPR_E1[16] = {   /* Silla Vacia */
    "................", "....11111111....", "...1111111111...", "..111111111111..",
    "..111111111111..", "..111211112111..", "..111111111111..", "..112222222211..",
    "...1111111111...", "....11111111....", "..555555555555..", ".55555555555555.",
    ".55555555555555.", "..22.5555555.22.", "..22.........22.", "..22.........22." };
static const char *const SPR_E2[16] = {   /* Eco Paterno */
    ".....222222.....", "...2222222222...", "..222222222222..", "..223322223322..",
    "..223322223322..", "..222222222222..", "..244444444442..", "..243434343432..",
    "..244444444442..", "...2444444442...", "..222222222222..", ".22222222222222.",
    "2522222222222252", "2.2222222222222.", "..222......222..", "..222......222.." };
static const char *const SPR_E3[16] = {   /* Eco Materno */
    "....22222222....", "...2222222222...", "..222222222222..", "..222111111222..",
    "..221131131122..", "..221111111122..", "..221114411122..", "..221111111122..",
    "..22.111111.22..", "..222.1111.222..", ".22222.11.22222.", ".22225555552222.",
    "..225555555522..", "..225555555522..", "..225555555522..", "...2.2.22.2.2..." };
static const char *const SPR_E4[16] = {   /* Puerta Cerrada */
    "..222222222222..", "..211111111112..", "..211111111112..", "..211222222112..",
    "..211233332112..", "..211234432112..", "..211233332112..", "..211222222112..",
    "..211111111112..", "..211111115112..", "..211111111112..", "..211222222112..",
    "..211222222112..", "..211111111112..", "..222222222222..", ".22222222222222." };
static const char *const *const SPR_ENEMY[5] = { SPR_E0, SPR_E1, SPR_E2, SPR_E3, SPR_E4 };
static const pix PAL_ENEMY[5][5] = {
    { RGB5(190,180,160), RGB5(60,40,40),  RGB5(20,10,10),   RGB5(120,10,15), RGB5(170,30,30) },
    { RGB5(170,165,160), RGB5(50,40,50),  RGB5(20,20,20),   RGB5(20,20,20),  RGB5(70,60,80) },
    { RGB5(0,0,0),       RGB5(60,60,90),  RGB5(240,230,120),RGB5(90,0,10),   RGB5(120,40,40) },
    { RGB5(180,170,170), RGB5(30,24,36),  RGB5(10,5,10),    RGB5(110,20,30), RGB5(80,40,60) },
    { RGB5(110,70,45),   RGB5(50,30,20),  RGB5(230,220,200),RGB5(150,10,10),  RGB5(190,150,50) }
};

/* ---------- Posters (dormitorio) ---------- */
static const char *const SPR_ALIEN[8] = {
    "..1..1..", "...11...", "..1111..", ".11..11.", "11111111", "1.1111.1", "1.1..1.1", "...11..." };
static const char *const SPR_HEART[8] = {
    ".11..11.", "11111111", "11111111", "11111111", ".111111.", "..1111..", "...11...", "........" };

static void poster_band(int x, int y) {
    pix bone = RGB5(118,110,96), dark = RGB5(6,4,4);
    gfx_rect(x, y, 70, 90, RGB5(22,12,14)); gfx_rect(x + 2, y + 2, 66, 86, RGB5(30,16,18));
    gfx_rect(x + 19, y + 10, 32, 24, bone); gfx_rect(x + 23, y + 34, 24, 10, bone);
    gfx_rect(x + 23, y + 20, 8, 8, dark); gfx_rect(x + 39, y + 20, 8, 8, dark); gfx_rect(x + 33, y + 30, 4, 4, dark);
    for (int i = 0; i < 5; i++) gfx_rect(x + 25 + i * 4, y + 38, 1, 6, dark);
    gfx_text(x + 3, y + 54, "DEAD", RGB5(140,36,30), 2); gfx_text(x + 3, y + 71, "WIRE", RGB5(140,36,30), 2);
    gfx_rect(x, y + 47, 70, 1, RGB5(44,28,28));
    gfx_rect(x - 2, y - 2, 9, 5, RGB5(120,110,80)); gfx_rect(x + 63, y - 2, 9, 5, RGB5(120,110,80));
}
static void poster_alt(int x, int y) {
    gfx_rect(x, y, 52, 70, RGB5(20,24,44));
    for (int r = 22; r > 3; r -= 6) gfx_ring(x + 26, y + 26, r, ((r / 6) & 1) ? RGB5(90,50,110) : RGB5(60,30,80));
    gfx_disc(x + 26, y + 26, 3, RGB5(140,60,60));
    gfx_text(x + 2, y + 52, "VELVET", RGB5(140,140,170), 1); gfx_text(x + 2, y + 61, "STATIC", RGB5(100,100,130), 1);
    gfx_rect(x - 2, y - 2, 7, 4, RGB5(120,110,80));
}
static void poster_game(int x, int y, int k) {
    static const pix gpal[1] = { RGB5(60,150,70) }, hpal[1] = { RGB5(170,40,50) };
    gfx_rect(x, y, 44, 64, k ? RGB5(20,30,70) : RGB5(8,14,10));
    if (k == 0) {
        gfx_sprite(SPR_ALIEN, gpal, x + 10, y + 8, 3, 0);
        gfx_text(x + 2, y + 40, "SPACE", RGB5(150,150,120), 1); gfx_text(x + 6, y + 50, "RAID", RGB5(150,150,120), 1);
    } else {
        gfx_sprite(SPR_HEART, hpal, x + 10, y + 10, 3, 0);
        gfx_text(x + 2, y + 40, "PIXEL", RGB5(170,170,120), 1); gfx_text(x + 2, y + 50, "QUEST", RGB5(170,170,120), 1);
    }
    gfx_rect(x - 2, y - 2, 7, 4, RGB5(120,110,80));
}

/* ---------- Trueno ---------- */
static int lightning(int t) {
    static const int st[6] = { 90, 260, 430, 600, 760, 900 };
    int a = 0;
    for (int i = 0; i < 6; i++) {
        int d = t - st[i], v;
        if (d < 0 || d > 26) continue;
        if (d < 4) v = 10; else if (d < 8) v = 2; else if (d < 12) v = 12; else v = (26 - d) / 2;
        if (v > a) a = v;
    }
    return a;
}

/* ---------- Dormitorio de adolescente de los 2000 ---------- */
static void draw_room(int t, int fl) {
    for (int x = 0; x < SCR_W; x += 8)
        gfx_rect(x, 0, 8, 190, ((x / 8) & 1) ? RGB5(34,38,54) : RGB5(30,34,48));
    for (int y = 10; y < 185; y += 24)
        for (int x = 4; x < SCR_W; x += 24) gfx_rect(x, y, 2, 2, RGB5(46,50,70));
    gfx_rect(0, 188, SCR_W, 8, RGB5(22,22,30));
    for (int y = 196; y < SCR_H; y += 12) {
        gfx_rect(0, y, SCR_W, 12, ((y / 12) & 1) ? RGB5(54,38,28) : RGB5(48,34,24));
        gfx_rect(0, y, SCR_W, 1, RGB5(30,20,14));
        for (int x = ((y / 12) * 53) % 90; x < SCR_W; x += 90) gfx_rect(x, y, 1, 12, RGB5(30,20,14));
    }
    int wx = 366, wy = 24, ww = 90, wh = 108;
    pix frame = RGB5(74,66,58), sil = fl > 4 ? RGB5(40,44,70) : RGB5(8,10,18);
    gfx_rect(wx - 5, wy - 5, ww + 10, wh + 10, frame);
    gfx_rect(wx, wy, ww, wh, fl > 4 ? RGB5(150,160,210) : RGB5(14,20,36));
    gfx_rect(wx, wy + wh - 26, ww, 26, sil); gfx_rect(wx + 10, wy + wh - 40, 22, 14, sil); gfx_rect(wx + 52, wy + wh - 36, 18, 10, sil);
    for (int i = 0; i < 48; i++) {                      /* lluvia diagonal */
        int sx = (int)(hash2(i, 11) % ww), sy = (int)(hash2(i, 12) % wh);
        int y = (sy + t * 6 + i * 7) % wh, x = (((sx - y / 2) % ww) + ww) % ww;
        for (int k = 0; k < 5; k++) {
            int px = x - k / 2, py = y + k;
            if (px >= 0 && px < ww && py < wh) gfx_px(wx + px, wy + py, RGB5(96,120,170));
        }
    }
    gfx_rect(wx + ww / 2 - 1, wy, 3, wh, frame); gfx_rect(wx, wy + wh / 2 - 1, ww, 3, frame);
    gfx_rect(wx - 8, wy + wh + 5, ww + 16, 6, RGB5(84,76,66));
    gfx_rect(wx - 18, wy - 8, 12, wh + 34, RGB5(60,22,28)); gfx_rect(wx + ww + 6, wy - 8, 12, wh + 34, RGB5(60,22,28));
    poster_band(26, 20); poster_alt(108, 34); poster_game(170, 30, 0); poster_game(220, 38, 1);
    pix wood = RGB5(74,52,36);                           /* escritorio y monitor CRT apagado */
    gfx_rect(256, 148, 92, 6, wood);
    gfx_rect(258, 154, 6, 46, RGB5(58,40,28)); gfx_rect(340, 154, 6, 46, RGB5(58,40,28));
    gfx_rect(300, 154, 38, 32, RGB5(64,44,30)); gfx_rect(314, 168, 10, 3, RGB5(120,110,90));
    gfx_rect(280, 82, 40, 12, RGB5(70,66,58)); gfx_rect(268, 94, 66, 48, RGB5(88,84,74)); gfx_rect(268, 94, 66, 2, RGB5(110,106,96));
    gfx_rect(274, 99, 54, 38, RGB5(14,16,18));
    for (int i = 0; i < 6; i++) gfx_rect(278 + i * 2, 104 + i * 2, 12, 1, RGB5(30,36,40));
    gfx_rect(322, 139, 4, 2, RGB5(40,120,40)); gfx_rect(290, 142, 22, 6, RGB5(80,76,68));
    gfx_rect(276, 143, 46, 4, RGB5(82,82,88));
    for (int i = 0; i < 10; i++) gfx_rect(278 + i * 4, 144, 2, 1, RGB5(120,120,126));
    gfx_rect(330, 144, 6, 4, RGB5(70,70,76)); gfx_rect(258, 120, 10, 28, RGB5(50,50,56));
    gfx_rect(226, 228, 176, 38, RGB5(62,22,30)); gfx_rect(230, 232, 168, 30, RGB5(74,30,38));   /* alfombra */
    for (int x = 226; x < 402; x += 6) { gfx_rect(x, 226, 1, 2, RGB5(90,50,50)); gfx_rect(x, 266, 1, 2, RGB5(90,50,50)); }
    gfx_rect(150, 240, 40, 10, RGB5(30,40,70)); gfx_rect(158, 236, 24, 6, RGB5(30,40,70));      /* desorden */
    gfx_rect(172, 246, 26, 8, RGB5(20,20,24)); gfx_rect(176, 248, 18, 2, RGB5(160,160,150));
    gfx_rect(236, 252, 14, 6, RGB5(90,90,86)); gfx_rect(236, 258, 14, 2, RGB5(40,40,40));
    gfx_rect(254, 254, 14, 6, RGB5(90,90,86)); gfx_rect(254, 260, 14, 2, RGB5(40,40,40));
    gfx_disc(300, 240, 5, RGB5(100,104,110)); gfx_disc(300, 240, 1, RGB5(20,20,24));
    gfx_disc(314, 250, 5, RGB5(90,96,104));  gfx_disc(314, 250, 1, RGB5(20,20,24));
    gfx_disc(286, 256, 5, RGB5(110,100,110)); gfx_disc(286, 256, 1, RGB5(20,20,24));
    gfx_rect(332, 232, 6, 10, RGB5(130,30,30)); gfx_rect(332, 232, 6, 2, RGB5(150,150,150));
    gfx_rect(350, 248, 14, 14, RGB5(40,40,90)); gfx_rect(366, 252, 14, 14, RGB5(90,40,40));
    gfx_rect(384, 226, 30, 32, RGB5(50,60,40)); gfx_rect(390, 236, 18, 8, RGB5(36,44,30));
    gfx_rect(200, 256, 16, 8, RGB5(70,70,76)); gfx_rect(204, 258, 2, 4, RGB5(30,30,30));
    gfx_rect(208, 259, 2, 2, RGB5(140,40,40)); gfx_rect(211, 259, 2, 2, RGB5(40,40,140));
    gfx_rect(120, 250, 22, 10, RGB5(150,150,140)); gfx_rect(124, 252, 14, 1, RGB5(60,60,60));
}

/* ---------- Cama que se traga a Leo: sabanas como arenas movedizas ----------
 * phase 0 acostado mirando el techo | 1 crujido metalico y humedo
 *       2 las sabanas lo envuelven y se hunde | 3 el pozo se expande */
static void draw_leo_bed(int lx, int ly) {
    gfx_rect(lx + 2, ly + 16, 20, 7, RGB5(20,20,26));
    gfx_rect(lx, ly, 20, 18, RGB5(130,96,78));
    gfx_rect(lx - 1, ly - 4, 22, 8, RGB5(36,24,18)); gfx_rect(lx - 1, ly + 2, 4, 8, RGB5(36,24,18));
    gfx_rect(lx + 4, ly + 6, 4, 4, RGB5(210,210,200)); gfx_rect(lx + 12, ly + 6, 4, 4, RGB5(210,210,200));
    gfx_rect(lx + 5, ly + 6, 2, 2, BLACK); gfx_rect(lx + 13, ly + 6, 2, 2, BLACK);   /* mira hacia arriba */
    gfx_rect(lx + 7, ly + 14, 6, 1, RGB5(90,50,50));
}
static void draw_bed(int phase, int pt, int t) {
    int ox = 0, oy = 0;
    if (phase == 1) { ox = (t % 4 < 2) ? 1 : -1; oy = (t % 6 < 3) ? 0 : 1; }
    if (phase >= 2) ox = (t & 1) ? 2 : -2;
    gfx_rect(14 + ox, 128 + oy, 10, 86, RGB5(48,30,20)); gfx_rect(12 + ox, 124 + oy, 14, 6, RGB5(62,40,26));
    gfx_rect(20 + ox, 198 + oy, 196, 12, RGB5(52,34,22));
    gfx_rect(22 + ox, 210 + oy, 8, 10, RGB5(40,26,18)); gfx_rect(204 + ox, 210 + oy, 8, 10, RGB5(40,26,18));
    gfx_rect(24 + ox, 184 + oy, 188, 16, RGB5(120,116,106));
    for (int x = 40; x < 208; x += 24) gfx_rect(x + ox, 192 + oy, 3, 3, RGB5(80,76,70));
    gfx_rect(28 + ox, 160 + oy, 40, 22, RGB5(130,128,122)); gfx_rect(28 + ox, 180 + oy, 40, 2, RGB5(100,98,92));

    int lx = 40, ly = 154;
    if (phase == 2) { lx += pt * 70 / 180; ly += pt * 26 / 180; }
    if (phase < 2) draw_leo_bed(lx + ox, ly + oy);

    float amp = (phase == 0) ? 1.2f : (phase == 1) ? 2.0f + pt * 0.07f : 10.0f;
    for (int x = 58; x < 208; x++) {
        float hump = 10.0f * expf(-(((x - 110) / 40.0f) * ((x - 110) / 40.0f)));
        float w = sinf(x * 0.12f + t * (phase == 0 ? 0.04f : 0.35f)) * amp;
        int top = 172 + (x - 58) / 10 - (int)hump + (int)w;
        gfx_rect(x + ox, top + oy, 1, 198 - top, ((x / 8) & 1) ? RGB5(38,58,70) : RGB5(46,70,82));
        gfx_px(x + ox, top + oy, RGB5(80,110,122));
    }
    gfx_rect(58 + ox, 196 + oy, 150, 2, RGB5(26,40,50));
    if (phase >= 2) {
        draw_leo_bed(lx + ox, ly + oy);
        int tx = lx + 10 + ox, ty = ly + 10 + oy, n = pt * 36 / 120;     /* sabanas que lo envuelven */
        if (n > 36) n = 36;
        static const int sx[3] = { 70, 196, 130 };
        for (int b = 0; b < 3; b++)
            for (int s = 0; s < n; s++) {
                int x = sx[b] + (tx - sx[b]) * s / 36 + (int)(sinf(s * 0.35f + t * 0.2f + b) * 7);
                int y = 190 + (ty - 190) * s / 36 - (int)(sinf(s * 0.09f) * 24);
                gfx_rect(x + ox, y + oy, 6, 6, (s & 1) ? RGB5(46,70,82) : RGB5(38,58,70));
            }
        int rx, cx = 128 + ox, cy = 190 + oy;                              /* pozo de arena movediza */
        if (phase == 2) { rx = pt * 70 / 140; if (rx > 70) rx = 70; } else rx = 70 + pt * pt / 8;
        int ry = (phase == 2) ? rx * 3 / 10 : rx / 2;
        gfx_ellipse(cx, cy, rx + 6, ry + 4, RGB5(40,60,70));
        gfx_ellipse(cx, cy, rx, ry, RGB5(14,22,28));
        for (int k = 1; k < 4; k++) {                                       /* ondas */
            int rr = (rx * (4 - k) / 4 + (t / 3) % 8);
            if (rr > 2 && rr < rx) { for (int a = 0; a < 40; a++) gfx_px(cx + (int)(cosf(a * 0.157f) * rr), cy + (int)(sinf(a * 0.157f) * rr * 0.4f), RGB5(70,100,112)); }
        }
        gfx_ellipse(cx, cy, rx * 6 / 10, ry * 6 / 10, RGB5(4,0,2));
    }
}

#endif
