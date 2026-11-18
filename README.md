# SUB-CAMA: SECTOR 0 — Una semana (Domingo a Domingo)

RPG de progresión estructurada para PSP / PPSSPP. C++ sin STL, 480x272, 16 bits, 60 FPS.

## Compilar
    docker run --rm -v "$PWD":/src -w /src pspdev/pspdev make
(o `make` con pspdev, o automático con GitHub Actions: `.github/workflows/build.yml`). Resultado: `EBOOT.PBP`.

## Controles
D-Pad / Analógico: mover y elegir · X: confirmar / interactuar · Cuadrado: menú del jugador
Círculo: cancelar (en combate: huir) · Start: pausa · L/R o ← →: cambiar de pestaña

## Cómo está estructurado el tiempo (day_system.h)
- 8 jornadas (`day` 0..7: Domingo → Domingo) × 3 franjas (`slot` 0 Mañana, 1 Tarde, 2 Noche).
- Cada día empieza en la cama (X para levantarte). Estudiar, hacer tareas y entrar a la Zona Oscura consumen 1 franja.
- Con `slot == 3` el día se agotó: se vuelve a casa y se duerme. Dormir → `DaySystem::end_day()`:
  avanza el día, paga el bono de tareas, restaura PV/SP/linterna y muestra la tarjeta del día.
- La dificultad progresiva sale solo de `(day)`: monstruos nuevos (`EnemyDef.unlock_day`), grupos más grandes
  (`max_group`), más vida/daño (`enemy_scale`), la linterna se gasta más rápido (`battery_drain`).
- Domingo inicial, al anochecer (o al dormir): cinemática familiar → caes a la Zona Oscura.
  Domingo final: jefe "La Cena Fría"; al dormir, final según jefe y vínculos.
- Sin permadeath ni mapas aleatorios: todas las zonas son fijas. Si pierdes un combate te desmayas,
  pierdes un % de Notas según la dificultad y despiertas al día siguiente.

## Guardado (save_system.h)
- 3 ranuras: `slot1.sav`, `slot2.sav`, `slot3.sav` (junto al EBOOT, o en `ms0:/PSP/SAVEDATA/`).
- `SaveData` es una "foto" plana de toda la partida (jugador, calendario, amigos, zona + posición, monstruos derrotados).
- `pack()` guarda, `unpack()` restaura. Para añadir datos: nuevo campo en `SaveData` + `SAVE_VERSION++` + pack/unpack.
- Menú → pestaña GUARDAR (cualquier momento fuera de combate) o Título → CARGAR PARTIDA.

## Módulos
main.cpp (estados) · day_system.h · save_system.h · world.h (zonas y texturas) · battle_system.h
narrative_engine.h · lighting.h · player_stats.h · assets.h · render.h (fuente propia con tildes) · platform.h
