/* player_stats.h - Combatientes, jugador, armaduras, habilidades, objetos,
 * bestiario y dificultades (multiplicador de EXP). */
#ifndef PLAYER_STATS_H
#define PLAYER_STATS_H

#include <string.h>
#include <stdio.h>
#include "render.h"

/* ---------- Elementos y estados alterados psicológicos ---------- */
enum { EL_PHYS, EL_LIGHT, EL_NOISE, EL_MEMORY, EL_COUNT };
enum { SX_DESPAIR, SX_APATHY, SX_TERROR, SX_COUNT };
static const char *const EL_NAME[EL_COUNT] = { "FÍSICO", "LUZ", "RUIDO", "RECUERDO" };
static const char *const SX_NAME[SX_COUNT] = { "DESESPERANZA", "APATÍA", "TERROR" };
static const char *const SX_SHORT[SX_COUNT] = { "DESESP.", "APATÍA", "TERROR" };

/* ---------- Dificultad ---------- */
struct DiffCfg {
    const char *name, *d1, *d2, *d3;
    float exp_mult;       /* multiplicador de EXP al ganar */
    float edmg, ehp;      /* daño y vida de los enemigos */
    int light;            /* radio base de la luz (70-90 px) */
    int notes_loss;       /* % de Notas que se pierden al desmayarse */
    int drop;             /* % de probabilidad de objeto */
    int ai;               /* 0 predecible .. 3 explota debilidades */
    float espeed;         /* velocidad de los monstruos en el mapa */
};
static const DiffCfg DIFFS[4] = {
    { "FÁCIL",     "EXP x2.0. Enemigos más predecibles.", "Recursos un poco más abundantes.", "Luz amplia (90 px). Sin penalización.",
      2.0f, 0.80f, 0.80f, 90, 0,  60, 0, 0.80f },
    { "NORMAL",    "EXP x1.0. Balance estándar.", "Supervivencia y grindeo equilibrados.", "Al desmayarte pierdes 20% de Notas.",
      1.0f, 1.00f, 1.00f, 85, 20, 40, 1, 1.00f },
    { "DIFÍCIL",   "EXP x0.7. Enemigos agresivos.", "Mayor costo de progresión.", "Al desmayarte pierdes 40% de Notas.",
      0.7f, 1.15f, 1.20f, 78, 40, 30, 2, 1.15f },
    { "PESADILLA", "EXP x0.4. Grindeo extremo.", "Enemigos letales que explotan tus debilidades.", "Penumbra opresiva (70 px). Pierdes 60%.",
      0.4f, 1.35f, 1.40f, 70, 60, 20, 3, 1.30f }
};

/* ---------- Calendario ---------- */
#define N_DAYS 8
static const char *const DAY_NAME[N_DAYS] = { "Domingo", "Lunes", "Martes", "Miércoles", "Jueves", "Viernes", "Sábado", "Domingo" };
static const char *const SLOT_NAME[3] = { "Mañana", "Tarde", "Noche" };

/* ---------- Combatiente base ---------- */
class Combatant {
public:
    char name[28];
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

/* ---------- Habilidades activas de Leo ---------- */
enum { TG_ONE, TG_ALL, TG_SELF };
enum { K_DMG, K_HEAL, K_CLEANSE };
struct Skill { const char *name; int el, power, cost, target, unlock, kind; const char *desc; };
#define N_SKILLS 5
static const Skill SKILLS[N_SKILLS] = {
    { "Luz de Linterna", EL_LIGHT,  10, 4, TG_ONE,  1, K_DMG,     "Daño de LUZ a un enemigo" },
    { "Respirar",        EL_PHYS,   22, 5, TG_SELF, 1, K_HEAL,    "Recupera vida" },
    { "Estática",        EL_NOISE,   8, 6, TG_ALL,  3, K_DMG,     "Daño de RUIDO a todos" },
    { "Recuerdo Feliz",  EL_MEMORY, 20, 8, TG_ONE,  5, K_DMG,     "Daño fuerte de RECUERDO" },
    { "Aferrarse",       EL_PHYS,   10, 6, TG_SELF, 7, K_CLEANSE, "Cura estados y algo de vida" }
};

/* ---------- Objetos ---------- */
#define N_ITEMS 4
static const char *const ITEM_NAME[N_ITEMS] = { "Café Frío", "Caramelo Rancio", "Foto Vieja", "Pilas" };
static const char *const ITEM_DESC[N_ITEMS] = { "Recupera 35 de vida", "Recupera 12 SP", "Cura estados alterados", "Recarga 40% de la linterna" };
static const int ITEM_COST[N_ITEMS] = { 12, 10, 14, 15 };      /* en Notas, en la tienda */

/* ---------- Armaduras temáticas (se compran y se mejoran en la tienda) ---------- */
#define N_ARMORS 4
struct ArmorDef { const char *name, *desc; int cost, up_cost[3], req_study[3], buy_study; };
static const ArmorDef ARMORS[N_ARMORS] = {
    { "Armadura de Plantas",       "Regenera vida cada turno y resiste un poco.", 40, { 25, 50, 90 },   { 2, 6, 12 },  0 },
    { "Armadura de Huesos",        "Defensa física sólida.",                      80, { 40, 80, 140 },  { 3, 8, 14 },  0 },
    { "Armadura de Carne",         "Gran vitalidad: el tanque ideal.",            80, { 40, 80, 140 },  { 3, 8, 14 },  0 },
    { "Armadura de Boletas Buenas","El mejor equipo: escala con tus notas.",     160, { 60, 110, 180 }, { 4, 10, 16 }, 8 }
};

/* ---------- Jugador ---------- */
class Player : public Combatant {
public:
    float x, y;                         /* posición en el mapa */
    int lv, exp, notas, kills;
    int base_maxhp, base_def, base_mag; /* sin equipo */
    int study;                          /* puntos académicos (escalan la armadura de Boletas) */
    int battery;                        /* luz de la linterna 0..100 */
    int items[N_ITEMS];
    int armor_tier[N_ARMORS];           /* -1 = no la tienes, 0..3 = nivel de mejora */
    int armor_eq;                       /* armadura equipada o -1 */
    int regen;                          /* regeneración por turno (armadura de Plantas) */
    int known_weak[8];                  /* debilidades descubiertas por tipo de enemigo */

    void reset() {
        clear();
        x = y = 0;
        strcpy(name, "LEO");
        lv = 1; exp = 0; notas = 20; kills = 0; study = 0; battery = 100;
        base_maxhp = 40; base_def = 3; base_mag = 5;
        maxsp = sp = 15; atk = 6; agi = 6;
        weak_mask = 1 << EL_MEMORY;     /* su mente es su punto débil */
        items[0] = 2; items[1] = 1; items[2] = 1; items[3] = 1;
        for (int i = 0; i < N_ARMORS; i++) armor_tier[i] = -1;
        armor_eq = -1; regen = 0;
        for (int i = 0; i < 8; i++) known_weak[i] = 0;
        recalc();
        hp = maxhp;
    }
    /* Estadísticas efectivas = base + armadura equipada */
    void recalc() {
        maxhp = base_maxhp; def = base_def; mag = base_mag; regen = 0;
        if (armor_eq >= 0 && armor_tier[armor_eq] >= 0) {
            int t = armor_tier[armor_eq];
            switch (armor_eq) {
                case 0: def += 2 + t; regen = 1 + t; break;
                case 1: def += 5 + 2 * t; break;
                case 2: maxhp += 20 + 10 * t; def += 1 + t; break;
                case 3: def += 3 + t + study / 3; mag += 1 + t + study / 4; maxhp += study; break;
            }
        }
        if (hp > maxhp) hp = maxhp;
    }
    int exp_needed() const { return 24 + lv * lv * 3; }
    float average() const { float a = 5.0f + study * 0.25f; return a > 10.0f ? 10.0f : a; }   /* "Calificación" */
    bool knows(int s) const { return lv >= SKILLS[s].unlock; }

    /* Subida de nivel AUTOMÁTICA. Devuelve cuántos niveles subió. */
    int gain_exp(int amount) {
        exp += amount;
        int n = 0;
        while (exp >= exp_needed()) {
            exp -= exp_needed(); lv++; n++;
            base_maxhp += 9; maxsp += 3; atk += 1; base_mag += 1;
            if (lv % 2 == 0) base_def += 1; else agi += 1;
            sp = maxsp;
        }
        if (n) { int old = maxhp; recalc(); hp += maxhp - old; if (hp > maxhp) hp = maxhp; }
        return n;
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
    const char *name; int hp, atk, def, agi, mag, weak, resist, exp, notas, sk1, sk2, unlock_day; const char *defeat;
};
#define N_ETYPES 6
#define BOSS_TYPE 5
static const EnemyDef EDEFS[N_ETYPES] = {
    { "BOLETA REPROBADA", 22, 5, 2, 5, 3, EL_LIGHT,  EL_NOISE,  14, 4,  1, 0, 0, "Una nota que nunca mostraste." },
    { "SILLA VACÍA",      28, 4, 3, 4, 5, EL_NOISE,  EL_PHYS,   18, 5,  3, 1, 0, "Nadie la ocupó. Nadie preguntó." },
    { "ECO PATERNO",      34, 8, 3, 7, 4, EL_LIGHT,  EL_MEMORY, 24, 7,  2, 4, 2, "'A tu edad yo ya...' Se apaga." },
    { "ECO MATERNO",      30, 6, 4, 6, 8, EL_PHYS,   EL_NOISE,  26, 8,  1, 3, 4, "Un suspiro largo. Sigue sonando." },
    { "PUERTA CERRADA",   52, 9, 7, 3, 3, EL_MEMORY, EL_PHYS,   38, 12, 4, 2, 5, "Nunca la abriste. Ni ella a ti." },
    { "LA CENA FRÍA",    150, 11, 6, 6, 9, EL_LIGHT, EL_NOISE, 120, 60, 1, 2, 7, "La mesa queda vacía. Y en silencio." }
};

#endif
