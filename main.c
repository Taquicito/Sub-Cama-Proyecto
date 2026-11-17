/* SUB-CAMA: SECTOR 0  -  homebrew de terror retro para PSP / PPSSPP
 * Modulos: render.h (video), platform.h (PSP), game_state.h (mundo/guardado),
 *          assets.h (arte/textos), combat.h (enemigos/RPG), main.c (estados). */
#include <math.h>
#include "render.h"
#include "platform.h"
#include "game_state.h"
#include "assets.h"
#include "combat.h"

static int g_menu_sel = 0, g_intro_t = 0, g_dead_t = 0, g_death_n = 0;
static int g_rest_sel = 0, g_pause_sel = 0, g_diff_sel = 0;
static const char *g_msg = ""; static int g_msg_t = 0;

static void say(const char *m) { g_msg = m; g_msg_t = 120; }
static void start_dialog(const char *txt, int ret) { dlg_text = txt; dlg_ret = ret; dlg_t = 0; g_state = ST_DIALOG; }

/* ===================== Partidas ===================== */
static void new_game(void) {
    g_depth = 0; g_story_idx = 0; g_last_bf = 0;
    player_new(); world_generate(0); enemies_spawn(); player_place(0);
    start_dialog(FALL_TXT, ST_PLAY);
}
static int load_game(void) {
    SaveData s;
    if (!save_read(&s)) return 0;
    g_diff = s.diff & 1; g_depth = s.depth; g_story_idx = s.story_idx; g_last_bf = s.last_bf;
    player_new();
    pl.lv = s.lv; pl.atk = s.atk; pl.def = s.def; pl.vit = s.vit; pl.blood = s.blood;
    player_recalc(); pl.hp = pl.maxhp;
    world_generate(g_depth); enemies_spawn(); player_place(g_last_bf);
    g_state = ST_PLAY;
    return 1;
}
static int nearest_bonfire(float *dist) {
    int best = 0; float bd = 1e9f;
    for (int i = 0; i < n_bf; i++) {
        float dx = bf[i].tx * TILE + 8 - pl.x, dy = bf[i].ty * TILE + 8 - pl.y, d = sqrtf(dx * dx + dy * dy);
        if (d < bd) { bd = d; best = i; }
    }
    if (dist) *dist = bd;
    return best;
}

/* ===================== Dibujo de mundo ===================== */
static pix tile_pixel(int t, int wx, int wy) {
    uint32_t h = hash2(wx, wy);
    int lx = wx & 15, ly = wy & 15, tx = wx >> 4, ty = wy >> 4;
    if (t) {                                         /* pared de carne */
        int r = 96 + (h & 15), g = 28 + ((h >> 4) & 7), b = 38 + ((h >> 8) & 7);
        if (((wx / 3 + (wy >> 2) * 5) & 7) == 0 && ((h >> 12) & 3) == 0) { r = 140; g = 40; b = 50; }  /* venas */
        if (lx == 0 || ly == 0) { r -= 30; g -= 10; b -= 10; }
        return rgb(r, g, b);
    }
    int base = 52 + (h & 7);                         /* suelo de colchon */
    int r = base + 10, g = base - 18, b = base - 8;
    if (((tx + ty) & 3) == 0 && ly == 8 && (lx & 3) < 2) { r = 30; g = 20; b = 24; }   /* costuras */
    if (((tx * 7 + ty * 13) % 11) == 0) {                                              /* botones */
        int dx = lx - 8, dy = ly - 8, d = dx * dx + dy * dy;
        if (d <= 9) { r = 34; g = 22; b = 28; } else if (d <= 16) { r += 14; g += 4; b += 4; }
    }
    return rgb(r, g, b);
}

static void draw_babble(int psx, int psy, int R) {   /* balbuceos invisibles = texto */
    int shown = 0;
    for (int i = 0; i < n_en && shown < 4; i++) {
        Enemy *e = &en[i];
        float dx = e->x - pl.x, dy = e->y - pl.y, d = sqrtf(dx * dx + dy * dy);
        if (!e->alive || d < R - 4 || d > 170 || e->babble_t < 12 || d < 0.1f) continue;
        char buf[24]; strncpy(buf, BABBLE[e->babble_i], 20); buf[20] = 0;
        for (int k = 0; buf[k]; k++)
            if (hash2(g_frame / 3, k + i * 7) % 6 == 0) buf[k] = "#%&*"[hash2(k, g_frame / 3) & 3];
        int w = (int)strlen(buf) * 8;
        int x = psx + (int)(dx / d * (R + 16)) - w / 2, y = psy + (int)(dy / d * (R + 16)) - 4;
        if (x < 2) x = 2;
        if (x > SCR_W - w - 2) x = SCR_W - w - 2;
        if (y < 2) y = 2;
        if (y > SCR_H - 12) y = SCR_H - 12;
        int v = 190 - (int)d; if (v < 50) v = 50; if (v > 170) v = 170;
        gfx_text(x, y, buf, rgb(v, v / 2, v / 2), 1);
        shown++;
    }
}

static void draw_hud(void) {
    gfx_rect(6, 6, 104, 10, RGB5(20,6,8));
    gfx_rect(8, 8, 100 * pl.hp / (pl.maxhp ? pl.maxhp : 1), 6, (pl.inv_timer > 40) ? RGB5(255,120,120) : RGB5(170,24,30));
    char b[48];
    snprintf(b, sizeof b, "HP %d/%d  NV %d", pl.hp, pl.maxhp, pl.lv);
    gfx_text_sh(8, 19, b, RGB5(150,140,140), 1);
    snprintf(b, sizeof b, "SANGRE %d", pl.blood);
    gfx_text_sh(SCR_W - 8 - (int)strlen(b) * 8, 8, b, RGB5(190,40,40), 1);
    snprintf(b, sizeof b, "PROFUNDIDAD %d", g_depth);
    gfx_text_sh(SCR_W - 8 - (int)strlen(b) * 8, 19, b, RGB5(110,100,110), 1);
    if (g_blood_msg_t > 0) {
        snprintf(b, sizeof b, "+%d SANGRE", g_blood_msg);
        gfx_text_c(40, b, RGB5(200,50,50), 1);
    }
    float d; nearest_bonfire(&d);
    if (d < 20) gfx_text_c(SCR_H - 16, "[X] DESCANSAR EN EL RESORTE", RGB5(220,150,60), 1);
    if (g_msg_t > 0) gfx_text_c(60, g_msg, RGB5(200,200,200), 1);
    if (pl.inv_timer > 40) { gfx_rect(0, 0, SCR_W, 3, RGB5(150,0,0)); gfx_rect(0, SCR_H - 3, SCR_W, 3, RGB5(150,0,0)); }
}

static void draw_world(void) {
    static const int FX[4] = { 0, 1, 0, -1 }, FY[4] = { -1, 0, 1, 0 };
    int R = DIFFS[g_diff].light + (int)(2.0f * sinf(g_frame * 0.11f)) - ((hash2(g_frame / 4, 7) & 7) == 0 ? 3 : 0);
    int camx = (int)pl.x - SCR_W / 2, camy = (int)pl.y - SCR_H / 2;
    if (camx < 0) camx = 0;
    if (camy < 0) camy = 0;
    if (camx > MAP_W * TILE - SCR_W) camx = MAP_W * TILE - SCR_W;
    if (camy > MAP_H * TILE - SCR_H) camy = MAP_H * TILE - SCR_H;
    if (g_shake > 0) { camx += (g_frame & 1) ? 2 : -2; if (camx < 0) camx = 0; if (camx > MAP_W * TILE - SCR_W) camx = MAP_W * TILE - SCR_W; }
    int psx = (int)pl.x - camx, psy = (int)pl.y - camy, B = R + 20;

    gfx_clear(BLACK);
    for (int y = psy - B; y <= psy + B; y++) {                  /* solo se pinta cerca de la luz */
        if (y < 0 || y >= SCR_H) continue;
        for (int x = psx - B; x <= psx + B; x++) {
            if (x < 0 || x >= SCR_W) continue;
            int wx = x + camx, wy = y + camy;
            g_fb[y * BUF_W + x] = tile_pixel(tiles[wy >> 4][wx >> 4], wx, wy);
        }
    }
    float cull = (float)(R + 10);
#define NEAR(ex, ey) (fabsf((ex) - pl.x) < cull && fabsf((ey) - pl.y) < cull && ((ex) - pl.x) * ((ex) - pl.x) + ((ey) - pl.y) * ((ey) - pl.y) < cull * cull)
    for (int i = 0; i < n_bf; i++) {
        float bx = bf[i].tx * TILE + 8, by = bf[i].ty * TILE + 8;
        if (NEAR(bx, by)) gfx_sprite(SPR_BONFIRE, PAL_BONFIRE, (int)bx - 8 - camx, (int)by - 8 - camy, 2, 0);
    }
    {
        float ex = exit_tx * TILE + 8, ey = exit_ty * TILE + 8;
        if (NEAR(ex, ey)) {
            gfx_rect((int)ex - 6 - camx, (int)ey - 8 - camy, 12, 16, RGB5(190,200,190));
            gfx_rect((int)ex - 4 - camx, (int)ey - 6 - camy, 8, 12, RGB5(10,10,14));
            gfx_rect((int)ex + 2 - camx, (int)ey - camy, 2, 2, RGB5(200,160,40));
        }
    }
    for (int i = 0; i < MAX_DROPS; i++)
        if (drops[i].active && NEAR(drops[i].x, drops[i].y)) {
            int x = (int)drops[i].x - camx, y = (int)drops[i].y - camy;
            gfx_rect(x - 2, y - 2, 5, 5, RGB5(170,15,20)); gfx_rect(x - 1, y - 2, 1, 1, RGB5(255,120,120));
        }
    for (int i = 0; i < n_en; i++) {
        Enemy *e = &en[i];
        if (!e->alive || !NEAR(e->x, e->y)) continue;
        static pix hp[4];
        const pix *pal = PAL_ENEMY[e->type];
        if (e->hurt > 0 && (e->hurt & 1)) { for (int k = 0; k < 4; k++) hp[k] = RGB5(255,255,255); pal = hp; }
        gfx_sprite(SPR_ENEMY[e->type], pal, (int)e->x - 8 - camx, (int)e->y - 8 - camy, 2, (g_frame / 20 + i) & 1);
    }
    if (!(pl.inv_timer > 0 && (pl.inv_timer / 3) % 2))
        gfx_sprite(SPR_LEO, PAL_LEO, psx - 8, psy - 9, 2, pl.facing == 3);
    if (pl.atk_timer > 12) {                                     /* destello del golpe */
        int ax = psx + FX[pl.facing] * 14, ay = psy + FY[pl.facing] * 14;
        if (FX[pl.facing]) gfx_rect(ax - 1, ay - 7, 2, 14, RGB5(230,230,220));
        else gfx_rect(ax - 7, ay - 1, 14, 2, RGB5(230,230,220));
    }
#undef NEAR
    gfx_light(psx, psy, R, B);                                   /* NEGRO ABSOLUTO fuera del aura */

    for (int i = 0; i < n_bf; i++) {                             /* brasas lejanas */
        int x = bf[i].tx * TILE + 8 - camx, y = bf[i].ty * TILE + 8 - camy;
        int dx = x - psx, dy = y - psy;
        if (dx * dx + dy * dy > R * R && dx * dx + dy * dy < 230 * 230 && rndr(4))
            gfx_rect(x - 1, y - 1, 2, 2, RGB5(220,110,30));
    }
    if (((g_frame / 40) & 1)) {                                  /* puerta de salida: pulso palido */
        int x = exit_tx * TILE + 8 - camx, y = exit_ty * TILE + 8 - camy;
        gfx_rect(x - 1, y - 1, 2, 2, RGB5(150,200,180));
    }
    draw_babble(psx, psy, R);
    draw_hud();
}

/* ===================== Cuadros de texto y menus ===================== */
static void draw_box(const char *txt, int n) {
    gfx_rect(10, 190, 460, 74, RGB5(6,6,12));
    gfx_frame(10, 190, 460, 74, RGB5(150,150,160));
    gfx_frame(12, 192, 456, 70, RGB5(50,50,60));
    gfx_text_n(22, 202, txt, n, RGB5(210,210,200), 1);
}
static void draw_menu(const char *title, const char *const *items, int n, int sel, int y0) {
    int w = 420, x = (SCR_W - w) / 2, h = 40 + n * 20;
    gfx_rect(x, y0, w, h, RGB5(8,6,10));
    gfx_frame(x, y0, w, h, RGB5(150,150,160));
    gfx_text_c(y0 + 8, title, RGB5(200,60,60), 1);
    for (int i = 0; i < n; i++) {
        char b[64]; snprintf(b, sizeof b, i == sel ? "> %s <" : "  %s  ", items[i]);
        gfx_text_c(y0 + 28 + i * 20, b, i == sel ? RGB5(240,240,230) : RGB5(100,100,110), 1);
    }
}

static void draw_sfx(int t, int scale, int v) {      /* sonido metalico/gutural en texto */
    for (int i = 0; i < 3; i++) {
        int idx = hash2(t / 10, i) % N_SFX, x = (int)(hash2(t / 10, i + 9) % 200) + 10;
        int y = 50 + (int)(hash2(t / 10, i + 20) % 120);
        x += (t % 3) - 1; y += ((t / 2) % 3) - 1;
        gfx_text(x, y, SFX_TXT[idx], rgb(v, v / 5, v / 5), scale);
    }
}

/* ===================== Titulo ===================== */
static void draw_title(void) {
    gfx_clear(BLACK);
    for (int i = 0; i < 1100; i++) { int v = 40 + rndr(90); gfx_px(rndr(SCR_W), rndr(SCR_H), rgb(v, v, v)); }   /* ruido blanco */
    for (int i = 0; i < 40; i++)                                                                            /* lluvia estatica */
        gfx_rect((i * 37) % SCR_W, (g_frame * (3 + i % 4) + i * 53) % SCR_H, 1, 6, RGB5(60,60,70));
    if (rndr(70) == 0) gfx_rect(0, rndr(SCR_H), SCR_W, 1, RGB5(200,200,200));
    int j = (g_frame / 3 % 23 == 0) ? 120 : 190;
    gfx_text_c(30, "SUB-CAMA", rgb(j, j, j), 4);
    gfx_text_c(70, "SECTOR 0", RGB5(160,20,25), 3);
    gfx_text_c(104, "- el colchon de carne -", RGB5(90,90,100), 1);
    static const char *items[3] = { "[JUGAR]", "[CARGAR PARTIDA]", "[OPCIONES DE DIFICULTAD]" };
    for (int i = 0; i < 3; i++) {
        char b[48]; snprintf(b, sizeof b, i == g_menu_sel ? "> %s <" : "%s", items[i]);
        gfx_text_c(140 + i * 24, b, i == g_menu_sel ? RGB5(240,240,230) : RGB5(90,90,100), 2);
    }
    char b[48]; snprintf(b, sizeof b, "DIFICULTAD: %s", DIFFS[g_diff].name);
    gfx_text_c(222, b, RGB5(130,60,60), 1);
    gfx_text_c(250, "X: ACEPTAR    D-PAD: MOVER", RGB5(80,80,90), 1);
    if (g_msg_t > 0) gfx_text_c(236, g_msg, RGB5(220,120,60), 1);
}
static void frame_title(const Input *in) {
    if (in->pressed & IN_UP) g_menu_sel = (g_menu_sel + 2) % 3;
    if (in->pressed & IN_DOWN) g_menu_sel = (g_menu_sel + 1) % 3;
    if (in->pressed & IN_CROSS) {
        if (g_menu_sel == 0) { g_state = ST_INTRO; g_intro_t = 0; }
        else if (g_menu_sel == 1) { if (!load_game()) say("NO HAY PARTIDA GUARDADA"); }
        else { g_state = ST_DIFF; g_diff_sel = g_diff; }
    }
    draw_title();
}
static void frame_diff(const Input *in) {
    if (in->pressed & (IN_UP | IN_DOWN)) g_diff_sel ^= 1;
    if (in->pressed & IN_CROSS) { g_diff = g_diff_sel; g_state = ST_TITLE; }
    if (in->pressed & IN_CIRCLE) g_state = ST_TITLE;
    gfx_clear(BLACK);
    for (int i = 0; i < 500; i++) { int v = 30 + rndr(60); gfx_px(rndr(SCR_W), rndr(SCR_H), rgb(v, v, v)); }
    gfx_text_c(24, "DIFICULTAD", RGB5(160,20,25), 3);
    for (int i = 0; i < 2; i++) {
        int y = 80 + i * 76; int s = (i == g_diff_sel);
        gfx_text_c(y, DIFFS[i].name, s ? RGB5(240,240,230) : RGB5(90,90,100), 2);
        pix c = s ? RGB5(170,170,170) : RGB5(70,70,80);
        gfx_text_c(y + 22, DIFFS[i].d1, c, 1); gfx_text_c(y + 34, DIFFS[i].d2, c, 1); gfx_text_c(y + 46, DIFFS[i].d3, c, 1);
    }
    gfx_text_c(250, "X: ELEGIR    O: VOLVER", RGB5(80,80,90), 1);
}

/* ===================== Introduccion ===================== */
static void frame_intro(const Input *in) {
    int t = g_intro_t++;
    if (in->pressed & IN_START || t >= 1100) { new_game(); return; }
    int phase = t < 420 ? 0 : t < 600 ? 1 : t < 780 ? 2 : t < 900 ? 3 : 4;
    int pt = phase == 1 ? t - 420 : phase == 2 ? t - 600 : phase == 3 ? t - 780 : 0;
    if (phase < 4) {
        int fl = lightning(t);
        draw_room(t, fl);
        draw_bed(phase, pt, t);
        if (phase == 3 && pt > 70) gfx_clear(BLACK);
        gfx_add(fl);
        if (t < 60) gfx_fade(t * 32 / 60);
    } else gfx_clear(BLACK);

    if (phase == 0) {
        int idx = t < 70 ? -1 : t < 190 ? 0 : t < 310 ? 1 : 2, st = idx == 0 ? 70 : idx == 1 ? 190 : 310;
        if (idx >= 0) draw_box(INTRO_LINES[idx], (t - st) / 2);
    } else if (phase == 1) {
        if (t >= 450) draw_box(INTRO_LINES[3], (t - 450) / 2);
        if (pt > 20) draw_sfx(t, 1, 80 + pt / 2);
    } else if (phase == 2) {
        if (pt >= 20) draw_box(INTRO_LINES[4], (pt - 20) / 2);
        draw_sfx(t, 2, 200);
    } else if (phase == 3) {
        draw_sfx(t, 3, 220);
    } else {
        draw_sfx(t, 2, 110 - (t - 900) / 4);
        if (t > 1000) gfx_text_c(130, "...", RGB5(80,80,80), 2);
    }
}

/* ===================== Juego ===================== */
static void frame_play(const Input *in) {
    float vx = 0, vy = 0;
    if (in->held & IN_LEFT) vx -= 1;
    if (in->held & IN_RIGHT) vx += 1;
    if (in->held & IN_UP) vy -= 1;
    if (in->held & IN_DOWN) vy += 1;
    if (in->ax || in->ay) { vx = in->ax / 128.0f; vy = in->ay / 128.0f; }
    float l = sqrtf(vx * vx + vy * vy);
    if (l > 1) { vx /= l; vy /= l; }
    if (l > 0.1f) pl.facing = (fabsf(vx) > fabsf(vy)) ? (vx > 0 ? 1 : 3) : (vy > 0 ? 2 : 0);
    move_ent(&pl.x, &pl.y, vx * 1.3f, vy * 1.3f, 5);

    float bd; int nb = nearest_bonfire(&bd);
    if (in->pressed & IN_CROSS) {
        if (bd < 20) { g_last_bf = nb; g_rest_sel = 0; g_state = ST_REST; }
        else player_attack();
    }
    if (in->pressed & IN_SQUARE) g_state = ST_STATUS;
    if (in->pressed & IN_START) { g_pause_sel = 0; g_state = ST_PAUSE; }

    if (pl.atk_timer > 0) pl.atk_timer--;
    if (pl.inv_timer > 0) pl.inv_timer--;
    if (g_shake > 0) g_shake--;
    if (g_blood_msg_t > 0) g_blood_msg_t--;
    if (g_msg_t > 0) g_msg_t--;
    if (g_frame % 8 == 0) flow_compute((int)(pl.x / TILE), (int)(pl.y / TILE));
    enemies_update();
    drops_update();

    float ex = exit_tx * TILE + 8 - pl.x, ey = exit_ty * TILE + 8 - pl.y;
    if (ex * ex + ey * ey < 100) {                           /* la salida solo te hunde mas */
        g_depth++; world_generate(g_depth); enemies_spawn(); player_place(0); g_last_bf = 0;
        start_dialog(DEPTH_TXT[g_depth - 1 < N_DEPTH_TXT - 1 ? g_depth - 1 : N_DEPTH_TXT - 1], ST_PLAY);
    }
    if (pl.hp <= 0) { g_state = ST_DEAD; g_dead_t = 0; }
    draw_world();
}

static void frame_dialog(const Input *in) {
    dlg_t++;
    int len = (int)strlen(dlg_text);
    if (in->pressed & IN_CROSS) { if (dlg_t / 2 < len) dlg_t = len * 2 + 2; else g_state = dlg_ret; }
    draw_world();
    draw_box(dlg_text, dlg_t / 2);
    if (dlg_t / 2 >= len && (g_frame / 20) & 1) gfx_text(440, 252, "X", RGB5(200,200,200), 1);
}

static void frame_status(const Input *in) {
    if (in->pressed & (IN_SQUARE | IN_CIRCLE)) g_state = ST_PLAY;
    gfx_clear(RGB5(6,4,8));
    gfx_frame(20, 14, 440, 244, RGB5(120,120,130));
    gfx_text_c(24, "LEO", RGB5(200,60,60), 2);
    char b[64]; int y = 56;
    gfx_sprite(SPR_LEO, PAL_LEO, 60, 70, 4, 0);
    snprintf(b, sizeof b, "NIVEL        %d", pl.lv);               gfx_text(150, y, b, RGB5(200,200,200), 1); y += 16;
    snprintf(b, sizeof b, "VIDA         %d/%d", pl.hp, pl.maxhp);  gfx_text(150, y, b, RGB5(200,200,200), 1); y += 16;
    snprintf(b, sizeof b, "ATAQUE       %d", pl.atk);              gfx_text(150, y, b, RGB5(200,200,200), 1); y += 16;
    snprintf(b, sizeof b, "DEFENSA      %d", pl.def);              gfx_text(150, y, b, RGB5(200,200,200), 1); y += 16;
    snprintf(b, sizeof b, "VITALIDAD    %d", pl.vit);              gfx_text(150, y, b, RGB5(200,200,200), 1); y += 16;
    snprintf(b, sizeof b, "SANGRE/EXP   %d  (siguiente nivel: %d)", pl.blood, levelup_cost()); gfx_text(150, y, b, RGB5(200,50,50), 1); y += 16;
    snprintf(b, sizeof b, "PROFUNDIDAD  %d", g_depth);             gfx_text(150, y, b, RGB5(200,200,200), 1); y += 16;
    snprintf(b, sizeof b, "DERROTADOS   %d", pl.kills);            gfx_text(150, y, b, RGB5(200,200,200), 1); y += 16;
    snprintf(b, sizeof b, "DIFICULTAD   %s", DIFFS[g_diff].name);  gfx_text(150, y, b, RGB5(200,200,200), 1);
    gfx_text_c(230, "Nunca sales. Solo bajas.", RGB5(110,40,40), 1);
    gfx_text_c(244, "[] / O: VOLVER", RGB5(80,80,90), 1);
}

static void frame_rest(const Input *in) {
    static const char *items[5] = { "ATAQUE  +1", "DEFENSA +1", "VITALIDAD +1", "DESCANSAR (cura, guarda, revive enemigos)", "SALIR" };
    if (in->pressed & IN_UP) g_rest_sel = (g_rest_sel + 4) % 5;
    if (in->pressed & IN_DOWN) g_rest_sel = (g_rest_sel + 1) % 5;
    if (in->pressed & IN_CIRCLE) g_state = ST_PLAY;
    if (in->pressed & IN_CROSS) {
        if (g_rest_sel < 3) {
            if (player_levelup(g_rest_sel)) {
                const char *s = story_check();
                if (s) start_dialog(s, ST_REST);
            } else say("SANGRE INSUFICIENTE");
        } else if (g_rest_sel == 3) {
            pl.hp += pl.maxhp * DIFFS[g_diff].rest_heal / 100; if (pl.hp > pl.maxhp) pl.hp = pl.maxhp;
            enemies_spawn(); flow_compute((int)(pl.x / TILE), (int)(pl.y / TILE));
            say(save_game() ? "DESCANSAS... PARTIDA GUARDADA" : "DESCANSAS... (ERROR AL GUARDAR)");
        } else g_state = ST_PLAY;
    }
    if (g_msg_t > 0) g_msg_t--;
    draw_world();
    char t[64]; snprintf(t, sizeof t, "RESORTE OXIDADO   SANGRE %d   COSTE %d", pl.blood, levelup_cost());
    draw_menu(t, items, 5, g_rest_sel, 70);
}

static void frame_pause(const Input *in) {
    static const char *items[3] = { "CONTINUAR", "GUARDAR", "VOLVER AL TITULO" };
    if (in->pressed & IN_UP) g_pause_sel = (g_pause_sel + 2) % 3;
    if (in->pressed & IN_DOWN) g_pause_sel = (g_pause_sel + 1) % 3;
    if (in->pressed & (IN_START | IN_CIRCLE)) g_state = ST_PLAY;
    if (in->pressed & IN_CROSS) {
        if (g_pause_sel == 0) g_state = ST_PLAY;
        else if (g_pause_sel == 1) say(save_game() ? "PARTIDA GUARDADA" : "ERROR AL GUARDAR");
        else g_state = ST_TITLE;
    }
    if (g_msg_t > 0) g_msg_t--;
    draw_world();
    draw_menu("PAUSA", items, 3, g_pause_sel, 80);
}

static void frame_dead(const Input *in) {
    (void)in;
    g_dead_t++;
    draw_world();
    gfx_fade(32 - (g_dead_t > 60 ? 28 : g_dead_t / 2));
    if (g_dead_t > 30) gfx_text_c(100, "TE HAS HUNDIDO", RGB5(170,20,25), 3);
    if (g_dead_t > 70) gfx_text_c(150, DEATH_TXT[g_death_n % 3], RGB5(150,150,150), 1);
    if (g_dead_t > 220) {
        g_death_n++;
        pl.blood /= 2; pl.hp = pl.maxhp; pl.inv_timer = 60;
        enemies_spawn(); player_place(g_last_bf);
        g_state = ST_PLAY;
    }
}

/* ===================== Bucle principal ===================== */
static void game_frame(const Input *in) {
    g_frame++;
    if (g_msg_t > 0 && g_state == ST_TITLE) g_msg_t--;
    switch (g_state) {
        case ST_TITLE:  frame_title(in);  break;
        case ST_DIFF:   frame_diff(in);   break;
        case ST_INTRO:  frame_intro(in);  break;
        case ST_PLAY:   frame_play(in);   break;
        case ST_DIALOG: frame_dialog(in); break;
        case ST_STATUS: frame_status(in); break;
        case ST_REST:   frame_rest(in);   break;
        case ST_PAUSE:  frame_pause(in);  break;
        case ST_DEAD:   frame_dead(in);   break;
    }
}

int main(int argc, char **argv) {
    plat_init(argc, argv);
    g_rng ^= 0x9E3779B9u;
    while (plat_running()) {
        Input in;
        plat_poll(&in);
        game_frame(&in);
        plat_present(g_fb);
    }
    plat_shutdown();
    return 0;
}
