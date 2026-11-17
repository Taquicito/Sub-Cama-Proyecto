/* platform.h - Capa de plataforma: video, entrada, callbacks de salida.
 * Todo lo especifico de PSP vive aqui; el resto del juego es C portable. */
#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define IN_UP       0x001
#define IN_DOWN     0x002
#define IN_LEFT     0x004
#define IN_RIGHT    0x008
#define IN_CROSS    0x010
#define IN_CIRCLE   0x020
#define IN_SQUARE   0x040
#define IN_TRIANGLE 0x080
#define IN_START    0x100

typedef struct { unsigned held, pressed; int ax, ay; } Input;  /* ax, ay: -128..127 */

static char g_save_path[256] = "save.dat";

#ifdef __PSP__
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>

PSP_MODULE_INFO("SubCama", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);

#define FB_BYTES (512 * 272 * 2)

static int g_running = 1;
static int g_back = 1;
static unsigned g_prev = 0;

static int exit_cb(int a1, int a2, void *c) { (void)a1; (void)a2; (void)c; g_running = 0; return 0; }
static int cb_thread(SceSize args, void *argp) {
    (void)args; (void)argp;
    int id = sceKernelCreateCallback("Exit Callback", exit_cb, NULL);
    sceKernelRegisterExitCallback(id);
    sceKernelSleepThreadCB();
    return 0;
}

static void plat_init(int argc, char **argv) {
    int th = sceKernelCreateThread("cb_thread", cb_thread, 0x11, 0xFA0, 0, 0);
    if (th >= 0) sceKernelStartThread(th, 0, 0);

    sceDisplaySetMode(0, 480, 272);
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    memset((void *)0x44000000, 0, FB_BYTES * 2);
    sceDisplaySetFrameBuf((void *)0x04000000, 512, PSP_DISPLAY_PIXEL_FORMAT_5551, PSP_DISPLAY_SETBUF_NEXTFRAME);

    /* Ruta de guardado: junto al EBOOT si esta en memory stick; si no, SAVEDATA */
    int ok = 0;
    if (argc > 0 && argv && argv[0] && (strncmp(argv[0], "ms0:", 4) == 0 || strncmp(argv[0], "ef0:", 4) == 0)) {
        strncpy(g_save_path, argv[0], 200); g_save_path[200] = 0;
        char *s = strrchr(g_save_path, '/');
        if (s) { s[1] = 0; strcat(g_save_path, "save.dat"); ok = 1; }
    }
    if (!ok) {
        sceIoMkdir("ms0:/PSP/SAVEDATA", 0777);
        strcpy(g_save_path, "ms0:/PSP/SAVEDATA/SubCama.sav");
    }
}

static void plat_poll(Input *in) {
    SceCtrlData pad;
    sceCtrlReadBufferPositive(&pad, 1);
    unsigned b = 0;
    if (pad.Buttons & PSP_CTRL_UP) b |= IN_UP;
    if (pad.Buttons & PSP_CTRL_DOWN) b |= IN_DOWN;
    if (pad.Buttons & PSP_CTRL_LEFT) b |= IN_LEFT;
    if (pad.Buttons & PSP_CTRL_RIGHT) b |= IN_RIGHT;
    if (pad.Buttons & PSP_CTRL_CROSS) b |= IN_CROSS;
    if (pad.Buttons & PSP_CTRL_CIRCLE) b |= IN_CIRCLE;
    if (pad.Buttons & PSP_CTRL_SQUARE) b |= IN_SQUARE;
    if (pad.Buttons & PSP_CTRL_TRIANGLE) b |= IN_TRIANGLE;
    if (pad.Buttons & PSP_CTRL_START) b |= IN_START;
    int ax = (int)pad.Lx - 128, ay = (int)pad.Ly - 128;
    if (abs(ax) < 32) ax = 0;
    if (abs(ay) < 32) ay = 0;
    in->held = b; in->pressed = b & ~g_prev; g_prev = b; in->ax = ax; in->ay = ay;
}

/* Copia el cuadro a la VRAM (doble buffer), espera vblank = 60 FPS */
static void plat_present(const uint16_t *fb) {
    memcpy((void *)(0x44000000 + g_back * FB_BYTES), fb, FB_BYTES);
    sceDisplaySetFrameBuf((void *)(0x04000000 + g_back * FB_BYTES), 512,
                          PSP_DISPLAY_PIXEL_FORMAT_5551, PSP_DISPLAY_SETBUF_NEXTFRAME);
    sceDisplayWaitVblankStart();
    g_back ^= 1;
}
static int plat_running(void) { return g_running; }
static void plat_shutdown(void) { sceKernelExitGame(); }

#else
#include "platform_host.h"   /* solo para pruebas en PC */
#endif

#endif
