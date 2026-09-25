# SHITWAVE 💀

A tiny, stupidly playable top-down 2D survival shooter written in **C++17**.

You are a little white murder-dot in an increasingly rude rectangle. Enemies keep coming. You shoot them with the mouse, dodge with WASD, dash with Space, and try not to become floor paste.

**Primary platform: Linux desktop (X11)**

The game intentionally has **no external runtime assets, no game engine, no asset pack, no network connection, and no giant dependency pile**. The entire game is rendered from basic X11 primitives, so the executable can stay tiny and the project is easy to understand for a C++ learner.

> The game is called SHITWAVE because apparently naming things professionally was too much work.

---

## 1. Gameplay

### Controls

| Input | What it does |
|---|---|
| `W A S D` / Arrow keys | Move |
| Left mouse button | Shoot toward cursor |
| `Space` | Dash in your movement direction |
| `P` | Pause / unpause |
| `R` | Restart after death |
| `Esc` | Quit |

### Rules

- Survive as long as possible.
- Kill enemies for score.
- Every few seconds the wave advances.
- Later waves spawn faster enemies and occasional beefy bastards.
- A small chance of killing an enemy drops a pickup.
- Red pickup: heal 1 HP.
- Cyan pickup: reset the next-shot delay.
- Consecutive kills create a combo multiplier.
- Touching an enemy costs 1 HP.
- At 0 HP: **you fucked up.**

### Enemy types

- **Red chaser:** normal enemy.
- **Cyan speeder:** small and annoyingly fast.
- **Yellow brute:** bigger, slower, takes several hits.

---

## 2. Why Linux X11?

This was deliberately chosen as the primary target because it keeps the project brutally simple:

- C++17 only.
- X11 is already available on most traditional Linux desktop installations.
- No texture pipeline.
- No audio middleware.
- No game engine.
- No web server.
- No JavaScript build step.

That means the project is useful as a **small C++ game-programming exercise**, not just as a folder full of engine boilerplate.

### Important limitation

This is **X11-first**, not Wayland-native. On Linux systems running an XWayland compatibility layer, it will generally run there too. If you want a native Wayland build later, porting the rendering/input layer to SDL2, SDL3, GLFW, or native Wayland is the clean next step.

---

## 3. Prerequisites

### Ubuntu / Debian / Linux Mint

```bash
sudo apt update
sudo apt install build-essential libx11-dev cmake
```

### Arch Linux

```bash
sudo pacman -S base-devel libx11 cmake
```

### Fedora

```bash
sudo dnf install gcc-c++ libX11-devel cmake
```

You need:

- a C++17 compiler (GCC or Clang)
- X11 development headers/libraries
- `make` **or** CMake

---

## 4. Build with Make

From the project directory:

```bash
make
```

Run it:

```bash
./shitwave
```

Clean the executable:

```bash
make clean
```

Or build + launch:

```bash
make run
```

---

## 5. Build with CMake

Recommended for IDEs, packaging, and CI.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Run:

```bash
./build/shitwave
```

Install system-wide:

```bash
sudo cmake --install build
```

The executable installs under the normal CMake `bin` destination.

---

## 6. Project structure

```text
shitwave/
├── source/
│   └── main.cpp          # the native X11 game
├── web/                  # playable static browser build
├── assets/               # reserved for future art/audio
├── CMakeLists.txt        # CMake build
├── Makefile              # dead-simple build
├── .github/workflows/    # CI, Pages deploy, release automation
├── LICENSE               # MIT
├── README.md             # this file
└── .gitignore
```

Yes, `main.cpp` is intentionally huge for a project this size. For a learning project, having the complete gameplay loop in one file makes it easier to read and fuck with. For a bigger game, split this into renderer/input/game/entities/audio modules.

---

## 7. Code overview

The game loop does roughly this:

```text
poll X11 input
      ↓
calculate delta time
      ↓
update player
      ↓
spawn / update enemies
      ↓
move bullets
      ↓
resolve bullet collisions
      ↓
resolve player collisions
      ↓
update particles + pickups
      ↓
render everything
      ↓
repeat
```

The important structs are:

- `Vec2` — tiny 2D vector helper.
- `Bullet` — projectile position, velocity, lifetime.
- `Enemy` — position, HP, type, size, score value.
- `Particle` — lightweight hit/death visual effect.
- `Pickup` — temporary power-up.
- `Game` — gameplay state and update logic.
- `Palette` — the deliberately limited visual palette.

Rendering uses X11 calls such as `XFillRectangle`, `XFillArc`, `XDrawLine`, and `XDrawString`.

---

## 8. Tuning the game

The easiest place to experiment is `src/main.cpp`.

### Player speed

Search for:

```cpp
double speed=265;
```

### Fire rate

Search for:

```cpp
shootCd=0.105;
```

Lower number = faster shooting.

### Dash cooldown

Search for:

```cpp
dashCd=1.0;
```

### Wave timing

The first wave starts with roughly 15 seconds:

```cpp
waveTime=15;
```

### Enemy spawn pressure

Search for:

```cpp
int maxEnemies=8+wave*3;
```

and:

```cpp
spawnCd=std::max(0.18,0.82-wave*0.035);
```

Change those and you can turn the game from a chill little pew-pew into complete bullshit.

---

## 9. Adding actual art

The current game intentionally uses primitives, but there is already an `assets/` directory.

For a bigger version, I would recommend one of these routes:

### Option A — stay tiny

Keep X11 and add a very small custom BMP/PPM loader. This keeps dependencies low, but you will have to write more rendering code yourself.

### Option B — move to SDL3

This is the route I would take for a proper cross-platform 2D version. SDL gives you input, windows, controllers, audio, textures, and multiple platform backends without turning the project into an engine monster.

### Option C — Raylib

Great for quick game prototyping. Very beginner friendly, but it becomes another dependency that must be packaged for every target.

---

## 10. Where should this be hosted?

### Source code

**GitHub** is the correct home for this project.

Suggested repo:

```text
https://github.com/<your-user>/shitwave
```

Use GitHub Issues for bugs and GitHub Discussions if you want people to throw feature ideas at the wall.

### Releases

Use **GitHub Releases** for downloadable builds and publish the static browser build with **GitHub Pages**.

A sensible release layout:

```text
SHITWAVE-v1.0.0-linux-x86_64.tar.gz
SHITWAVE-v1.0.0-web.tar.gz
```

### Static playable website

The `web/` directory is a no-build static game build. It is deployed directly to GitHub Pages by `.github/workflows/pages.yml`.

### Itch.io

For actual players, I would put the downloadable game on **itch.io** as well. GitHub is excellent for source/releases; itch.io is better as the front door for people who just want to play.

A clean setup is:

```text
GitHub       → source + issues + releases
itch.io      → player-facing game page + downloads
```

---

## 11. Git setup

```bash
git init
git add .
git commit -m "initial shitwave release"
git branch -M main
git remote add origin git@github.com:<your-user>/shitwave.git
git push -u origin main
```

Or with GitHub CLI:

```bash
gh repo create shitwave --public --source=. --remote=origin --push
```

---

## 12. Release workflow

For a local release:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Then package both native + static web builds:

```bash
mkdir -p dist/SHITWAVE-v1.0.0-linux-x86_64
cp build/shitwave dist/SHITWAVE-v1.0.0-linux-x86_64/
tar -C dist -czf dist/SHITWAVE-v1.0.0-linux-x86_64.tar.gz SHITWAVE-v1.0.0-linux-x86_64
cp -r web dist/SHITWAVE-v1.0.0-web
tar -C dist -czf dist/SHITWAVE-v1.0.0-web.tar.gz SHITWAVE-v1.0.0-web
```

Create a GitHub release with:

```bash
gh release create v1.0.0 \
  dist/SHITWAVE-v1.0.0-linux-x86_64.tar.gz \
  dist/SHITWAVE-v1.0.0-web.tar.gz \
  --title "SHITWAVE v1.0.0" \
  --notes "First public release. Try not to die like an idiot."
```

Or use the automated release workflow by pushing a version tag:

```bash
git tag v1.0.0
git push origin v1.0.0
```

That triggers `.github/workflows/release.yml`, which builds and publishes both release assets.

---

## 13. CI / automated builds

GitHub Actions is already configured:

1. `.github/workflows/build.yml` builds and smoke-tests the Linux binary on pushes/PRs.
2. `.github/workflows/pages.yml` deploys `web/` to GitHub Pages from `main`.
3. `.github/workflows/release.yml` creates tagged GitHub releases with Linux + web archives.

For Windows/macOS, I recommend doing the SDL3 port first rather than trying to force X11 concepts onto those platforms.

---

## 14. Distribution notes

The binary still needs the target system's X11 runtime libraries. This is **not** a statically bundled universal binary.

For a more polished Linux release later, package it as one of:

- `.deb`
- AppImage
- Flatpak

### AppImage

This is probably the nicest future single-file Linux download because players can download one file and run it.

### Flatpak

Good if you want a clean sandboxed desktop app. It also makes future asset/audio dependencies easier to keep under control.

---

## 15. Suggested v2 roadmap

Keep the first version small. Then add things in roughly this order:

1. Proper title screen.
2. High-score persistence.
3. Sound effects.
4. Screen shake.
5. More enemy patterns.
6. Weapon pickups.
7. Boss waves.
8. Controller support.
9. Native Wayland / SDL3 port.
10. WebAssembly build.
11. Steam / itch.io distribution.

Do **not** immediately turn this into a 900 MB game with a 40 GB dependency folder. The charm is that it is tiny.

---

## 16. Troubleshooting

### `X11/Xlib.h: No such file or directory`

Install the X11 development package.

Ubuntu/Debian/Mint:

```bash
sudo apt install libx11-dev
```

### `cannot open display`

You launched the game from an environment without an available X display, such as a headless SSH session or some containers.

Run it from your graphical Linux desktop.

### The window opens but feels tiny

The game intentionally uses a fixed 1280×720 coordinate system. Most modern desktop environments will scale the window normally. A future SDL3 port can support proper resize-aware rendering and DPI scaling.

### Input gets weird after alt-tabbing

The game clears held input when it loses focus. Click the window again and continue.

### The game is brutally difficult

Congratulations, the game is working. Edit `speed`, `spawnCd`, `maxEnemies`, enemy speed, or player HP in `src/main.cpp`.

---

## 17. License

MIT. Do whatever you want with it. Keep the license notice, don't pretend you invented the code, and have fun.

---

## 18. One-line summary for the repo

> A tiny C++17 top-down survival shooter where your mouse is a gun and the universe is personally offended by your existence.
