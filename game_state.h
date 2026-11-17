/* game_state.h - Datos del juego, dificultades, mundo procedural, guardado */
#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "render.h"

enum { ST_TITLE, ST_DIFF, ST_INTRO, ST_PLAY, ST_DIALOG, ST_STATUS, ST_REST, ST_PAUSE, ST_DEAD };

#define MAP_W 41
#define MAP_H 31
#define TILE 16
#define CELLS_W ((MAP_W - 1) / 2)
#define CELLS_H ((MAP_H - 1) / 2)
#define MAX_ENEMIES 40
#define MAX_DROPS 64
#define MAX_BONFIRES 6

/* ---------- Dificultad ---------- */
typedef struct {
    const char *name, *d1, *d2, *d3;
    int light;          /* radio de la luz en pixeles */
    float espeed, edmg, ehp, blood, lvcost;
    int rest_heal;      /* % de vida que cura descansar */
} DiffCfg;

static const DiffCfg DIFFS[2] = {
    { "PESADILLA", "Enemigos agresivos y veloces.", "Vision ultra reducida (22 px).", "Golpean mas fuerte.",
      22, 1.35f, 1.25f, 1.0f, 1.0f, 1.0f, 100 },
    { "ABISMO", "Mayor grindeo: enemigos con mas vida.", "Sangre escasa y niveles muy caros.", "Descansar solo cura 60%.",
      36, 1.0f, 1.0f, 1.6f, 0.55f, 1.7f, 60 }
};

/* ---------- Entidades ---------- */
typedef struct {
    float x, y;
    int hp, maxhp, lv, atk, def, vit, blood, facing;
    int atk_timer, inv_timer, kills;
} Player;
typedef struct { float x, y; int hp, maxhp, type, alive, hurt, babble_t, babble_i; } Enemy;
typedef struct { float x, y; int value, active; } Drop;
typedef struct { int tx, ty; } Bonfire;

static Player pl;
static Enemy en[MAX_ENEMIES];
static int n_en;
static Drop drops[MAX_DROPS];
static Bonfire bf[MAX_BONFIRES];
static int n_bf;
static uint8_t tiles[MAP_H][MAP_W];   /* 0 = suelo, 1 = pared */
static int16_t flow[MAP_H][MAP_W];    /* distancia BFS (en tiles) */
static int exit_tx = MAP_W - 2, exit_ty = MAP_H - 2;

static int g_state = ST_TITLE, g_diff = 0, g_depth = 0, g_story_idx = 0, g_last_bf = 0;
static int g_frame = 0;
static const char *dlg_text = ""; static int dlg_ret = ST_PLAY, dlg_t = 0;

/* ---------- Colision ---------- */
static int solid(float x, float y) {
    int tx = (int)floorf(x / TILE), ty = (int)floorf(y / TILE);
    if (tx < 0 || ty < 0 || tx >= MAP_W || ty >= MAP_H) return 1;
    return tiles[ty][tx];
}
static int blocked(float x, float y, int h) {
    return solid(x - h, y - h) || solid(x + h - 1, y - h) || solid(x - h, y + h - 1) || solid(x + h - 1, y + h - 1);
}
static void move_ent(float *x, float *y, float dx, float dy, int h) {
    if (!blocked(*x + dx, *y, h)) *x += dx;
    if (!blocked(*x, *y + dy, h)) *y += dy;
}

/* ---------- Campo de distancias (los monstruos siempre saben donde estas) ---------- */
static void flow_compute(int sx, int sy) {
    static int qx[MAP_W * MAP_H], qy[MAP_W * MAP_H];
    static const int DX[4] = { 0, 1, 0, -1 }, DY[4] = { -1, 0, 1, 0 };
    for (int y = 0; y < MAP_H; y++) for (int x = 0; x < MAP_W; x++) flow[y][x] = 9999;
    if (sx < 0 || sy < 0 || sx >= MAP_W || sy >= MAP_H || tiles[sy][sx]) return;
    int h = 0, t = 0;
    flow[sy][sx] = 0; qx[t] = sx; qy[t++] = sy;
    while (h < t) {
        int x = qx[h], y = qy[h++];
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (nx >= 0 && ny >= 0 && nx < MAP_W && ny < MAP_H && !tiles[ny][nx] && flow[ny][nx] == 9999) {
                flow[ny][nx] = flow[y][x] + 1; qx[t] = nx; qy[t++] = ny;
            }
        }
    }
}

/* ---------- Generacion del laberinto "Sub-Cama Sector 0" ---------- */
static void world_generate(int depth) {
    static const int DX[4] = { 0, 1, 0, -1 }, DY[4] = { -1, 0, 1, 0 };
    static uint8_t vis[CELLS_H][CELLS_W];
    static int stk[CELLS_W * CELLS_H][2];
    g_rng = 1337u + (uint32_t)depth * 977u;
    memset(tiles, 1, sizeof tiles);
    memset(vis, 0, sizeof vis);

    int sp = 0;                                   /* backtracker recursivo iterativo */
    vis[0][0] = 1; tiles[1][1] = 0; stk[sp][0] = 0; stk[sp][1] = 0; sp++;
    while (sp > 0) {
        int cx = stk[sp - 1][0], cy = stk[sp - 1][1], dirs[4], nd = 0;
        for (int d = 0; d < 4; d++) {
            int nx = cx + DX[d], ny = cy + DY[d];
            if (nx >= 0 && ny >= 0 && nx < CELLS_W && ny < CELLS_H && !vis[ny][nx]) dirs[nd++] = d;
        }
        if (nd == 0) { sp--; continue; }
        int d = dirs[rndr(nd)], nx = cx + DX[d], ny = cy + DY[d];
        tiles[cy * 2 + 1 + DY[d]][cx * 2 + 1 + DX[d]] = 0;
        tiles[ny * 2 + 1][nx * 2 + 1] = 0;
        vis[ny][nx] = 1; stk[sp][0] = nx; stk[sp][1] = ny; sp++;
    }
    for (int i = 0; i < 90; i++) {                /* bucles para no ser un laberinto perfecto */
        int x = 1 + rndr(MAP_W - 2), y = 1 + rndr(MAP_H - 2);
        if (tiles[y][x] && ((x & 1) != (y & 1))) tiles[y][x] = 0;
    }
    n_bf = 0;
    bf[n_bf].tx = 1; bf[n_bf].ty = 1; n_bf++;     /* hoguera 0 = inicio */
    for (int k = 0; k < 4; k++) {                 /* salas amplias con hogueras */
        int cx = 3 + 2 * rndr(17), cy = 3 + 2 * rndr(12);
        for (int dy = -2; dy <= 2; dy++)
            for (int dx = -2; dx <= 2; dx++) {
                int x = cx + dx, y = cy + dy;
                if (x >= 1 && y >= 1 && x < MAP_W - 1 && y < MAP_H - 1) tiles[y][x] = 0;
            }
        bf[n_bf].tx = cx; bf[n_bf].ty = cy; n_bf++;
    }
    tiles[exit_ty][exit_tx] = 0;
}

/* ---------- Guardado ---------- */
typedef struct { uint32_t magic; int32_t diff, depth, lv, atk, def, vit, blood, last_bf, story_idx; } SaveData;
#define SAVE_MAGIC 0x53434D41u

static int save_game(void) {
    FILE *f = fopen(g_save_path, "wb");
    if (!f) return 0;
    SaveData s = { SAVE_MAGIC, g_diff, g_depth, pl.lv, pl.atk, pl.def, pl.vit, pl.blood, g_last_bf, g_story_idx };
    int ok = fwrite(&s, sizeof s, 1, f) == 1;
    fclose(f);
    return ok;
}
static int save_read(SaveData *s) {
    FILE *f = fopen(g_save_path, "rb");
    if (!f) return 0;
    int ok = fread(s, sizeof *s, 1, f) == 1 && s->magic == SAVE_MAGIC;
    fclose(f);
    return ok;
}

#endif
