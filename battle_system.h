/* battle_system.h - Combate por turnos estilo Persona 3 / Pokémon.
 *  - Iniciativa por AGILIDAD (Leo vs enemigos), ronda a ronda.
 *  - Comandos: ATACAR / HABILIDADES-PSI / OBJETOS / HUIR.
 *  - Debilidades elementales: golpear una debilidad da "UN TURNO MÁS".
 *  - Estados psicológicos: Desesperanza, Apatía, Terror.
 *  - LUZ: la batería de la linterna sube o baja el daño que das y recibes.
 *  - Armadura de Plantas: regenera vida cada turno.
 *  - Resumen: EXP (x dificultad), Notas y SUBIDA DE NIVEL AUTOMÁTICA. */
#ifndef BATTLE_SYSTEM_H
#define BATTLE_SYSTEM_H

#include <stdarg.h>
#include <stdlib.h>
#include "render.h"
#include "platform.h"
#include "player_stats.h"
#include "assets.h"

class Battle {
public:
    enum Phase { PH_INTRO, PH_ROUND, PH_NEXT, PH_MENU, PH_SKILLS, PH_ITEMS, PH_TARGET, PH_LOG, PH_WIN, PH_LOSE, PH_END };
    int phase, result;                  /* result: 0 en curso, 1 victoria, 2 derrota, 3 huida */
    Player *P;
    Combatant foe[3];
    int ftype[3], ow_index[3], nf;
    int diff, day;
    float scale;
    int r_exp, r_notas, r_item;         /* recompensas */
    int lvups, old_lv, win_page;        /* subida de nivel automática */
    int gain_hp, gain_sp, gain_atk, gain_def, gain_agi, gain_mag;
    int new_skill[N_SKILLS], n_new;
    bool boss_beaten;

    Battle() : phase(PH_END), result(0), P(0), nf(0), diff(1), day(0), scale(1.0f), r_exp(0), r_notas(0), r_item(-1), lvups(0), old_lv(1), win_page(0),
               gain_hp(0), gain_sp(0), gain_atk(0), gain_def(0), gain_agi(0), gain_mag(0), n_new(0), boss_beaten(false),
               order_n(0), cur(0), bonus(false), fled(false), menu_sel(0), sub_sel(0), tgt_sel(0),
               pend_kind(0), pend_idx(0), n_msgs(0), msg_i(0), msg_t(0), flash(0), shake(0), t(0), round_no(0),
               disp_php(0), disp_psp(0), hint_t(0) {
        for (int i = 0; i < 4; i++) blink[i] = 0;
        for (int i = 0; i < 3; i++) { ftype[i] = 0; ow_index[i] = -1; disp_hp[i] = 0; }
        for (int i = 0; i < NPOP; i++) pop[i].t = 0;
        for (int i = 0; i < NBL; i++) bl[i].life = 0;
        hint[0] = 0;
    }

    /* ---------- Inicio: crea enemigos escalados por profundidad y dificultad ---------- */
    void start(Player *p, const int *types, const int *ow, int n, int day_, float scale_, int dif) {
        P = p; nf = n > 3 ? 3 : n; day = day_; scale = scale_; diff = dif; boss_beaten = false;
        const DiffCfg &c = DIFFS[diff];
        for (int i = 0; i < nf; i++) {
            const EnemyDef &d = EDEFS[types[i]];
            Combatant &f = foe[i];
            f.clear();
            float sc = scale;
            f.maxhp = f.hp = (int)(d.hp * sc * c.ehp); f.atk = (int)(d.atk * sc); f.def = d.def + day / 2;
            f.agi = d.agi; f.mag = (int)(d.mag * sc);
            f.weak_mask = 1 << d.weak; f.resist_mask = 1 << d.resist;
            int dup = 0; for (int j = 0; j < i; j++) if (types[j] == types[i]) dup++;
            snprintf(f.name, sizeof f.name, dup ? "%s %c" : "%s", d.name, 'A' + dup);
            ftype[i] = types[i]; ow_index[i] = ow[i]; disp_hp[i] = f.hp;
        }
        P->clear_status();
        disp_php = P->hp; disp_psp = P->sp;
        phase = PH_INTRO; result = 0; t = 0; round_no = 0; bonus = false; fled = false;
        n_msgs = 0; flash = 6; shake = 0; hint[0] = 0; lvups = 0; win_page = 0; n_new = 0;
        for (int i = 0; i < NPOP; i++) pop[i].t = 0;
        for (int i = 0; i < NBL; i++) bl[i].life = 0;
    }

    /* ---------- Actualizacion por cuadro ---------- */
    void update(const Input &in) {
        t++;
        animate();
        bool ok = (in.pressed & IN_CROSS) != 0, back = (in.pressed & IN_CIRCLE) != 0;
        int mv = ((in.pressed & IN_DOWN) ? 1 : 0) - ((in.pressed & IN_UP) ? 1 : 0);
        int mh = ((in.pressed & IN_RIGHT) ? 1 : 0) - ((in.pressed & IN_LEFT) ? 1 : 0);
        switch (phase) {
        case PH_INTRO: if (t > 50) phase = PH_ROUND; break;
        case PH_ROUND: build_order(); break;
        case PH_NEXT: next_actor(); break;
        case PH_MENU:
            menu_sel = (menu_sel + 4 + mv) % 4;
            if (ok) menu_choose();
            else if (back) exec_flee();            /* Circulo: huir */
            break;
        case PH_SKILLS: {
            int n = known_count();
            if (n == 0) { phase = PH_MENU; break; }
            sub_sel = (sub_sel + n + mv) % n;
            if (back) { phase = PH_MENU; break; }
            if (ok) {
                int s = known_skill(sub_sel);
                if (P->sp < SKILLS[s].cost) { set_hint("SP INSUFICIENTE"); break; }
                pend_kind = 1; pend_idx = s;
                if (SKILLS[s].target == TG_ONE) begin_target(); else exec_skill(s, -1);
            }
            break; }
        case PH_ITEMS:
            sub_sel = (sub_sel + N_ITEMS + mv) % N_ITEMS;
            if (back) { phase = PH_MENU; break; }
            if (ok) { if (P->items[sub_sel] > 0) exec_item(sub_sel); else set_hint("NO QUEDA"); }
            break;
        case PH_TARGET:
            if (mh || mv) cycle_target(mh ? mh : mv);
            if (back) { phase = (pend_kind == 1) ? PH_SKILLS : PH_MENU; break; }
            if (ok) { if (pend_kind == 0) exec_attack(tgt_sel); else exec_skill(pend_idx, tgt_sel); }
            break;
        case PH_LOG:
            msg_t++;
            if ((ok && msg_t > 8) || msg_t > 70) { msg_i++; msg_t = 0; if (msg_i >= n_msgs) after_log(); }
            break;
        case PH_WIN:
            if (ok && t > 20) { if (win_page == 0 && lvups > 0) { win_page = 1; t = 0; flash = 10; } else { result = 1; phase = PH_END; } }
            break;
        case PH_LOSE: if (t > 200 || (ok && t > 90)) { result = 2; phase = PH_END; } break;
        default: break;
        }
    }

    /* ---------- Dibujo ---------- */
    void draw() {
        int ox = shake > 0 ? ((t & 1) ? shake : -shake) : 0;
        for (int y = 0; y < 164; y++) gfx_rect(0, y, SCR_W, 1, rgb(16 + y / 14, 8 + y / 30, 18 + y / 20));
        for (int y = 112; y < 164; y += 8)                               /* suelo de colchon */
            for (int x = ((y / 8) & 1) * 16; x < SCR_W; x += 32) gfx_rect(x, y, 2, 2, RGB5(60,30,44));
        gfx_rect(0, 110, SCR_W, 1, RGB5(70,30,44));
        gfx_static(260, 30, 90);
        for (int i = 0; i < nf; i++) {
            if (!foe[i].alive() && blink[i + 1] <= 0) continue;
            int fx = foe_x(i) + ox, fy = foe_y();
            if (blink[i + 1] > 0 && (blink[i + 1] & 2)) { /* parpadeo al recibir dano */ }
            else gfx_sprite_n(SPR_ENEMY[ftype[i]], 16, 16, PAL_ENEMY[ftype[i]], fx - 32, fy, 4, 0);
            if (foe[i].alive()) {
                int nw = u8len(foe[i].name) * 8;
                gfx_text_sh(fx - nw / 2, fy - 22, foe[i].name, RGB5(200,190,190), 1);
                gfx_bar(fx - 28, fy - 10, 56, 5, disp_hp[i], foe[i].maxhp, RGB5(170,24,30), RGB5(30,10,12));
                if (phase == PH_TARGET && i == tgt_sel && ((t / 6) & 1)) gfx_text(fx - 4, fy + 68, "^", RGB5(255,220,80), 1);
            }
        }
        gfx_sprite(SPR_LEO, PAL_LEO, 44 + ox, 106, 4, 0);               /* Leo de espaldas al mundo */
        for (int i = 0; i < NBL; i++) if (bl[i].life > 0) gfx_rect((int)bl[i].x, (int)bl[i].y, 2, 2, RGB5(150,10,16));
        for (int i = 0; i < NPOP; i++) if (pop[i].t > 0) gfx_text_sh(pop[i].x, pop[i].y - (40 - pop[i].t) / 2, pop[i].txt, pop[i].col, 2);
        if (flash > 0) gfx_add(flash > 12 ? 12 : flash);
        if (P->hp * 100 / (P->maxhp ? P->maxhp : 1) < 25) {              /* borde rojo con poca vida */
            gfx_rect(0, 0, SCR_W, 2, RGB5(120,0,0)); gfx_rect(0, 162, SCR_W, 2, RGB5(120,0,0));
        }
        draw_order();
        draw_panel();
    }

private:
    enum { NPOP = 8, NBL = 48 };
    struct Popup { int x, y, t, col; char txt[12]; } pop[NPOP];
    struct Blood { float x, y, vx, vy; int life; } bl[NBL];
    int order[4], order_n, cur;
    bool bonus, fled;
    int menu_sel, sub_sel, tgt_sel, pend_kind, pend_idx;
    char msgs[10][80]; int n_msgs, msg_i, msg_t;
    int flash, shake, blink[4], t, round_no;
    int disp_hp[3], disp_php, disp_psp, hint_t;
    char hint[32];

    /* ----- utilidades ----- */
    Combatant &actor(int a) { return a == 0 ? (Combatant &)*P : foe[a - 1]; }
    int foe_x(int i) const { return nf == 1 ? 300 : nf == 2 ? 190 + i * 150 : 150 + i * 140; }
    int foe_y() const { return 36; }
    void set_hint(const char *s) { strncpy(hint, s, 31); hint[31] = 0; hint_t = 60; }
    void say(const char *fmt, ...) {
        if (n_msgs >= 10) return;
        va_list ap; va_start(ap, fmt); vsnprintf(msgs[n_msgs], 80, fmt, ap); va_end(ap); n_msgs++;
    }
    void play_log() { phase = PH_LOG; msg_i = 0; msg_t = 0; }
    int known_count() const { int n = 0; for (int i = 0; i < N_SKILLS; i++) if (P->knows(i)) n++; return n; }
    int known_skill(int k) const { for (int i = 0; i < N_SKILLS; i++) if (P->knows(i) && k-- == 0) return i; return 0; }
    int alive_foes() const { int n = 0; for (int i = 0; i < nf; i++) if (foe[i].alive()) n++; return n; }

    void animate() {
        for (int i = 0; i < nf; i++) { if (disp_hp[i] > foe[i].hp) disp_hp[i] -= 1 + (disp_hp[i] - foe[i].hp) / 8; if (disp_hp[i] < foe[i].hp) disp_hp[i] = foe[i].hp; }
        if (disp_php > P->hp) disp_php -= 1 + (disp_php - P->hp) / 8; else if (disp_php < P->hp) disp_php += 1 + (P->hp - disp_php) / 8;
        if (disp_psp > P->sp) disp_psp--; else if (disp_psp < P->sp) disp_psp++;
        if (flash > 0) flash--;
        if (shake > 0) shake--;
        if (hint_t > 0) hint_t--;
        for (int i = 0; i < 4; i++) if (blink[i] > 0) blink[i]--;
        for (int i = 0; i < NPOP; i++) if (pop[i].t > 0) pop[i].t--;
        for (int i = 0; i < NBL; i++) if (bl[i].life > 0) { bl[i].life--; bl[i].x += bl[i].vx; bl[i].y += bl[i].vy; bl[i].vy += 0.25f; }
    }
    void hit_fx(int a, int amount, bool weak) {          /* a: 0 Leo, 1..3 enemigo */
        int cx = a == 0 ? 60 : foe_x(a - 1), cy = a == 0 ? 120 : foe_y() + 30;
        blink[a] = 16; flash = weak ? 14 : 8; shake = weak ? 8 : 5;
        char b[12]; snprintf(b, sizeof b, "%d", amount);
        for (int i = 0; i < NPOP; i++) if (pop[i].t <= 0) {
            pop[i].x = cx - 8; pop[i].y = cy - 10; pop[i].t = 40; strcpy(pop[i].txt, b);
            pop[i].col = weak ? RGB5(255,230,80) : (a == 0 ? RGB5(255,90,90) : RGB5(240,240,240)); break;
        }
        for (int k = 0, n = 0; k < NBL && n < 14; k++) if (bl[k].life <= 0) {   /* sangre */
            bl[k].x = (float)cx; bl[k].y = (float)cy; bl[k].vx = (float)(rndr(9) - 4) * 0.7f; bl[k].vy = -(float)rndr(5) - 1;
            bl[k].life = 30 + rndr(20); n++;
        }
    }

    /* ----- orden por iniciativa (agilidad) ----- */
    void build_order() {
        round_no++;
        if (round_no > 1) { P->battery -= 1 + day / 4; if (P->battery < 0) P->battery = 0; }   /* la linterna se gasta */
        int keys[4]; order_n = 0;
        order[order_n] = 0; keys[order_n++] = P->agi * 4 + rndr(4);
        for (int i = 0; i < nf; i++) if (foe[i].alive()) { order[order_n] = i + 1; keys[order_n++] = foe[i].agi * 4 + rndr(4); }
        for (int i = 0; i < order_n; i++)
            for (int j = i + 1; j < order_n; j++)
                if (keys[j] > keys[i]) { int k = keys[i]; keys[i] = keys[j]; keys[j] = k; k = order[i]; order[i] = order[j]; order[j] = k; }
        cur = 0; phase = PH_NEXT;
    }
    void next_actor() {
        while (cur < order_n && !actor(order[cur]).alive()) cur++;
        if (cur >= order_n) { phase = PH_ROUND; return; }
        int a = order[cur];
        n_msgs = 0;
        if (a == 0) {                                      /* turno de Leo */
            if (P->regen > 0 && P->hp < P->maxhp) { int h = P->regen; P->heal(h); say("La %s regenera %d PV.", ARMORS[P->armor_eq].name, h); }
            if (P->has(SX_DESPAIR) && P->sp > 0) { P->sp -= 2; if (P->sp < 0) P->sp = 0; say("La desesperanza drena tus fuerzas. (-2 SP)"); }
            if (P->has(SX_APATHY) && rndr(100) < 50) { say("Leo está demasiado apático para moverse..."); play_log(); return; }
            if (n_msgs) { menu_sel = 0; play_log_then_menu = true; play_log(); return; }
            menu_sel = 0; phase = PH_MENU;
        } else foe_act(a - 1);
    }
    bool play_log_then_menu = false;

    void after_log() {
        n_msgs = 0;
        if (alive_foes() == 0) { enter_win(); return; }
        if (P->hp <= 0) { phase = PH_LOSE; t = 0; return; }
        if (fled) { result = 3; phase = PH_END; return; }
        if (play_log_then_menu) { play_log_then_menu = false; menu_sel = 0; phase = PH_MENU; return; }
        if (bonus) bonus = false;
        else { if (cur < order_n && order[cur] == 0) P->tick_status(); cur++; }
        phase = PH_NEXT;
    }

    /* ----- seleccion ----- */
    void menu_choose() {
        if (menu_sel == 0) { pend_kind = 0; if (alive_foes() == 1) exec_attack(first_alive()); else begin_target(); }
        else if (menu_sel == 1) { sub_sel = 0; phase = PH_SKILLS; }
        else if (menu_sel == 2) { sub_sel = 0; phase = PH_ITEMS; }
        else exec_flee();
    }
    int first_alive() const { for (int i = 0; i < nf; i++) if (foe[i].alive()) return i; return 0; }
    void begin_target() { tgt_sel = first_alive(); phase = PH_TARGET; }
    void cycle_target(int d) {
        for (int k = 0; k < nf; k++) { tgt_sel = (tgt_sel + nf + d) % nf; if (foe[tgt_sel].alive()) break; }
    }

    /* ----- formulas de dano ----- */
    float light_p() const { return P->battery >= 60 ? 1.10f : (P->battery <= 20 ? 0.85f : 1.0f); }   /* luz buena = más daño */
    float light_f() const { return P->battery >= 60 ? 0.90f : (P->battery <= 20 ? 1.20f : 1.0f); }   /* luz mala = enemigos más fuertes */
    int calc(const Combatant &a, const Combatant &d, int power, int el, bool magic, bool &weak, bool &res, bool &crit) {
        int base = magic ? power + a.mag * 2 + rndr(4) - d.def / 2 : a.atk * 2 + power + rndr(4) - d.def;
        if (a.has(SX_DESPAIR)) base = base * 3 / 4;
        float m = light_p(); weak = res = crit = false;
        if (d.weak_mask & (1 << el)) { m *= 1.5f; weak = true; }
        if (d.resist_mask & (1 << el)) { m *= 0.5f; res = true; }
        if (!magic && rndr(12) == 0) { m *= 1.5f; crit = true; }
        int dmg = (int)(base * m);
        return dmg < 1 ? 1 : dmg;
    }
    void foe_down(int i) {
        const EnemyDef &d = EDEFS[ftype[i]];
        foe[i].hp = 0; P->kills++;
        say("%s se desvanece...", foe[i].name);
        say("%s", d.defeat);
    }
    /* Golpea a un enemigo: devuelve true si fue debilidad */
    bool hit_foe(int i, int power, int el, bool magic) {
        bool w, r, c;
        int dmg = calc(*P, foe[i], power, el, magic, w, r, c);
        foe[i].hp -= dmg; if (foe[i].hp < 0) foe[i].hp = 0;
        hit_fx(i + 1, dmg, w);
        if (c) say("¡Golpe crítico!");
        if (w) { say("¡DÉBIL! %s tiembla.", foe[i].name); P->known_weak[ftype[i]] |= (1 << el); }
        if (r) say("%s resiste el golpe.", foe[i].name);
        if (!foe[i].alive()) foe_down(i);
        return w;
    }
    bool terror_fail() {
        if (P->has(SX_TERROR) && rndr(100) < 35) { say("El terror paraliza a Leo. ¡Falla!"); return true; }
        return false;
    }

    /* ----- acciones de Leo ----- */
    void exec_attack(int i) {
        n_msgs = 0;
        if (!terror_fail()) {
            say("Leo ataca a %s.", foe[i].name);
            if (hit_foe(i, 0, EL_PHYS, false)) { bonus = true; say("¡UN TURNO MÁS para Leo!"); }
        }
        play_log();
    }
    void exec_skill(int s, int tg) {
        const Skill &k = SKILLS[s];
        n_msgs = 0; P->sp -= k.cost;
        if (k.kind == K_DMG && terror_fail()) { play_log(); return; }
        say("¡Leo usa %s!", k.name);
        if (k.kind == K_DMG) {
            bool any = false;
            if (k.target == TG_ALL) { for (int i = 0; i < nf; i++) if (foe[i].alive()) any |= hit_foe(i, k.power, k.el, true); }
            else any = hit_foe(tg, k.power, k.el, true);
            if (any) { bonus = true; say("¡UN TURNO MÁS para Leo!"); }
        } else if (k.kind == K_HEAL) {
            int h = k.power + P->mag * 2; P->heal(h); say("Leo respira hondo. Recupera %d de vida.", h);
        } else {
            P->clear_status(); P->heal(10 + P->mag); say("Te aferras a un buen recuerdo. Los estados se disipan.");
        }
        play_log();
    }
    void exec_item(int it) {
        n_msgs = 0; P->items[it]--;
        say("Leo usa %s.", ITEM_NAME[it]);
        if (it == 0) { P->heal(35); say("Recupera 35 de vida."); }
        else if (it == 1) { P->heal_sp(12); say("Recupera 12 SP."); }
        else if (it == 2) { P->clear_status(); P->heal(10); say("Recuerdas un día bueno. Los estados se disipan."); }
        else { P->battery += 40; if (P->battery > 100) P->battery = 100; say("La linterna recupera 40% de luz."); }
        play_log();
    }
    void exec_flee() {
        n_msgs = 0;
        int avg = 0; for (int i = 0; i < nf; i++) avg += foe[i].agi; avg /= (nf ? nf : 1);
        int ch = 50 + (P->agi - avg) * 5 - (diff == 3 ? 10 : 0) - (P->has(SX_TERROR) ? 20 : 0);
        if (ch < 15) ch = 15;
        if (ch > 90) ch = 90;
        say("Leo intenta huir...");
        if (rndr(100) < ch) { fled = true; say("Escapas entre las sombras."); }
        else say("¡No hay adónde correr!");
        play_log();
    }

    /* ----- IA enemiga ----- */
    int ai_pick(int i) {
        const EnemyDef &d = EDEFS[ftype[i]];
        int ids[3] = { 0, d.sk1, d.sk2 }, ai = DIFFS[diff].ai;
        if (ai == 0) return rndr(100) < 70 ? 0 : d.sk1;
        if (ai == 1) return ids[rndr(3)];
        if (ai == 2) {
            if (!P->any_status() && rndr(100) < 60) { int c = rndr(2) ? d.sk1 : d.sk2; if (ESK[c].status >= 0) return c; }
            return ids[rndr(3)];
        }
        for (int k = 1; k < 3; k++)                         /* Pesadilla: explota tu debilidad */
            if (ESK[ids[k]].power > 0 && (P->weak_mask & (1 << ESK[ids[k]].el)) && rndr(100) < 85) return ids[k];
        if (!P->any_status()) for (int k = 1; k < 3; k++) if (ESK[ids[k]].status >= 0) return ids[k];
        return ESK[ids[1]].power >= ESK[ids[2]].power ? ids[1] : ids[2];
    }
    void foe_act(int i) {
        Combatant &f = foe[i];
        const EnemySkill &s = ESK[ai_pick(i)];
        say("%s usa %s.", f.name, s.name);
        bool status_only = (s.power == 0 && s.status >= 0);
        if (!status_only) {
            bool phys = (s.el == EL_PHYS);
            int base = phys ? f.atk * 2 + s.power + rndr(4) - P->def : s.power + f.mag * 2 + rndr(4) - P->def / 2;
            float m = DIFFS[diff].edmg * light_f(); bool weak = false;
            if (P->weak_mask & (1 << s.el)) { m *= 1.5f; weak = true; }
            int dmg = (int)(base * m); if (dmg < 1) dmg = 1;
            P->hp -= dmg; if (P->hp < 0) P->hp = 0;
            hit_fx(0, dmg, weak);
            if (weak) {
                say("¡DÉBIL! Leo siente el golpe en lo más hondo.");
                if (DIFFS[diff].ai >= 2) { bonus = true; say("¡El enemigo explota tu debilidad y actúa otra vez!"); }
            }
        }
        if (s.status >= 0 && P->hp > 0 && rndr(100) < s.chance + (diff == 3 ? 15 : 0)) {
            P->add_status(s.status, s.turns);
            say("¡Leo sufre %s!", SX_NAME[s.status]);
        } else if (status_only) say("Leo resiste... esta vez.");
        if (P->hp <= 0) say("Leo cae de rodillas. La oscuridad lo reclama.");
        play_log();
    }

    /* ----- victoria: EXP x dificultad, Notas, y SUBIDA DE NIVEL AUTOMÁTICA ----- */
    void enter_win() {
        float exp_f = 0, nt_f = 0;
        for (int i = 0; i < nf; i++) {
            exp_f += EDEFS[ftype[i]].exp * (1.0f + 0.05f * day); nt_f += EDEFS[ftype[i]].notas * (1.0f + 0.1f * day);
            if (ftype[i] == BOSS_TYPE) boss_beaten = true;
        }
        r_exp = (int)(exp_f * DIFFS[diff].exp_mult + 0.5f); if (r_exp < 1) r_exp = 1;
        r_notas = (int)(nt_f + 0.5f);
        r_item = (rndr(100) < DIFFS[diff].drop) ? rndr(N_ITEMS) : -1;
        P->notas += r_notas; if (r_item >= 0) P->items[r_item]++;
        old_lv = P->lv;
        int b_hp = P->base_maxhp, b_sp = P->maxsp, b_atk = P->atk, b_def = P->base_def, b_agi = P->agi, b_mag = P->base_mag;
        lvups = P->gain_exp(r_exp);
        gain_hp = P->base_maxhp - b_hp; gain_sp = P->maxsp - b_sp; gain_atk = P->atk - b_atk;
        gain_def = P->base_def - b_def; gain_agi = P->agi - b_agi; gain_mag = P->base_mag - b_mag;
        n_new = 0;
        for (int s = 0; s < N_SKILLS; s++) if (P->knows(s) && SKILLS[s].unlock > old_lv) new_skill[n_new++] = s;
        phase = PH_WIN; win_page = 0; t = 0;
    }

    /* ----- interfaz ----- */
    void draw_order() {
        char b[64] = "TURNO:"; int n = 6;
        for (int i = cur; i < order_n && n < 52; i++) {
            if (!actor(order[i]).alive()) continue;
            const char *nm = actor(order[i]).name;
            n += snprintf(b + n, sizeof b - n, " %c%c%c%c", nm[0], nm[1], nm[2] ? nm[2] : ' ', nm[2] && nm[3] ? nm[3] : ' ');
        }
        if (phase != PH_INTRO) gfx_text_sh(6, 6, b, RGB5(150,140,150), 1);
        char r[24]; snprintf(r, sizeof r, "RONDA %d", round_no);
        gfx_text_sh(SCR_W - 8 - u8len(r) * 8, 6, r, RGB5(110,100,110), 1);
    }
    void draw_box(int x, int y, int w, int h) { gfx_rect(x, y, w, h, RGB5(8,6,12)); gfx_frame(x, y, w, h, RGB5(150,150,160)); gfx_frame(x + 2, y + 2, w - 4, h - 4, RGB5(50,50,60)); }
    void draw_panel() {
        gfx_rect(0, 164, SCR_W, 108, RGB5(4,3,6));
        if (phase == PH_INTRO) { draw_box(8, 170, 464, 94); gfx_text_c(210, "Algo se mueve en la oscuridad...", RGB5(210,210,200), 1); return; }
        if (phase == PH_LOG && msg_i < n_msgs) { draw_box(8, 170, 464, 94); gfx_text(22, 190, msgs[msg_i], RGB5(210,210,200), 1); if ((t / 20) & 1) gfx_text(448, 246, "X", RGB5(200,200,200), 1); return; }
        if (phase == PH_WIN) { draw_win(); return; }
        if (phase == PH_LOSE) { draw_box(8, 170, 464, 94); gfx_text_c(200, "LEO HA CAIDO", RGB5(170,20,25), 2); gfx_text_c(232, "La oscuridad se lo traga un poco más...", RGB5(140,140,140), 1); return; }
        /* estado de Leo */
        draw_box(8, 170, 226, 94);
        char b[48]; snprintf(b, sizeof b, "LEO   Nv.%d", P->lv); gfx_text(18, 178, b, RGB5(220,220,210), 1);
        gfx_text(18, 194, "HP", RGB5(200,60,60), 1); gfx_bar(40, 195, 120, 7, disp_php, P->maxhp, RGB5(170,24,30), RGB5(30,10,12));
        snprintf(b, sizeof b, "%d/%d", P->hp, P->maxhp); gfx_text(166, 194, b, RGB5(200,200,200), 1);
        gfx_text(18, 208, "SP", RGB5(80,120,200), 1); gfx_bar(40, 209, 120, 7, disp_psp, P->maxsp, RGB5(60,100,190), RGB5(10,14,30));
        snprintf(b, sizeof b, "%d/%d", P->sp, P->maxsp); gfx_text(166, 208, b, RGB5(200,200,200), 1);
        { gfx_text(18, 222, "LUZ", RGB5(230,210,120), 1); gfx_bar(46, 223, 80, 6, P->battery, 100, P->battery <= 20 ? RGB5(220,60,40) : RGB5(230,200,90), RGB5(30,26,10));
          snprintf(b, sizeof b, "%d%%", P->battery); gfx_text(132, 222, b, RGB5(200,200,200), 1);
          if (P->battery >= 60) gfx_text(170, 222, "+DAÑO", RGB5(120,200,120), 1); else if (P->battery <= 20) gfx_text(170, 222, "-DAÑO", RGB5(220,90,70), 1); }
        int sx = 18;
        for (int i = 0; i < SX_COUNT; i++) if (P->has(i)) { snprintf(b, sizeof b, "[%s %d]", SX_SHORT[i], P->status_t[i]); gfx_text(sx, 236, b, RGB5(200,120,60), 1); sx += u8len(b) * 8 + 4; if (sx > 160) { sx = 18; } }
        if (hint_t > 0) gfx_text(18, 250, hint, RGB5(255,120,80), 1);
        /* menu / submenus */
        draw_box(242, 170, 230, 94);
        if (phase == PH_MENU) {
            static const char *cmd[4] = { "ATACAR", "HABILIDADES/PSI", "OBJETOS", "HUIR" };
            for (int i = 0; i < 4; i++) {
                snprintf(b, sizeof b, i == menu_sel ? "> [%s]" : "  [%s]", cmd[i]);
                gfx_text(254, 180 + i * 19, b, i == menu_sel ? RGB5(250,250,240) : RGB5(110,110,120), 1);
            }
        } else if (phase == PH_SKILLS) {
            int n = known_count();
            for (int k = 0; k < n; k++) {
                int s = known_skill(k);
                char pn[40]; padf(pn, sizeof pn, SKILLS[s].name, 15);
                snprintf(b, sizeof b, "%c%s %2dSP", k == sub_sel ? '>' : ' ', pn, SKILLS[s].cost);
                gfx_text(250, 176 + k * 13, b, k == sub_sel ? RGB5(250,250,240) : (P->sp >= SKILLS[s].cost ? RGB5(140,140,150) : RGB5(80,60,60)), 1);
            }
            gfx_text(250, 246, SKILLS[known_skill(sub_sel)].desc, RGB5(170,150,100), 1);
        } else if (phase == PH_ITEMS) {
            for (int k = 0; k < N_ITEMS; k++) {
                char pn[40]; padf(pn, sizeof pn, ITEM_NAME[k], 16);
                snprintf(b, sizeof b, "%c%s x%d", k == sub_sel ? '>' : ' ', pn, P->items[k]);
                gfx_text(250, 178 + k * 15, b, k == sub_sel ? RGB5(250,250,240) : RGB5(110,110,120), 1);
            }
            gfx_text(250, 246, ITEM_DESC[sub_sel], RGB5(170,150,100), 1);
        } else if (phase == PH_TARGET) {
            gfx_text(254, 180, "ELEGIR OBJETIVO", RGB5(220,220,210), 1);
            gfx_text(254, 198, foe[tgt_sel].name, RGB5(250,220,120), 1);
            int kw = P->known_weak[ftype[tgt_sel]];
            if (kw) { int el = EDEFS[ftype[tgt_sel]].weak; snprintf(b, sizeof b, "DÉBIL: %s", EL_NAME[el]); }
            else snprintf(b, sizeof b, "DÉBIL: ???");
            gfx_text(254, 214, b, RGB5(200,100,100), 1);
            snprintf(b, sizeof b, "HP %d/%d", foe[tgt_sel].hp, foe[tgt_sel].maxhp); gfx_text(254, 230, b, RGB5(180,180,180), 1);
        }
    }
    void draw_win() {
        draw_box(8, 170, 464, 94);
        char b[96];
        if (win_page == 1) {                               /* notificación de nivel */
            gfx_text_c(176, "¡SUBES DE NIVEL!", ((t / 6) & 1) ? RGB5(255,230,120) : RGB5(230,190,80), 2);
            snprintf(b, sizeof b, "Nv.%d  ->  Nv.%d", old_lv, P->lv);
            gfx_text_c(198, b, RGB5(240,240,230), 1);
            snprintf(b, sizeof b, "PV +%d  SP +%d  FUE +%d  DEF +%d  AGI +%d  PSI +%d", gain_hp, gain_sp, gain_atk, gain_def, gain_agi, gain_mag);
            gfx_text_c(214, b, RGB5(170,220,170), 1);
            for (int k = 0; k < n_new; k++) { snprintf(b, sizeof b, "Nueva habilidad: %s", SKILLS[new_skill[k]].name); gfx_text_c(230 + k * 11, b, RGB5(120,200,255), 1); }
            if ((t / 20) & 1) gfx_text(448, 250, "X", RGB5(200,200,200), 1);
            return;
        }
        gfx_text_c(176, "VICTORIA", RGB5(230,230,200), 2);
        snprintf(b, sizeof b, "EXP +%d  (x%.1f, %s)", r_exp, DIFFS[diff].exp_mult, DIFFS[diff].name);
        gfx_text(26, 202, b, RGB5(210,210,200), 1);
        snprintf(b, sizeof b, "NOTAS +%d", r_notas); gfx_text(26, 216, b, RGB5(120,200,140), 1);
        if (r_item >= 0) { snprintf(b, sizeof b, "Encuentras: %s", ITEM_NAME[r_item]); gfx_text(26, 230, b, RGB5(200,170,90), 1); }
        snprintf(b, sizeof b, "EXP %d/%d   Nv.%d", P->exp, P->exp_needed(), P->lv);
        gfx_text(300, 216, b, RGB5(150,150,160), 1);
        if (lvups > 0) gfx_text(26, 246, "¡Has subido de nivel!", RGB5(255,230,120), 1);
        if ((t / 20) & 1) gfx_text(448, 250, "X", RGB5(200,200,200), 1);
    }
};

#endif
