/* SUB-CAMA: SECTOR 0 - Combate por turnos / Terror psicologico (PSP / PPSSPP)
 * main.cpp: estados del juego, mapa de exploracion, menus y bucle principal.
 * Modulos: player_stats.h  battle_system.h  narrative_engine.h  lighting.h
 *          assets.h  render.h  world.h  platform.h */
#include <math.h>
#include "render.h"
#include "platform.h"
#include "player_stats.h"
#include "lighting.h"
#include "assets.h"
#include "narrative_engine.h"
#include "battle_system.h"
#include "world.h"

enum { ST_TITLE, ST_DIFF, ST_CUTSCENE, ST_PLAY, ST_DIALOG, ST_MENU, ST_REFLECT, ST_PAUSE, ST_ENCOUNTER, ST_BATTLE, ST_GAMEOVER };

static Player pl;
static Battle battle;
static Lighting light;
static Cutscene cut;
static DialogBox dlg;

static int g_state = ST_TITLE, g_diff = 1, g_depth = 0, g_story_idx = 0, g_last_pt = 0, g_frame = 0;
static int dlg_ret = ST_PLAY, g_sel = 0, g_sel2 = 0, g_msg_t = 0, safe_t = 0, enc_t = 0, dead_t = 0, death_n = 0, g_shake = 0;
static const char *g_msg = "";

/* ---------- Enemigos y notas en el mapa ---------- */
struct OWEnemy { float x, y; int type, alive, stun, babble_t, babble_i; };
struct Note { int tx, ty, idx; };
#define MAX_OW 28
#define MAX_NOTES 6
static OWEnemy ow[MAX_OW];
static int n_ow;
static Note notes[MAX_NOTES];
static int n_notes;
static int enc_idx[3], enc_types[3], enc_n;

static const char *const BABBLE[10] = {
    "le...o...", "otra vez...", "ngh... ngh...", "hhhaaah", "no-no-NO",
    "sirve... nada", "mmh-mh-mh", "esfuerzate...", "kshhh...", "por que... tu..."
};
static const float OW_SPEED[5] = { 0.50f, 0.45f, 0.70f, 0.55f, 0.40f };

static void say(const char *m) { g_msg = m; g_msg_t = 120; }
static void start_dialog(const char *txt, int ret) { dlg.begin(txt, ""); dlg_ret = ret; g_state = ST_DIALOG; }

/* ---------- Texturas de baldosa precalculadas (para ir a 60 FPS) ---------- */
static pix tex_wall[4][256], tex_floor[5][256];
static void tex_init() {
    for (int v = 0; v < 5; v++)
        for (int ly = 0; ly < 16; ly++)
            for (int lx = 0; lx < 16; lx++) {
                uint32_t h = hash2(lx + v * 17, ly + v * 31);
                if (v < 4) {                                              /* pared de carne con venas */
                    int r = 96 + (h & 15), g = 28 + ((h >> 4) & 7), b = 38 + ((h >> 8) & 7);
                    if (((lx / 3 + (ly >> 2) * 5 + v) & 7) == 0 && ((h >> 12) & 3) == 0) { r = 140; g = 40; b = 50; }
                    if (lx == 0 || ly == 0) { r -= 30; g -= 10; b -= 10; }
                    tex_wall[v][ly * 16 + lx] = rgb(r, g, b);
                }
                int base = 52 + (h & 7), r = base + 10, g = base - 18, b = base - 8;
                if (v == 3 && ly == 8 && (lx & 3) < 2) { r = 30; g = 20; b = 24; }          /* costuras */
                if (v == 4) {                                                               /* boton de colchon */
                    int dx = lx - 8, dy = ly - 8, d = dx * dx + dy * dy;
                    if (d <= 9) { r = 34; g = 22; b = 28; } else if (d <= 16) { r += 14; g += 4; b += 4; }
                }
                tex_floor[v][ly * 16 + lx] = rgb(r, g, b);
            }
}
static void draw_tiles(int camx, int camy) {
    int tx0 = camx >> 4, ty0 = camy >> 4;
    for (int ty = ty0; ty <= ty0 + SCR_H / TILE + 1 && ty < MAP_H; ty++)
        for (int tx = tx0; tx <= tx0 + SCR_W / TILE + 1 && tx < MAP_W; tx++) {
            const pix *tex = tiles[ty][tx] ? tex_wall[hash2(tx, ty) & 3]
                           : (((tx * 7 + ty * 13) % 11 == 0) ? tex_floor[4] : tex_floor[hash2(tx, ty) & 3]);
            int dx = tx * TILE - camx, dy = ty * TILE - camy;
            int sx0 = dx < 0 ? -dx : 0, ex = dx + 16 > SCR_W ? SCR_W - dx : 16;
            if (ex <= sx0) continue;
            for (int r = 0; r < 16; r++) {
                int y = dy + r;
                if (y < 0 || y >= SCR_H) continue;
                memcpy(&g_fb[y * BUF_W + dx + sx0], &tex[r * 16 + sx0], (size_t)(ex - sx0) * 2);
            }
        }
}

/* ---------- Partida ---------- */
static void enemies_spawn() {
    g_rng = 4242u + (uint32_t)g_depth * 31u;
    flow_compute(points[0].tx, points[0].ty);
    n_ow = 0; n_notes = 0;
    int want = 10 + g_depth * 2; if (want > MAX_OW) want = MAX_OW;
    for (int tries = 0; tries < 5000 && n_ow < want; tries++) {
        int tx = 1 + rndr(MAP_W - 2), ty = 1 + rndr(MAP_H - 2);
        if (tiles[ty][tx] || flow[ty][tx] < 12 || flow[ty][tx] >= 9999) continue;
        int near_pt = 0;
        for (int b = 0; b < n_points; b++) if (abs(points[b].tx - tx) < 4 && abs(points[b].ty - ty) < 4) near_pt = 1;
        if (near_pt) continue;
        int maxt = 1 + flow[ty][tx] / 25 + g_depth; if (maxt > 4) maxt = 4;
        OWEnemy &e = ow[n_ow++];
        e.x = tx * TILE + 8; e.y = ty * TILE + 8; e.type = rndr(maxt + 1);
        e.alive = 1; e.stun = 0; e.babble_t = rndr(60); e.babble_i = rndr(10);
    }
    for (int tries = 0; tries < 5000 && n_notes < MAX_NOTES; tries++) {
        int tx = 1 + rndr(MAP_W - 2), ty = 1 + rndr(MAP_H - 2);
        if (tiles[ty][tx] || flow[ty][tx] < 14 || flow[ty][tx] >= 9999) continue;
        notes[n_notes].tx = tx; notes[n_notes].ty = ty; notes[n_notes].idx = rndr(8); n_notes++;
    }
}
static void place_player(int pt) {
    if (pt < 0 || pt >= n_points) pt = 0;
    pl.x = (float)(points[pt].tx * TILE + 8); pl.y = (float)(points[pt].ty * TILE + 8);
}
static void new_game() {
    pl.reset(); g_depth = 0; g_story_idx = 0; g_last_pt = 0; death_n = 0;
    light.set_radius(DIFFS[g_diff].light);
    world_generate(0); enemies_spawn(); place_player(0); safe_t = 60;
    start_dialog(FALL_TXT, ST_PLAY);
}
static int load_game() {
    SaveData s;
    if (!save_read(&s)) return 0;
    pl.reset();
    g_diff = s.diff & 3; g_depth = s.depth; g_story_idx = s.story_idx; g_last_pt = s.last_pt;
    pl.lv = s.lv; pl.exp = s.exp; pl.blood = s.blood; pl.maxhp = s.maxhp; pl.maxsp = s.maxsp;
    pl.atk = s.atk; pl.def = s.def; pl.agi = s.agi; pl.mag = s.mag; pl.kills = s.kills;
    for (int i = 0; i < N_ITEMS; i++) pl.items[i] = s.items[i];
    pl.hp = pl.maxhp; pl.sp = pl.maxsp;
    light.set_radius(DIFFS[g_diff].light);
    world_generate(g_depth); enemies_spawn(); place_player(g_last_pt); safe_t = 60;
    g_state = ST_PLAY;
    return 1;
}
static int nearest_point(float *dist) {
    int best = 0; float bd = 1e9f;
    for (int i = 0; i < n_points; i++) {
        float dx = points[i].tx * TILE + 8 - pl.x, dy = points[i].ty * TILE + 8 - pl.y, d = sqrtf(dx * dx + dy * dy);
        if (d < bd) { bd = d; best = i; }
    }
    if (dist) *dist = bd;
    return best;
}
static int nearest_note(float *dist) {
    int best = -1; float bd = 1e9f;
    for (int i = 0; i < n_notes; i++) {
        float dx = notes[i].tx * TILE + 8 - pl.x, dy = notes[i].ty * TILE + 8 - pl.y, d = sqrtf(dx * dx + dy * dy);
        if (d < bd) { bd = d; best = i; }
    }
    if (dist) *dist = bd;
    return best;
}

/* ---------- Exploracion: dibujo ---------- */
static void draw_babble(int camx, int camy) {          /* voces familiares distorsionadas */
    int shown = 0;
    for (int i = 0; i < n_ow && shown < 4; i++) {
        OWEnemy &e = ow[i];
        float dx = e.x - pl.x, dy = e.y - pl.y, d = sqrtf(dx * dx + dy * dy);
        if (!e.alive || d > 170 || e.babble_t < 12) continue;
        char buf[24]; strncpy(buf, BABBLE[e.babble_i], 20); buf[20] = 0;
        for (int k = 0; buf[k]; k++)
            if (hash2(g_frame / 3, k + i * 7) % 6 == 0) buf[k] = "#%&*"[hash2(k, g_frame / 3) & 3];
        int w = (int)strlen(buf) * 8;
        int x = (int)e.x - camx - w / 2, y = (int)e.y - camy - 16;
        if (x < 2) x = 2;
        if (x > SCR_W - w - 2) x = SCR_W - w - 2;
        if (y < 2) y = 2;
        if (y > SCR_H - 12) y = SCR_H - 12;
        int v = 200 - (int)d; if (v < 60) v = 60;
        gfx_text(x, y, buf, rgb(v, v / 2, v / 2), 1);
        shown++;
    }
}
static void draw_hud() {
    char b[48];
    gfx_rect(6, 6, 104, 8, RGB5(20,6,8)); gfx_rect(8, 8, 100 * pl.hp / (pl.maxhp ? pl.maxhp : 1), 4, RGB5(170,24,30));
    gfx_rect(6, 16, 104, 6, RGB5(8,10,24)); gfx_rect(8, 17, 100 * pl.sp / (pl.maxsp ? pl.maxsp : 1), 3, RGB5(60,100,190));
    snprintf(b, sizeof b, "NV %d  HP %d/%d  SP %d", pl.lv, pl.hp, pl.maxhp, pl.sp);
    gfx_text_sh(8, 25, b, RGB5(170,160,160), 1);
    snprintf(b, sizeof b, "SANGRE %d", pl.blood);
    gfx_text_sh(SCR_W - 8 - (int)strlen(b) * 8, 8, b, RGB5(190,40,40), 1);
    snprintf(b, sizeof b, "EXP %d/%d", pl.exp, pl.exp_needed());
    gfx_text_sh(SCR_W - 8 - (int)strlen(b) * 8, 19, b, RGB5(150,150,160), 1);
    snprintf(b, sizeof b, "PROF. %d  %s", g_depth, DIFFS[g_diff].name);
    gfx_text_sh(SCR_W - 8 - (int)strlen(b) * 8, 30, b, RGB5(110,100,110), 1);
    float d, dn; nearest_point(&d); int ni = nearest_note(&dn);
    if (d < 20) gfx_text_c(SCR_H - 16, "[X] PUNTO DE REFLEXION", RGB5(220,150,60), 1);
    else if (ni >= 0 && dn < 14) gfx_text_c(SCR_H - 16, "[X] LEER", RGB5(200,200,200), 1);
    if (g_msg_t > 0) gfx_text_c(60, g_msg, RGB5(200,200,200), 1);
}
static void draw_scene(bool hud) {
    int R = light.radius(g_frame, pl.hp, pl.maxhp);
    int camx = (int)pl.x - SCR_W / 2, camy = (int)pl.y - SCR_H / 2;
    if (camx < 0) camx = 0;
    if (camy < 0) camy = 0;
    if (camx > MAP_W * TILE - SCR_W) camx = MAP_W * TILE - SCR_W;
    if (camy > MAP_H * TILE - SCR_H) camy = MAP_H * TILE - SCR_H;
    if (g_shake > 0) { camx += (g_frame & 1) ? 2 : -2; if (camx < 0) camx = 0; if (camx > MAP_W * TILE - SCR_W) camx = MAP_W * TILE - SCR_W; }
    int psx = (int)pl.x - camx, psy = (int)pl.y - camy;

    draw_tiles(camx, camy);
    for (int i = 0; i < n_points; i++)
        gfx_sprite(SPR_POINT, PAL_POINT, points[i].tx * TILE - camx, points[i].ty * TILE - camy, 2, 0);
    for (int i = 0; i < n_notes; i++) {
        int x = notes[i].tx * TILE + 5 - camx, y = notes[i].ty * TILE + 4 - camy;
        gfx_rect(x, y, 6, 8, RGB5(200,200,185)); gfx_rect(x + 1, y + 2, 4, 1, RGB5(90,90,90)); gfx_rect(x + 1, y + 4, 3, 1, RGB5(90,90,90));
    }
    {   /* puerta de salida: solo te hunde mas */
        int x = exit_tx * TILE - camx, y = exit_ty * TILE - camy;
        gfx_rect(x + 2, y, 12, 16, RGB5(190,200,190)); gfx_rect(x + 4, y + 2, 8, 12, RGB5(10,10,14)); gfx_rect(x + 10, y + 8, 2, 2, RGB5(200,160,40));
    }
    for (int i = 0; i < n_ow; i++) {                    /* los monstruos se intuyen en la penumbra */
        OWEnemy &e = ow[i];
        if (!e.alive) continue;
        gfx_sprite_n(SPR_ENEMY[e.type], 16, 16, PAL_ENEMY[e.type], (int)e.x - 8 - camx, (int)e.y - 8 - camy, 1, (g_frame / 20 + i) & 1);
    }
    if (!(safe_t > 0 && (g_frame / 3) % 2)) gfx_sprite(SPR_LEO, PAL_LEO, psx - 8, psy - 9, 2, 0);
    light.apply(psx, psy, R);
    draw_babble(camx, camy);
    if (hud) draw_hud();
}

/* ---------- Titulo y dificultad ---------- */
static void draw_title() {
    gfx_clear(BLACK);
    gfx_static(1100, 40, 130);
    for (int i = 0; i < 40; i++) gfx_rect((i * 37) % SCR_W, (g_frame * (3 + i % 4) + i * 53) % SCR_H, 1, 6, RGB5(60,60,70));
    if (rndr(70) == 0) gfx_rect(0, rndr(SCR_H), SCR_W, 1, RGB5(200,200,200));
    int j = (g_frame / 3 % 23 == 0) ? 120 : 190;
    gfx_text_c(30, "SUB-CAMA", rgb(j, j, j), 4);
    gfx_text_c(70, "SECTOR 0", RGB5(160,20,25), 3);
    gfx_text_c(104, "- el colchon de carne -", RGB5(90,90,100), 1);
    static const char *items[3] = { "[NUEVA PARTIDA]", "[CARGAR PARTIDA]", "[OPCIONES DE DIFICULTAD]" };
    for (int i = 0; i < 3; i++) {
        char b[48]; snprintf(b, sizeof b, i == g_sel ? "> %s <" : "%s", items[i]);
        gfx_text_c(140 + i * 24, b, i == g_sel ? RGB5(240,240,230) : RGB5(90,90,100), 2);
    }
    char b[64]; snprintf(b, sizeof b, "DIFICULTAD: %s (EXP x%.1f)", DIFFS[g_diff].name, DIFFS[g_diff].exp_mult);
    gfx_text_c(222, b, RGB5(130,60,60), 1);
    gfx_text_c(250, "X: ACEPTAR    D-PAD: MOVER", RGB5(80,80,90), 1);
    if (g_msg_t > 0) gfx_text_c(236, g_msg, RGB5(220,120,60), 1);
}
static void frame_title(const Input &in) {
    if (in.pressed & IN_UP) g_sel = (g_sel + 2) % 3;
    if (in.pressed & IN_DOWN) g_sel = (g_sel + 1) % 3;
    if (in.pressed & IN_CROSS) {
        if (g_sel == 0) { cut.start(); g_state = ST_CUTSCENE; }
        else if (g_sel == 1) { if (!load_game()) say("NO HAY PARTIDA GUARDADA"); }
        else { g_state = ST_DIFF; g_sel2 = g_diff; }
    }
    if (g_msg_t > 0) g_msg_t--;
    draw_title();
}
static void frame_diff(const Input &in) {
    if (in.pressed & IN_UP) g_sel2 = (g_sel2 + 3) % 4;
    if (in.pressed & IN_DOWN) g_sel2 = (g_sel2 + 1) % 4;
    if (in.pressed & IN_CROSS) { g_diff = g_sel2; g_state = ST_TITLE; }
    if (in.pressed & IN_CIRCLE) g_state = ST_TITLE;
    gfx_clear(BLACK);
    gfx_static(500, 30, 90);
    gfx_text_c(12, "DIFICULTAD", RGB5(160,20,25), 3);
    for (int i = 0; i < 4; i++) {
        int y = 50 + i * 52, s = (i == g_sel2);
        char b[48]; snprintf(b, sizeof b, "%s   EXP x%.1f", DIFFS[i].name, DIFFS[i].exp_mult);
        gfx_text_c(y, b, s ? RGB5(240,240,230) : RGB5(90,90,100), 1);
        pix c = s ? RGB5(170,170,170) : RGB5(70,70,80);
        gfx_text_c(y + 12, DIFFS[i].d1, c, 1); gfx_text_c(y + 22, DIFFS[i].d2, c, 1); gfx_text_c(y + 32, DIFFS[i].d3, c, 1);
        if (s) gfx_frame(60, y - 5, 360, 46, RGB5(120,30,30));
    }
    gfx_text_c(262, "X: ELEGIR    O: VOLVER", RGB5(80,80,90), 1);
}

/* ---------- Exploracion: logica ---------- */
static void start_encounter(int ei) {
    enc_n = 0;
    enc_idx[enc_n] = ei; enc_types[enc_n++] = ow[ei].type;
    for (int i = 0; i < n_ow && enc_n < 3; i++) {
        if (i == ei || !ow[i].alive || ow[i].stun > 0) continue;
        if (fabsf(ow[i].x - ow[ei].x) < 56 && fabsf(ow[i].y - ow[ei].y) < 56) { enc_idx[enc_n] = i; enc_types[enc_n++] = ow[i].type; }
    }
    enc_t = 0; g_state = ST_ENCOUNTER;
}
static void enemies_update() {
    static const int DX[4] = { 0, 1, 0, -1 }, DY[4] = { -1, 0, 1, 0 };
    for (int i = 0; i < n_ow; i++) {
        OWEnemy &e = ow[i];
        if (!e.alive) continue;
        if (e.babble_t-- <= 0) { e.babble_t = 40 + rndr(50); e.babble_i = rndr(10); }
        if (e.stun > 0) { e.stun--; continue; }
        float spd = OW_SPEED[e.type] * DIFFS[g_diff].espeed * (1.0f + 0.03f * g_depth);
        float tgx = pl.x, tgy = pl.y, dxp = pl.x - e.x, dyp = pl.y - e.y;
        if (dxp * dxp + dyp * dyp > 24 * 24) {         /* siempre saben donde estas */
            int tx = (int)(e.x / TILE), ty = (int)(e.y / TILE);
            if (tx >= 0 && ty >= 0 && tx < MAP_W && ty < MAP_H && flow[ty][tx] < 9999) {
                int best = flow[ty][tx], bx = tx, by = ty;
                for (int d = 0; d < 4; d++) {
                    int nx = tx + DX[d], ny = ty + DY[d];
                    if (nx >= 0 && ny >= 0 && nx < MAP_W && ny < MAP_H && flow[ny][nx] < best) { best = flow[ny][nx]; bx = nx; by = ny; }
                }
                tgx = bx * TILE + 8.0f; tgy = by * TILE + 8.0f;
            }
        }
        float dx = tgx - e.x, dy = tgy - e.y, l = sqrtf(dx * dx + dy * dy);
        if (l > 0.5f) { dx /= l; dy /= l; } else dx = dy = 0;
        move_ent(&e.x, &e.y, dx * spd, dy * spd, 5);
        if (safe_t == 0 && fabsf(e.x - pl.x) < 11 && fabsf(e.y - pl.y) < 11) { start_encounter(i); return; }
    }
}
static void frame_play(const Input &in) {
    float vx = 0, vy = 0;
    if (in.held & IN_LEFT) vx -= 1;
    if (in.held & IN_RIGHT) vx += 1;
    if (in.held & IN_UP) vy -= 1;
    if (in.held & IN_DOWN) vy += 1;
    if (in.ax || in.ay) { vx = in.ax / 128.0f; vy = in.ay / 128.0f; }
    float l = sqrtf(vx * vx + vy * vy);
    if (l > 1) { vx /= l; vy /= l; }
    move_ent(&pl.x, &pl.y, vx * 1.3f, vy * 1.3f, 5);

    float d, dn; int pt = nearest_point(&d); int ni = nearest_note(&dn);
    if (in.pressed & IN_CROSS) {
        if (d < 20) { g_last_pt = pt; g_sel = 0; g_state = ST_REFLECT; }
        else if (ni >= 0 && dn < 14) start_dialog(ENV_TXT[notes[ni].idx], ST_PLAY);
    }
    if (in.pressed & IN_SQUARE) { g_sel = 0; g_sel2 = 0; g_state = ST_MENU; }
    if (in.pressed & IN_START) { g_sel = 0; g_state = ST_PAUSE; }
    if (safe_t > 0) safe_t--;
    if (g_msg_t > 0) g_msg_t--;
    if (g_shake > 0) g_shake--;
    if (g_frame % 8 == 0) flow_compute((int)(pl.x / TILE), (int)(pl.y / TILE));
    if (g_state == ST_PLAY) enemies_update();

    float ex = exit_tx * TILE + 8 - pl.x, ey = exit_ty * TILE + 8 - pl.y;
    if (g_state == ST_PLAY && ex * ex + ey * ey < 100) {       /* la salida solo te hunde mas */
        g_depth++; world_generate(g_depth); enemies_spawn(); place_player(0); g_last_pt = 0; safe_t = 90;
        start_dialog(DEPTH_TXT[g_depth - 1 < 3 ? g_depth - 1 : 3], ST_PLAY);
    }
    draw_scene(true);
}
static void frame_encounter() {
    enc_t++;
    draw_scene(false);
    gfx_static(enc_t * 260, 80, 255);
    if ((enc_t / 3) & 1) gfx_add(6);
    if (enc_t > 8) gfx_text_c(120, "!", RGB5(220,40,40), 6);
    if (enc_t >= 45) {
        light.set_radius(DIFFS[g_diff].light);
        battle.start(&pl, enc_types, enc_idx, enc_n, g_depth, g_diff);
        g_state = ST_BATTLE;
    }
}
static void frame_battle(const Input &in) {
    battle.update(in);
    battle.draw();
    if (battle.phase != Battle::PH_END) return;
    int res = battle.result;
    if (res == 1) {                                     /* victoria */
        for (int i = 0; i < battle.nf; i++) ow[battle.ow_index[i]].alive = 0;
        safe_t = 60; g_state = ST_PLAY;
        if (pl.exp >= pl.exp_needed()) say("Puedes reflexionar en un Punto.");
    } else if (res == 3) {                              /* huida: los monstruos quedan aturdidos */
        for (int i = 0; i < battle.nf; i++) {
            OWEnemy &e = ow[battle.ow_index[i]];
            e.stun = 240;
            float dx = e.x - pl.x, dy = e.y - pl.y, l = sqrtf(dx * dx + dy * dy);
            if (l > 0.1f) move_ent(&e.x, &e.y, dx / l * 20, dy / l * 20, 5);
        }
        safe_t = 90; g_state = ST_PLAY;
    } else {                                            /* derrota: penalizacion segun dificultad */
        pl.blood -= pl.blood * DIFFS[g_diff].blood_loss / 100;
        pl.exp -= pl.exp * DIFFS[g_diff].exp_loss / 100;
        pl.hp = pl.maxhp; pl.sp = pl.maxsp; pl.clear_status();
        enemies_spawn(); place_player(g_last_pt); safe_t = 120;
        dead_t = 0; g_state = ST_GAMEOVER;
    }
}
static void frame_gameover() {
    dead_t++;
    draw_scene(false);
    gfx_fade(32 - (dead_t > 60 ? 28 : dead_t / 2));
    if (dead_t > 30) gfx_text_c(100, "TE HAS HUNDIDO", RGB5(170,20,25), 3);
    if (dead_t > 70) gfx_text_c(150, DEATH_TXT[death_n % 3], RGB5(150,150,150), 1);
    if (dead_t > 90 && DIFFS[g_diff].blood_loss) {
        char b[64]; snprintf(b, sizeof b, "Pierdes %d%% de Sangre%s.", DIFFS[g_diff].blood_loss, DIFFS[g_diff].exp_loss ? " y algo de EXP" : "");
        gfx_text_c(170, b, RGB5(130,50,50), 1);
    }
    if (dead_t > 220) { death_n++; g_state = ST_PLAY; }
}

/* ---------- Menus ---------- */
static void draw_menu_box(const char *title, const char *const *items, int n, int sel, int y0, int w) {
    int x = (SCR_W - w) / 2, h = 40 + n * 18;
    gfx_rect(x, y0, w, h, RGB5(8,6,10)); gfx_frame(x, y0, w, h, RGB5(150,150,160));
    gfx_text_c(y0 + 8, title, RGB5(200,60,60), 1);
    for (int i = 0; i < n; i++) {
        char b[80]; snprintf(b, sizeof b, i == sel ? "> %s <" : "  %s  ", items[i]);
        gfx_text_c(y0 + 28 + i * 18, b, i == sel ? RGB5(240,240,230) : RGB5(100,100,110), 1);
    }
}
static void frame_dialog(const Input &in) {
    if (dlg.update((in.pressed & IN_CROSS) != 0)) g_state = dlg_ret;
    draw_scene(true);
    dlg.draw();
}
static void frame_pause(const Input &in) {
    static const char *items[3] = { "CONTINUAR", "GUARDAR", "VOLVER AL TITULO" };
    if (in.pressed & IN_UP) g_sel = (g_sel + 2) % 3;
    if (in.pressed & IN_DOWN) g_sel = (g_sel + 1) % 3;
    if (in.pressed & (IN_START | IN_CIRCLE)) g_state = ST_PLAY;
    if (in.pressed & IN_CROSS) {
        if (g_sel == 0) g_state = ST_PLAY;
        else if (g_sel == 1) say(save_game(pl, g_diff, g_depth, g_last_pt, g_story_idx) ? "PARTIDA GUARDADA" : "ERROR AL GUARDAR");
        else g_state = ST_TITLE;
    }
    if (g_msg_t > 0) g_msg_t--;
    draw_scene(true);
    draw_menu_box("PAUSA", items, 3, g_sel, 80, 260);
}
/* Cuadrado: equipo, inventario y estadisticas */
static void frame_menu(const Input &in) {
    if (in.pressed & (IN_SQUARE | IN_CIRCLE)) { g_state = ST_PLAY; return; }
    if (in.pressed & IN_UP) g_sel = (g_sel + N_ITEMS - 1) % N_ITEMS;
    if (in.pressed & IN_DOWN) g_sel = (g_sel + 1) % N_ITEMS;
    if ((in.pressed & IN_CROSS) && pl.items[g_sel] > 0) {
        bool used = false;
        if (g_sel == 0 && pl.hp < pl.maxhp) { pl.heal(35); used = true; }
        else if (g_sel == 1 && pl.sp < pl.maxsp) { pl.heal_sp(12); used = true; }
        else if (g_sel == 2 && (pl.any_status() || pl.hp < pl.maxhp)) { pl.clear_status(); pl.heal(10); used = true; }
        if (used) pl.items[g_sel]--; else say("NO TIENE EFECTO AHORA");
    }
    if (g_msg_t > 0) g_msg_t--;
    gfx_clear(RGB5(6,4,8));
    gfx_frame(10, 8, 460, 256, RGB5(120,120,130));
    gfx_text_c(14, "LEO", RGB5(200,60,60), 2);
    gfx_sprite(SPR_LEO, PAL_LEO, 30, 50, 4, 0);
    char b[64]; int y = 44;
    snprintf(b, sizeof b, "NIVEL     %d", pl.lv);                         gfx_text(90, y, b, RGB5(200,200,200), 1); y += 14;
    snprintf(b, sizeof b, "HP        %d/%d", pl.hp, pl.maxhp);           gfx_text(90, y, b, RGB5(200,200,200), 1); y += 14;
    snprintf(b, sizeof b, "SP        %d/%d", pl.sp, pl.maxsp);           gfx_text(90, y, b, RGB5(200,200,200), 1); y += 14;
    snprintf(b, sizeof b, "FUERZA    %d", pl.atk);                       gfx_text(90, y, b, RGB5(200,200,200), 1); y += 14;
    snprintf(b, sizeof b, "DEFENSA   %d", pl.def);                       gfx_text(90, y, b, RGB5(200,200,200), 1); y += 14;
    snprintf(b, sizeof b, "AGILIDAD  %d", pl.agi);                       gfx_text(90, y, b, RGB5(200,200,200), 1); y += 14;
    snprintf(b, sizeof b, "PSIQUE    %d", pl.mag);                       gfx_text(90, y, b, RGB5(200,200,200), 1); y += 14;
    snprintf(b, sizeof b, "EXP       %d/%d", pl.exp, pl.exp_needed());   gfx_text(90, y, b, RGB5(150,150,160), 1); y += 14;
    snprintf(b, sizeof b, "SANGRE    %d (nivel: %d)", pl.blood, pl.blood_cost()); gfx_text(90, y, b, RGB5(200,50,50), 1);
    gfx_text(250, 44, "EQUIPO", RGB5(200,60,60), 1);
    gfx_text(250, 58, "Linterna oxidada   PSI+", RGB5(150,150,160), 1);
    gfx_text(250, 70, "Chaqueta vieja     DEF+", RGB5(150,150,160), 1);
    gfx_text(250, 82, "Walkman roto       AGI+", RGB5(150,150,160), 1);
    gfx_text(250, 104, "OBJETOS", RGB5(200,60,60), 1);
    for (int i = 0; i < N_ITEMS; i++) {
        snprintf(b, sizeof b, "%c%-16s x%d", i == g_sel ? '>' : ' ', ITEM_NAME[i], pl.items[i]);
        gfx_text(250, 118 + i * 13, b, i == g_sel ? RGB5(250,250,240) : RGB5(110,110,120), 1);
    }
    gfx_text(250, 160, ITEM_DESC[g_sel], RGB5(170,150,100), 1);
    gfx_text(250, 180, "PSI:", RGB5(200,60,60), 1);
    for (int i = 0, k = 0; i < N_SKILLS; i++) if (pl.knows(i)) gfx_text(250, 192 + 11 * k++, SKILLS[i].name, RGB5(130,130,150), 1);
    if (g_msg_t > 0) gfx_text_c(236, g_msg, RGB5(220,120,60), 1);
    gfx_text_c(250, "X: USAR    [] / O: VOLVER", RGB5(80,80,90), 1);
}
/* Punto de Reflexion: aqui se sube de nivel (EXP + Sangre) */
static void frame_reflect(const Input &in) {
    static char rows[6][80];
    snprintf(rows[0], 80, "REFLEXIONAR: subir a Nv.%d (EXP %d/%d, SANGRE %d/%d)", pl.lv + 1, pl.exp, pl.exp_needed(), pl.blood, pl.blood_cost());
    snprintf(rows[1], 80, "DESCANSAR (cura, guarda, revive monstruos)");
    for (int i = 0; i < N_ITEMS; i++) snprintf(rows[2 + i], 80, "CANJEAR %s (%d SANGRE)", ITEM_NAME[i], ITEM_COST[i]);
    snprintf(rows[5], 80, "SALIR");
    const char *items[6] = { rows[0], rows[1], rows[2], rows[3], rows[4], rows[5] };
    if (in.pressed & IN_UP) g_sel = (g_sel + 5) % 6;
    if (in.pressed & IN_DOWN) g_sel = (g_sel + 1) % 6;
    if (in.pressed & IN_CIRCLE) g_state = ST_PLAY;
    if (in.pressed & IN_CROSS) {
        if (g_sel == 0) {
            if (pl.exp >= pl.exp_needed() && pl.blood >= pl.blood_cost()) {
                pl.level_up();
                if (g_story_idx < N_REFLECT && pl.lv >= REFLECT[g_story_idx].lv) start_dialog(REFLECT[g_story_idx++].text, ST_REFLECT);
                else say("Subes de nivel. Nada cambia.");
            } else say("FALTA EXP O SANGRE");
        } else if (g_sel == 1) {
            pl.hp = pl.maxhp; pl.sp = pl.maxsp; pl.clear_status();
            enemies_spawn(); flow_compute((int)(pl.x / TILE), (int)(pl.y / TILE));
            say(save_game(pl, g_diff, g_depth, g_last_pt, g_story_idx) ? "DESCANSAS... PARTIDA GUARDADA" : "DESCANSAS... (ERROR AL GUARDAR)");
        } else if (g_sel <= 4) {
            int it = g_sel - 2;
            if (pl.blood >= ITEM_COST[it]) { pl.blood -= ITEM_COST[it]; pl.items[it]++; say("CANJEADO"); } else say("SANGRE INSUFICIENTE");
        } else g_state = ST_PLAY;
    }
    if (g_msg_t > 0) g_msg_t--;
    if (g_state == ST_REFLECT) { draw_scene(true); draw_menu_box("PUNTO DE REFLEXION", items, 6, g_sel, 60, 440); }
}

/* ---------- Bucle ---------- */
static void game_frame(const Input &in) {
    g_frame++;
    switch (g_state) {
        case ST_TITLE:     frame_title(in); break;
        case ST_DIFF:      frame_diff(in); break;
        case ST_CUTSCENE:  cut.update(in); cut.draw(); if (cut.finished) new_game(); break;
        case ST_PLAY:      frame_play(in); break;
        case ST_DIALOG:    frame_dialog(in); break;
        case ST_MENU:      frame_menu(in); break;
        case ST_REFLECT:   frame_reflect(in); break;
        case ST_PAUSE:     frame_pause(in); break;
        case ST_ENCOUNTER: frame_encounter(); break;
        case ST_BATTLE:    frame_battle(in); break;
        case ST_GAMEOVER:  frame_gameover(); break;
    }
}

int main(int argc, char **argv) {
    plat_init(argc, argv);
    tex_init();
    pl.reset();
    g_rng ^= 0x9E3779B9u;
    while (plat_running()) {
        Input in;
        plat_poll(&in);
        game_frame(in);
        plat_present(g_fb);
    }
    plat_shutdown();
    return 0;
}
