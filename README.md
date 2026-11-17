# SUB-CAMA: SECTOR 0  (homebrew PSP / PPSSPP)

Juego de terror RPG en pixel art 8-bits, 480x272, render por software en 16 bits (5551), 60 FPS.

## Compilar (necesitas el toolchain pspdev)
Opcion A, Docker (la mas facil, sin instalar nada):
    docker run --rm -v "$PWD":/src -w /src pspdev/pspdev make

Opcion B, pspdev instalado (https://pspdev.github.io):
    make

Resultado: `EBOOT.PBP`. Para limpiar: `make clean`.

## Ejecutar
- PPSSPP: abre `EBOOT.PBP` directamente (File > Load).
- PSP real (CFW): copia la carpeta a `ms0:/PSP/GAME/SubCama/` con el EBOOT.PBP dentro.

## Controles
D-Pad / Analogico: mover | X: atacar / descansar en el resorte (hoguera) / avanzar texto
Cuadrado: estado del personaje | Circulo: cancelar | Start: pausa (en la intro: saltarla)

## Estructura
- main.c        estados (titulo, dificultad, intro, juego, dialogo, estado, descanso, pausa, muerte) y bucle
- platform.h    TODO lo especifico de PSP: video/VRAM doble buffer, controles, callback de salida, ruta de guardado
- render.h      framebuffer, primitivas, texto bitmap, sprites, luz/oscuridad, fade, destello
- game_state.h  datos, dificultades, laberinto procedural, BFS, guardado
- assets.h      sprites, textos, habitacion de Leo, cama devoradora
- combat.h      IA de enemigos, combate, Sangre de Monstruo, niveles, historia

## Notas
- Texto sin tildes ni enie (la fuente MSX incluida en libpspdebug es ASCII).
- Sonido: simulado con texto en pantalla (no hay audio real).
- Guardado: junto al EBOOT (ms0:/PSP/GAME/...) o en ms0:/PSP/SAVEDATA/SubCama.sav. Se guarda al descansar o desde Pausa.
