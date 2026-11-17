/* narrative_engine.h - Cinematica familiar (3 escenas), cuadros de dialogo,
 * monologos de Leo en los Puntos de Reflexion y textos del entorno. */
#ifndef NARRATIVE_ENGINE_H
#define NARRATIVE_ENGINE_H

#include "render.h"
#include "platform.h"
#include "assets.h"

enum { W_NARR, W_PAPA, W_MAMA, W_LEO };
struct DLine { int who; const char *text; };
static const char *const WHO_NAME[4] = { "", "PAPA", "MAMA", "LEO" };

/* Escena 1: comedor, 2000s */
static const DLine L_DINING[] = {
    { W_NARR, "Domingo, 8:40 PM. La cena se enfria.\nNadie ha tocado nada." },
    { W_PAPA, "Otra boleta con rojo. Otra mas.\nDime, Leo, que haces todo el dia?" },
    { W_MAMA, "Ni siquiera nos miras cuando te hablamos..." },
    { W_PAPA, "A tu edad yo ya trabajaba.\nAsi no tienes futuro. Entiendes?" },
    { W_MAMA, "Te la pasas encerrado. No sales.\nYa ni siquiera tienes amigos." },
    { W_NARR, "Sus voces suenan lejos,\ncomo si hablaran bajo el agua." },
    { W_PAPA, "CONTESTAME CUANDO TE HABLO!" },
    { W_LEO,  "...Estoy cansado." },
    { W_MAMA, "Cansado de que? Si no haces nada!" },
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
    "Caes. La lluvia se vuelve estatica blanca\ny luego, silencio.\n"
    "SUB-CAMA, SECTOR 0. Aqui las voces de casa\nte esperan en la oscuridad.";

/* Monologos al subir de nivel: cada victoria revela la verdad */
struct ReflectLine { int lv; const char *text; };
static const ReflectLine REFLECT[] = {
    {  2, "Un monstruo cae y oyes tu propia voz: 'ya lo sabias'.\nCada uno es una nota que no quisiste leer." },
    {  3, "La boleta se deshace entre tus dedos.\nEra tuya. Siempre fue tuya." },
    {  4, "Aqui las voces de casa no gritan: susurran.\nEs peor. Suenan como si tuvieran razon." },
    {  6, "Subes de nivel y nada mejora.\nSolo te haces mas capaz de cargar el peso." },
    {  8, "Hay una puerta al fondo. No se abre hacia afuera.\nSe abre hacia mas adentro." },
    { 10, "Recuerdas cuando alguien te pregunto como estabas.\nDijiste 'bien'. Nadie insistio." },
    { 13, "Ya no recuerdas el borde de tu cama.\nEsto es tu cabeza, y no tiene paredes." },
    { 16, "No hay salida, Leo. Solo hay pasillos.\nY los monstruos son lo unico que aun te habla." }
};
#define N_REFLECT (int)(sizeof REFLECT / sizeof REFLECT[0])

static const char *const ENV_TXT[8] = {
    "Un papel arrugado: 'MATEMATICAS 4.0'.\nAlguien lo tacho con rabia.",
    "Una foto de familia. Tu cara esta rasgada.",
    "Escrito mil veces en la pared:\n'PODRIAS ESFORZARTE MAS'.",
    "Una silla vacia junto a un plato frio.\nNadie te llamo a cenar.",
    "Un telefono descolgado. Alguien dijo\n'luego te llamo' hace anos.",
    "Un diario sin paginas.\nSolo queda una palabra: cansado.",
    "Una puerta pintada en la pared.\nNo tiene manija.",
    "Voces tras el colchon: 'esta bien...\nsolo es una etapa. Ya se te pasara.'"
};
static const char *const DEPTH_TXT[4] = {
    "La puerta se abre. Del otro lado esta el mismo pasillo.\nSolo que mas hondo.",
    "Otra puerta. Otra caida.\nEl colchon se ensancha para recibirte.",
    "Reconoces las manchas del techo. Son de tu cuarto.\nPero ahora estas por dentro.",
    "No hay afuera, Leo.\nSolo hay mas adentro."
};
static const char *const DEATH_TXT[3] = {
    "Te hundes un poco mas...", "El colchon te devuelve. No te deja ir.", "Despertar no es una opcion."
};
static const char *const SFX_TXT[6] = {
    "KRRRRRRK-KRRRK", "*crujido humedo*", "GRRRMMMMMMM", "HHHHRRRRGGGGH", "glub... glub...", "TSSSSSSSK"
};

/* ================= Cuadro de dialogo con maquina de escribir ================= */
class DialogBox {
public:
    const char *text; const char *who; int t;
    DialogBox() : text(""), who(""), t(0) {}
    void begin(const char *txt, const char *speaker) { text = txt; who = speaker; t = 0; }
    int len() const { return (int)strlen(text); }
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
            gfx_rect(18, 182, (int)strlen(who) * 8 + 12, 14, RGB5(6,6,12));
            gfx_frame(18, 182, (int)strlen(who) * 8 + 12, 14, RGB5(150,150,160));
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
                if (t > 20 && t < 100) { gfx_rect(10, 214, 460, 40, RGB5(6,6,12)); gfx_text(22, 228, "(Las manchas del techo... se mueven?)", RGB5(210,210,200), 1); }
                if (ph == 1) draw_sfx(80 + pt, 1);
                if (ph == 2) { draw_sfx(200, 2); if (pt > 40) gfx_text_sh(30, 232, "LEO: NO... SUELTAME--", RGB5(220,220,210), 1); }
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
            if (trans > 24 && trans < 36) gfx_text_c(120, "*PUM*", RGB5(200,200,200), 3);
        }
    }
};

#endif
