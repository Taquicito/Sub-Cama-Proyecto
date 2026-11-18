/* day_system.h - Calendario de 8 jornadas (Domingo -> Domingo).
 *
 * ESTRUCTURA:  8 días (day 0..7)  x  3 franjas (slot 0 Mañana, 1 Tarde, 2 Noche).
 *  - Cada día empieza en la cama de la habitación (Mañana).
 *  - Estudiar, hacer tareas y entrar a la Zona Oscura consumen UNA franja.
 *  - Cuando slot llega a 3 el día se agotó: hay que volver a casa y dormir.
 *  - Dormir llama a end_day(): avanza el día, aplica el bono de tareas del día
 *    anterior, restaura vida/SP/linterna y prepara el informe matinal.
 *  - El día 7 (Domingo final) al dormir se muestra el desenlace.
 * Todo lo que cambia con el calendario (dificultad, monstruos nuevos, horario de
 * tiendas) se calcula a partir de (day, slot): no hay nada aleatorio ni permadeath. */
#ifndef DAY_SYSTEM_H
#define DAY_SYSTEM_H

#include "player_stats.h"

#define N_FRIENDS 3          /* 0 Marcos, 1 Valeria, 2 Sofía */

class DaySystem {
public:
    int day, slot;
    bool hw_done;            /* hizo tareas hoy: bono al despertar */
    int study_today;         /* veces que estudió hoy */
    int talked_mask;         /* amigos con los que ya habló hoy */
    bool dark_today;         /* visitó la Zona Oscura hoy */
    bool intro_done;         /* ya vio la cinemática del primer domingo */
    bool boss_down;          /* venció a La Cena Fría */
    int study_bonus;         /* bono de Sofía para el próximo estudio */
    int bond[N_FRIENDS];     /* vínculo con cada amigo (0..5) */

    DaySystem() { reset(); }
    void reset() {
        day = 0; slot = 0; hw_done = false; study_today = 0; talked_mask = 0; dark_today = false;
        intro_done = false; boss_down = false; study_bonus = 0;
        for (int i = 0; i < N_FRIENDS; i++) bond[i] = 0;
    }
    bool day_over() const { return slot >= 3; }
    bool is_last_day() const { return day >= N_DAYS - 1; }
    const char *day_name() const { return DAY_NAME[day]; }
    const char *slot_name() const { return slot < 3 ? SLOT_NAME[slot] : "Medianoche"; }
    /* Consume una franja. Devuelve true si el día quedó agotado. */
    bool consume_slot() { if (slot < 3) slot++; return day_over(); }

    /* Monstruos que ya pueden aparecer: se desbloquean con los días */
    int max_enemy_type() const {
        int m = 0;
        for (int t = 0; t < BOSS_TYPE; t++) if (day >= EDEFS[t].unlock_day) m = t;
        return m;
    }
    /* Tamaño máximo de grupo enemigo: crece con los días */
    int max_group() const { int g = 1 + day / 3; return g > 3 ? 3 : g; }
    /* Escalado de estadísticas enemigas */
    float enemy_scale() const { return 1.0f + 0.12f * day; }
    /* Consumo de linterna por minuto en la Zona Oscura (aumenta con los días) */
    int battery_drain() const { return 1 + day / 3; }

    /* Termina el día: aplica bonos, restaura y avanza. Devuelve líneas del informe. */
    int end_day(Player &p, char rep[4][96]) {
        int n = 0;
        if (hw_done) {
            int bonus = 18 + 4 * day + p.study / 2;
            p.notas += bonus; p.study += 2; p.recalc();
            snprintf(rep[n++], 96, "Bono por tareas: +%d Notas", bonus);
            snprintf(rep[n++], 96, "Puntos académicos +2 (total %d)", p.study);
        }
        day++; slot = 0; hw_done = false; study_today = 0; talked_mask = 0; dark_today = false;
        p.hp = p.maxhp; p.sp = p.maxsp; p.battery = 100; p.clear_status();
        snprintf(rep[n++], 96, "Descansas. Vida, SP y linterna restaurados.");
        return n;
    }
};

#endif
