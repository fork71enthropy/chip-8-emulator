# Emulateur CHIP-8 en C

Ce projet propose un emulateur CHIP-8 ecrit en C, structure de maniere modulaire et avec des garde-fous de securite (validation d'entrees, controles de bornes memoire, gestion explicite des erreurs runtime).

## Objectifs

- Fournir un coeur d'emulation CHIP-8 clair et maintenable.
- Eviter les comportements indefinis avec des verifications systematiques.
- Offrir une base propre pour ajouter plus tard un frontend graphique (SDL2, OpenGL, ncurses, etc.).

## Arborescence

- `include/chip8.h` : API publique du coeur emulateur.
- `src/chip8.c` : implementation CPU, memoire, opcodes, timers, clavier, rendu logique.
- `src/main.c` : runner CLI (chargement ROM, boucle d'execution, rendu ASCII optionnel).
- `Makefile` : compilation avec options strictes.

## Build

Prerequis:

- Un compilateur C11 (`cc`, `gcc` ou `clang`).
- `make`.

Compilation:

```bash
make
```

Nettoyage:

```bash
make clean
```

## Execution

```bash
./chip8 <rom_path> [--cycles-per-frame N] [--max-cycles N] [--ascii]
```

Exemple:

```bash
./chip8 roms/PONG.ch8 --cycles-per-frame 10 --max-cycles 200000 --ascii
```

Options:

- `--cycles-per-frame N` : nombre de cycles CPU entre deux ticks de timers (par defaut `10`).
- `--max-cycles N` : nombre maximal de cycles executes avant arret (par defaut `1000000`).
- `--ascii` : affiche le framebuffer 64x32 dans le terminal quand une instruction de dessin modifie l'ecran.

## Bonnes pratiques de securite appliquees

- Validation stricte des arguments CLI (`strtoul/strtoull` + verification d'overflow).
- Validation de la taille de ROM avant chargement en memoire.
- Verification des bornes memoire avant chaque acces sensible (PC, I, sprite fetch, FX55/FX65, FX33).
- Protection pile d'appels CHIP-8:
	- detection `stack overflow` sur `2NNN`
	- detection `stack underflow` sur `00EE`
- Gestion explicite des erreurs via `chip8_status_t` et messages comprehensibles.
- Compilation avec flags stricts:
	- `-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`

## Etat fonctionnel

Le coeur implemente le jeu principal d'instructions CHIP-8 classique:

- Controle de flux: `00EE`, `1NNN`, `2NNN`, `BNNN`
- Tests et sauts conditionnels: `3XKK`, `4XKK`, `5XY0`, `9XY0`, `EX9E`, `EXA1`
- Registres et ALU: `6XKK`, `7XKK`, `8XY0..8XYE`, `CXKK`
- Index/memoire: `ANNN`, `FX1E`, `FX29`, `FX33`, `FX55`, `FX65`
- Timers et clavier: `FX07`, `FX0A`, `FX15`, `FX18`
- Affichage: `00E0`, `DXYN`

Notes de compatibilite:

- Les opcodes de shift `8XY6` et `8XYE` utilisent la variante moderne (operation sur `Vx`).
- Le runner fourni est volontairement simple (pas de gestion clavier temps reel avancee).

## Ameliorations recommandees

- Ajouter un frontend SDL2 (video + clavier + timing 60 Hz reel).
- Ajouter une suite de tests ROM de validation (IBM logo, tests opcode).
- Ajouter un mode debug pas-a-pas (dump registres/opcode/PC).
