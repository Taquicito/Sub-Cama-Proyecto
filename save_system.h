/* save_system.h - Guardado múltiple (3 ranuras: slot1.sav, slot2.sav, slot3.sav).
 *
 * ESTRUCTURA: SaveData es una "foto" completa y plana de la partida:
 *   jugador (stats, equipo, objetos), calendario (día/franja/flags), amigos,
 *   ubicación exacta (zona + posición) y monstruos derrotados en la Zona Oscura.
 * pack() copia el estado vivo -> SaveData; unpack() lo restaura. El archivo es
 * binario (en PSP) y lleva un número mágico y versión para rechazar basura.
 * Para añadir datos nuevos: agrega el campo a SaveData, súbele SAVE_VERSION,
 * y actualiza pack()/unpack(). */
#ifndef SAVE_SYSTEM_H
#define SAVE_SYSTEM_H

#include <stdio.h>
#include "platform.h"
#include "player_stats.h"
#include "day_system.h"

#define N_SLOTS 3
#define SAVE_MAGIC 0x53434D43u
#define SAVE_VERSION 3

struct SaveData {
    uint32_t magic, version;
    int32_t diff, zone, in_bed, dz_killed, playsecs;
    int32_t px, py;
    int32_t lv, exp, notas, kills, base_maxhp, base_def, base_mag, maxsp, atk, agi, hp, sp, study, battery, armor_eq;
    int32_t items[N_ITEMS], armor_tier[N_ARMORS], known_weak[8];
    int32_t day, slot, hw_done, study_today, talked_mask, dark_today, intro_done, boss_down, study_bonus, bond[N_FRIENDS];
};

static void pack(SaveData &s, const Player &p, const DaySystem &d, int diff, int zone, int in_bed, uint32_t killed, int playsecs) {
    memset(&s, 0, sizeof s);
    s.magic = SAVE_MAGIC; s.version = SAVE_VERSION;
    s.diff = diff; s.zone = zone; s.in_bed = in_bed; s.dz_killed = (int32_t)killed; s.playsecs = playsecs;
    s.px = (int32_t)p.x; s.py = (int32_t)p.y;
    s.lv = p.lv; s.exp = p.exp; s.notas = p.notas; s.kills = p.kills; s.base_maxhp = p.base_maxhp; s.base_def = p.base_def;
    s.base_mag = p.base_mag; s.maxsp = p.maxsp; s.atk = p.atk; s.agi = p.agi; s.hp = p.hp; s.sp = p.sp;
    s.study = p.study; s.battery = p.battery; s.armor_eq = p.armor_eq;
    for (int i = 0; i < N_ITEMS; i++) s.items[i] = p.items[i];
    for (int i = 0; i < N_ARMORS; i++) s.armor_tier[i] = p.armor_tier[i];
    for (int i = 0; i < 8; i++) s.known_weak[i] = p.known_weak[i];
    s.day = d.day; s.slot = d.slot; s.hw_done = d.hw_done; s.study_today = d.study_today; s.talked_mask = d.talked_mask;
    s.dark_today = d.dark_today; s.intro_done = d.intro_done; s.boss_down = d.boss_down; s.study_bonus = d.study_bonus;
    for (int i = 0; i < N_FRIENDS; i++) s.bond[i] = d.bond[i];
}
static void unpack(const SaveData &s, Player &p, DaySystem &d, int &diff, int &zone, int &in_bed, uint32_t &killed, int &playsecs) {
    p.reset();
    diff = s.diff & 3; zone = s.zone; in_bed = s.in_bed; killed = (uint32_t)s.dz_killed; playsecs = s.playsecs;
    p.x = (float)s.px; p.y = (float)s.py;
    p.lv = s.lv; p.exp = s.exp; p.notas = s.notas; p.kills = s.kills; p.base_maxhp = s.base_maxhp; p.base_def = s.base_def;
    p.base_mag = s.base_mag; p.maxsp = s.maxsp; p.atk = s.atk; p.agi = s.agi; p.study = s.study; p.battery = s.battery;
    p.armor_eq = s.armor_eq;
    for (int i = 0; i < N_ITEMS; i++) p.items[i] = s.items[i];
    for (int i = 0; i < N_ARMORS; i++) p.armor_tier[i] = s.armor_tier[i];
    for (int i = 0; i < 8; i++) p.known_weak[i] = s.known_weak[i];
    p.recalc(); p.hp = s.hp; if (p.hp > p.maxhp) p.hp = p.maxhp; p.sp = s.sp;
    d.day = s.day; d.slot = s.slot; d.hw_done = s.hw_done; d.study_today = s.study_today; d.talked_mask = s.talked_mask;
    d.dark_today = s.dark_today; d.intro_done = s.intro_done; d.boss_down = s.boss_down; d.study_bonus = s.study_bonus;
    for (int i = 0; i < N_FRIENDS; i++) d.bond[i] = s.bond[i];
}

static bool save_write(int slot, const SaveData &s) {
    char path[256]; save_path(slot, path, sizeof path);
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    bool ok = fwrite(&s, sizeof s, 1, f) == 1;
    fclose(f);
    return ok;
}
static bool save_read(int slot, SaveData &s) {
    char path[256]; save_path(slot, path, sizeof path);
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    bool ok = fread(&s, sizeof s, 1, f) == 1 && s.magic == SAVE_MAGIC && s.version == SAVE_VERSION;
    fclose(f);
    return ok;
}

#endif
