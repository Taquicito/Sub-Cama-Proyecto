/* assets.h - Sprites pixel art, paletas, textos y arte procedural de la intro */
#ifndef ASSETS_H
#define ASSETS_H

#include "render.h"

/* ---------- Sprites 8x8 del mundo de pesadilla ---------- */
static const char *const SPR_LEO[8] = {
    "..2222..", ".222222.", ".211112.", "..1111..", ".333333.", "13333331", ".344443.", ".44..44." };
static const pix PAL_LEO[4] = { RGB5(200,150,120), RGB5(70,44,30), RGB5(60,70,110), RGB5(40,40,60) };

static const char *const SPR_E0[8] = {      /* Babeante */
    "..1111..", ".111111.", "11311311", "11111111", "12444421", "12444421", ".122221.", "..1..1.." };
static const char *const SPR_E1[8] = {      /* Costurado */
    ".111111.", "13111131", "11111111", ".144441.", "..1111..", ".111111.", "1.1111.1", "..1..1.." };
static const char *const SPR_E2[8] = {      /* Reptante */
    "........", ".1....1.", "1.1111.1", "11311311", ".111111.", "1.4444.1", "1......1", "........" };
static const char *const SPR_E3[8] = {      /* Ojos */
    "11111111", "13311331", "13311331", "11111111", "14444441", "11111111", ".11..11.", "11....11" };
static const char *const *const SPR_ENEMY[4] = { SPR_E0, SPR_E1, SPR_E2, SPR_E3 };
static const pix PAL_ENEMY[4][4] = {
    { RGB5(120,70,80),  RGB5(60,30,40),  RGB5(230,220,120), RGB5(150,20,30) },
    { RGB5(110,100,110),RGB5(60,50,60),  RGB5(240,240,200), RGB5(40,10,20) },
    { RGB5(90,110,80),  RGB5(40,60,40),  RGB5(255,230,80),  RGB5(120,10,10) },
    { RGB5(140,60,60),  RGB5(70,20,20),  RGB5(255,255,230), RGB5(30,0,0) }
};

static const char *const SPR_BONFIRE[8] = {
    "...33...", "..3333..", ".333333.", "..3223..", ".1.22.1.", "..1111..", ".1....1.", "1......1" };
static const pix PAL_BONFIRE[3] = { RGB5(110,80,60), RGB5(90,50,20), RGB5(255,150,40) };

static const char *const SPR_ALIEN[8] = {
    "..1..1..", "...11...", "..1111..", ".11..11.", "11111111", "1.1111.1", "1.1..1.1", "...11..." };
static const char *const SPR_HEART[8] = {
    ".11..11.", "11111111", "11111111", "11111111", ".111111.", "..1111..", "...11...", "........" };

/* ---------- Textos (sin tildes ni enie: la fuente es ASCII) ---------- */
static const char *const INTRO_LINES[] = {
    "Leo, 15 anos. Otra noche sin poder dormir.",
    "La lluvia golpea el vidrio desde la tarde.\nEl monitor apagado te devuelve tu reflejo.",
    "La casa cruje. Es el viento.\nSeguro es el viento.",
    "...el colchon esta... respirando?",
    "NO. NO. SUELTAME--"
};
static const char *const SFX_TXT[] = {
    "KRRRRRRK-KRRRK", "*chirrriiiiiiik*", "GRRRMMMMMMM", "HHHHRRRRGGGGH", "glub... glub...", "TSSSSSSSK"
};
#define N_SFX 6
static const char *const DEPTH_TXT[] = {
    "La puerta se abre. Del otro lado esta el mismo pasillo.\nSolo que mas hondo.",
    "Otra puerta. Otra caida.\nEl colchon se ensancha para recibirte.",
    "Reconoces las manchas del techo. Son de tu cuarto.\nPero ahora estas por dentro.",
    "No hay afuera, Leo.\nSolo hay mas adentro."
};
#define N_DEPTH_TXT 4
static const char *const DEATH_TXT[] = {
    "Te hundes un poco mas...", "El colchon te devuelve. No te deja ir.", "Despertar no es una opcion."
};
static const char *const FALL_TXT =
    "Caes. Caes durante lo que parecen horas.\nEl suelo es blando y tibio. Respira contigo.\n"
    "SUB-CAMA SECTOR 0. Aqui nadie duerme.";

/* ---------- Trueno ---------- */
static int lightning(int t) {
    static const int st[6] = { 150, 330, 500, 700, 860, 1000 };
    int a = 0;
    for (int i = 0; i < 6; i++) {
        int d = t - st[i], v;
        if (d < 0 || d > 26) continue;
        if (d < 4) v = 10; else if (d < 8) v = 2; else if (d < 12) v = 12; else v = (26 - d) / 2;
        if (v > a) a = v;
    }
    return a;
}

/* ---------- Posters ---------- */
static void poster_band(int x, int y) {       /* nu-metal descolorido (banda ficticia) */
    pix bone = RGB5(118,110,96), dark = RGB5(6,4,4);
    gfx_rect(x, y, 70, 90, RGB5(22,12,14));
    gfx_rect(x + 2, y + 2, 66, 86, RGB5(30,16,18));
    gfx_rect(x + 19, y + 10, 32, 24, bone); gfx_rect(x + 23, y + 34, 24, 10, bone);
    gfx_rect(x + 23, y + 20, 8, 8, dark); gfx_rect(x + 39, y + 20, 8, 8, dark);
    gfx_rect(x + 33, y + 30, 4, 4, dark);
    for (int i = 0; i < 5; i++) gfx_rect(x + 25 + i * 4, y + 38, 1, 6, dark);
    gfx_text(x + 3, y + 54, "DEAD", RGB5(140,36,30), 2);
    gfx_text(x + 3, y + 71, "WIRE", RGB5(140,36,30), 2);
    gfx_rect(x, y + 47, 70, 1, RGB5(44,28,28));              /* doblez */
    gfx_rect(x - 2, y - 2, 9, 5, RGB5(120,110,80)); gfx_rect(x + 63, y - 2, 9, 5, RGB5(120,110,80));
}
static void poster_alt(int x, int y) {        /* rock alternativo, espiral */
    gfx_rect(x, y, 52, 70, RGB5(20,24,44));
    for (int r = 22; r > 3; r -= 6) gfx_ring(x + 26, y + 26, r, ((r / 6) & 1) ? RGB5(90,50,110) : RGB5(60,30,80));
    gfx_disc(x + 26, y + 26, 3, RGB5(140,60,60));
    gfx_text(x + 2, y + 52, "VELVET", RGB5(140,140,170), 1);
    gfx_text(x + 2, y + 61, "STATIC", RGB5(100,100,130), 1);
    gfx_rect(x - 2, y - 2, 7, 4, RGB5(120,110,80));
}
static void poster_game(int x, int y, int k) {  /* videojuegos clasicos */
    static const pix gpal[1] = { RGB5(60,150,70) }, hpal[1] = { RGB5(170,40,50) };
    gfx_rect(x, y, 44, 64, k ? RGB5(20,30,70) : RGB5(8,14,10));
    if (k == 0) {
        gfx_sprite(SPR_ALIEN, gpal, x + 10, y + 8, 3, 0);
        gfx_text(x + 2, y + 40, "SPACE", RGB5(150,150,120), 1);
        gfx_text(x + 6, y + 50, "RAID", RGB5(150,150,120), 1);
    } else {
        gfx_sprite(SPR_HEART, hpal, x + 10, y + 10, 3, 0);
        gfx_text(x + 2, y + 40, "PIXEL", RGB5(170,170,120), 1);
        gfx_text(x + 2, y + 50, "QUEST", RGB5(170,170,120), 1);
    }
    gfx_rect(x - 2, y - 2, 7, 4, RGB5(120,110,80));
}

/* ---------- Habitacion de Leo (adolescente de los 2000) ---------- */
static void draw_room(int t, int fl) {
    /* pared con papel tapiz */
    for (int x = 0; x < SCR_W; x += 8)
        gfx_rect(x, 0, 8, 190, ((x / 8) & 1) ? RGB5(34,38,54) : RGB5(30,34,48));
    for (int y = 10; y < 185; y += 24)
        for (int x = 4; x < SCR_W; x += 24) gfx_rect(x, y, 2, 2, RGB5(46,50,70));
    gfx_rect(0, 188, SCR_W, 8, RGB5(22,22,30));
    /* piso de madera */
    for (int y = 196; y < SCR_H; y += 12) {
        gfx_rect(0, y, SCR_W, 12, ((y / 12) & 1) ? RGB5(54,38,28) : RGB5(48,34,24));
        gfx_rect(0, y, SCR_W, 1, RGB5(30,20,14));
        for (int x = ((y / 12) * 53) % 90; x < SCR_W; x += 90) gfx_rect(x, y, 1, 12, RGB5(30,20,14));
    }
    /* ventana con lluvia diagonal */
    int wx = 366, wy = 24, ww = 90, wh = 108;
    pix frame = RGB5(74,66,58);
    gfx_rect(wx - 5, wy - 5, ww + 10, wh + 10, frame);
    gfx_rect(wx, wy, ww, wh, fl > 4 ? RGB5(150,160,210) : RGB5(14,20,36));
    gfx_rect(wx, wy + wh - 26, ww, 26, fl > 4 ? RGB5(40,44,70) : RGB5(8,10,18));
    gfx_rect(wx + 10, wy + wh - 40, 22, 14, fl > 4 ? RGB5(40,44,70) : RGB5(8,10,18));
    gfx_rect(wx + 52, wy + wh - 36, 18, 10, fl > 4 ? RGB5(40,44,70) : RGB5(8,10,18));
    for (int i = 0; i < 48; i++) {
        int sx = (int)(hash2(i, 11) % ww), sy = (int)(hash2(i, 12) % wh);
        int y = (sy + t * 6 + i * 7) % wh;
        int x = (((sx - y / 2) % ww) + ww) % ww;
        for (int k = 0; k < 5; k++) {
            int px = x - k / 2, py = y + k;
            if (px >= 0 && px < ww && py < wh) gfx_px(wx + px, wy + py, RGB5(96,120,170));
        }
    }
    gfx_rect(wx + ww / 2 - 1, wy, 3, wh, frame); gfx_rect(wx, wy + wh / 2 - 1, ww, 3, frame);
    gfx_rect(wx - 8, wy + wh + 5, ww + 16, 6, RGB5(84,76,66));
    gfx_rect(wx - 18, wy - 8, 12, wh + 34, RGB5(60,22,28));          /* cortinas */
    gfx_rect(wx + ww + 6, wy - 8, 12, wh + 34, RGB5(60,22,28));
    /* posters */
    poster_band(26, 20); poster_alt(108, 34); poster_game(170, 30, 0); poster_game(220, 38, 1);
    /* escritorio + monitor CRT apagado y abultado */
    pix wood = RGB5(74,52,36);
    gfx_rect(256, 148, 92, 6, wood);
    gfx_rect(258, 154, 6, 46, RGB5(58,40,28)); gfx_rect(340, 154, 6, 46, RGB5(58,40,28));
    gfx_rect(300, 154, 38, 32, RGB5(64,44,30)); gfx_rect(314, 168, 10, 3, RGB5(120,110,90));
    gfx_rect(280, 82, 40, 12, RGB5(70,66,58));                       /* trasera abultada */
    gfx_rect(268, 94, 66, 48, RGB5(88,84,74));                       /* carcasa */
    gfx_rect(268, 94, 66, 2, RGB5(110,106,96));
    gfx_rect(274, 99, 54, 38, RGB5(14,16,18));                       /* vidrio apagado */
    for (int i = 0; i < 6; i++) gfx_rect(278 + i * 2, 104 + i * 2, 12, 1, RGB5(30,36,40)); /* reflejo */
    gfx_rect(322, 139, 4, 2, RGB5(40,120,40));                       /* led */
    gfx_rect(290, 142, 22, 6, RGB5(80,76,68));                       /* base */
    gfx_rect(276, 143, 46, 4, RGB5(82,82,88));                       /* teclado */
    for (int i = 0; i < 10; i++) gfx_rect(278 + i * 4, 144, 2, 1, RGB5(120,120,126));
    gfx_rect(330, 144, 6, 4, RGB5(70,70,76));                        /* raton */
    gfx_rect(258, 120, 10, 28, RGB5(50,50,56));                      /* parlante */
    /* alfombra */
    gfx_rect(226, 228, 176, 38, RGB5(62,22,30)); gfx_rect(230, 232, 168, 30, RGB5(74,30,38));
    for (int x = 226; x < 402; x += 6) { gfx_rect(x, 226, 1, 2, RGB5(90,50,50)); gfx_rect(x, 266, 1, 2, RGB5(90,50,50)); }
    /* desorden en el suelo */
    gfx_rect(150, 240, 40, 10, RGB5(30,40,70)); gfx_rect(158, 236, 24, 6, RGB5(30,40,70));       /* jeans */
    gfx_rect(172, 246, 26, 8, RGB5(20,20,24)); gfx_rect(176, 248, 18, 2, RGB5(160,160,150));      /* playera */
    gfx_rect(236, 252, 14, 6, RGB5(90,90,86)); gfx_rect(236, 258, 14, 2, RGB5(40,40,40));         /* zapatillas */
    gfx_rect(254, 254, 14, 6, RGB5(90,90,86)); gfx_rect(254, 260, 14, 2, RGB5(40,40,40));
    gfx_disc(300, 240, 5, RGB5(100,104,110)); gfx_disc(300, 240, 1, RGB5(20,20,24));              /* CDs */
    gfx_disc(314, 250, 5, RGB5(90,96,104));  gfx_disc(314, 250, 1, RGB5(20,20,24));
    gfx_disc(286, 256, 5, RGB5(110,100,110)); gfx_disc(286, 256, 1, RGB5(20,20,24));
    gfx_rect(332, 232, 6, 10, RGB5(130,30,30)); gfx_rect(332, 232, 6, 2, RGB5(150,150,150));      /* lata */
    gfx_rect(350, 248, 14, 14, RGB5(40,40,90)); gfx_rect(366, 252, 14, 14, RGB5(90,40,40));       /* estuches */
    gfx_rect(384, 226, 30, 32, RGB5(50,60,40)); gfx_rect(390, 236, 18, 8, RGB5(36,44,30));        /* mochila */
    gfx_rect(200, 256, 16, 8, RGB5(70,70,76)); gfx_rect(204, 258, 2, 4, RGB5(30,30,30));          /* control */
    gfx_rect(208, 259, 2, 2, RGB5(140,40,40)); gfx_rect(211, 259, 2, 2, RGB5(40,40,140));
    gfx_rect(120, 250, 22, 10, RGB5(150,150,140)); gfx_rect(124, 252, 14, 1, RGB5(60,60,60));     /* revista */
}

/* ---------- Cama que cobra vida. phase: 0 calma, 1 se agita, 2 boca, 3 expande ---------- */
static void tooth(int x, int y, int dir, int h) {
    for (int i = 0; i < h; i++) {
        int w = (h - i) * 3 / 2 + 1;
        gfx_rect(x - w / 2, dir > 0 ? y + i : y - i, w, 1, RGB5(175,170,150));
    }
}
static void draw_leo_bed(int lx, int ly, int eyes) {
    gfx_rect(lx + 2, ly + 16, 20, 7, RGB5(20,20,26));               /* hombros, playera negra */
    gfx_rect(lx, ly, 20, 18, RGB5(130,96,78));                       /* cara */
    gfx_rect(lx - 1, ly - 4, 22, 8, RGB5(36,24,18));                 /* pelo */
    gfx_rect(lx - 1, ly + 2, 4, 8, RGB5(36,24,18));
    if (eyes) {
        gfx_rect(lx + 4, ly + 7, 4, 4, RGB5(210,210,200)); gfx_rect(lx + 12, ly + 7, 4, 4, RGB5(210,210,200));
        gfx_rect(lx + 5, ly + 8, 2, 2, BLACK); gfx_rect(lx + 13, ly + 8, 2, 2, BLACK);
    } else {
        gfx_rect(lx + 4, ly + 9, 4, 1, RGB5(30,20,20)); gfx_rect(lx + 12, ly + 9, 4, 1, RGB5(30,20,20));
    }
    gfx_rect(lx + 7, ly + 14, 6, 1, RGB5(90,50,50));
}
static void draw_bed(int phase, int pt, int t) {
    int ox = 0, oy = 0;
    if (phase == 1) { ox = (t % 4 < 2) ? 1 : -1; oy = (t % 6 < 3) ? 0 : 1; }
    if (phase >= 2) ox = (t & 1) ? 2 : -2;
    gfx_rect(14 + ox, 128 + oy, 10, 86, RGB5(48,30,20));              /* cabecera */
    gfx_rect(12 + ox, 124 + oy, 14, 6, RGB5(62,40,26));
    gfx_rect(20 + ox, 198 + oy, 196, 12, RGB5(52,34,22));             /* marco y patas */
    gfx_rect(22 + ox, 210 + oy, 8, 10, RGB5(40,26,18)); gfx_rect(204 + ox, 210 + oy, 8, 10, RGB5(40,26,18));
    gfx_rect(24 + ox, 184 + oy, 188, 16, RGB5(120,116,106));          /* colchon */
    for (int x = 40; x < 208; x += 24) gfx_rect(x + ox, 192 + oy, 3, 3, RGB5(80,76,70));
    gfx_rect(28 + ox, 160 + oy, 40, 22, RGB5(130,128,122));           /* almohada */
    gfx_rect(28 + ox, 180 + oy, 40, 2, RGB5(100,98,92));

    int lx = 40, ly = 154;
    if (phase == 2) { lx += pt * 80 / 180; ly += pt * 24 / 180; }
    if (phase == 0 || phase == 1) draw_leo_bed(lx + ox, ly + oy, phase == 1 && pt > 30);

    /* sabana por columnas con ondas */
    float amp = (phase == 0) ? 1.5f : (phase == 1) ? 2.0f + pt * 0.07f : 12.0f;
    for (int x = 58; x < 208; x++) {
        float hump = 10.0f * expf(-(((x - 110) / 40.0f) * ((x - 110) / 40.0f)));
        float w = sinf(x * 0.12f + t * (phase == 0 ? 0.05f : 0.35f)) * amp;
        int top = 172 + (x - 58) / 10 - (int)hump + (int)w;
        gfx_rect(x + ox, top + oy, 1, 198 - top, ((x / 8) & 1) ? RGB5(38,58,70) : RGB5(46,70,82));
        gfx_px(x + ox, top + oy, RGB5(80,110,122));
    }
    gfx_rect(58 + ox, 196 + oy, 150, 2, RGB5(26,40,50));
    if (phase >= 2) draw_leo_bed(lx + ox, ly + oy, 1);

    if (phase >= 2) {                                                 /* la boca del colchon */
        int rx, ry, cx = 128 + ox, cy = 188 + oy;
        if (phase == 2) { rx = pt * 72 / 120; if (rx > 72) rx = 72; }
        else rx = 72 + pt * pt / 8;
        ry = (phase == 2) ? rx * 5 / 16 : rx / 2;
        gfx_ellipse(cx, cy, rx + 4, ry + 4, RGB5(110,22,32));
        gfx_ellipse(cx, cy, rx, ry, RGB5(4,0,2));
        if (phase == 2 && rx > 20)
            for (int i = -rx + 8; i < rx - 8; i += 10) {
                float k = (float)i / (float)rx, e = sqrtf(1.0f - k * k) * ry;
                tooth(cx + i, cy - (int)e + 1, 1, 7); tooth(cx + i + 5, cy + (int)e - 1, -1, 6);
                if (hash2(i, 5) % 3 == 0) gfx_rect(cx + i, cy - (int)e + 8, 1, 6 + (t / 3 + i) % 5, RGB5(150,160,150));
            }
    }
}

#endif
