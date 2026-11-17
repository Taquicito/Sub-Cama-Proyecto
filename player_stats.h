/* player_stats.h - Clases de combatiente, jugador, habilidades PSI, objetos,
 * bestiario y dificultades (multiplicador de EXP). */
#ifndef PLAYER_STATS_H
#define PLAYER_STATS_H

#include <string.h>
#include <stdio.h>
#include "render.h"

/* ---------- Elementos y estados alterados psicologicos ---------- */
enum { EL_PHYS, EL_LIGHT, EL_NOISE, EL_MEMORY, EL_COUNT };
enum { SX_DESPAIR, SX_APATHY, SX_TERROR, SX_COUNT };
static const char *const EL_NAME[EL_COUNT] = { "FISICO", "LUZ", "RUIDO", "RECUERDO" };
static const char *const SX_NAME[SX_COUNT] = { "DESESPERANZA", "APATIA", "TERROR" };
static const char *const SX_SHORT[SX_COUNT] = { "DESESP", "APATIA", "TERROR" };

/* ---------- Dificultad: modifica EXP y la agresividad ---------- */
struct DiffCfg {
    const char *name, *d1, *d2, *d3;
    float exp_mult;       /* multiplicador de EXP al ganar */
    float edmg, ehp;      /* dano y vida de enemigos */
    int light;            /* radio de luz (70-90 px) */
    int blood_loss;       /* % de Sangre perdida al caer */
    int exp_loss;         /* % de EXP perdida al caer */
    int drop;             /* % de probabilidad de objeto */
    int ai;               /* 0 predecible .. 3 explota debilidades */
    float espeed;         /* velocidad de los monstruos en el mapa */
};
static const DiffCfg DIFFS[4] = {
    { "FACIL",     "EXP x2.0. Enemigos predecibles.", "Recursos un poco mas abundantes.", "Luz amplia (90 px).",
      2.0f, 0.80f, 0.80f, 90, 0,  0, 60, 0, 0.80f },
    { "NORMAL",    "EXP x1.0. Balance estandar.", "Supervivencia y grindeo equilibrados.", "Pierdes 25% de Sangre al caer.",
      1.0f, 1.00f, 1.00f, 85, 25, 0, 40, 1, 1.00f },
    { "DIFICIL",   "EXP x0.7. Enemigos agresivos.", "Mayor costo de progresion.", "Al caer: -50% Sangre, -20% EXP.",
      0.7f, 1.15f, 1.20f, 78, 50, 20, 30, 2, 1.15f },
    { "PESADILLA", "EXP x0.4. Grindeo extremo.", "Explotan tus debilidades. Letales.", "Penumbra opresiva (70 px).",
      0.4f, 1.35f, 1.40f, 70, 75, 40, 20, 3, 1.30f }
};

/* ---------- Combatiente base (jugador y enemigos) ---------- */
class Combatant {
public:
    char name[24];
    int hp, maxhp, sp, maxsp, atk, def, agi, mag;
    int weak_mask, resist_mask;
    int status_t[SX_COUNT];

    Combatant() { clear(); }
    void clear() {
        name[0] = 0; hp = maxhp = 1; sp = maxsp = 0; atk = def = agi = mag = 1;
        weak_mask = resist_mask = 0;
        for (int i = 0; i < SX_COUNT; i++) status_t[i] = 0;
    }
    bool alive() const { return hp > 0; }
    bool has(int s) const { return status_t[s] > 0; }
    bool any_status() const { for (int i = 0; i < SX_COUNT; i++) if (status_t[i] > 0) return true; return false; }
    void add_status(int s, int turns) { status_t[s] = turns; }
    void clear_status() { for (int i = 0; i < SX_COUNT; i++) status_t[i] = 0; }
    void tick_status() { for (int i = 0; i < SX_COUNT; i++) if (status_t[i] > 0) status_t[i]--; }
    void heal(int v) { hp += v; if (hp > maxhp) hp = maxhp; }
    void heal_sp(int v) { sp += v; if (sp > maxsp) sp = maxsp; }
};

/* ---------- Habilidades PSI de Leo ---------- */
enum { TG_ONE, TG_ALL, TG_SELF };
enum { K_DMG, K_HEAL, K_CLEANSE };
struct Skill { const char *name; int el, power, cost, target, unlock, kind; const char *desc; };
#define N_SKILLS 5
static const Skill SKILLS[N_SKILLS] = {
    { "Luz de Linterna", EL_LIGHT,  10, 4, TG_ONE,  1, K_DMG,     "Dano de LUZ a un enemigo" },
    { "Respirar",        EL_PHYS,   22, 5, TG_SELF, 1, K_HEAL,    "Recupera vida" },
    { "Estatica",        EL_NOISE,   8, 6, TG_ALL,  3, K_DMG,     "Dano de RUIDO a todos" },
    { "Recuerdo Feliz",  EL_MEMORY, 20, 8, TG_ONE,  5, K_DMG,     "Dano fuerte de RECUERDO" },
    { "Aferrarse",       EL_PHYS,   10, 6, TG_SELF, 7, K_CLEANSE, "Cura estados y algo de vida" }
};

/* ---------- Objetos ---------- */
#define N_ITEMS 3
static const char *const ITEM_NAME[N_ITEMS] = { "Cafe Frio", "Caramelo Rancio", "Foto Vieja" };
static const char *const ITEM_DESC[N_ITEMS] = { "Recupera 35 de vida", "Recupera 12 SP", "Cura estados alterados" };
static const int ITEM_COST[N_ITEMS] = { 10, 8, 12 };    /* en Sangre, en puntos de reflexion */

/* ---------- Jugador ---------- */
class Player : public Combatant {
public:
    float x, y;                        /* posicion en el mapa */
    int lv, exp, blood, kills;
    int items[N_ITEMS];
    int known_weak[8];                 /* debilidades descubiertas por tipo de enemigo */

    void reset() {
        clear();
        x = y = 0;
        strcpy(name, "LEO");
        lv = 1; exp = 0; blood = 0; kills = 0;
        maxhp = hp = 40; maxsp = sp = 15;
        atk = 6; def = 3; agi = 6; mag = 5;
        weak_mask = 1 << EL_MEMORY;    /* su mente es su punto debil */
        items[0] = 3; items[1] = 2; items[2] = 1;
        for (int i = 0; i < 8; i++) known_weak[i] = 0;
    }
    int exp_needed() const { return 20 + lv * lv * 5; }
    int blood_cost() const { return 6 + lv * 4; }
    bool knows(int s) const { return lv >= SKILLS[s].unlock; }
    /* Se llama SOLO en un Punto de Reflexion: el nivel no trae esperanza */
    void level_up() {
        exp -= exp_needed(); blood -= blood_cost(); lv++;
        maxhp += 9; hp += 9; maxsp += 3; atk += 1; mag += 1;
        if (lv % 2 == 0) def += 1; else agi += 1;
    }
};

/* ---------- Bestiario: fragmentos de inseguridades y culpa ---------- */
struct EnemySkill { const char *name; int el, power, status, chance, turns; };
static const EnemySkill ESK[5] = {
    { "Embiste",      EL_PHYS,    0, -1,         0, 0 },
    { "Reproche",     EL_MEMORY, 10, SX_DESPAIR, 55, 3 },
    { "Grito",        EL_NOISE,   8, SX_TERROR,  55, 3 },
    { "Indiferencia", EL_MEMORY,  0, SX_APATHY,  75, 3 },
    { "Portazo",      EL_PHYS,   12, -1,         0, 0 }
};
struct EnemyDef {
    const char *name; int hp, atk, def, agi, mag, weak, resist, exp, blood, sk1, sk2; const char *defeat;
};
#define N_ETYPES 5
static const EnemyDef EDEFS[N_ETYPES] = {
    { "BOLETA REPROBADA", 22, 5, 2, 5, 3, EL_LIGHT,  EL_NOISE,  14, 4,  1, 0, "Una nota que nunca mostraste." },
    { "SILLA VACIA",      28, 4, 3, 4, 5, EL_NOISE,  EL_PHYS,   18, 5,  3, 1, "Nadie la ocupo. Nadie pregunto." },
    { "ECO PATERNO",      34, 8, 3, 7, 4, EL_LIGHT,  EL_MEMORY, 24, 7,  2, 4, "'A tu edad yo ya...' Se apaga." },
    { "ECO MATERNO",      30, 6, 4, 6, 8, EL_PHYS,   EL_NOISE,  26, 8,  1, 3, "Un suspiro largo. Sigue sonando." },
    { "PUERTA CERRADA",   52, 9, 7, 3, 3, EL_MEMORY, EL_PHYS,   38, 12, 4, 2, "Nunca la abriste. Ni ella a ti." }
};

#endif
