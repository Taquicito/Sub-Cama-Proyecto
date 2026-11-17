/* world.h - Laberinto procedural "Sub-Cama Sector 0", colision, BFS y guardado */
#ifndef WORLD_H
#define WORLD_H

#include <stdio.h>
#include "render.h"
#include "platform.h"
#include "player_stats.h"

#define MAP_W 41
#define MAP_H 31
#define TILE 16
#define CELLS_W ((MAP_W - 1) / 2)
#define CELLS_H ((MAP_H - 1) / 2)
#define MAX_POINTS 6

struct Point { int tx, ty; };
static Point points[MAX_POINTS];            /* Puntos de Reflexion (hogueras decadentes) */
static int n_points;
static uint8_t tiles[MAP_H][MAP_W];         /* 0 suelo, 1 pared de carne */
static int16_t flow[MAP_H][MAP_W];          /* distancia BFS */
static const int exit_tx = MAP_W - 2, exit_ty = MAP_H - 2;

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

static void world_generate(int depth) {
    static const int DX[4] = { 0, 1, 0, -1 }, DY[4] = { -1, 0, 1, 0 };
    static uint8_t vis[CELLS_H][CELLS_W];
    static int stk[CELLS_W * CELLS_H][2];
    g_rng = 1337u + (uint32_t)depth * 977u;
    memset(tiles, 1, sizeof tiles);
    memset(vis, 0, sizeof vis);
    int sp = 0;
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
    for (int i = 0; i < 90; i++) {
        int x = 1 + rndr(MAP_W - 2), y = 1 + rndr(MAP_H - 2);
        if (tiles[y][x] && ((x & 1) != (y & 1))) tiles[y][x] = 0;
    }
    n_points = 0;
    points[n_points].tx = 1; points[n_points].ty = 1; n_points++;
    for (int k = 0; k < 4; k++) {
        int cx = 3 + 2 * rndr(17), cy = 3 + 2 * rndr(12);
        for (int dy = -2; dy <= 2; dy++)
            for (int dx = -2; dx <= 2; dx++) {
                int x = cx + dx, y = cy + dy;
                if (x >= 1 && y >= 1 && x < MAP_W - 1 && y < MAP_H - 1) tiles[y][x] = 0;
            }
        points[n_points].tx = cx; points[n_points].ty = cy; n_points++;
    }
    tiles[exit_ty][exit_tx] = 0;
}

/* ---------- Guardado ---------- */
struct SaveData {
    uint32_t magic;
    int32_t diff, depth, lv, exp, blood, maxhp, maxsp, atk, def, agi, mag, items[N_ITEMS], last_pt, story_idx, kills;
};
#define SAVE_MAGIC 0x53434D42u

static int save_game(const Player &p, int diff, int depth, int last_pt, int story_idx) {
    FILE *f = fopen(g_save_path, "wb");
    if (!f) return 0;
    SaveData s;
    memset(&s, 0, sizeof s);
    s.magic = SAVE_MAGIC; s.diff = diff; s.depth = depth; s.lv = p.lv; s.exp = p.exp; s.blood = p.blood;
    s.maxhp = p.maxhp; s.maxsp = p.maxsp; s.atk = p.atk; s.def = p.def; s.agi = p.agi; s.mag = p.mag;
    for (int i = 0; i < N_ITEMS; i++) s.items[i] = p.items[i];
    s.last_pt = last_pt; s.story_idx = story_idx; s.kills = p.kills;
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
