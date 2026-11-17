/* combat.h - Enemigos (IA), combate, drops de Sangre de Monstruo, niveles */
#ifndef COMBAT_H
#define COMBAT_H

#include "game_state.h"

/* Tipos: 0 Babeante, 1 Costurado, 2 Reptante (rapido), 3 Ojos (bruto) */
static const int   E_HP[4]    = { 8, 14, 10, 30 };
static const int   E_ATK[4]   = { 3, 5, 4, 9 };
static const int   E_DEF[4]   = { 0, 1, 0, 3 };
static const int   E_BLOOD[4] = { 4, 8, 7, 20 };
static const float E_SPD[4]   = { 0.55f, 0.70f, 1.00f, 0.50f };

#define N_BABBLE 10
static const char *const BABBLE[N_BABBLE] = {
    "ghhh...", "mmh-mh-mh", "kshhhhh", "ba-ba-ba", "hhhrrrr",
    "glrk glrk", "..eh..eh..", "nnnnn-", "leooo...", "shh-shh"
};

static int  g_blood_msg = 0, g_blood_msg_t = 0;   /* aviso "+N SANGRE" */
static int  g_shake = 0;

/* ---------- Jugador ---------- */
static void player_recalc(void) {
    pl.maxhp = 15 + pl.vit * 5;
    if (pl.hp > pl.maxhp) pl.hp = pl.maxhp;
}
static void player_new(void) {
    memset(&pl, 0, sizeof pl);
    pl.lv = 1; pl.atk = 3; pl.def = 2; pl.vit = 3; pl.facing = 2;
    player_recalc();
    pl.hp = pl.maxhp;
}
static void player_place(int bonfire) {
    if (bonfire < 0 || bonfire >= n_bf) bonfire = 0;
    pl.x = bf[bonfire].tx * TILE + TILE / 2; pl.y = bf[bonfire].ty * TILE + TILE / 2;
}

/* ---------- Enemigos ---------- */
static void enemies_spawn(void) {
    g_rng = 4242u + (uint32_t)g_depth * 31u;       /* mismas posiciones al reaparecer */
    flow_compute(bf[0].tx, bf[0].ty);
    n_en = 0;
    memset(drops, 0, sizeof drops);
    int want = 12 + g_depth * 3;
    if (want > MAX_ENEMIES) want = MAX_ENEMIES;
    for (int tries = 0; tries < 4000 && n_en < want; tries++) {
        int tx = 1 + rndr(MAP_W - 2), ty = 1 + rndr(MAP_H - 2);
        if (tiles[ty][tx] || flow[ty][tx] < 12 || flow[ty][tx] >= 9999) continue;
        int near_bf = 0;
        for (int b = 0; b < n_bf; b++)
            if (abs(bf[b].tx - tx) < 4 && abs(bf[b].ty - ty) < 4) near_bf = 1;
        if (near_bf) continue;
        int maxt = 1 + flow[ty][tx] / 25 + g_depth;
        if (maxt > 3) maxt = 3;
        Enemy *e = &en[n_en++];
        memset(e, 0, sizeof *e);
        e->type = rndr(maxt + 1);
        e->maxhp = (int)(E_HP[e->type] * (1.0f + 0.25f * g_depth) * DIFFS[g_diff].ehp);
        e->hp = e->maxhp;
        e->x = tx * TILE + TILE / 2; e->y = ty * TILE + TILE / 2;
        e->alive = 1; e->babble_t = rndr(60);
    }
}

static void player_hit(int dmg) {
    pl.hp -= dmg; pl.inv_timer = 50; g_shake = 8;
}
static void drop_blood(float x, float y, int value) {
    for (int i = 0; i < MAX_DROPS; i++)
        if (!drops[i].active) { drops[i].x = x; drops[i].y = y; drops[i].value = value; drops[i].active = 1; return; }
}
static void enemy_die(Enemy *e) {
    e->alive = 0; pl.kills++;
    int v = (int)(E_BLOOD[e->type] * DIFFS[g_diff].blood * (1.0f + 0.2f * g_depth) + 0.5f);
    drop_blood(e->x, e->y, v < 1 ? 1 : v);
}

/* Cruz: golpe frente al jugador */
static void player_attack(void) {
    static const int FX[4] = { 0, 1, 0, -1 }, FY[4] = { -1, 0, 1, 0 };
    if (pl.atk_timer > 0) return;
    pl.atk_timer = 20;
    float ax = pl.x + FX[pl.facing] * 13, ay = pl.y + FY[pl.facing] * 13;
    for (int i = 0; i < n_en; i++) {
        Enemy *e = &en[i];
        if (!e->alive || fabsf(e->x - ax) >= 14 || fabsf(e->y - ay) >= 14) continue;
        int dmg = pl.atk + rndr(3) - E_DEF[e->type] * (1 + g_depth / 2);
        if (dmg < 1) dmg = 1;
        e->hp -= dmg; e->hurt = 10;
        float dx = e->x - pl.x, dy = e->y - pl.y, l = sqrtf(dx * dx + dy * dy);
        if (l > 0.1f) move_ent(&e->x, &e->y, dx / l * 6, dy / l * 6, 5);
        if (e->hp <= 0) enemy_die(e);
    }
}

static void enemies_update(void) {
    static const int DX[4] = { 0, 1, 0, -1 }, DY[4] = { -1, 0, 1, 0 };
    for (int i = 0; i < n_en; i++) {
        Enemy *e = &en[i];
        if (!e->alive) continue;
        if (e->babble_t-- <= 0) { e->babble_t = 40 + rndr(50); e->babble_i = rndr(N_BABBLE); }
        if (e->hurt > 0) { e->hurt--; continue; }          /* aturdido */
        float spd = E_SPD[e->type] * DIFFS[g_diff].espeed * (1.0f + 0.04f * g_depth);
        float tgx = pl.x, tgy = pl.y;
        float dxp = pl.x - e->x, dyp = pl.y - e->y;
        if (dxp * dxp + dyp * dyp > 24 * 24) {             /* seguir el campo de distancias */
            int tx = (int)(e->x / TILE), ty = (int)(e->y / TILE);
            if (tx >= 0 && ty >= 0 && tx < MAP_W && ty < MAP_H && flow[ty][tx] < 9999) {
                int best = flow[ty][tx], bx = tx, by = ty;
                for (int d = 0; d < 4; d++) {
                    int nx = tx + DX[d], ny = ty + DY[d];
                    if (nx >= 0 && ny >= 0 && nx < MAP_W && ny < MAP_H && flow[ny][nx] < best) {
                        best = flow[ny][nx]; bx = nx; by = ny;
                    }
                }
                tgx = bx * TILE + TILE / 2; tgy = by * TILE + TILE / 2;
            }
        }
        float dx = tgx - e->x, dy = tgy - e->y, l = sqrtf(dx * dx + dy * dy);
        if (l > 0.5f) { dx /= l; dy /= l; } else { dx = dy = 0; }
        move_ent(&e->x, &e->y, dx * spd, dy * spd, 5);

        if (pl.inv_timer == 0 && fabsf(e->x - pl.x) < 9 && fabsf(e->y - pl.y) < 9) {
            int dmg = (int)(E_ATK[e->type] * (1.0f + 0.2f * g_depth) * DIFFS[g_diff].edmg) - pl.def / 2;
            player_hit(dmg < 1 ? 1 : dmg);
        }
    }
}

static void drops_update(void) {
    for (int i = 0; i < MAX_DROPS; i++) {
        if (!drops[i].active) continue;
        if (fabsf(drops[i].x - pl.x) < 10 && fabsf(drops[i].y - pl.y) < 10) {
            pl.blood += drops[i].value; g_blood_msg = drops[i].value; g_blood_msg_t = 60;
            drops[i].active = 0;
        }
    }
}

/* ---------- Niveles (se compran con Sangre en las hogueras) ---------- */
static int levelup_cost(void) { return (int)((8 + pl.lv * 6) * DIFFS[g_diff].lvcost); }
static int player_levelup(int stat) {                /* 0 ATK, 1 DEF, 2 VIT */
    int c = levelup_cost();
    if (pl.blood < c) return 0;
    pl.blood -= c; pl.lv++;
    if (stat == 0) pl.atk++;
    else if (stat == 1) pl.def++;
    else { pl.vit++; pl.hp += 5; }
    player_recalc();
    return 1;
}

/* ---------- Narrativa por nivel (sin esperanza) ---------- */
typedef struct { int lv; const char *text; } StoryLine;
static const StoryLine STORY[] = {
    {  2, "Cada golpe te hace mas fuerte.\nPor que te sientes mas pesado?" },
    {  3, "Hay recuerdos pegados a las paredes.\nUna voz de nino repite: 'fue tu culpa'." },
    {  5, "Los monstruos balbucean con tu voz.\nEscuchas tu nombre al reves." },
    {  7, "Recuerdas aquella noche.\nNo hiciste nada. Ese es el problema." },
    {  9, "Buscas la salida, pero los pasillos\nson las costuras de tu propio colchon." },
    { 12, "Cuanto mas luchas, mas hondo te hundes.\nEl colchon se alimenta de tu culpa." },
    { 15, "Ya no hay salida, Leo. Nunca la hubo.\nDespertar tambien es una cama." }
};
#define N_STORY (int)(sizeof STORY / sizeof STORY[0])

static const char *story_check(void) {
    if (g_story_idx < N_STORY && pl.lv >= STORY[g_story_idx].lv) return STORY[g_story_idx++].text;
    return NULL;
}

#endif
