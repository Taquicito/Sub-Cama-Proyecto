/* narrative_engine.h - Cinemática familiar (3 escenas), cuadros de diálogo,
 * amigos, tarjetas de cada día y finales. Todos los textos en español con tildes. */
#ifndef NARRATIVE_ENGINE_H
#define NARRATIVE_ENGINE_H

#include "render.h"
#include "platform.h"
#include "assets.h"

enum { W_NARR, W_PAPA, W_MAMA, W_LEO };
struct DLine { int who; const char *text; };
static const char *const WHO_NAME[4] = { "", "PAPÁ", "MAMÁ", "LEO" };

/* Escena 1: comedor, domingo por la noche (años 2000) */
static const DLine L_DINING[] = {
    { W_NARR, "Domingo, 8:40 p. m. La cena se enfría.\nNadie ha tocado nada." },
    { W_PAPA, "Otra boleta con rojo. Otra más.\n¿Qué haces todo el día, Leo?" },
    { W_MAMA, "Ni siquiera nos miras cuando te hablamos..." },
    { W_PAPA, "A tu edad yo ya trabajaba.\nAsí no tienes futuro. ¿Entiendes?" },
    { W_MAMA, "Te la pasas encerrado. No sales.\nYa ni siquiera tienes amigos." },
    { W_NARR, "Sus voces suenan lejos,\ncomo si hablaran bajo el agua." },
    { W_PAPA, "¡CONTÉSTAME CUANDO TE HABLO!" },
    { W_LEO,  "...Estoy cansado." },
    { W_MAMA, "¿Cansado de qué? ¡Si no haces nada!" },
    { W_NARR, "Solo oyes el zumbido del refrigerador.\nY un peso en el pecho que no se va." },
    { W_LEO,  "Con permiso." }
};
/* Escena 2: la huida al cuarto */
static const DLine L_BEDROOM[] = {
    { W_NARR, "Cierras la puerta. El pasillo se queda\ncon los gritos. Te dejas caer en la cama." },
    { W_NARR, "Afuera llueve con fuerza. El monitor apagado\nte devuelve el reflejo de alguien cansado." },
    { W_LEO,  "Todo pesa. Hasta respirar." },
    { W_LEO,  "Nada de esto tiene sentido." },
    { W_NARR, "Miras el techo. Las manchas de humedad\nparecen rostros que te observan." }
};
#define N_DINING (int)(sizeof L_DINING / sizeof L_DINING[0])
#define N_BEDROOM (int)(sizeof L_BEDROOM / sizeof L_BEDROOM[0])

static const char *const FALL_TXT =
    "Caes. La lluvia se vuelve estática blanca\ny luego, silencio.\n"
    "ZONA OSCURA. Aquí las voces de casa\nte esperan entre las sombras.\n"
    "(Vuelve al inicio del pasillo para salir.)";

/* Notas y pistas del entorno en la Zona Oscura */
static const char *const ENV_TXT[8] = {
    "Un papel arrugado: 'MATEMÁTICAS 4.0'.\nAlguien lo tachó con rabia.",
    "Una foto de familia. Tu cara está rasgada.",
    "Escrito mil veces en la pared:\n'PODRÍAS ESFORZARTE MÁS'.",
    "Una silla vacía junto a un plato frío.\nNadie te llamó a cenar.",
    "Un teléfono descolgado. Alguien dijo\n'luego te llamo' hace años.",
    "Un diario sin páginas.\nSolo queda una palabra: cansado.",
    "Una puerta pintada en la pared.\nNo tiene manija.",
    "Voces tras el colchón: 'está bien...\nsolo es una etapa. Ya se te pasará.'"
};
static const char *const SFX_TXT[6] = {
    "KRRRRRRK-KRRRK", "*crujido húmedo*", "GRRRMMMMMMM", "HHHHRRRRGGGGH", "glub... glub...", "TSSSSSSSK"
};

/* ---------- Amigos: diálogos según el vínculo (0..5) ---------- */
static const char *const FRIEND_NAME[3] = { "MARCOS", "VALERIA", "SOFÍA" };
static const char *const FRIEND_TXT[3][6] = {
    { "¡Leo! Te ves fatal, ¿has dormido? Toma, para tus apuntes.",
      "Mi hermano dice que de noche se oyen cosas en el callejón.\nYo no iría... pero tú eres terco.",
      "Cuando algo me pesa, salgo a correr.\nNo arregla nada, pero ayuda a respirar.",
      "Si un día no quieres hablar, igual aquí estoy.\nSin preguntas.",
      "Me da igual tu promedio, Leo.\nA mí me importa que estés bien.",
      "Domingo otra vez. Lo que pase, no lo cargues solo.\n¿Trato?" },
    { "Leo, no te asustes: he visto sombras en el callejón.\nLes asusta la luz de una linterna.",
      "Las boletas reprobadas son frágiles.\nUn buen rayo de luz y se deshacen.",
      "Los ecos de voces familiares odian el ruido blanco.\nPrueba la Estática.",
      "Cuando la linterna se apaga, ellos pegan más fuerte.\nCuida las pilas.",
      "Creo que esas criaturas son tus propios miedos.\nY los miedos se hablan, no solo se pelean.",
      "Pase lo que pase el domingo, cuéntame cómo te fue.\nLo digo en serio." },
    { "Leo, qué bueno verte. Siéntate a estudiar conmigo:\nde dos se aprende mejor.",
      "Mi truco: resumir en voz alta.\nLa próxima vez que estudies te saldrá mejor.",
      "¿Tienes sueño? Esas ojeras no mienten.\nDescansa cuando puedas.",
      "A veces el bajo rendimiento es cansancio disfrazado.\nNo eres tonto, Leo.",
      "Si te cuesta, pide ayuda.\nPedir ayuda también cuenta como esforzarse.",
      "Estoy orgullosa de ti. En serio.\nPase lo que pase con las notas." }
};
static const char *const FRIEND_AGAIN[3] = {
    "Ya hablamos hoy, Leo. ¡Nos vemos mañana!",
    "Ya te dije lo que sé por hoy. Ten cuidado.",
    "Sigue con tus apuntes. ¡Tú puedes!"
};
static const char *const SHOP_TXT = "¡Bienvenido a la tienda!\nArmaduras raras, pilas y algo de comer.";

/* ---------- Tarjeta de cada día (monólogo matutino) y objetivo sugerido ---------- */
static const char *const DAY_CARD[N_DAYS] = {
    "Domingo por la mañana. La casa huele a café recalentado.\nHoy toca salir y conocer el barrio.",
    "Lunes. Anoche soñaste con un colchón húmedo.\nTodo parece normal... salvo el callejón.",
    "Martes. Las voces de anoche siguen en tu cabeza.\nEn el callejón hay criaturas nuevas.",
    "Miércoles. A mitad de semana el cansancio pesa el doble.\nLos monstruos huelen tu miedo.",
    "Jueves. Las sombras ya no se esconden.\nTu linterna se agota más rápido.",
    "Viernes. Todos esperan el fin de semana. Tú, no.\nAlgo grande se mueve al fondo de la oscuridad.",
    "Sábado. Mañana es domingo otra vez.\nEs tu última oportunidad de prepararte.",
    "Domingo. La cena fría te espera, en casa y en la oscuridad.\nSe acabó el tiempo de dudar."
};
static const char *const DAY_GOAL[N_DAYS] = {
    "Explora la ciudad y habla con tus amigos.",
    "Estudia, compra equipo y prueba el callejón.",
    "Aparecen los Ecos Paternos: lleva pilas.",
    "Haz tareas por la noche: mañana recibes un bono.",
    "Aparecen los Ecos Maternos: mejora tu armadura.",
    "Una Puerta Cerrada bloquea pasillos: usa recuerdos.",
    "Prepárate: mañana es el último domingo.",
    "Vence a La Cena Fría en la Zona Oscura y duerme."
};
static const char *const FAINT_TXT =
    "Te desmayas. Todo se vuelve blanco...\nDespiertas en tu habitación.";

/* ---------- Finales ---------- */
struct Ending { const char *title, *text; };
static const Ending ENDINGS[3] = {
    { "FINAL: UN DOMINGO DISTINTO",
      "Despiertas en tu cama. La lluvia por fin paró.\nEsta vez, antes de que empiece la cena, hablas.\n"
      "No lo arregla todo. Pero por primera vez\nno cargas solo con el peso.\nPedir ayuda también fue una forma de ganar." },
    { "FINAL: LA CENA TERMINA",
      "Venciste a la cena fría, pero la casa sigue en silencio.\nSabes que el peso no se va solo.\n"
      "Mañana intentarás hablar con alguien.\nMañana." },
    { "FINAL: OTRO DOMINGO IGUAL",
      "Domingo por la noche. La mesa, el silencio, el reproche.\nLa oscuridad sigue ahí, esperando.\n"
      "Tal vez la próxima semana alguien te escuche.\nTal vez tú mismo." }
};
static const char *const ENDING_NOTE = "Si algo de esto te suena familiar,\nhabla con alguien de confianza.";

/* ================= Cuadro de dialogo con maquina de escribir ================= */
class DialogBox {
public:
    const char *text; const char *who; int t;
    DialogBox() : text(""), who(""), t(0) {}
    void begin(const char *txt, const char *speaker) { text = txt; who = speaker; t = 0; }
    int len() const { return u8len(text); }
    bool complete() const { return t / 2 >= len(); }
    /* devuelve true cuando el jugador confirma con el texto completo */
    bool update(bool confirm) {
        t++;
        if (!confirm) return false;
        if (!complete()) { t = len() * 2 + 2; return false; }
        return true;
    }
    void draw() const {
        gfx_rect(10, 190, 460, 74, RGB5(6,6,12));
        gfx_frame(10, 190, 460, 74, RGB5(150,150,160));
        gfx_frame(12, 192, 456, 70, RGB5(50,50,60));
        if (who[0]) {
            gfx_rect(18, 182, u8len(who) * 8 + 12, 14, RGB5(6,6,12));
            gfx_frame(18, 182, u8len(who) * 8 + 12, 14, RGB5(150,150,160));
            gfx_text(24, 186, who, RGB5(200,60,60), 1);
        }
        gfx_text_n(22, 204, text, t / 2, RGB5(210,210,200), 1);
        if (complete() && ((t / 20) & 1)) gfx_text(448, 250, "X", RGB5(200,200,200), 1);
    }
};

/* ================= Comedor de los 2000 ================= */
static void light_cone(int cx, int y0, int y1, int w0, int w1, int amt) {
    for (int y = y0; y < y1; y++) {
        int w = w0 + (w1 - w0) * (y - y0) / (y1 - y0);
        for (int x = cx - w; x <= cx + w; x++) {
            if ((unsigned)x >= SCR_W || (unsigned)y >= SCR_H) continue;
            pix *p = &g_fb[y * BUF_W + x];
            int a = amt * (w - abs(x - cx) * 3 / 4) / (w ? w : 1); if (a < 0) a = 0;
            int r = (*p & 31) + a, g = ((*p >> 5) & 31) + a, b = ((*p >> 10) & 31) + a / 2;
            if (r > 31) r = 31;
            if (g > 31) g = 31;
            if (b > 31) b = 31;
            *p = (pix)(0x8000 | (b << 10) | (g << 5) | r);
        }
    }
}
static void figure(int x, int y, pix skin, pix hair, pix shirt, pix pants, int mouth, int arm, int seated) {
    int h = seated ? 0 : 40;
    gfx_rect(x - 6, y + 22, 30, seated ? 34 : 50, shirt);                    /* torso */
    if (!seated) { gfx_rect(x - 2, y + 72, 12, h, pants); gfx_rect(x + 12, y + 72, 12, h, pants); }
    gfx_rect(x - 12, y + 24, 7, 28, shirt);                                   /* brazo en reposo */
    gfx_rect(x + 24, y + (arm ? 0 : 26), 7, 26, shirt);                       /* brazo que gesticula */
    gfx_rect(x + 24, y + (arm ? -4 : 50), 7, 5, skin);
    gfx_rect(x + 5, y + 16, 8, 8, skin);                                      /* cuello */
    gfx_rect(x, y, 18, 20, skin);                                             /* cabeza */
    gfx_rect(x - 1, y - 3, 20, 7, hair);
    gfx_rect(x + 3, y + 8, 3, 3, BLACK); gfx_rect(x + 12, y + 8, 3, 3, BLACK);
    gfx_rect(x + 5, y + 14, 8, mouth ? 5 : 1, mouth ? RGB5(60,10,10) : RGB5(80,40,40));
}
static void draw_dining(int t, int speaker) {
    for (int y = 0; y < 190; y++) gfx_rect(0, y, SCR_W, 1, rgb(30 + y / 20, 36 + y / 18, 34 + y / 22));
    for (int x = 0; x < SCR_W; x += 12) gfx_rect(x, 0, 1, 190, RGB5(26,32,30));
    gfx_rect(0, 150, SCR_W, 40, RGB5(48,54,50));                              /* friso */
    for (int y = 190; y < SCR_H; y += 16)
        for (int x = 0; x < SCR_W; x += 16) gfx_rect(x, y, 16, 16, (((x + y) / 16) & 1) ? RGB5(52,52,46) : RGB5(42,42,38));
    /* ventana con lluvia */
    gfx_rect(436, 36, 36, 70, RGB5(70,62,56)); gfx_rect(440, 40, 28, 62, RGB5(12,18,30));
    for (int i = 0; i < 12; i++) { int y = (int)((hash2(i, 3) % 60 + t * 5) % 60); gfx_rect(440 + (int)(hash2(i, 4) % 28), 40 + y, 1, 4, RGB5(90,110,150)); }
    /* televisor CRT con estatica */
    gfx_rect(14, 132, 62, 52, RGB5(70,66,58)); gfx_rect(20, 138, 46, 38, RGB5(20,24,26));
    for (int i = 0; i < 70; i++) { int v = 20 + rndr(70); gfx_px(20 + rndr(46), 138 + rndr(38), rgb(v, v, v + 6)); }
    gfx_rect(8, 184, 74, 6, RGB5(60,44,30));
    /* reloj y foto familiar */
    gfx_disc(330, 56, 14, RGB5(150,146,130)); gfx_disc(330, 56, 11, RGB5(30,30,30));
    float a = t * 0.01f;
    gfx_rect(330 + (int)(sinf(a) * 8), 56 - (int)(cosf(a) * 8), 2, 2, RGB5(200,200,190));
    gfx_rect(330 + (int)(sinf(a * 12) * 6), 56 - (int)(cosf(a * 12) * 6), 2, 2, RGB5(200,180,150));
    gfx_rect(96, 52, 34, 26, RGB5(80,60,40)); gfx_rect(99, 55, 28, 20, RGB5(110,110,100));
    for (int i = 0; i < 3; i++) gfx_rect(103 + i * 8, 62, 4, 8, i == 2 ? RGB5(60,60,60) : RGB5(150,130,110));
    /* lampara */
    gfx_rect(239, 0, 2, 42, RGB5(20,20,20)); gfx_rect(212, 42, 56, 12, RGB5(46,40,32));
    /* personas */
    int shake = speaker ? ((t % 6 < 3) ? 1 : -1) : 0;
    figure(112, 86, RGB5(150,112,96), RGB5(60,40,30), RGB5(90,50,60), RGB5(30,30,40), speaker == 2, speaker == 2 ? 1 : 0, 1);
    /* mesa */
    gfx_rect(100, 150, 280, 8, RGB5(104,76,52)); gfx_rect(100, 158, 280, 26, RGB5(128,116,98)); gfx_rect(100, 183, 280, 2, RGB5(80,70,60));
    for (int x = 130; x < 360; x += 56) { gfx_ellipse(x, 148, 18, 4, RGB5(150,150,140)); gfx_ellipse(x, 147, 12, 2, RGB5(80,60,40)); }
    gfx_rect(150, 138, 5, 10, RGB5(110,120,140)); gfx_rect(302, 138, 5, 10, RGB5(110,120,140));
    figure(382 + shake, 60, RGB5(160,120,100), RGB5(90,90,90), RGB5(40,50,80), RGB5(30,30,36), speaker == 1, speaker == 1 ? ((t / 6) & 1) : 0, 0);
    /* Leo encogido, mirando la mesa */
    gfx_rect(228, 118, 36, 34, RGB5(60,70,110));
    gfx_rect(232, 96, 28, 24, RGB5(130,96,78)); gfx_rect(230, 92, 32, 14, RGB5(36,24,18)); gfx_rect(230, 100, 8, 16, RGB5(36,24,18));
    gfx_rect(220, 140, 52, 10, RGB5(60,70,110));
    light_cone(240, 54, 190, 14, 90, 9 + ((t / 7 % 29 == 0) ? -4 : 0));
    if (speaker) for (int i = 0; i < 3; i++)                                   /* ondas de grito apagado */
        gfx_text((speaker == 1 ? 340 : 160) + i * 7, 70 + i * 6 + ((t / 5) & 1), "!", RGB5(120,50,50), 1);
    /* vineta */
    for (int y = 0; y < SCR_H; y++) for (int x = 0; x < SCR_W; x++) {
        int dx = x - 240, dy = (y - 120) * 2, d2 = dx * dx + dy * dy;
        if (d2 > 60000) { int f = 32 - (d2 - 60000) / 4500; g_fb[y * BUF_W + x] = shade(g_fb[y * BUF_W + x], f < 6 ? 6 : f); }
    }
}

/* ================= Cinematica completa ================= */
class Cutscene {
public:
    int scene, line, t, trans;
    bool finished;
    DialogBox box;

    void start() { scene = 0; line = 0; t = 0; trans = 0; finished = false; begin_line(); }

    void begin_line() {
        const DLine &l = (scene == 0) ? L_DINING[line] : L_BEDROOM[line];
        box.begin(l.text, WHO_NAME[l.who]);
    }
    void update(const Input &in) {
        t++;
        if (in.pressed & IN_START) { finished = true; return; }
        if (trans > 0) {                                  /* cierre de puerta / cambio de escena */
            trans--;
            if (trans == 20) { scene++; line = 0; t = 0; if (scene < 2) begin_line(); }
            return;
        }
        if (scene < 2) {
            int n = (scene == 0) ? N_DINING : N_BEDROOM;
            if (box.update((in.pressed & IN_CROSS) != 0)) {
                line++;
                if (line >= n) trans = 40; else begin_line();
            }
        } else if (t >= 640) finished = true;
    }
    int speaker() const {
        if (scene != 0 || line >= N_DINING || trans > 0) return 0;
        int w = L_DINING[line].who;
        return (w == W_PAPA || w == W_MAMA) && !box.complete() ? w : 0;
    }
    void draw_sfx(int v, int sc) const {
        for (int i = 0; i < 3; i++) {
            int idx = hash2(t / 10, i) % 6, x = (int)(hash2(t / 10, i + 9) % 200) + 10, y = 50 + (int)(hash2(t / 10, i + 20) % 120);
            gfx_text(x + (t % 3) - 1, y + ((t / 2) % 3) - 1, SFX_TXT[idx], rgb(v, v / 5, v / 5), sc);
        }
    }
    void draw() const {
        if (scene == 0) {
            draw_dining(t, speaker());
            box.draw();
            if (t < 40) gfx_fade(t * 32 / 40);
        } else if (scene == 1) {
            int fl = lightning(t);
            draw_room(t, fl); draw_bed(0, t, t);
            gfx_add(fl);
            box.draw();
            if (t < 40) gfx_fade(t * 32 / 40);
        } else {
            int ph = t < 110 ? 0 : t < 230 ? 1 : t < 400 ? 2 : 3;
            int pt = ph == 1 ? t - 110 : ph == 2 ? t - 230 : ph == 3 ? t - 400 : t;
            if (t < 470) {
                int fl = lightning(t);
                draw_room(t, fl); draw_bed(ph, pt, t);
                if (ph == 3 && pt > 60) gfx_clear(BLACK);
                gfx_add(fl);
                if (t > 20 && t < 100) { gfx_rect(10, 214, 460, 40, RGB5(6,6,12)); gfx_text(22, 228, "(Las manchas del techo... ¿se mueven?)", RGB5(210,210,200), 1); }
                if (ph == 1) draw_sfx(80 + pt, 1);
                if (ph == 2) { draw_sfx(200, 2); if (pt > 40) gfx_text_sh(30, 232, "LEO: NO... ¡SUÉLTAME!", RGB5(220,220,210), 1); }
                if (ph == 3) draw_sfx(220, 3);
            } else if (t < 560) {                        /* la lluvia se vuelve estatica blanca */
                gfx_clear(BLACK);
                gfx_static(4000 + (t - 470) * 220, 40, 255);
                if ((t / 4) & 1) gfx_text_c(120, "SHHHHHHHHHHHHHH", RGB5(255,255,255), 3);
            } else {
                gfx_clear(BLACK);
                gfx_static(300 - (t - 560) * 3, 10, 60);
                gfx_text_c(120, "...caes...", RGB5(90,90,90), 2);
            }
            if (trans == 0 && t >= 600) gfx_fade(32 - (t - 600) * 32 / 40);
        }
        if (trans > 0) {                                  /* portazo: oscurece */
            int f = trans > 20 ? (40 - trans) * 32 / 20 : trans * 32 / 20;
            gfx_fade(32 - f);
            if (trans > 24 && trans < 36) gfx_text_c(120, "¡PUM!", RGB5(200,200,200), 3);
        }
    }
};

#endif
