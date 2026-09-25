const canvas = document.getElementById("game");
const ctx = canvas.getContext("2d");
const hud = document.getElementById("hud");

const W = canvas.width;
const H = canvas.height;

const keyDown = new Set();
const mouse = { x: W / 2, y: H / 2, down: false };

const rand = (a, b) => a + Math.random() * (b - a);
const clamp = (v, a, b) => Math.max(a, Math.min(b, v));
const len = (x, y) => Math.hypot(x, y) || 1;

const state = {
  t: 0,
  wave: 1,
  waveTime: 15,
  score: 0,
  combo: 0,
  comboTimer: 0,
  paused: false,
  gameOver: false,
  player: { x: W / 2, y: H / 2, r: 12, hp: 5, vx: 0, vy: 0, dashCd: 0, shootCd: 0 },
  bullets: [],
  enemies: [],
  pickups: [],
};

function reset() {
  Object.assign(state, {
    t: 0,
    wave: 1,
    waveTime: 15,
    score: 0,
    combo: 0,
    comboTimer: 0,
    paused: false,
    gameOver: false,
    bullets: [],
    enemies: [],
    pickups: [],
  });
  Object.assign(state.player, { x: W / 2, y: H / 2, hp: 5, vx: 0, vy: 0, dashCd: 0, shootCd: 0 });
}

window.addEventListener("keydown", (e) => {
  keyDown.add(e.key.toLowerCase());
  if (e.key === " ") e.preventDefault();
  if (e.key.toLowerCase() === "p") state.paused = !state.paused;
  if ((e.key.toLowerCase() === "r") && state.gameOver) reset();
});
window.addEventListener("keyup", (e) => keyDown.delete(e.key.toLowerCase()));
canvas.addEventListener("mousemove", (e) => {
  const rect = canvas.getBoundingClientRect();
  mouse.x = ((e.clientX - rect.left) / rect.width) * W;
  mouse.y = ((e.clientY - rect.top) / rect.height) * H;
});
canvas.addEventListener("mousedown", () => (mouse.down = true));
window.addEventListener("mouseup", () => (mouse.down = false));

function spawnEnemy() {
  const edge = (Math.random() * 4) | 0;
  let x = 0, y = 0;
  if (edge === 0) { x = rand(0, W); y = -20; }
  if (edge === 1) { x = W + 20; y = rand(0, H); }
  if (edge === 2) { x = rand(0, W); y = H + 20; }
  if (edge === 3) { x = -20; y = rand(0, H); }

  const r = Math.random();
  let type = "red", hp = 1, speed = 80 + state.wave * 6, size = 14, points = 10;
  if (r < 0.18) { type = "cyan"; speed = 130 + state.wave * 8; size = 10; points = 15; }
  if (r > 0.88) { type = "yellow"; hp = 3; speed = 55 + state.wave * 4; size = 20; points = 30; }
  state.enemies.push({ x, y, hp, speed, size, type, points });
}

let spawnTimer = 0;
function update(dt) {
  if (state.paused || state.gameOver) return;

  state.t += dt;
  state.waveTime -= dt;
  if (state.waveTime <= 0) {
    state.wave += 1;
    state.waveTime = Math.max(7, 15 - state.wave * 0.35);
  }

  const p = state.player;
  p.dashCd = Math.max(0, p.dashCd - dt);
  p.shootCd = Math.max(0, p.shootCd - dt);
  state.comboTimer = Math.max(0, state.comboTimer - dt);
  if (state.comboTimer === 0) state.combo = 0;

  let mx = 0, my = 0;
  if (keyDown.has("w") || keyDown.has("arrowup")) my -= 1;
  if (keyDown.has("s") || keyDown.has("arrowdown")) my += 1;
  if (keyDown.has("a") || keyDown.has("arrowleft")) mx -= 1;
  if (keyDown.has("d") || keyDown.has("arrowright")) mx += 1;
  const m = len(mx, my);
  const speed = 265;
  p.vx = (mx / m) * speed;
  p.vy = (my / m) * speed;

  if (keyDown.has(" ") && p.dashCd <= 0 && (mx || my)) {
    p.x += (mx / m) * 130;
    p.y += (my / m) * 130;
    p.dashCd = 1.0;
  }

  p.x = clamp(p.x + p.vx * dt, p.r, W - p.r);
  p.y = clamp(p.y + p.vy * dt, p.r, H - p.r);

  if (mouse.down && p.shootCd <= 0) {
    const dx = mouse.x - p.x;
    const dy = mouse.y - p.y;
    const d = len(dx, dy);
    state.bullets.push({ x: p.x, y: p.y, vx: (dx / d) * 720, vy: (dy / d) * 720, ttl: 1.2 });
    p.shootCd = 0.105;
  }

  const maxEnemies = 8 + state.wave * 3;
  spawnTimer -= dt;
  if (state.enemies.length < maxEnemies && spawnTimer <= 0) {
    spawnEnemy();
    spawnTimer = Math.max(0.18, 0.82 - state.wave * 0.035);
  }

  for (const b of state.bullets) {
    b.x += b.vx * dt;
    b.y += b.vy * dt;
    b.ttl -= dt;
  }
  state.bullets = state.bullets.filter((b) => b.ttl > 0 && b.x > -10 && b.y > -10 && b.x < W + 10 && b.y < H + 10);

  for (const e of state.enemies) {
    const dx = p.x - e.x;
    const dy = p.y - e.y;
    const d = len(dx, dy);
    e.x += (dx / d) * e.speed * dt;
    e.y += (dy / d) * e.speed * dt;
  }

  for (let bi = state.bullets.length - 1; bi >= 0; bi--) {
    const b = state.bullets[bi];
    let hit = false;
    for (let ei = state.enemies.length - 1; ei >= 0; ei--) {
      const e = state.enemies[ei];
      if (Math.hypot(b.x - e.x, b.y - e.y) <= e.size + 3) {
        hit = true;
        e.hp -= 1;
        if (e.hp <= 0) {
          state.enemies.splice(ei, 1);
          state.combo += 1;
          state.comboTimer = 2.5;
          state.score += e.points * Math.max(1, Math.min(6, state.combo));
          if (Math.random() < 0.12) {
            state.pickups.push({ x: e.x, y: e.y, ttl: 6, kind: Math.random() < 0.5 ? "heal" : "rapid" });
          }
        }
        break;
      }
    }
    if (hit) state.bullets.splice(bi, 1);
  }

  for (let ei = state.enemies.length - 1; ei >= 0; ei--) {
    const e = state.enemies[ei];
    if (Math.hypot(p.x - e.x, p.y - e.y) <= p.r + e.size) {
      state.enemies.splice(ei, 1);
      p.hp -= 1;
      if (p.hp <= 0) {
        state.gameOver = true;
        break;
      }
    }
  }

  for (let i = state.pickups.length - 1; i >= 0; i--) {
    const pk = state.pickups[i];
    pk.ttl -= dt;
    if (pk.ttl <= 0) {
      state.pickups.splice(i, 1);
      continue;
    }
    if (Math.hypot(p.x - pk.x, p.y - pk.y) <= p.r + 8) {
      if (pk.kind === "heal") p.hp = Math.min(5, p.hp + 1);
      else p.shootCd = 0;
      state.pickups.splice(i, 1);
    }
  }
}

function draw() {
  ctx.fillStyle = "#131318";
  ctx.fillRect(0, 0, W, H);

  for (const b of state.bullets) {
    ctx.fillStyle = "#f5f5f7";
    ctx.beginPath();
    ctx.arc(b.x, b.y, 3, 0, Math.PI * 2);
    ctx.fill();
  }

  for (const e of state.enemies) {
    ctx.fillStyle = e.type === "red" ? "#ff4d4d" : e.type === "cyan" ? "#34f0ff" : "#ffd447";
    ctx.beginPath();
    ctx.arc(e.x, e.y, e.size, 0, Math.PI * 2);
    ctx.fill();
  }

  for (const pk of state.pickups) {
    ctx.fillStyle = pk.kind === "heal" ? "#ff5c6b" : "#43e8ff";
    ctx.fillRect(pk.x - 6, pk.y - 6, 12, 12);
  }

  const p = state.player;
  ctx.fillStyle = "#ffffff";
  ctx.beginPath();
  ctx.arc(p.x, p.y, p.r, 0, Math.PI * 2);
  ctx.fill();

  ctx.strokeStyle = "#ffffff";
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.moveTo(p.x, p.y);
  ctx.lineTo(mouse.x, mouse.y);
  ctx.stroke();

  ctx.fillStyle = "#c9cad1";
  ctx.font = "22px sans-serif";
  ctx.fillText(`HP ${p.hp}   SCORE ${state.score}   WAVE ${state.wave}   COMBO x${Math.max(1, state.combo)}`, 24, 36);

  if (state.paused && !state.gameOver) {
    ctx.fillStyle = "rgba(0,0,0,0.45)";
    ctx.fillRect(0, 0, W, H);
    ctx.fillStyle = "#fff";
    ctx.font = "44px sans-serif";
    ctx.fillText("PAUSED", W / 2 - 90, H / 2);
  }

  if (state.gameOver) {
    ctx.fillStyle = "rgba(0,0,0,0.6)";
    ctx.fillRect(0, 0, W, H);
    ctx.fillStyle = "#ff5b64";
    ctx.font = "52px sans-serif";
    ctx.fillText("YOU FUCKED UP", W / 2 - 220, H / 2 - 16);
    ctx.fillStyle = "#fff";
    ctx.font = "30px sans-serif";
    ctx.fillText("Press R to restart", W / 2 - 120, H / 2 + 32);
  }

  hud.textContent = `Tip: keep moving. Dash cooldown ${state.player.dashCd.toFixed(2)}s.`;
}

let last = performance.now();
function loop(now) {
  const dt = Math.min(0.033, (now - last) / 1000);
  last = now;
  update(dt);
  draw();
  requestAnimationFrame(loop);
}
requestAnimationFrame(loop);
