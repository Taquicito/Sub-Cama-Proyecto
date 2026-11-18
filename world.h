/* world.h - Zonas del juego (mapas FIJOS, sin aleatoriedad entre partidas):
 *   Habitación, Ciudad (calles y plaza), Escuela/Biblioteca, Tienda y Zona Oscura.
 * Incluye texturas de baldosa, puertas, NPCs, colisión y BFS para los monstruos. */
#ifndef WORLD_H
#define WORLD_H

#include <math.h>
#include "render.h"

#define MAXW 48
#define MAXH 32
#define TILE 16
#define CELLS_W 20
#define CELLS_H 15

enum { Z_ROOM, Z_CITY, Z_SCHOOL, Z_SHOP, Z_DARK, N_ZONES };
enum { K_VOID, K_FLOOR, K_WALL, K_RUG, K_BED, K_DESK, K_SHELF, K_COUNTER, K_DOOR, K_ROAD, K_GRASS,
       K_SIDEWALK, K_BLDG, K_TREE, K_PLAZA, K_FOUNTAIN, K_STUDY, K_PORTAL, K_FLESHW, K_FLESHF,
       K_BENCH, K_LAMP, K_WINDOW, K_COUNT };

static uint8_t gmap[MAXH][MAXW];
static int map_w = 20, map_h = 12, g_zone = Z_ROOM;
static int16_t flow[MAXH][MAXW];
static int dark_sx = 2, dark_sy = 1;          /* casilla de aparición en la Zona Oscura */

static bool kind_solid(int k) {
    switch (k) {
        case K_FLOOR: case K_RUG: case K_DOOR: case K_ROAD: case K_GRASS: case K_SIDEWALK:
        case K_PLAZA: case K_PORTAL: case K_FLESHF: return false;
        default: return true;
    }
}
static int solid(float x, float y) {
    int tx = (int)floorf(x / TILE), ty = (int)floorf(y / TILE);
    if (tx < 0 || ty < 0 || tx >= map_w || ty >= map_h) return 1;
    return kind_solid(gmap[ty][tx]);
}
static int blocked(float x, float y, int h) {
    return solid(x - h, y - h) || solid(x + h - 1, y - h) || solid(x - h, y + h - 1) || solid(x + h - 1, y + h - 1);
}
static void move_ent(float *x, float *y, float dx, float dy, int h) {
    if (!blocked(*x + dx, *y, h)) *x += dx;
    if (!blocked(*x, *y + dy, h)) *y += dy;
}
static void flow_compute(int sx, int sy) {
    static int qx[MAXW * MAXH], qy[MAXW * MAXH];
    static const int DX[4] = { 0, 1, 0, -1 }, DY[4] = { -1, 0, 1, 0 };
    for (int y = 0; y < MAXH; y++) for (int x = 0; x < MAXW; x++) flow[y][x] = 9999;
    if (sx < 0 || sy < 0 || sx >= map_w || sy >= map_h || kind_solid(gmap[sy][sx])) return;
    int h = 0, t = 0;
    flow[sy][sx] = 0; qx[t] = sx; qy[t++] = sy;
    while (h < t) {
        int x = qx[h], y = qy[h++];
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (nx >= 0 && ny >= 0 && nx < map_w && ny < map_h && !kind_solid(gmap[ny][nx]) && flow[ny][nx] == 9999) {
                flow[ny][nx] = flow[y][x] + 1; qx[t] = nx; qy[t++] = ny;
            }
        }
    }
}

/* ---------- Puertas y NPCs ---------- */
struct DoorDef { int zone, tx, ty, to_zone, to_tx, to_ty; };
static const DoorDef DOORS[] = {
    { Z_ROOM, 9, 11, Z_CITY, 5, 10 },   { Z_ROOM, 10, 11, Z_CITY, 5, 10 },
    { Z_CITY, 5, 9, Z_ROOM, 9, 10 },
    { Z_CITY, 16, 9, Z_SCHOOL, 7, 10 }, { Z_CITY, 17, 9, Z_SCHOOL, 8, 10 },
    { Z_CITY, 27, 9, Z_SHOP, 6, 8 },
    { Z_SCHOOL, 7, 11, Z_CITY, 16, 10 }, { Z_SCHOOL, 8, 11, Z_CITY, 17, 10 },
    { Z_SHOP, 6, 9, Z_CITY, 27, 10 },   { Z_SHOP, 7, 9, Z_CITY, 27, 10 }
};
#define N_DOORS (int)(sizeof DOORS / sizeof DOORS[0])
static const DoorDef *door_at(int zone, int tx, int ty) {
    for (int i = 0; i < N_DOORS; i++) if (DOORS[i].zone == zone && DOORS[i].tx == tx && DOORS[i].ty == ty) return &DOORS[i];
    return 0;
}
/* id: 0 Marcos, 1 Valeria, 2 Sofía, 3 Don Ramiro */
struct NPCDef { int zone, tx, ty, id; };
static const NPCDef NPCS[] = { { Z_CITY, 14, 16, 0 }, { Z_CITY, 23, 17, 1 }, { Z_SCHOOL, 4, 6, 2 }, { Z_SHOP, 6, 2, 3 } };
#define N_NPCS (int)(sizeof NPCS / sizeof NPCS[0])
static bool npc_visible(const NPCDef &n, int slot) {
    if (n.zone == Z_CITY) return slot < 2;       /* los amigos se van a casa de noche */
    return true;
}
static bool building_open(int zone, int slot) { return !(zone == Z_SCHOOL || zone == Z_SHOP) || slot < 2; }

/* ---------- Construcción de zonas ---------- */
static void fillk(int k) { memset(gmap, k, sizeof gmap); }
static void rectk(int x0, int y0, int x1, int y1, int k) {
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) if (x >= 0 && y >= 0 && x < MAXW && y < MAXH) gmap[y][x] = (uint8_t)k;
}
static void dark_build() {
    static const int DX[4] = { 0, 1, 0, -1 }, DY[4] = { -1, 0, 1, 0 };
    static uint8_t vis[CELLS_H][CELLS_W];
    static int stk[CELLS_W * CELLS_H][2];
    g_rng = 777u;                                  /* semilla FIJA: el mismo laberinto siempre */
    fillk(K_FLESHW); map_w = 41; map_h = 31;
    memset(vis, 0, sizeof vis);
    int sp = 0;
    vis[0][0] = 1; gmap[1][1] = K_FLESHF; stk[sp][0] = 0; stk[sp][1] = 0; sp++;
    while (sp > 0) {
        int cx = stk[sp - 1][0], cy = stk[sp - 1][1], dirs[4], nd = 0;
        for (int d = 0; d < 4; d++) {
            int nx = cx + DX[d], ny = cy + DY[d];
            if (nx >= 0 && ny >= 0 && nx < CELLS_W && ny < CELLS_H && !vis[ny][nx]) dirs[nd++] = d;
        }
        if (nd == 0) { sp--; continue; }
        int d = dirs[rndr(nd)], nx = cx + DX[d], ny = cy + DY[d];
        gmap[cy * 2 + 1 + DY[d]][cx * 2 + 1 + DX[d]] = K_FLESHF;
        gmap[ny * 2 + 1][nx * 2 + 1] = K_FLESHF;
        vis[ny][nx] = 1; stk[sp][0] = nx; stk[sp][1] = ny; sp++;
    }
    for (int i = 0; i < 90; i++) {
        int x = 1 + rndr(map_w - 2), y = 1 + rndr(map_h - 2);
        if (gmap[y][x] == K_FLESHW && ((x & 1) != (y & 1))) gmap[y][x] = K_FLESHF;
    }
    for (int k = 0; k < 4; k++) {
        int cx = 3 + 2 * rndr(17), cy = 3 + 2 * rndr(12);
        for (int dy = -2; dy <= 2; dy++) for (int dx = -2; dx <= 2; dx++) {
            int x = cx + dx, y = cy + dy;
            if (x >= 1 && y >= 1 && x < map_w - 1 && y < map_h - 1) gmap[y][x] = K_FLESHF;
        }
    }
    gmap[1][1] = K_PORTAL;                         /* la grieta de luz: salida */
    dark_sx = 2; dark_sy = 1;
    if (gmap[1][2] != K_FLESHF) { dark_sx = 1; dark_sy = 2; }
}
static void zone_build(int z) {
    g_zone = z;
    if (z == Z_DARK) { dark_build(); return; }
    if (z == Z_ROOM) {
        fillk(K_WALL); map_w = 20; map_h = 12;
        rectk(1, 2, 18, 10, K_FLOOR);
        rectk(2, 3, 3, 4, K_BED);                  /* cama 2x2 */
        rectk(14, 3, 15, 3, K_DESK);               /* escritorio */
        rectk(9, 2, 11, 2, K_SHELF);
        rectk(7, 6, 12, 8, K_RUG);
        gmap[1][5] = K_WINDOW; gmap[1][6] = K_WINDOW; gmap[1][13] = K_WINDOW;
        gmap[11][9] = K_DOOR; gmap[11][10] = K_DOOR;
    } else if (z == Z_CITY) {
        fillk(K_GRASS); map_w = 40; map_h = 24;
        rectk(0, 0, 39, 0, K_TREE); rectk(0, 23, 39, 23, K_TREE); rectk(0, 0, 0, 23, K_TREE); rectk(39, 0, 39, 23, K_TREE);
        rectk(1, 10, 38, 10, K_SIDEWALK); rectk(1, 13, 38, 13, K_SIDEWALK);
        rectk(1, 11, 38, 12, K_ROAD);
        rectk(3, 4, 8, 9, K_BLDG); gmap[9][5] = K_DOOR;                       /* casa */
        rectk(12, 3, 21, 9, K_BLDG); gmap[9][16] = K_DOOR; gmap[9][17] = K_DOOR; /* escuela */
        rectk(25, 5, 30, 9, K_BLDG); gmap[9][27] = K_DOOR;                    /* tienda */
        rectk(10, 14, 28, 21, K_PLAZA);                                       /* plaza */
        rectk(18, 16, 19, 17, K_FOUNTAIN);
        gmap[15][13] = K_BENCH; gmap[15][24] = K_BENCH; gmap[20][13] = K_BENCH; gmap[20][24] = K_BENCH;
        gmap[14][11] = K_LAMP; gmap[14][27] = K_LAMP;
        static const int tr[][2] = { {3,16},{5,18},{7,15},{4,20},{7,20},{2,14},{34,3},{36,6},{33,7},{36,2},{10,5},{23,4},{32,3} };
        for (unsigned i = 0; i < sizeof tr / sizeof tr[0]; i++) gmap[tr[i][1]][tr[i][0]] = K_TREE;
        rectk(30, 14, 38, 22, K_BLDG);                                        /* manzana con callejón */
        rectk(34, 14, 34, 20, K_SIDEWALK); gmap[21][34] = K_PORTAL;
    } else if (z == Z_SCHOOL) {
        fillk(K_WALL); map_w = 16; map_h = 12;
        rectk(1, 2, 14, 10, K_FLOOR);
        rectk(1, 2, 5, 2, K_SHELF); rectk(10, 2, 14, 2, K_SHELF);
        rectk(11, 5, 12, 5, K_STUDY); rectk(3, 5, 3, 5, K_STUDY); rectk(11, 8, 12, 8, K_STUDY);
        gmap[1][4] = K_WINDOW; gmap[1][8] = K_WINDOW; gmap[1][12] = K_WINDOW;
        gmap[11][7] = K_DOOR; gmap[11][8] = K_DOOR;
    } else {
        fillk(K_WALL); map_w = 14; map_h = 10;
        rectk(1, 2, 12, 8, K_FLOOR);
        rectk(4, 3, 9, 3, K_COUNTER);
        rectk(1, 2, 2, 2, K_SHELF); rectk(11, 2, 12, 2, K_SHELF); rectk(1, 5, 1, 7, K_SHELF); rectk(12, 5, 12, 7, K_SHELF);
        gmap[1][6] = K_WINDOW;
        gmap[9][6] = K_DOOR; gmap[9][7] = K_DOOR;
    }
}

/* ---------- Texturas 16x16 por tipo de baldosa ---------- */
static pix tex[K_COUNT][5][256];
static pix tex_px(int k, int v, int lx, int ly);
static pix tex_base(int k, int v, int lx, int ly) { return tex_px(k, v, lx, ly); }
static pix tex_px(int k, int v, int lx, int ly) {
    uint32_t h = hash2(lx + v * 17 + k * 5, ly + v * 31 + k * 7);
    int n = (int)(h & 7), r = 0, g = 0, b = 0;
    switch (k) {
    case K_FLOOR: r = 118 + n; g = 84 + n; b = 58 + n;
        if (ly % 8 == 0) { r -= 30; g -= 24; b -= 18; }
        if (((lx + (ly / 8) * 7) % 16) == 0 && ly % 8 != 0) { r -= 20; g -= 16; b -= 12; }
        break;
    case K_WALL: r = 74 + n / 2; g = 70 + n / 2; b = 96 + n / 2;
        if (lx % 8 == 0) { r -= 8; g -= 8; b -= 8; }
        if (ly < 2) { r += 20; g += 20; b += 20; }
        if (ly > 13) { r -= 24; g -= 24; b -= 24; }
        break;
    case K_RUG: r = 130 + n; g = 44; b = 52;
        if (lx < 2 || lx > 13 || ly < 2 || ly > 13) { r = 170; g = 110; b = 70; } else if (((lx + ly) & 3) == 0) { r += 20; g += 8; }
        break;
    case K_BED: {
        bool right = v & 1, bottom = v & 2;
        bool frame = (bottom ? ly > 13 : ly < 2) || (right ? lx > 13 : lx < 2);
        if (frame) { r = 96; g = 64; b = 42; }
        else if (!right) { r = 215 - (ly > 8 ? 25 : 0); g = r; b = r + 8; if (lx > 12) { r -= 30; g -= 30; b -= 30; } }
        else { r = 56 + n; g = 100 + n; b = 125 + n; if ((lx % 5) == 0) { r -= 14; g -= 14; b -= 14; } }
        break; }
    case K_DESK:
        r = 140 + n; g = 100 + n; b = 64 + n;
        if (lx >= 3 && lx <= 12 && ly >= 1 && ly <= 8) { r = 34; g = 38; b = 40; if (lx >= 4 && lx <= 11 && ly >= 2 && ly <= 7) { r = 20; g = 44; b = 40; } }
        if (lx >= 6 && lx <= 9 && ly >= 9 && ly <= 10) { r = 70; g = 70; b = 74; }
        if (ly >= 12 && ly <= 14 && lx >= 3 && lx <= 12) { r = 170; g = 170; b = 176; if (lx % 2) { r = 120; g = 120; b = 126; } }
        break;
    case K_SHELF: r = 80 + n; g = 54 + n; b = 36 + n;
        if ((ly >= 2 && ly <= 6) || (ly >= 9 && ly <= 13)) {
            int c = (lx / 3 + ly / 8 + v) % 5; static const int P[5][3] = { {170,50,50},{50,90,160},{60,140,80},{200,170,60},{140,60,150} };
            r = P[c][0]; g = P[c][1]; b = P[c][2]; if (lx % 3 == 0) { r -= 30; g -= 30; b -= 30; }
        }
        break;
    case K_COUNTER: r = 160 + n; g = 128 + n; b = 90 + n;
        if (ly < 5) { r = 200 + n; g = 170 + n; b = 120 + n; } else if (ly > 13) { r -= 40; g -= 40; b -= 40; }
        break;
    case K_DOOR: r = 104 + n; g = 66 + n; b = 42 + n;
        if (lx < 2 || lx > 13 || ly < 2) { r = 60; g = 40; b = 26; }
        if (lx >= 11 && lx <= 12 && ly >= 8 && ly <= 9) { r = 220; g = 190; b = 80; }
        if (lx >= 4 && lx <= 11 && ((ly >= 4 && ly <= 7) || (ly >= 10 && ly <= 13))) { r -= 18; g -= 14; b -= 10; }
        break;
    case K_ROAD: r = 52 + n; g = 54 + n; b = 60 + n;
        if (v == 1 && ly >= 14 && lx < 9) { r = 205; g = 195; b = 90; }
        break;
    case K_GRASS: r = 62 + n; g = 112 + n; b = 62 + n;
        if (((h >> 8) & 15) == 0) { r = 40; g = 92; b = 46; } else if (((h >> 8) & 15) == 1) { r = 96; g = 152; b = 82; }
        break;
    case K_SIDEWALK: r = 156 + n; g = 154 + n; b = 150 + n;
        if (lx == 0 || ly == 0) { r = 120; g = 118; b = 114; }
        break;
    case K_BLDG: r = 150 + n; g = 84 + n; b = 72 + n;
        if (ly % 4 == 0) { r -= 34; g -= 26; b -= 22; }
        if (v == 0 && lx >= 3 && lx <= 12 && ly >= 3 && ly <= 11) {
            r = 100; g = 140; b = 180; if (lx == 7 || lx == 8 || ly == 7) { r = 232; g = 230; b = 220; }
        }
        break;
    case K_TREE: {
        pix bg = tex_base(K_GRASS, 0, lx, ly);
        r = (bg & 31) << 3; g = ((bg >> 5) & 31) << 3; b = ((bg >> 10) & 31) << 3;
        int dx = lx - 8, dy = ly - 6;
        if (dx * dx + dy * dy <= 40) { r = 34 + n; g = 92 + n; b = 46 + n; if (dx + dy < -4) { r += 20; g += 22; b += 14; } }
        if (lx >= 7 && lx <= 8 && ly >= 11) { r = 90; g = 60; b = 40; }
        break; }
    case K_PLAZA: r = 176 + n; g = 156 + n; b = 124 + n;
        if (lx == 0 || ly == 0) { r = 150; g = 132; b = 104; }
        if (v & 1) { r -= 8; g -= 8; b -= 8; }
        break;
    case K_FOUNTAIN: {
        bool right = v & 1, bottom = v & 2;
        bool ring = (bottom ? ly > 12 : ly < 3) || (right ? lx > 12 : lx < 3);
        if (ring) { r = 140; g = 140; b = 146; } else { r = 70 + n; g = 120 + n; b = 180 + n; if (((lx + ly) % 6) == 0) { r += 40; g += 40; b += 30; } }
        break; }
    case K_STUDY: r = 50; g = 110; b = 90;
        if (lx >= 3 && lx <= 12 && ly >= 3 && ly <= 10) { r = 232; g = 230; b = 214; if (ly % 3 == 0 && lx != 7 && lx != 8) { r = 120; g = 120; b = 130; } if (lx == 7 || lx == 8) { r = 190; g = 188; b = 176; } }
        if (ly > 12) { r = 110; g = 76; b = 50; }
        break;
    case K_PORTAL: {
        int dx = lx - 8, dy = ly - 8, d2 = dx * dx + dy * dy;
        if (v == 1) { r = 240 - d2 * 2; g = 240 - d2 * 2; b = 255 - d2; if (d2 > 90) { r = 60; g = 30; b = 40; } }
        else { r = 24 + n; g = 10; b = 30 + n; if (d2 < 50) { r = 90; g = 10; b = 50; } if (d2 < 20) { r = 10; g = 0; b = 12; } if (((lx * 3 + ly * 5) % 11) == 0) { r = 150; g = 20; b = 60; } }
        break; }
    case K_FLESHW: r = 96 + (h & 15); g = 28 + ((h >> 4) & 7); b = 38 + ((h >> 8) & 7);
        if (((lx / 3 + (ly >> 2) * 5 + v) & 7) == 0 && ((h >> 12) & 3) == 0) { r = 140; g = 40; b = 50; }
        if (lx == 0 || ly == 0) { r -= 30; g -= 10; b -= 10; }
        break;
    case K_FLESHF: {
        int base = 52 + (int)(h & 7); r = base + 10; g = base - 18; b = base - 8;
        if (v == 3 && ly == 8 && (lx & 3) < 2) { r = 30; g = 20; b = 24; }
        if (v == 4) { int dx = lx - 8, dy = ly - 8, d = dx * dx + dy * dy; if (d <= 9) { r = 34; g = 22; b = 28; } else if (d <= 16) { r += 14; g += 4; b += 4; } }
        break; }
    case K_BENCH: {
        pix bg = tex_base(K_GRASS, 0, lx, ly);
        r = (bg & 31) << 3; g = ((bg >> 5) & 31) << 3; b = ((bg >> 10) & 31) << 3;
        if (lx >= 1 && lx <= 14 && ly >= 5 && ly <= 10 && (ly % 3) != 0) { r = 132 + n; g = 92 + n; b = 56 + n; }
        if ((lx == 2 || lx == 13) && ly >= 10 && ly <= 14) { r = 70; g = 70; b = 76; }
        break; }
    case K_LAMP: {
        pix bg = tex_base(K_SIDEWALK, 0, lx, ly);
        r = (bg & 31) << 3; g = ((bg >> 5) & 31) << 3; b = ((bg >> 10) & 31) << 3;
        if (lx >= 7 && lx <= 8 && ly >= 3) { r = 60; g = 60; b = 66; }
        if (lx >= 5 && lx <= 10 && ly >= 1 && ly <= 4) { r = 240; g = 220; b = 130; }
        break; }
    case K_WINDOW: r = 74 + n / 2; g = 70 + n / 2; b = 96 + n / 2;
        if (lx >= 2 && lx <= 13 && ly >= 3 && ly <= 14) { r = 214; g = 204; b = 190; if (lx >= 4 && lx <= 11 && ly >= 5 && ly <= 12) { r = 120; g = 150; b = 200; if (lx == 7 || lx == 8 || ly == 8) { r = 214; g = 204; b = 190; } } }
        break;
    default: r = g = b = 0;
    }
    return rgb(r, g, b);
}
static void tex_init() {
    for (int k = 0; k < K_COUNT; k++)
        for (int v = 0; v < 5; v++)
            for (int ly = 0; ly < 16; ly++)
                for (int lx = 0; lx < 16; lx++) tex[k][v][ly * 16 + lx] = tex_px(k, v, lx, ly);
}
/* Variante de textura según la posición (ventanas de edificios, línea de carretera, cama, etc.) */
static int tile_variant(int tx, int ty) {
    int k = gmap[ty][tx];
    uint32_t h = hash2(tx, ty);
    switch (k) {
        case K_WALL: return (int)(h & 3);
        case K_BED: return ((tx > 0 && gmap[ty][tx - 1] == K_BED) ? 1 : 0) + ((ty > 0 && gmap[ty - 1][tx] == K_BED) ? 2 : 0);
        case K_FOUNTAIN: return ((tx > 0 && gmap[ty][tx - 1] == K_FOUNTAIN) ? 1 : 0) + ((ty > 0 && gmap[ty - 1][tx] == K_FOUNTAIN) ? 2 : 0);
        case K_ROAD: return (ty == 11 && (tx & 1) == 0) ? 1 : 0;
        case K_BLDG: return ((tx + ty) & 1) && ty < 9 && gmap[ty + 1][tx] == K_BLDG ? 0 : 1;
        case K_PLAZA: return (tx + ty) & 1;
        case K_PORTAL: return g_zone == Z_DARK ? 1 : 0;
        case K_SHELF: return (int)(h & 3);
        case K_FLESHW: return (int)(h & 3);
        case K_FLESHF: return (((tx * 7 + ty * 13) % 11) == 0) ? 4 : (int)(h & 3);
        default: return 0;
    }
}
/* Tinte ambiental (1 = tarde cálida, 2 = noche azulada) aplicado a las texturas UNA vez al cambiar de franja:
 * así el color del ambiente no cuesta nada por cuadro. */
static pix tex_t[K_COUNT][5][256];
static int tex_mode = -1;
static void tex_set_tint(int mode) {
    if (mode == tex_mode) return;
    tex_mode = mode;
    for (int k = 0; k < K_COUNT; k++)
        for (int v = 0; v < 5; v++)
            for (int i = 0; i < 256; i++) {
                pix c = tex[k][v][i];
                int r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
                if (mode == 1) { g = g * 27 / 32; b = b * 19 / 32; }
                else if (mode == 2) { r = r * 18 / 32; g = g * 21 / 32; b = b * 28 / 32; }
                tex_t[k][v][i] = (pix)(0x8000 | (b << 10) | (g << 5) | r);
            }
}
static void draw_tiles(int camx, int camy) {
    int tx0 = camx >> 4, ty0 = camy >> 4;
    if (tx0 < 0) tx0 = 0;
    if (ty0 < 0) ty0 = 0;
    for (int ty = ty0; ty <= ty0 + SCR_H / TILE + 1 && ty < map_h; ty++)
        for (int tx = tx0; tx <= tx0 + SCR_W / TILE + 1 && tx < map_w; tx++) {
            const pix *t = tex_t[gmap[ty][tx]][tile_variant(tx, ty)];
            int dx = tx * TILE - camx, dy = ty * TILE - camy;
            int sx0 = dx < 0 ? -dx : 0, ex = dx + 16 > SCR_W ? SCR_W - dx : 16;
            if (ex <= sx0) continue;
            for (int r = 0; r < 16; r++) {
                int y = dy + r;
                if (y < 0 || y >= SCR_H) continue;
                memcpy(&g_fb[y * BUF_W + dx + sx0], &t[r * 16 + sx0], (size_t)(ex - sx0) * 2);
            }
        }
}

#endif
