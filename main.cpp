/* SUB-CAMA: SECTOR 0 - Una semana (Domingo a Domingo) - PSP / PPSSPP
 * main.cpp: estados del juego, exploración de zonas, interacciones y bucle principal.
 * Módulos: day_system.h (calendario)  save_system.h (ranuras)  world.h (zonas)
 *          battle_system.h (combate)  narrative_engine.h (historia)  lighting.h (luz)
 *          player_stats.h (RPG)  assets.h  render.h  platform.h */
#include <math.h>
#include "render.h"
#include "platform.h"
#include "player_stats.h"
#include "day_system.h"
#include "lighting.h"
#include "assets.h"
#include "narrative_engine.h"
#include "battle_system.h"
#include "world.h"
#include "save_system.h"

enum { ST_TITLE, ST_DIFF, ST_LOADMENU, ST_CUTSCENE, ST_PLAY, ST_DIALOG, ST_MENU, ST_PAUSE, ST_CHOICE, ST_SHOP,
       ST_ENCOUNTER, ST_BATTLE, ST_FAINT, ST_DAYCARD, ST_ENDING };
enum { CH_SLEEP, CH_HOMEWORK, CH_STUDY, CH_PORTAL, CH_SLOT };
enum { TAB_STATE, TAB_INV, TAB_SKILLS, TAB_TASKS, TAB_SAVE, N_TABS };

static Player pl;
static DaySystem ds;
static Battle battle;
static Lighting light;
static Cutscene cut;
static DialogBox dlg;

static int g_state = ST_TITLE, g_diff = 1, g_frame = 0, g_playframes = 0;
static int dlg_ret = ST_PLAY, g_sel = 0, g_msg_t = 0, safe_t = 0, enc_t = 0, faint_t = 0, face = 2, g_fade = 0, grace = 0;
static int menu_tab = 0, menu_sel = 0, shop_sel = 0, card_t = 0, end_page = 0, bat_acc = 0, load_sel = 0;
static bool in_bed = true, pending_time = false;
static uint32_t dz_killed = 0;
static const char *g_msg = "";
static char card_rep[4][96]; static int card_n = 0;
static char talk_buf[256], act_buf[256];

/* Elección genérica (popup de opciones) */
static int choice_id = 0, choice_n = 0, choice_sel = 0, choice_ret = ST_PLAY, choice_arg = 0;
static char choice_title[120]; static const char *choice_opts[4];

/* Monstruos y notas en la Zona Oscura */
struct OWEnemy { float x, y; int type, alive, stun, babble_t, babble_i; };
struct Note { int tx, ty, idx; };
#define MAX_OW 32
#define MAX_NOTES 5
static OWEnemy ow[MAX_OW]; static int n_ow = 0;
static Note notes[MAX_NOTES]; static int n_notes = 0;
static int enc_idx[3], enc_types[3], enc_n;

static const char *const BABBLE[10] = {
    "le...o...", "otra vez...", "ngh... ngh...", "hhhaaah", "no-no-NO",
    "¿por qué... tú?", "mmh-mh-mh", "esfuérzate...", "kshhh...", "no sirves..."
};
static const float OW_SPEED[6] = { 0.50f, 0.45f, 0.70f, 0.55f, 0.40f, 0.35f };

static void say(const char *m) { g_msg = m; g_msg_t = 120; }
static void start_dialog(const char *txt, int ret, const char *who = "") { dlg.begin(txt, who); dlg_ret = ret; g_state = ST_DIALOG; }
static void open_choice(int id, const char *title, const char *a, const char *b, const char *c = 0) {
    choice_id = id; choice_sel = 0; choice_n = c ? 3 : 2; choice_ret = g_state;
    strncpy(choice_title, title, sizeof choice_title - 1); choice_title[sizeof choice_title - 1] = 0;
    choice_opts[0] = a; choice_opts[1] = b; choice_opts[2] = c; g_state = ST_CHOICE;
}
static void ui_box(int x, int y, int w, int h) {
    gfx_rect(x, y, w, h, RGB5(8,6,12)); gfx_frame(x, y, w, h, RGB5(150,150,160)); gfx_frame(x + 2, y + 2, w - 4, h - 4, RGB5(50,50,60));
}

/* ======================= Zonas y partida ======================= */
static void dz_spawn() {                      /* monstruos de la Zona Oscura (posiciones fijas, tipos según el día) */
    n_ow = 0; n_notes = 0;
    flow_compute(dark_sx, dark_sy);
    int tx[MAX_OW], ty[MAX_OW], n = 0, want = 10 + ds.day;
    g_rng = 4242u;
    for (int tries = 0; tries < 6000 && n < want; tries++) {
        int x = 1 + rndr(map_w - 2), y = 1 + rndr(map_h - 2);
        if (kind_solid(gmap[y][x]) || flow[y][x] < 12 || flow[y][x] >= 9999) continue;
        bool dup = false;
        for (int i = 0; i < n; i++) if (abs(tx[i] - x) < 3 && abs(ty[i] - y) < 3) dup = true;
        if (dup) continue;
        tx[n] = x; ty[n] = y; n++;
    }
    g_rng = 9000u + (uint32_t)ds.day * 17u;
    int maxt = ds.max_enemy_type();
    for (int i = 0; i < n; i++) {
        OWEnemy &e = ow[n_ow++];
        e.x = tx[i] * TILE + 8.0f; e.y = ty[i] * TILE + 8.0f; e.type = rndr(maxt + 1);
        e.alive = 1; e.stun = 0; e.babble_t = rndr(60); e.babble_i = rndr(10);
    }
    if (ds.day == N_DAYS - 1 && !ds.boss_down) {              /* jefe final: al fondo del laberinto */
        int bx = 1, by = 1, bd = -1;
        for (int y = 0; y < map_h; y++) for (int x = 0; x < map_w; x++) if (flow[y][x] < 9999 && flow[y][x] > bd) { bd = flow[y][x]; bx = x; by = y; }
        OWEnemy &e = ow[n_ow++];
        e.x = bx * TILE + 8.0f; e.y = by * TILE + 8.0f; e.type = BOSS_TYPE; e.alive = 1; e.stun = 0; e.babble_t = 30; e.babble_i = 5;
    }
    for (int i = 0; i < n_ow; i++) if ((dz_killed >> i) & 1u) ow[i].alive = 0;
    g_rng = 555u;
    for (int tries = 0; tries < 6000 && n_notes < MAX_NOTES; tries++) {
        int x = 1 + rndr(map_w - 2), y = 1 + rndr(map_h - 2);
        if (kind_solid(gmap[y][x]) || flow[y][x] < 14 || flow[y][x] >= 9999) continue;
        notes[n_notes].tx = x; notes[n_notes].ty = y; notes[n_notes].idx = (n_notes * 3 + ds.day) % 8; n_notes++;
    }
}
static void enter_zone(int z, int tx, int ty) {
    zone_build(z);
    n_ow = 0; n_notes = 0;
    pl.x = tx * TILE + 8.0f; pl.y = ty * TILE + 8.0f;
    if (z == Z_DARK) { dz_spawn(); grace = 90; bat_acc = 0; }
    g_fade = 14; safe_t = 30;
}
static void begin_day_card(bool with_report) {
    card_t = 0; if (!with_report) card_n = 0; g_state = ST_DAYCARD;
}
static void new_game() {
    pl.reset(); ds.reset(); dz_killed = 0; g_playframes = 0;
    light.set_radius(DIFFS[g_diff].light);
    enter_zone(Z_ROOM, 3, 5); in_bed = true; face = 2; pending_time = false;
    begin_day_card(false);
}
static bool load_slot(int slot) {
    SaveData s; int zone, bed, secs; uint32_t killed;
    if (!save_read(slot, s)) return false;
    unpack(s, pl, ds, g_diff, zone, bed, killed, secs);
    dz_killed = killed; g_playframes = secs * 60; in_bed = bed != 0;
    light.set_radius(DIFFS[g_diff].light);
    float px = pl.x, py = pl.y;
    zone_build(zone);
    n_ow = 0; n_notes = 0;
    if (zone == Z_DARK) dz_spawn();
    pl.x = px; pl.y = py; g_fade = 14; safe_t = 60; pending_time = false; g_state = ST_PLAY;
    return true;
}
static bool save_slot_now(int slot) {
    SaveData s; pack(s, pl, ds, g_diff, g_zone, in_bed ? 1 : 0, dz_killed, g_playframes / 60);
    return save_write(slot, s);
}
static void slot_text(int i, char *out, int sz) {
    SaveData s;
    if (!save_read(i, s)) { snprintf(out, sz, "Ranura %d:  - vacía -", i + 1); return; }
    int m = s.playsecs / 60;
    snprintf(out, sz, "Ranura %d: %s, %s | Nv.%d | %d Notas | %d:%02d h", i + 1, DAY_NAME[s.day % N_DAYS],
             s.slot < 3 ? SLOT_NAME[s.slot] : "Medianoche", s.lv, s.notas, m / 60, m % 60);
}

/* ======================= Tiempo y eventos del día ======================= */
static void go_home_forced() {
    enter_zone(Z_ROOM, 4, 5); in_bed = false; face = 2;
    start_dialog("Se hizo muy tarde. Vuelves a casa agotado.\nLo mejor es ir directo a la cama.", ST_PLAY);
}
static void start_intro_cutscene() {
    if (ds.slot < 2) ds.slot = 2;
    cut.start(); g_state = ST_CUTSCENE;
}
static void time_events() {
    pending_time = false;
    if (ds.day == 0 && ds.slot >= 2 && !ds.intro_done && g_zone != Z_DARK) { start_intro_cutscene(); return; }
    if (!building_open(g_zone, ds.slot)) {
        int tx = g_zone == Z_SCHOOL ? 16 : 27;
        enter_zone(Z_CITY, tx, 10);
        start_dialog("Es de noche y cierran el local.\nTe echan amablemente a la calle.", ST_PLAY);
        pending_time = true; return;
    }
    if (ds.day_over() && g_zone != Z_DARK && g_zone != Z_ROOM) { go_home_forced(); return; }
    if (ds.day_over() && g_zone == Z_ROOM && !in_bed) start_dialog("Ya no queda tiempo hoy.\nUsa la cama para dormir.", ST_PLAY);
}
static void do_sleep() {
    if (ds.is_last_day()) { end_page = 0; g_state = ST_ENDING; return; }
    card_n = ds.end_day(pl, card_rep);
    dz_killed = 0;
    enter_zone(Z_ROOM, 3, 5); in_bed = true; face = 2;
    begin_day_card(true);
}
static void leave_dark() {
    if (ds.day_over()) { go_home_forced(); return; }
    enter_zone(Z_CITY, 34, 20); face = 0;
}

/* ======================= Amigos ======================= */
static void talk_friend(int fid) {
    if (ds.talked_mask & (1 << fid)) { start_dialog(FRIEND_AGAIN[fid], ST_PLAY, FRIEND_NAME[fid]); return; }
    int b = ds.bond[fid], idx = b < 5 ? b : 5;
    ds.talked_mask |= 1 << fid; if (ds.bond[fid] < 5) ds.bond[fid]++;
    char eff[100];
    if (fid == 0) {
        int g = 6 + 2 * b; pl.notas += g;
        if (b >= 2) { pl.items[3]++; snprintf(eff, sizeof eff, "[Marcos te presta %d Notas y unas Pilas]", g); }
        else snprintf(eff, sizeof eff, "[Marcos te presta %d Notas]", g);
    } else if (fid == 1) {
        int t = b < ds.max_enemy_type() ? b : ds.max_enemy_type();
        pl.known_weak[t] |= 1 << EDEFS[t].weak;
        snprintf(eff, sizeof eff, "[Descubres: %s es débil a %s]", EDEFS[t].name, EL_NAME[EDEFS[t].weak]);
    } else {
        ds.study_bonus = 2 + (b >= 3 ? 1 : 0);
        snprintf(eff, sizeof eff, "[Sofía te ayuda: tu próximo estudio rinde +%d]", ds.study_bonus);
    }
    snprintf(talk_buf, sizeof talk_buf, "%s\n\n%s", FRIEND_TXT[fid][idx], eff);
    start_dialog(talk_buf, ST_PLAY, FRIEND_NAME[fid]);
}

/* ======================= Interacciones ======================= */
struct Interact { int type, id; };       /* 1 cama 2 escritorio 3 estudio 4 mostrador 5 NPC 6 nota */
static Interact find_interact() {
    static const int FX[4] = { 0, 1, 0, -1 }, FY[4] = { -1, 0, 1, 0 };
    Interact r = { 0, 0 };
    float ax = pl.x + FX[face] * 12, ay = pl.y + FY[face] * 12;
    for (int i = 0; i < N_NPCS; i++) {
        if (NPCS[i].zone != g_zone || !npc_visible(NPCS[i], ds.slot)) continue;
        if (fabsf(NPCS[i].tx * TILE + 8 - ax) < 14 && fabsf(NPCS[i].ty * TILE + 8 - ay) < 14) { r.type = 5; r.id = NPCS[i].id; return r; }
    }
    for (int i = 0; i < n_notes; i++)
        if (g_zone == Z_DARK && fabsf(notes[i].tx * TILE + 8 - pl.x) < 14 && fabsf(notes[i].ty * TILE + 8 - pl.y) < 14) { r.type = 6; r.id = notes[i].idx; return r; }
    int tx = (int)floorf(ax / TILE), ty = (int)floorf(ay / TILE);
    if (tx < 0 || ty < 0 || tx >= map_w || ty >= map_h) return r;
    switch (gmap[ty][tx]) {
        case K_BED: r.type = 1; break; case K_DESK: r.type = 2; break;
        case K_STUDY: r.type = 3; break; case K_COUNTER: r.type = 4; break;
    }
    return r;
}
static void interact(const Interact &it) {
    char b[160];
    if (it.type == 1) {
        if (in_bed) { in_bed = false; enter_zone(Z_ROOM, 4, 5); safe_t = 0; g_fade = 0; face = 2; say("Te levantas. ¡A salir!"); return; }
        if (ds.day == 0 && !ds.intro_done) open_choice(CH_SLEEP, "Es domingo por la noche. ¿Quieres dormir ya?", "Dormir", "Cancelar");
        else open_choice(CH_SLEEP, "¿Dormir y terminar el día?", "Dormir", "Cancelar");
    } else if (it.type == 2) {
        if (ds.hw_done) start_dialog("Ya hiciste tus tareas de hoy.\nMañana recibirás el bono.", ST_PLAY);
        else if (ds.day_over()) start_dialog("Ya no queda tiempo hoy.", ST_PLAY);
        else open_choice(CH_HOMEWORK, "¿Hacer tareas? Consume una franja.\nMañana recibes bono de Notas y puntos.", "Hacer tareas", "Cancelar");
    } else if (it.type == 3) {
        if (ds.day_over()) start_dialog("Ya no queda tiempo hoy.", ST_PLAY);
        else open_choice(CH_STUDY, "¿Estudiar? Consume una franja.\n+3 puntos académicos y algunas Notas.", "Estudiar", "Cancelar");
    } else if (it.type == 4) { shop_sel = 0; start_dialog(SHOP_TXT, ST_SHOP, "DON RAMIRO"); }
    else if (it.type == 5) { if (it.id == 3) { shop_sel = 0; start_dialog(SHOP_TXT, ST_SHOP, "DON RAMIRO"); } else talk_friend(it.id); }
    else if (it.type == 6) { snprintf(b, sizeof b, "%s", ENV_TXT[it.id & 7]); strcpy(act_buf, b); start_dialog(act_buf, ST_PLAY); }
}
static void handle_choice(int id, int sel) {
    g_state = choice_ret;
    if (id == CH_SLEEP) { if (sel == 0) { if (ds.day == 0 && !ds.intro_done) start_intro_cutscene(); else do_sleep(); } }
    else if (id == CH_HOMEWORK) {
        if (sel == 0) {
            ds.hw_done = true; ds.consume_slot(); pending_time = true;
            start_dialog("Haces tareas hasta que los ojos arden.\nMañana recibirás un bono por esto.", ST_PLAY);
        }
    } else if (id == CH_STUDY) {
        if (sel == 0) {
            int pts = 3 + ds.study_bonus, gain = 10 + ds.study_bonus * 2;
            ds.study_bonus = 0; pl.study += pts; pl.notas += gain; pl.recalc(); ds.study_today++; ds.consume_slot(); pending_time = true;
            snprintf(act_buf, sizeof act_buf, "Estudias durante horas.\n+%d puntos académicos (total %d)  +%d Notas\nTu promedio sube a %.1f.", pts, pl.study, gain, pl.average());
            start_dialog(act_buf, ST_PLAY);
        }
    } else if (id == CH_PORTAL) {
        if (sel == 0) {
            ds.consume_slot(); ds.dark_today = true; dz_killed = 0;
            enter_zone(Z_DARK, dark_sx, dark_sy);
        } else { pl.y -= 22; }
    } else if (id == CH_SLOT) {
        if (sel == 0) say(save_slot_now(choice_arg) ? "PARTIDA GUARDADA" : "ERROR AL GUARDAR");
        else if (sel == 1) { if (!load_slot(choice_arg)) say("RANURA VACÍA"); }
    }
}

/* ======================= Dibujo del mundo ======================= */
static void camera(int &cx, int &cy) {
    int pw = map_w * TILE, ph = map_h * TILE;
    if (pw <= SCR_W) cx = -(SCR_W - pw) / 2; else { cx = (int)pl.x - SCR_W / 2; if (cx < 0) cx = 0; if (cx > pw - SCR_W) cx = pw - SCR_W; }
    if (ph <= SCR_H) cy = -(SCR_H - ph) / 2; else { cy = (int)pl.y - SCR_H / 2; if (cy < 0) cy = 0; if (cy > ph - SCR_H) cy = ph - SCR_H; }
}
static void distort(const char *src, char *dst, int sz, int seed) {     /* voces familiares distorsionadas */
    int k = 0, n = 0; const char *p = src;
    while (*p && k < sz - 6) {
        const char *q = p; uint32_t cp = u8next(&p);
        if (cp < 128 && cp != ' ' && hash2(g_frame / 3, n + seed) % 6 == 0) dst[k++] = "#%&*"[hash2(n, g_frame / 3) & 3];
        else while (q < p) dst[k++] = *q++;
        n++;
    }
    dst[k] = 0;
}
static void draw_babble(int camx, int camy) {
    int shown = 0;
    for (int i = 0; i < n_ow && shown < 4; i++) {
        OWEnemy &e = ow[i];
        float dx = e.x - pl.x, dy = e.y - pl.y, d = sqrtf(dx * dx + dy * dy);
        if (!e.alive || d > 170 || e.babble_t < 12) continue;
        char buf[48]; distort(BABBLE[e.babble_i], buf, sizeof buf, i * 7);
        int w = u8len(buf) * 8, x = (int)e.x - camx - w / 2, y = (int)e.y - camy - 16;
        if (x < 2) x = 2;
        if (x > SCR_W - w - 2) x = SCR_W - w - 2;
        if (y < 6) y = 6;
        if (y > SCR_H - 14) y = SCR_H - 14;
        int v = 200 - (int)d; if (v < 60) v = 60;
        gfx_text(x, y, buf, rgb(v, v / 2, v / 2), 1);
        shown++;
    }
}
static const char *prompt_text(const Interact &it, char *b, int sz) {
    switch (it.type) {
        case 1: return in_bed ? "[X] Levantarte" : "[X] Cama: dormir";
        case 2: return "[X] Escritorio: hacer tareas";
        case 3: return "[X] Estudiar";
        case 4: return "[X] Tienda";
        case 5: snprintf(b, sz, "[X] Hablar con %s", it.id == 3 ? "Don Ramiro" : FRIEND_NAME[it.id]); return b;
        case 6: return "[X] Leer";
    }
    return 0;
}
static void draw_hud() {
    char b[80];
    snprintf(b, sizeof b, "%s - %s", ds.day_name(), ds.slot_name());
    gfx_text_sh(8, 4, b, RGB5(230,220,200), 1);
    gfx_rect(6, 15, 104, 7, RGB5(20,6,8)); gfx_rect(8, 16, 100 * pl.hp / (pl.maxhp ? pl.maxhp : 1), 5, RGB5(170,24,30));
    gfx_rect(6, 24, 104, 6, RGB5(8,10,24)); gfx_rect(8, 25, 100 * pl.sp / (pl.maxsp ? pl.maxsp : 1), 4, RGB5(60,100,190));
    snprintf(b, sizeof b, "PV %d/%d  SP %d", pl.hp, pl.maxhp, pl.sp);
    gfx_text_sh(8, 33, b, RGB5(170,160,160), 1);
    snprintf(b, sizeof b, "NOTAS %d", pl.notas); gfx_text_sh(SCR_W - 8 - u8len(b) * 8, 4, b, RGB5(120,200,140), 1);
    snprintf(b, sizeof b, "Nv.%d  EXP %d/%d", pl.lv, pl.exp, pl.exp_needed()); gfx_text_sh(SCR_W - 8 - u8len(b) * 8, 14, b, RGB5(170,170,180), 1);
    gfx_text_sh(SCR_W - 8 - 80 - 28, 24, "LUZ", RGB5(230,210,120), 1);
    gfx_bar(SCR_W - 8 - 80, 25, 80, 6, pl.battery, 100, pl.battery <= 20 ? RGB5(220,60,40) : RGB5(230,200,90), RGB5(30,26,10));
    if (g_state == ST_PLAY) {
        Interact it = find_interact(); char pb[64]; const char *pt = prompt_text(it, pb, sizeof pb);
        if (pt) gfx_text_c(SCR_H - 16, pt, RGB5(235,200,110), 1);
    }
    if (g_msg_t > 0) gfx_text_c(52, g_msg, RGB5(220,220,220), 1);
}
static void draw_scene(bool hud) {
    int camx, camy; camera(camx, camy);
    tex_set_tint((g_zone == Z_CITY || g_zone == Z_ROOM) ? (ds.slot == 1 ? 1 : ds.slot >= 2 ? 2 : 0) : 0);
    gfx_clear(BLACK);
    draw_tiles(camx, camy);
    for (int i = 0; i < n_notes; i++) {
        int x = notes[i].tx * TILE + 5 - camx, y = notes[i].ty * TILE + 4 - camy;
        gfx_rect(x, y, 6, 8, RGB5(200,200,185)); gfx_rect(x + 1, y + 2, 4, 1, RGB5(90,90,90)); gfx_rect(x + 1, y + 4, 3, 1, RGB5(90,90,90));
    }
    for (int i = 0; i < N_NPCS; i++) {
        const NPCDef &n = NPCS[i];
        if (n.zone != g_zone || !npc_visible(n, ds.slot)) continue;
        const pix *pal = n.id == 0 ? PAL_MARCOS : n.id == 1 ? PAL_VALERIA : n.id == 2 ? PAL_SOFIA : PAL_RAMIRO;
        gfx_sprite(n.id == 1 || n.id == 2 ? SPR_GIRL : SPR_LEO, pal, n.tx * TILE - camx, n.ty * TILE - camy, 2, 0);
    }
    for (int i = 0; i < n_ow; i++) {
        OWEnemy &e = ow[i];
        if (!e.alive) continue;
        gfx_sprite_n(SPR_ENEMY[e.type], 16, 16, PAL_ENEMY[e.type], (int)e.x - 8 - camx, (int)e.y - 8 - camy, 1, (g_frame / 20 + i) & 1);
    }
    int psx = (int)pl.x - camx, psy = (int)pl.y - camy;
    if (in_bed && g_zone == Z_ROOM) {                       /* Leo durmiendo en la cama */
        int bx = 2 * TILE - camx, by = 3 * TILE - camy;
        gfx_rect(bx + 3, by + 6, 11, 9, RGB5(205,150,120)); gfx_rect(bx + 2, by + 4, 13, 5, RGB5(70,44,30));
        gfx_rect(bx + 5, by + 11, 2, 1, RGB5(40,30,30));
    } else if (!(safe_t > 0 && (g_frame / 3) % 2 && g_state == ST_PLAY)) gfx_sprite(SPR_LEO, PAL_LEO, psx - 8, psy - 9, 2, 0);

    int slot = ds.slot;
    if (g_zone == Z_DARK) {
        int R = light.radius(g_frame, pl.hp, pl.maxhp, pl.battery, slot);
        light.apply(psx, psy, R);
        draw_babble(camx, camy);
        if (((g_frame / 40) & 1)) gfx_rect(1 * TILE + 7 - camx, 1 * TILE + 7 - camy, 2, 2, RGB5(220,230,255));
    } else if (g_zone == Z_CITY && slot >= 2) {
        light.apply(psx, psy, light.radius(g_frame, pl.hp, pl.maxhp, pl.battery, 2) + 25);   /* de noche, solo tu aura */
    }
    if (g_fade > 0) { gfx_fade(32 - g_fade * 32 / 14); }
    if (hud) draw_hud();
}

/* ======================= Título, dificultad, ranuras ======================= */
static void draw_title() {
    gfx_clear(BLACK);
    gfx_static(1100, 40, 130);
    for (int i = 0; i < 40; i++) gfx_rect((i * 37) % SCR_W, (g_frame * (3 + i % 4) + i * 53) % SCR_H, 1, 6, RGB5(60,60,70));
    if (rndr(70) == 0) gfx_rect(0, rndr(SCR_H), SCR_W, 1, RGB5(200,200,200));
    int j = (g_frame / 3 % 23 == 0) ? 120 : 190;
    gfx_text_c(30, "SUB-CAMA", rgb(j, j, j), 4);
    gfx_text_c(70, "SECTOR 0", RGB5(160,20,25), 3);
    gfx_text_c(104, "- una semana en la oscuridad -", RGB5(90,90,100), 1);
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
        if (g_sel == 0) new_game();
        else if (g_sel == 1) { load_sel = 0; g_state = ST_LOADMENU; }
        else { g_state = ST_DIFF; g_sel = g_diff; }
    }
    if (g_msg_t > 0) g_msg_t--;
    if (g_state == ST_TITLE) draw_title();
}
static void frame_diff(const Input &in) {
    if (in.pressed & IN_UP) g_sel = (g_sel + 3) % 4;
    if (in.pressed & IN_DOWN) g_sel = (g_sel + 1) % 4;
    if (in.pressed & IN_CROSS) { g_diff = g_sel; g_state = ST_TITLE; g_sel = 2; }
    if (in.pressed & IN_CIRCLE) { g_state = ST_TITLE; g_sel = 2; }
    gfx_clear(BLACK);
    gfx_static(500, 30, 90);
    gfx_text_c(12, "DIFICULTAD", RGB5(160,20,25), 3);
    for (int i = 0; i < 4; i++) {
        int y = 50 + i * 52, s = (i == g_sel);
        char b[64]; snprintf(b, sizeof b, "%s   EXP x%.1f", DIFFS[i].name, DIFFS[i].exp_mult);
        gfx_text_c(y, b, s ? RGB5(240,240,230) : RGB5(90,90,100), 1);
        pix c = s ? RGB5(170,170,170) : RGB5(70,70,80);
        gfx_text_c(y + 12, DIFFS[i].d1, c, 1); gfx_text_c(y + 22, DIFFS[i].d2, c, 1); gfx_text_c(y + 32, DIFFS[i].d3, c, 1);
        if (s) gfx_frame(40, y - 5, 400, 46, RGB5(120,30,30));
    }
    gfx_text_c(262, "X: ELEGIR    O: VOLVER", RGB5(80,80,90), 1);
}
static void frame_loadmenu(const Input &in) {
    if (in.pressed & IN_UP) load_sel = (load_sel + N_SLOTS - 1) % N_SLOTS;
    if (in.pressed & IN_DOWN) load_sel = (load_sel + 1) % N_SLOTS;
    if (in.pressed & IN_CIRCLE) g_state = ST_TITLE;
    if (in.pressed & IN_CROSS) { if (!load_slot(load_sel)) say("RANURA VACÍA"); }
    if (g_msg_t > 0) g_msg_t--;
    if (g_state != ST_LOADMENU) return;
    gfx_clear(BLACK); gfx_static(500, 30, 90);
    gfx_text_c(20, "CARGAR PARTIDA", RGB5(160,20,25), 3);
    for (int i = 0; i < N_SLOTS; i++) {
        char b[160]; slot_text(i, b, sizeof b);
        ui_box(20, 70 + i * 50, 440, 40);
        gfx_text(32, 86 + i * 50, b, i == load_sel ? RGB5(250,250,240) : RGB5(110,110,120), 1);
        if (i == load_sel) gfx_text(24, 86 + i * 50, ">", RGB5(250,220,100), 1);
    }
    if (g_msg_t > 0) gfx_text_c(236, g_msg, RGB5(220,120,60), 1);
    gfx_text_c(256, "X: CARGAR    O: VOLVER", RGB5(80,80,90), 1);
}

/* ======================= Exploración: lógica ======================= */
static void start_encounter(int ei) {
    enc_n = 0;
    enc_idx[enc_n] = ei; enc_types[enc_n++] = ow[ei].type;
    int maxg = ds.max_group();
    for (int i = 0; i < n_ow && enc_n < maxg; i++) {
        if (i == ei || !ow[i].alive || ow[i].stun > 0 || ow[i].type == BOSS_TYPE || ow[ei].type == BOSS_TYPE) continue;
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
        float spd = OW_SPEED[e.type] * DIFFS[g_diff].espeed * (1.0f + 0.04f * ds.day);
        float tgx = pl.x, tgy = pl.y, dxp = pl.x - e.x, dyp = pl.y - e.y;
        if (dxp * dxp + dyp * dyp > 24 * 24) {         /* siempre saben dónde estás */
            int tx = (int)(e.x / TILE), ty = (int)(e.y / TILE);
            if (tx >= 0 && ty >= 0 && tx < map_w && ty < map_h && flow[ty][tx] < 9999) {
                int best = flow[ty][tx], bx = tx, by = ty;
                for (int d = 0; d < 4; d++) {
                    int nx = tx + DX[d], ny = ty + DY[d];
                    if (nx >= 0 && ny >= 0 && nx < map_w && ny < map_h && flow[ny][nx] < best) { best = flow[ny][nx]; bx = nx; by = ny; }
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
    if (pending_time) { time_events(); if (g_state != ST_PLAY) return; }
    if (in_bed) {
        if (in.pressed & IN_CROSS) { Interact bed = { 1, 0 }; interact(bed); }       /* levantarse */
        if (in.pressed & IN_SQUARE) { menu_tab = 0; menu_sel = 0; g_state = ST_MENU; }
        if (in.pressed & IN_START) { g_sel = 0; g_state = ST_PAUSE; }
        if (g_state == ST_PLAY) { if (g_fade > 0) g_fade--; draw_scene(true); gfx_text_c(SCR_H - 16, "[X] Levantarte", RGB5(235,200,110), 1); }
        return;
    }
    float vx = 0, vy = 0;
    if (in.held & IN_LEFT) vx -= 1;
    if (in.held & IN_RIGHT) vx += 1;
    if (in.held & IN_UP) vy -= 1;
    if (in.held & IN_DOWN) vy += 1;
    if (in.ax || in.ay) { vx = in.ax / 128.0f; vy = in.ay / 128.0f; }
    float l = sqrtf(vx * vx + vy * vy);
    if (l > 1) { vx /= l; vy /= l; }
    if (l > 0.1f) face = (fabsf(vx) > fabsf(vy)) ? (vx > 0 ? 1 : 3) : (vy > 0 ? 2 : 0);
    float ox = pl.x, oy = pl.y;
    move_ent(&pl.x, &pl.y, vx * 1.3f, vy * 1.3f, 5);
    for (int i = 0; i < N_NPCS; i++)                                   /* los NPCs son sólidos */
        if (NPCS[i].zone == g_zone && npc_visible(NPCS[i], ds.slot) && fabsf(NPCS[i].tx * TILE + 8 - pl.x) < 12 && fabsf(NPCS[i].ty * TILE + 8 - pl.y) < 12) { pl.x = ox; pl.y = oy; }

    if (in.pressed & IN_CROSS) interact(find_interact());
    if (in.pressed & IN_SQUARE) { menu_tab = 0; menu_sel = 0; g_state = ST_MENU; }
    if (in.pressed & IN_START) { g_sel = 0; g_state = ST_PAUSE; }
    if (safe_t > 0) safe_t--;
    if (grace > 0) grace--;
    if (g_msg_t > 0) g_msg_t--;
    if (g_fade > 0) g_fade--;

    if (g_state == ST_PLAY) {
        int tx = (int)(pl.x / TILE), ty = (int)(pl.y / TILE);
        const DoorDef *d = door_at(g_zone, tx, ty);
        if (d) {
            if (!building_open(d->to_zone, ds.slot)) { start_dialog("Está cerrado. Vuelve por la mañana o por la tarde.", ST_PLAY); pl.y += 20; }
            else if (g_zone == Z_ROOM && ds.day_over()) { start_dialog("Es demasiado tarde para salir.\nMejor a dormir.", ST_PLAY); pl.y -= 20; }
            else enter_zone(d->to_zone, d->to_tx, d->to_ty);
        } else if (g_zone == Z_CITY && gmap[ty][tx] == K_PORTAL) {
            if (ds.day == 0 && !ds.intro_done) { start_dialog("El callejón da mala espina...\nMejor vuelve cuando anochezca.", ST_PLAY); pl.y -= 22; }
            else open_choice(CH_PORTAL, "Una grieta respira en la pared del callejón.\n¿Entrar a la Zona Oscura? (consume una franja)", "Entrar", "Quedarme afuera");
        } else if (g_zone == Z_DARK && tx == 1 && ty == 1 && grace == 0) {
            leave_dark();
        }
    }
    if (g_state == ST_PLAY && g_zone == Z_DARK) {                      /* la linterna se gasta en la oscuridad */
        bat_acc += ds.battery_drain();
        if (bat_acc >= 420) { bat_acc -= 420; if (pl.battery > 0) { pl.battery--; if (pl.battery == 0) say("¡La linterna se apagó!"); } }
        if (g_frame % 8 == 0) flow_compute((int)(pl.x / TILE), (int)(pl.y / TILE));
        enemies_update();
    }
    if (g_state == ST_PLAY) draw_scene(true);
}
static void frame_encounter() {
    enc_t++;
    draw_scene(false);
    gfx_static(enc_t * 260, 80, 255);
    if ((enc_t / 3) & 1) gfx_add(6);
    if (enc_t > 8) gfx_text_c(120, "!", RGB5(220,40,40), 6);
    if (enc_t >= 45) {
        battle.start(&pl, enc_types, enc_idx, enc_n, ds.day, ds.enemy_scale(), g_diff);
        g_state = ST_BATTLE;
    }
}
static void frame_battle(const Input &in) {
    battle.update(in);
    battle.draw();
    if (battle.phase != Battle::PH_END) return;
    int res = battle.result;
    if (res == 1) {
        for (int i = 0; i < battle.nf; i++) { int k = battle.ow_index[i]; ow[k].alive = 0; if (k < 32) dz_killed |= 1u << k; }
        safe_t = 60; g_state = ST_PLAY;
        if (battle.boss_beaten) { ds.boss_down = true; start_dialog("La cena fría se disuelve en humo.\nPor primera vez, la mesa queda en silencio\nsin reproches. Es hora de volver y dormir.", ST_PLAY); }
    } else if (res == 3) {
        for (int i = 0; i < battle.nf; i++) {
            OWEnemy &e = ow[battle.ow_index[i]];
            e.stun = 240;
            float dx = e.x - pl.x, dy = e.y - pl.y, l = sqrtf(dx * dx + dy * dy);
            if (l > 0.1f) move_ent(&e.x, &e.y, dx / l * 20, dy / l * 20, 5);
        }
        safe_t = 90; g_state = ST_PLAY;
    } else {                                            /* derrota: te desmayas y despiertas en casa (sin muerte permanente) */
        pl.notas -= pl.notas * DIFFS[g_diff].notes_loss / 100;
        faint_t = 0; g_state = ST_FAINT;
    }
}
static void frame_faint() {
    faint_t++;
    draw_scene(false);
    gfx_fade(32 - (faint_t > 50 ? 30 : faint_t * 3 / 5));
    if (faint_t > 20) gfx_text_c(100, "TE DESMAYAS", RGB5(200,200,210), 3);
    if (faint_t > 60) gfx_text_c(150, FAINT_TXT, RGB5(150,150,160), 1);
    if (faint_t > 90 && DIFFS[g_diff].notes_loss) { char b[64]; snprintf(b, sizeof b, "Pierdes %d%% de tus Notas.", DIFFS[g_diff].notes_loss); gfx_text_c(190, b, RGB5(150,70,70), 1); }
    if (faint_t > 200) do_sleep();
}

/* ======================= Tarjeta de día y finales ======================= */
static void frame_daycard(const Input &in) {
    card_t++;
    gfx_clear(BLACK);
    gfx_static(200, 10, 50);
    char b[64]; snprintf(b, sizeof b, "DÍA %d", ds.day + 1);
    gfx_text_c(40, b, RGB5(200,200,210), 3);
    gfx_text_c(76, ds.day_name(), RGB5(200,60,60), 2);
    gfx_text_c(104, DAY_CARD[ds.day], RGB5(190,190,180), 1);
    for (int i = 0; i < card_n; i++) gfx_text_c(140 + i * 12, card_rep[i], RGB5(120,200,140), 1);
    snprintf(b, sizeof b, "Objetivo: %s", DAY_GOAL[ds.day]); gfx_text_c(200, b, RGB5(230,200,110), 1);
    if (card_t > 20 && (card_t / 20) & 1) gfx_text_c(240, "[X] Levantarte de la cama", RGB5(160,160,170), 1);
    if (card_t < 30) gfx_fade(card_t * 32 / 30);
    if (card_t > 20 && (in.pressed & IN_CROSS)) { g_state = ST_PLAY; g_fade = 14; }
}
static void frame_ending(const Input &in) {
    int e = ds.boss_down ? ((ds.bond[0] + ds.bond[1] + ds.bond[2]) >= 9 ? 0 : 1) : 2;
    gfx_clear(BLACK);
    gfx_static(250, 10, 60);
    if (end_page == 0) {
        gfx_text_c(30, ENDINGS[e].title, RGB5(230,200,110), 2);
        gfx_text_c(80, ENDINGS[e].text, RGB5(200,200,190), 1);
    } else {
        gfx_text_c(24, "RESUMEN DE LA SEMANA", RGB5(200,60,60), 2);
        char b[96]; int y = 64;
        snprintf(b, sizeof b, "Nivel %d   Calificación %.1f (Estudio %d)", pl.lv, pl.average(), pl.study); gfx_text_c(y, b, RGB5(200,200,200), 1); y += 16;
        snprintf(b, sizeof b, "Notas: %d    Monstruos derrotados: %d", pl.notas, pl.kills); gfx_text_c(y, b, RGB5(200,200,200), 1); y += 16;
        snprintf(b, sizeof b, "Vínculos: Marcos %d/5  Valeria %d/5  Sofía %d/5", ds.bond[0], ds.bond[1], ds.bond[2]); gfx_text_c(y, b, RGB5(200,200,200), 1); y += 16;
        snprintf(b, sizeof b, "Dificultad: %s    La Cena Fría: %s", DIFFS[g_diff].name, ds.boss_down ? "vencida" : "sin vencer"); gfx_text_c(y, b, RGB5(200,200,200), 1); y += 30;
        gfx_text_c(y, ENDING_NOTE, RGB5(150,200,160), 1);
    }
    gfx_text_c(250, "[X] Continuar", RGB5(120,120,130), 1);
    if (in.pressed & IN_CROSS) { if (end_page == 0) end_page = 1; else { g_state = ST_TITLE; g_sel = 0; } }
}

/* ======================= Menús ======================= */
static void frame_choice(const Input &in) {
    if (in.pressed & IN_UP) choice_sel = (choice_sel + choice_n - 1) % choice_n;
    if (in.pressed & IN_DOWN) choice_sel = (choice_sel + 1) % choice_n;
    if (in.pressed & IN_CIRCLE) { int id = choice_id; handle_choice(id, choice_n - 1); return; }
    if (in.pressed & IN_CROSS) { int id = choice_id, s = choice_sel; handle_choice(id, s); return; }
    if (choice_ret == ST_MENU) { /* el menú ya se dibuja debajo */ }
    else draw_scene(true);
    int w = 330, h = 52 + choice_n * 18, x = (SCR_W - w) / 2, y = 90;
    ui_box(x, y, w, h);
    gfx_text(x + 12, y + 10, choice_title, RGB5(220,220,210), 1);
    int ty = y + 10 + 10 * (1 + (int)(strchr(choice_title, '\n') != 0)) + 6;
    for (int i = 0; i < choice_n; i++) {
        char b[64]; snprintf(b, sizeof b, i == choice_sel ? "> %s" : "  %s", choice_opts[i]);
        gfx_text(x + 20, ty + i * 16, b, i == choice_sel ? RGB5(250,250,240) : RGB5(110,110,120), 1);
    }
}
static void frame_dialog(const Input &in) {
    if (dlg.update((in.pressed & IN_CROSS) != 0)) { g_state = dlg_ret; if (dlg_ret == ST_SHOP) shop_sel = 0; }
    if (g_state == ST_DIALOG) { draw_scene(true); dlg.draw(); }
}
static void frame_pause(const Input &in) {
    static const char *items[3] = { "CONTINUAR", "MENÚ / GUARDAR", "VOLVER AL TÍTULO" };
    if (in.pressed & IN_UP) g_sel = (g_sel + 2) % 3;
    if (in.pressed & IN_DOWN) g_sel = (g_sel + 1) % 3;
    if (in.pressed & (IN_START | IN_CIRCLE)) g_state = ST_PLAY;
    if (in.pressed & IN_CROSS) {
        if (g_sel == 0) g_state = ST_PLAY;
        else if (g_sel == 1) { menu_tab = TAB_SAVE; menu_sel = 0; g_state = ST_MENU; }
        else g_state = ST_TITLE;
    }
    if (g_state != ST_PAUSE) return;
    draw_scene(true);
    ui_box(110, 80, 260, 40 + 3 * 18);
    gfx_text_c(88, "PAUSA", RGB5(200,60,60), 1);
    for (int i = 0; i < 3; i++) { char b[48]; snprintf(b, sizeof b, i == g_sel ? "> %s <" : "  %s  ", items[i]); gfx_text_c(108 + i * 18, b, i == g_sel ? RGB5(240,240,230) : RGB5(100,100,110), 1); }
}

/* --- Menú del jugador con pestañas --- */
static void menu_use_item(int i) {
    bool used = false;
    if (i == 0 && pl.hp < pl.maxhp) { pl.heal(35); used = true; }
    else if (i == 1 && pl.sp < pl.maxsp) { pl.heal_sp(12); used = true; }
    else if (i == 2 && (pl.any_status() || pl.hp < pl.maxhp)) { pl.clear_status(); pl.heal(10); used = true; }
    else if (i == 3 && pl.battery < 100) { pl.battery += 40; if (pl.battery > 100) pl.battery = 100; used = true; }
    if (used) pl.items[i]--; else say("NO TIENE EFECTO AHORA");
}
static int tab_rows(int tab) { return tab == TAB_INV ? N_ITEMS + N_ARMORS : tab == TAB_SKILLS ? N_SKILLS : tab == TAB_SAVE ? N_SLOTS : 1; }
static void draw_tabs() {
    static const char *names[N_TABS] = { "ESTADO", "INVENTARIO", "HABILIDADES", "TAREAS", "GUARDAR" };
    gfx_clear(RGB5(6,4,8));
    for (int i = 0; i < N_TABS; i++) {
        int x = 10 + i * 92, a = (i == menu_tab);
        gfx_rect(x, 6, 90, 16, a ? RGB5(40,20,26) : RGB5(14,10,16)); gfx_frame(x, 6, 90, 16, a ? RGB5(210,70,70) : RGB5(70,70,80));
        gfx_text(x + (90 - u8len(names[i]) * 8) / 2, 11, names[i], a ? RGB5(250,240,220) : RGB5(110,110,120), 1);
    }
    gfx_text(2, 11, "L", RGB5(150,150,160), 1); gfx_text(SCR_W - 10, 11, "R", RGB5(150,150,160), 1);
    ui_box(10, 26, 460, 240);
}
static void tab_state() {
    char b[96];
    gfx_sprite(SPR_LEO, PAL_LEO, 28, 48, 5, 0);
    gfx_text(24, 94, "LEO", RGB5(220,70,70), 2);
    snprintf(b, sizeof b, "Nivel %d", pl.lv); gfx_text(24, 116, b, RGB5(230,230,220), 1);
    int x = 120, y = 40;
    snprintf(b, sizeof b, "EXP  %d / %d", pl.exp, pl.exp_needed()); gfx_text(x, y, b, RGB5(200,200,200), 1); gfx_bar(x + 150, y + 1, 160, 6, pl.exp, pl.exp_needed(), RGB5(120,170,230), RGB5(14,18,30)); y += 16;
    snprintf(b, sizeof b, "PV   %d / %d", pl.hp, pl.maxhp); gfx_text(x, y, b, RGB5(200,200,200), 1); gfx_bar(x + 150, y + 1, 160, 6, pl.hp, pl.maxhp, RGB5(170,24,30), RGB5(30,10,12)); y += 16;
    snprintf(b, sizeof b, "SP   %d / %d", pl.sp, pl.maxsp); gfx_text(x, y, b, RGB5(200,200,200), 1); gfx_bar(x + 150, y + 1, 160, 6, pl.sp, pl.maxsp, RGB5(60,100,190), RGB5(10,14,30)); y += 16;
    snprintf(b, sizeof b, "LUZ  %d%%", pl.battery); gfx_text(x, y, b, RGB5(230,210,120), 1); gfx_bar(x + 150, y + 1, 160, 6, pl.battery, 100, RGB5(230,200,90), RGB5(30,26,10)); y += 20;
    snprintf(b, sizeof b, "Fuerza %d     Defensa %d", pl.atk, pl.def); gfx_text(x, y, b, RGB5(210,210,200), 1); y += 14;
    snprintf(b, sizeof b, "Agilidad %d   Psique %d", pl.agi, pl.mag); gfx_text(x, y, b, RGB5(210,210,200), 1); y += 18;
    snprintf(b, sizeof b, "Armadura: %s", pl.armor_eq >= 0 ? ARMORS[pl.armor_eq].name : "ninguna"); gfx_text(x, y, b, RGB5(150,220,160), 1); y += 14;
    snprintf(b, sizeof b, "Calificación: %.1f  (estudio %d)", pl.average(), pl.study); gfx_text(x, y, b, RGB5(230,210,130), 1); y += 14;
    snprintf(b, sizeof b, "Notas: %d", pl.notas); gfx_text(x, y, b, RGB5(120,200,140), 1); y += 18;
    snprintf(b, sizeof b, "%s - %s   (Día %d de %d)", ds.day_name(), ds.slot_name(), ds.day + 1, N_DAYS); gfx_text(x, y, b, RGB5(180,180,190), 1);
    gfx_text(24, 150, "Debilidad:", RGB5(200,100,100), 1); gfx_text(24, 162, EL_NAME[EL_MEMORY], RGB5(200,100,100), 1);
}
static void tab_inventory() {
    char b[96], pn[48];
    gfx_text(24, 34, "OBJETOS", RGB5(220,70,70), 1);
    for (int i = 0; i < N_ITEMS; i++) {
        padf(pn, sizeof pn, ITEM_NAME[i], 16); snprintf(b, sizeof b, "%c%s x%d", menu_sel == i ? '>' : ' ', pn, pl.items[i]);
        gfx_text(24, 48 + i * 14, b, menu_sel == i ? RGB5(250,250,240) : RGB5(120,120,130), 1);
    }
    gfx_text(24, 116, "ARMADURAS", RGB5(220,70,70), 1);
    for (int i = 0; i < N_ARMORS; i++) {
        int row = N_ITEMS + i; padf(pn, sizeof pn, ARMORS[i].name, 26);
        if (pl.armor_tier[i] < 0) snprintf(b, sizeof b, "%c%s  (no la tienes)", menu_sel == row ? '>' : ' ', pn);
        else snprintf(b, sizeof b, "%c%s  Nv.%d %s", menu_sel == row ? '>' : ' ', pn, pl.armor_tier[i], pl.armor_eq == i ? "[EQUIPADA]" : "");
        gfx_text(24, 130 + i * 14, b, menu_sel == row ? RGB5(250,250,240) : RGB5(120,120,130), 1);
    }
    const char *desc = menu_sel < N_ITEMS ? ITEM_DESC[menu_sel] : ARMORS[menu_sel - N_ITEMS].desc;
    gfx_text(24, 200, desc, RGB5(200,170,100), 1);
    gfx_text(24, 214, menu_sel < N_ITEMS ? "[X] Usar" : "[X] Equipar / quitar", RGB5(130,130,140), 1);
    snprintf(b, sizeof b, "PV %d/%d  SP %d/%d  DEF %d  Regeneración %d", pl.hp, pl.maxhp, pl.sp, pl.maxsp, pl.def, pl.regen);
    gfx_text(24, 232, b, RGB5(150,150,160), 1);
}
static void tab_skills() {
    char b[96], pn[48];
    gfx_text(24, 34, "HABILIDADES ACTIVAS (se usan en combate)", RGB5(220,70,70), 1);
    for (int i = 0; i < N_SKILLS; i++) {
        padf(pn, sizeof pn, SKILLS[i].name, 16);
        char pe[24]; padf(pe, sizeof pe, EL_NAME[SKILLS[i].el], 8);
        if (pl.knows(i)) snprintf(b, sizeof b, "%c%s %s %2d SP", menu_sel == i ? '>' : ' ', pn, pe, SKILLS[i].cost);
        else snprintf(b, sizeof b, "%c%s se desbloquea en Nv.%d", menu_sel == i ? '>' : ' ', pn, SKILLS[i].unlock);
        gfx_text(24, 52 + i * 16, b, pl.knows(i) ? (menu_sel == i ? RGB5(250,250,240) : RGB5(150,150,160)) : RGB5(90,70,70), 1);
    }
    gfx_text(24, 150, SKILLS[menu_sel].desc, RGB5(200,170,100), 1);
    gfx_text(24, 176, "La luz de la linterna cambia el combate:", RGB5(150,150,160), 1);
    gfx_text(24, 190, "Luz alta (60%+): más daño. Luz baja (20%-): los", RGB5(150,150,160), 1);
    gfx_text(24, 202, "monstruos pegan más fuerte. Usa Pilas.", RGB5(150,150,160), 1);
}
static void tab_tasks() {
    char b[128]; int y = 36;
    int left = 3 - ds.slot; if (left < 0) left = 0;
    snprintf(b, sizeof b, "Hoy: %s - %s      Franjas restantes: %d", ds.day_name(), ds.slot_name(), left); gfx_text(24, y, b, RGB5(230,230,220), 1); y += 14;
    snprintf(b, sizeof b, "Objetivo: %s", DAY_GOAL[ds.day]); gfx_text(24, y, b, RGB5(230,200,110), 1); y += 20;
    snprintf(b, sizeof b, "[%c] Hacer tareas (escritorio)  %s", ds.hw_done ? 'X' : ' ', ds.hw_done ? "- hecho: bono mañana" : "- pendiente");
    gfx_text(24, y, b, ds.hw_done ? RGB5(120,200,140) : RGB5(170,170,180), 1); y += 14;
    snprintf(b, sizeof b, "[%c] Estudiar en la biblioteca     - %d vez/veces hoy", ds.study_today ? 'X' : ' ', ds.study_today);
    gfx_text(24, y, b, ds.study_today ? RGB5(120,200,140) : RGB5(170,170,180), 1); y += 14;
    for (int i = 0; i < N_FRIENDS; i++) {
        snprintf(b, sizeof b, "[%c] Hablar con %s  (vínculo %d/5)", (ds.talked_mask >> i) & 1 ? 'X' : ' ', FRIEND_NAME[i], ds.bond[i]);
        gfx_text(24, y, b, (ds.talked_mask >> i) & 1 ? RGB5(120,200,140) : RGB5(170,170,180), 1); y += 14;
    }
    snprintf(b, sizeof b, "[%c] Explorar la Zona Oscura", ds.dark_today ? 'X' : ' ');
    gfx_text(24, y, b, ds.dark_today ? RGB5(120,200,140) : RGB5(170,170,180), 1); y += 20;
    if (ds.hw_done) { snprintf(b, sizeof b, "Bono de tareas al despertar: +%d Notas y +2 pts", 18 + 4 * ds.day + pl.study / 2); gfx_text(24, y, b, RGB5(130,210,150), 1); y += 14; }
    if (ds.study_bonus) { snprintf(b, sizeof b, "Bono de Sofía listo: próximo estudio +%d", ds.study_bonus); gfx_text(24, y, b, RGB5(130,210,150), 1); }
}
static void tab_save() {
    gfx_text(24, 34, "GUARDAR / CARGAR  (3 ranuras)", RGB5(220,70,70), 1);
    for (int i = 0; i < N_SLOTS; i++) {
        char b[160]; slot_text(i, b, sizeof b);
        ui_box(20, 52 + i * 44, 440, 36);
        gfx_text(32, 66 + i * 44, b, menu_sel == i ? RGB5(250,250,240) : RGB5(110,110,120), 1);
        if (menu_sel == i) gfx_text(24, 66 + i * 44, ">", RGB5(250,220,100), 1);
    }
    gfx_text(24, 196, "[X] elige la ranura y luego Guardar o Cargar.", RGB5(150,150,160), 1);
    gfx_text(24, 210, "Puedes guardar en cualquier momento fuera de combate.", RGB5(150,150,160), 1);
}
static void frame_menu(const Input &in) {
    if (in.pressed & (IN_SQUARE | IN_CIRCLE)) { g_state = ST_PLAY; return; }
    int tabd = ((in.pressed & (IN_RIGHT | IN_R)) ? 1 : 0) - ((in.pressed & (IN_LEFT | IN_L)) ? 1 : 0);
    if (tabd) { menu_tab = (menu_tab + N_TABS + tabd) % N_TABS; menu_sel = 0; }
    int rows = tab_rows(menu_tab);
    if (in.pressed & IN_UP) menu_sel = (menu_sel + rows - 1) % rows;
    if (in.pressed & IN_DOWN) menu_sel = (menu_sel + 1) % rows;
    if (in.pressed & IN_CROSS) {
        if (menu_tab == TAB_INV) {
            if (menu_sel < N_ITEMS) { if (pl.items[menu_sel] > 0) menu_use_item(menu_sel); else say("NO QUEDA"); }
            else { int a = menu_sel - N_ITEMS; if (pl.armor_tier[a] >= 0) { pl.armor_eq = (pl.armor_eq == a) ? -1 : a; pl.recalc(); } else say("AÚN NO LA TIENES"); }
        } else if (menu_tab == TAB_SAVE) {
            choice_arg = menu_sel; char t[40]; snprintf(t, sizeof t, "Ranura %d", menu_sel + 1);
            open_choice(CH_SLOT, t, "Guardar aquí", "Cargar", "Cancelar");
            choice_ret = ST_MENU; return;
        }
    }
    if (g_msg_t > 0) g_msg_t--;
    draw_tabs();
    switch (menu_tab) { case TAB_STATE: tab_state(); break; case TAB_INV: tab_inventory(); break; case TAB_SKILLS: tab_skills(); break;
                        case TAB_TASKS: tab_tasks(); break; case TAB_SAVE: tab_save(); break; }
    if (g_msg_t > 0) gfx_text_c(238, g_msg, RGB5(230,150,70), 1);
    gfx_text(14, 256, "L/R o <- ->: pestañas    O/[]: cerrar", RGB5(80,80,90), 1);
}

/* --- Tienda: armaduras temáticas y objetos --- */
static void frame_shop(const Input &in) {
    int rows = N_ARMORS + N_ITEMS + 1;
    if (in.pressed & IN_UP) shop_sel = (shop_sel + rows - 1) % rows;
    if (in.pressed & IN_DOWN) shop_sel = (shop_sel + 1) % rows;
    if (in.pressed & IN_CIRCLE) { g_state = ST_PLAY; return; }
    if (in.pressed & IN_CROSS) {
        if (shop_sel == rows - 1) { g_state = ST_PLAY; return; }
        if (shop_sel < N_ARMORS) {
            int a = shop_sel; const ArmorDef &d = ARMORS[a]; int t = pl.armor_tier[a];
            if (t < 0) {
                if (pl.study < d.buy_study) say("NECESITAS MÁS PUNTOS ACADÉMICOS");
                else if (pl.notas < d.cost) say("NOTAS INSUFICIENTES");
                else { pl.notas -= d.cost; pl.armor_tier[a] = 0; if (pl.armor_eq < 0) pl.armor_eq = a; pl.recalc(); say("¡COMPRADA!"); }
            } else if (t >= 3) say("YA ESTÁ AL MÁXIMO");
            else if (pl.study < d.req_study[t]) say("ESTUDIA MÁS PARA MEJORARLA");
            else if (pl.notas < d.up_cost[t]) say("NOTAS INSUFICIENTES");
            else { pl.notas -= d.up_cost[t]; pl.armor_tier[a]++; pl.recalc(); say("¡ARMADURA MEJORADA!"); }
        } else {
            int it = shop_sel - N_ARMORS;
            if (pl.notas >= ITEM_COST[it]) { pl.notas -= ITEM_COST[it]; pl.items[it]++; say("COMPRADO"); } else say("NOTAS INSUFICIENTES");
        }
    }
    if (g_msg_t > 0) g_msg_t--;
    gfx_clear(RGB5(10,8,14));
    ui_box(8, 6, 464, 258);
    gfx_text(20, 14, "TIENDA DE DON RAMIRO", RGB5(230,200,110), 1);
    char b[120]; snprintf(b, sizeof b, "Notas: %d    Puntos académicos: %d  (Calificación %.1f)", pl.notas, pl.study, pl.average());
    gfx_text(20, 28, b, RGB5(120,200,140), 1);
    char pn[48];
    for (int i = 0; i < N_ARMORS + N_ITEMS + 1; i++) {
        int y = 46 + i * 14; bool s = (i == shop_sel);
        if (i < N_ARMORS) {
            padf(pn, sizeof pn, ARMORS[i].name, 26); int t = pl.armor_tier[i];
            if (t < 0) snprintf(b, sizeof b, "%c%s %4d", s ? '>' : ' ', pn, ARMORS[i].cost);
            else if (t >= 3) snprintf(b, sizeof b, "%c%s Nv.3 (máx.)", s ? '>' : ' ', pn);
            else snprintf(b, sizeof b, "%c%s Nv.%d -> %d: %d", s ? '>' : ' ', pn, t, t + 1, ARMORS[i].up_cost[t]);
        } else if (i < N_ARMORS + N_ITEMS) {
            padf(pn, sizeof pn, ITEM_NAME[i - N_ARMORS], 26); snprintf(b, sizeof b, "%c%s %4d  (x%d)", s ? '>' : ' ', pn, ITEM_COST[i - N_ARMORS], pl.items[i - N_ARMORS]);
        } else snprintf(b, sizeof b, "%cSalir", s ? '>' : ' ');
        gfx_text(20, y, b, s ? RGB5(250,250,240) : RGB5(120,120,130), 1);
    }
    /* detalle */
    int y = 46 + (N_ARMORS + N_ITEMS + 1) * 14 + 8;
    if (shop_sel < N_ARMORS) {
        const ArmorDef &d = ARMORS[shop_sel]; int t = pl.armor_tier[shop_sel];
        gfx_text(20, y, d.desc, RGB5(200,170,100), 1);
        if (t < 0 && d.buy_study) { snprintf(b, sizeof b, "Requiere %d puntos académicos para comprarla.", d.buy_study); gfx_text(20, y + 12, b, RGB5(220,120,100), 1); }
        else if (t >= 0 && t < 3) { snprintf(b, sizeof b, "Mejora a Nv.%d: requiere %d puntos académicos.", t + 1, d.req_study[t]); gfx_text(20, y + 12, b, pl.study >= d.req_study[t] ? RGB5(130,210,150) : RGB5(220,120,100), 1); }
    } else if (shop_sel < N_ARMORS + N_ITEMS) gfx_text(20, y, ITEM_DESC[shop_sel - N_ARMORS], RGB5(200,170,100), 1);
    if (g_msg_t > 0) gfx_text_c(244, g_msg, RGB5(240,160,70), 1);
    gfx_text(20, 252, "X: comprar / mejorar    O: salir", RGB5(80,80,90), 1);
}

/* ======================= Bucle principal ======================= */
static void game_frame(const Input &in) {
    g_frame++;
    if (g_state != ST_TITLE && g_state != ST_DIFF && g_state != ST_LOADMENU && g_state != ST_ENDING) g_playframes++;
    switch (g_state) {
        case ST_TITLE:     frame_title(in); break;
        case ST_DIFF:      frame_diff(in); break;
        case ST_LOADMENU:  frame_loadmenu(in); break;
        case ST_CUTSCENE:
            cut.update(in); cut.draw();
            if (cut.finished) {
                ds.intro_done = true; dz_killed = 0; in_bed = false;
                enter_zone(Z_DARK, dark_sx, dark_sy); face = 2;
                start_dialog(FALL_TXT, ST_PLAY);
            }
            break;
        case ST_PLAY:      frame_play(in); break;
        case ST_DIALOG:    frame_dialog(in); break;
        case ST_MENU:      frame_menu(in); break;
        case ST_PAUSE:     frame_pause(in); break;
        case ST_CHOICE:    frame_choice(in); break;
        case ST_SHOP:      frame_shop(in); break;
        case ST_ENCOUNTER: frame_encounter(); break;
        case ST_BATTLE:    frame_battle(in); break;
        case ST_FAINT:     frame_faint(); break;
        case ST_DAYCARD:   frame_daycard(in); break;
        case ST_ENDING:    frame_ending(in); break;
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
