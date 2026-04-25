'use strict';

const canvas = document.getElementById('zahlCanvas');
const ctx = canvas.getContext('2d');
ctx.imageSmoothingEnabled = false;

// state
let frame = 0;
let lastTick = 0;
const TICK_MS = 125; // 8 fps

let currentLevel = 1;
let corrosionPct = 0;
let hungerPct = 0;
let feralMode = false;
let petState = 'idle'; // idle | fed | petted | levelup
let stateTimer = 0;
let eventSource = null;

// draw helpers
const cls = () => { ctx.fillStyle = '#000811'; ctx.fillRect(0, 0, 64, 64); };
const r = (x, y, w, h, c) => { ctx.fillStyle = c; ctx.fillRect(x, y, w, h); };
const p = (x, y, c) => { ctx.fillStyle = c; ctx.fillRect(x, y, 1, 1); };

// animation helpers
const getBob   = () => Math.round(Math.sin(frame * 0.45) * 1);
const getBlink = () => (frame % 90) < 4;
const getFlapUp = () => (frame % 6) < 3;

// --- particles ---

function drawSparkles(cx, top) {
    const offsets = [[-5,-2],[3,-1],[-2,-4],[5,-3],[0,-5],[7,-2],[-4,-1],[2,-5]];
    const sp = offsets[frame % offsets.length];
    const x = cx + sp[0], y = top + sp[1];
    if (x >= 0 && x < 64 && y >= 0 && y < 64)
        p(x, y, frame % 2 === 0 ? '#44ff88' : '#88ffcc');
    const sp2 = offsets[(frame + 4) % offsets.length];
    const x2 = cx + sp2[0], y2 = top + sp2[1];
    if (x2 >= 0 && x2 < 64 && y2 >= 0 && y2 < 64)
        p(x2, y2, '#aaffdd');
}

function drawHearts(cx, top) {
    const t = frame % 24;
    const rise = -Math.floor(t / 3);
    if (t < 20) {
        const hx = cx + (frame % 8 < 4 ? -5 : 4);
        const hy = top + rise;
        if (hy > 0 && hy < 62 && hx > 1 && hx < 62) {
            r(hx,   hy,   2, 1, '#ff2255');
            r(hx-1, hy+1, 4, 1, '#ff2255');
            r(hx,   hy+2, 2, 1, '#ff2255');
            p(hx+1, hy+3,    '#ff2255');
        }
    }
}

// ============================================================
// STAGE 1-2 — WHELPLING
// tiny demonic slug-larva, green, red eyes
// ============================================================
function drawWhelpling(b, bl) {
    const Y = 38 + b;

    // shadow
    r(24, Y+13, 16, 1, '#001800');

    // body outline (1px darker border)
    r(24, Y+1,  16, 10, '#152b15');
    r(25, Y,    14, 12, '#152b15');
    r(27, Y-1,  10,  1, '#152b15');

    // body fill
    r(25, Y+1,  14, 10, '#2a5a2a');
    r(26, Y,    12, 12, '#2a5a2a');
    r(28, Y-1,   8,  1, '#2a5a2a');

    // belly
    r(27, Y+6,  10,  5, '#3d7a3d');

    // eye sockets (drawn on top of body)
    r(26, Y+1,   5,  4, '#152b15');
    r(35, Y+1,   5,  4, '#152b15');

    // eyes
    if (!bl) {
        r(27, Y+2,  3, 2, '#cc0000');
        r(36, Y+2,  3, 2, '#cc0000');
        p(27, Y+2,          '#ff5555');
        p(36, Y+2,          '#ff5555');
    } else {
        r(27, Y+3,  3, 1, '#152b15');
        r(36, Y+3,  3, 1, '#152b15');
    }

    // feelers
    p(29, Y-2, '#3d7a3d');
    p(34, Y-2, '#3d7a3d');
    p(29, Y-1, '#2a5a2a');
    p(34, Y-1, '#2a5a2a');

    // mouth
    r(29, Y+8, 6, 1, '#152b15');
    r(30, Y+9, 4, 1, '#152b15');

    // foot nubs
    r(26, Y+11, 3, 2, '#2a5a2a');
    r(35, Y+11, 3, 2, '#2a5a2a');

    // high-corrosion spots
    if (corrosionPct > 40) {
        p(30, Y+4, '#aa2200');
        p(36, Y+6, '#aa2200');
    }
}

// ============================================================
// STAGE 3-4 — CLANNFEAR PUP
// bony-headed bipedal beast, teal, amber eyes
// ============================================================
function drawClannfearPup(b, bl) {
    const Y = 34 + b;

    // shadow
    r(25, Y+27, 14, 1, '#000811');

    // tail
    r(41, Y+14, 5, 3, '#1a3a4a');
    r(45, Y+12, 3, 3, '#1a3a4a');
    r(47, Y+10, 2, 3, '#1a3a4a');

    // body outline
    r(26, Y+8,  12, 12, '#0d2233');
    r(25, Y+10, 14,  8, '#0d2233');

    // body fill
    r(27, Y+8,  10, 12, '#1a3a4a');
    r(26, Y+10, 12,  8, '#1a3a4a');

    // belly
    r(28, Y+12,  8,  6, '#22485a');

    // arms
    r(22, Y+10,  4,  6, '#0d2233');
    r(38, Y+10,  4,  6, '#0d2233');
    // claws
    r(21, Y+15,  3,  2, '#0d2233');
    r(39, Y+15,  3,  2, '#0d2233');
    p(21, Y+16,         '#4a8a9a');
    p(23, Y+16,         '#4a8a9a');
    p(39, Y+16,         '#4a8a9a');
    p(41, Y+16,         '#4a8a9a');

    // legs
    r(27, Y+20,  4,  6, '#0d2233');
    r(33, Y+20,  4,  6, '#0d2233');
    // feet
    r(25, Y+25,  6,  2, '#0d2233');
    r(33, Y+25,  6,  2, '#0d2233');

    // neck
    r(29, Y+4,   6,  5, '#0d2233');
    r(30, Y+3,   4,  5, '#1a3a4a');

    // head plate outline
    r(24, Y-1,  16,  6, '#0d2233');
    // head fill
    r(25, Y,    14,  7, '#1a3a4a');
    r(26, Y+1,  12,  5, '#223d50');

    // bony head crest
    r(25, Y-3,  14,  3, '#0d2233');
    r(27, Y-2,  10,  2, '#1a3a4a');

    // eye ridge
    r(26, Y+1,  12,  2, '#0d2233');

    // eyes
    if (!bl) {
        r(27, Y+2,  3, 2, '#ccaa00');
        r(34, Y+2,  3, 2, '#ccaa00');
        p(28, Y+2,         '#ffee44');
        p(35, Y+2,         '#ffee44');
    } else {
        r(27, Y+3,  3, 1, '#0d2233');
        r(34, Y+3,  3, 1, '#0d2233');
    }

    // teeth (snout edge)
    p(29, Y+6, '#ccbbaa');
    p(31, Y+6, '#ccbbaa');
    p(33, Y+6, '#ccbbaa');

    // bone texture hints on head
    r(26, Y, 2, 1, '#0d2233');
    r(36, Y, 2, 1, '#0d2233');
}

// ============================================================
// STAGE 5-6 — DAEDROTH WHELP
// stocky green croc-daemon, red slit eyes, tiny horns
// ============================================================
function drawDaedrothWhelp(b, bl) {
    const Y = 28 + b;

    // shadow
    r(17, Y+34, 30, 1, '#000811');

    // tail
    r(43, Y+18, 6, 4, '#1a4a1a');
    r(48, Y+16, 4, 4, '#1a4a1a');
    r(51, Y+13, 3, 4, '#1a4a1a');

    // body outline
    r(19, Y+8,  24, 20, '#0d2d0d');
    r(18, Y+12, 26, 12, '#0d2d0d');

    // body fill
    r(20, Y+8,  22, 20, '#1a4a1a');
    r(19, Y+12, 24, 12, '#1a4a1a');

    // belly
    r(22, Y+18, 16,  8, '#2d6a2d');

    // scale texture
    for (let row = 0; row < 3; row++) {
        for (let col = 0; col < 4; col++) {
            p(23 + col*5 + (row%2)*2, Y+12 + row*5, '#265c26');
        }
    }

    // arms
    r(14, Y+10,  5,  8, '#1a4a1a');
    r(43, Y+10,  5,  8, '#1a4a1a');
    // claws
    r(12, Y+17,  4,  2, '#0d2d0d');
    r(43, Y+17,  4,  2, '#0d2d0d');
    p(12, Y+18,         '#3a8a3a');
    p(15, Y+18,         '#3a8a3a');
    p(43, Y+18,         '#3a8a3a');
    p(46, Y+18,         '#3a8a3a');

    // legs
    r(21, Y+28,  7,  5, '#1a4a1a');
    r(34, Y+28,  7,  5, '#1a4a1a');
    // feet
    r(19, Y+31,  9,  3, '#0d2d0d');
    r(34, Y+31,  9,  3, '#0d2d0d');

    // neck
    r(26, Y+3,  10,  6, '#1a4a1a');

    // head outline
    r(20, Y-4,  22,  8, '#0d2d0d');
    // head fill
    r(21, Y-3,  20,  8, '#1a4a1a');
    r(22, Y-2,  18,  6, '#1d4d1d');

    // snout
    r(20, Y+2,  22,  4, '#0d2d0d');
    r(21, Y+3,  20,  3, '#1a4a1a');

    // horns
    r(22, Y-7,   3,  4, '#2d6a2d');
    r(38, Y-7,   3,  4, '#2d6a2d');
    p(23, Y-8,          '#3a7a3a');
    p(39, Y-8,          '#3a7a3a');

    // eye ridge
    r(22, Y-2,  18,  2, '#0d2d0d');

    // eyes (croc-style on top of head)
    if (!bl) {
        r(24, Y-1,  3, 2, '#cc0000');
        r(36, Y-1,  3, 2, '#cc0000');
        p(25, Y-1,         '#ff3333');
        p(37, Y-1,         '#ff3333');
    } else {
        r(24, Y,    3, 1, '#0d2d0d');
        r(36, Y,    3, 1, '#0d2d0d');
    }

    // bottom teeth
    for (let i = 0; i < 5; i++) {
        p(22 + i*4, Y+6, '#ccddcc');
    }
}

// ============================================================
// STAGE 7-8 — WINGED HORROR
// bat-winged flying daemon, glowing amber eyes, wing flap
// ============================================================
function drawWingedHorror(b, bl) {
    const Y = 20 + b;
    const flap = getFlapUp();

    // shadow
    r(18, Y+42, 28, 1, '#000811');

    // wings
    if (flap) {
        r(2,  Y+4,  16, 3, '#1a0840');
        r(2,  Y+2,  10, 3, '#1a0840');
        r(3,  Y+1,   6, 2, '#1a0840');
        r(4,  Y,     3, 2, '#1a0840');
        r(46, Y+4,  16, 3, '#1a0840');
        r(52, Y+2,  10, 3, '#1a0840');
        r(55, Y+1,   6, 2, '#1a0840');
        r(57, Y,     3, 2, '#1a0840');
        r(5,  Y+2,  12, 5, '#2d1060');
        r(47, Y+2,  12, 5, '#2d1060');
    } else {
        r(2,  Y+8,  16, 3, '#1a0840');
        r(3,  Y+6,  12, 3, '#1a0840');
        r(5,  Y+4,   8, 3, '#1a0840');
        r(46, Y+8,  16, 3, '#1a0840');
        r(49, Y+6,  12, 3, '#1a0840');
        r(51, Y+4,   8, 3, '#1a0840');
        r(6,  Y+5,  12, 6, '#2d1060');
        r(46, Y+5,  12, 6, '#2d1060');
    }

    // wing membrane veins
    p(16, Y + (flap ? 3 : 7), '#3d1a7a');
    p(46, Y + (flap ? 3 : 7), '#3d1a7a');

    // body outline
    r(26, Y+6,  12, 20, '#1a0840');
    r(25, Y+8,  14, 16, '#1a0840');

    // body fill
    r(27, Y+7,  10, 18, '#2d1060');
    r(26, Y+8,  12, 16, '#2d1060');

    // belly
    r(28, Y+16,  8,  8, '#3d1a7a');

    // neck
    r(28, Y+2,   8,  5, '#1a0840');
    r(29, Y+2,   6,  5, '#2d1060');

    // head outline
    r(24, Y-5,  16,  8, '#1a0840');
    // head fill
    r(25, Y-4,  14,  8, '#2d1060');
    r(26, Y-3,  12,  7, '#341270');

    // horns/crest
    r(27, Y-8,   2,  4, '#2d1060');
    r(35, Y-8,   2,  4, '#2d1060');
    p(28, Y-9,          '#3d1a7a');
    p(36, Y-9,          '#3d1a7a');

    // eye ambient glow
    if (frame % 4 < 2) {
        r(26, Y-3, 5, 4, '#2d1060');
        r(35, Y-3, 5, 4, '#2d1060');
    }

    // eyes
    if (!bl) {
        r(27, Y-2,  4, 3, '#cc8800');
        r(35, Y-2,  4, 3, '#cc8800');
        r(28, Y-2,  2, 2, '#ffcc00');
        r(36, Y-2,  2, 2, '#ffcc00');
        p(28, Y-2,         '#ffffff');
        p(36, Y-2,         '#ffffff');
    } else {
        r(27, Y-1,  4, 1, '#1a0840');
        r(35, Y-1,  4, 1, '#1a0840');
    }

    // snout
    r(28, Y+2,   8,  3, '#1a0840');
    r(29, Y+3,   6,  2, '#2d1060');

    // fangs
    p(29, Y+5, '#ccbbee');
    p(32, Y+5, '#ccbbee');
    p(34, Y+5, '#ccbbee');

    // dangling claws
    r(27, Y+24,  3,  4, '#1a0840');
    r(35, Y+24,  3,  4, '#1a0840');
    p(26, Y+27,         '#2d1060');
    p(30, Y+27,         '#2d1060');
    p(35, Y+27,         '#2d1060');
    p(38, Y+27,         '#2d1060');
}

// ============================================================
// STAGE 9-10 — DAEDROTH LORD
// massive horned daemon lord, gold eyes, scale texture
// ============================================================
function drawDaedrothLord(b, bl) {
    const Y = 10 + b;

    // shadow
    r(12, Y+52, 40, 2, '#000811');

    // body depth/shadow
    r(20, Y+20, 24, 28, '#220000');
    r(18, Y+24, 28, 22, '#220000');

    // body
    r(21, Y+20, 22, 26, '#440000');
    r(19, Y+24, 26, 20, '#440000');
    r(22, Y+21, 20, 24, '#550000');
    r(20, Y+24, 24, 18, '#550000');

    // belly
    r(24, Y+30, 16, 14, '#661111');

    // scale detail on belly
    for (let row = 0; row < 3; row++) {
        for (let col = 0; col < 3; col++) {
            p(25 + col*5 + (row%2)*2, Y+31 + row*4, '#771a1a');
        }
    }

    // arms (thick)
    r(11, Y+18,  8, 20, '#330000');
    r(10, Y+20, 10, 16, '#440000');
    r(45, Y+18,  8, 20, '#330000');
    r(44, Y+20, 10, 16, '#440000');

    // claws
    r( 8, Y+35,  4,  3, '#220000');
    r( 8, Y+37,  3,  3, '#220000');
    r(12, Y+37,  3,  3, '#220000');
    r(48, Y+35,  4,  3, '#220000');
    r(49, Y+37,  3,  3, '#220000');
    r(53, Y+37,  3,  3, '#220000');
    p( 8, Y+39,         '#553300');
    p(11, Y+39,         '#553300');
    p(14, Y+39,         '#553300');
    p(49, Y+39,         '#553300');
    p(52, Y+39,         '#553300');
    p(55, Y+39,         '#553300');

    // legs
    r(21, Y+46,  8,  6, '#330000');
    r(35, Y+46,  8,  6, '#330000');
    r(18, Y+50, 12,  3, '#220000');
    r(34, Y+50, 12,  3, '#220000');

    // neck
    r(26, Y+11, 12,  9, '#330000');
    r(27, Y+10, 10,  9, '#440000');

    // head outline
    r(19, Y+1,  26, 12, '#330000');
    r(18, Y+3,  28, 10, '#330000');
    // head fill
    r(20, Y+2,  24, 10, '#440000');
    r(19, Y+3,  26,  8, '#440000');

    // brow ridge
    r(20, Y+3,  24,  3, '#220000');

    // large horns
    r(17, Y-10,  5, 12, '#330000');
    r(18, Y-11,  3,  4, '#440000');
    p(19, Y-12,         '#550000');
    r(42, Y-10,  5, 12, '#330000');
    r(43, Y-11,  3,  4, '#440000');
    p(44, Y-12,         '#550000');

    // inner horn spikes
    r(22, Y-5,   3,  6, '#440000');
    r(39, Y-5,   3,  6, '#440000');

    // eye glow (ambient, pulsing)
    if (frame % 4 < 2) {
        r(22, Y+3,  7,  5, '#3a0000');
        r(36, Y+3,  7,  5, '#3a0000');
    }

    // eyes
    if (!bl) {
        r(23, Y+5,  4,  3, '#aa7700');
        r(37, Y+5,  4,  3, '#aa7700');
        r(24, Y+5,  3,  2, '#ffcc00');
        r(38, Y+5,  3,  2, '#ffcc00');
        p(24, Y+5,          '#ffffff');
        p(38, Y+5,          '#ffffff');
        p(25, Y+6,          '#000000');
        p(39, Y+6,          '#000000');
    } else {
        r(23, Y+6,  4,  1, '#220000');
        r(37, Y+6,  4,  1, '#220000');
    }

    // nose ridge
    r(29, Y+8,   6,  4, '#330000');

    // jaw / teeth
    r(20, Y+11, 24,  3, '#220000');
    for (let i = 0; i < 6; i++) {
        r(21 + i*4, Y+12, 2, 3, '#ccbbaa');
    }
}

// ============================================================
// OVERLAY EFFECTS
// ============================================================

function drawFeralEffect() {
    if (frame % 3 < 1) {
        r(0, Math.floor(Math.random() * 64), 64, 1, 'rgba(255,0,0,0.12)');
    }
    const grad = ctx.createRadialGradient(32, 32, 18, 32, 32, 38);
    grad.addColorStop(0, 'rgba(0,0,0,0)');
    grad.addColorStop(1, 'rgba(180,0,0,0.22)');
    ctx.fillStyle = grad;
    ctx.fillRect(0, 0, 64, 64);
}

function applyCorrosionGlitch(intensity) {
    if (intensity < 35 || Math.random() > 0.4) return;

    const imgData = ctx.getImageData(0, 0, 64, 64);
    const src = new Uint8ClampedArray(imgData.data);
    const data = imgData.data;
    const numLines = Math.ceil((intensity - 30) / 20);

    for (let i = 0; i < numLines; i++) {
        const y = Math.floor(Math.random() * 62);
        const h = 1 + Math.floor(Math.random() * 2);
        const dx = (Math.random() > 0.5 ? 1 : -1) * (1 + Math.floor(Math.random() * 3));
        for (let row = y; row < y + h && row < 64; row++) {
            for (let x = 0; x < 64; x++) {
                const sx = ((x - dx) + 64) % 64;
                const di = (row * 64 + x) * 4;
                const si = (row * 64 + sx) * 4;
                data[di]   = src[si];
                data[di+1] = src[si+1];
                data[di+2] = src[si+2];
                data[di+3] = src[si+3];
            }
        }
    }
    ctx.putImageData(imgData, 0, 0);
}

// ============================================================
// MAIN RENDER
// ============================================================

function render() {
    cls();

    const b  = getBob();
    const bl = getBlink();

    // feral shake
    let sx = 0, sy = 0;
    if (feralMode) {
        sx = Math.round((Math.random() - 0.5) * 3);
        sy = Math.round((Math.random() - 0.5) * 2);
    }

    ctx.save();
    if (sx || sy) ctx.translate(sx, sy);

    const lv = currentLevel;
    if      (lv <= 2) drawWhelpling(b, bl);
    else if (lv <= 4) drawClannfearPup(b, bl);
    else if (lv <= 6) drawDaedrothWhelp(b, bl);
    else if (lv <= 8) drawWingedHorror(b, bl);
    else              drawDaedrothLord(b, bl);

    ctx.restore();

    if (feralMode)         drawFeralEffect();
    if (petState === 'fed')    drawSparkles(32, 14);
    if (petState === 'petted') drawHearts(32, 10);
    if (petState === 'levelup' && frame % 4 < 2) {
        ctx.fillStyle = 'rgba(255,200,0,0.18)';
        ctx.fillRect(0, 0, 64, 64);
    }

    applyCorrosionGlitch(corrosionPct);

    if (stateTimer > 0 && --stateTimer === 0) petState = 'idle';
}

// ============================================================
// ANIMATION LOOP
// ============================================================

function tick(ts) {
    if (ts - lastTick >= TICK_MS) {
        lastTick = ts;
        frame++;
        render();
    }
    requestAnimationFrame(tick);
}

requestAnimationFrame(tick);

// ============================================================
// STATUS / API
// ============================================================

async function updateStatus() {
    try {
        const response = await fetch('/status');
        const data = await response.json();

        const hunger   = data.zahl.hunger;
        const level    = data.zahl.level;
        const corrosion = data.zahl.corrosion;
        const exp      = data.zahl.exp;
        const feral    = data.zahl.feral === '1';

        hungerPct    = hunger;
        corrosionPct = corrosion;
        feralMode    = feral;

        document.getElementById('hungerBar').style.width    = hunger + '%';
        document.getElementById('hungerVal').innerText      = hunger + '%';
        document.getElementById('levelBar').style.width     = (level * 10) + '%';
        document.getElementById('levelVal').innerText       = level + '/10';
        document.getElementById('corrosionBar').style.width = corrosion + '%';
        document.getElementById('corrosionVal').innerText   = corrosion + '%';
        document.getElementById('expBar').style.width       = exp + '%';
        document.getElementById('expVal').innerText         = exp + '/100';

        const moodElem = document.getElementById('mood');
        if (feral) {
            moodElem.innerText    = 'FERAL';
            moodElem.style.color  = '#f00';
        } else if (hunger < 20) {
            moodElem.innerText    = 'STARVING';
            moodElem.style.color  = '#f66';
        } else if (hunger > 80) {
            moodElem.innerText    = 'FULL';
            moodElem.style.color  = '#0f0';
        } else {
            moodElem.innerText    = 'HUNGRY';
            moodElem.style.color  = '#f90';
        }

        if (currentLevel !== level) {
            currentLevel = level;
            petState     = 'levelup';
            stateTimer   = 32;
            const mistress = document.getElementById('mistress');
            mistress.innerText = `MISTRESS DRATHA: "Level ${level}? Don't get cocky, n'wah."`;
            setTimeout(() => {
                mistress.innerText = 'MISTRESS DRATHA: "Feed the beast or lose your stack."';
            }, 3000);
        }
    } catch(e) {
        console.error('status update failed', e);
    }
}

async function fetchCaptures() {
    try {
        const response = await fetch('/captures');
        const data = await response.json();

        const captureDiv = document.getElementById('captureLog');
        captureDiv.innerHTML = '';
        data.captures.slice(0, 10).forEach(cap => {
            const div = document.createElement('div');
            div.className = 'log-entry';
            div.innerText = `> ${cap.mac} - RSSI ${cap.rssi}`;
            captureDiv.appendChild(div);
        });

        const networkDiv = document.getElementById('networkLog');
        networkDiv.innerHTML = '';
        data.networks.slice(0, 10).forEach(net => {
            const div = document.createElement('div');
            div.className = 'log-entry';
            div.innerText = `> ${net.ssid} (${net.rssi}dBm)`;
            networkDiv.appendChild(div);

            const select = document.getElementById('targetSelect');
            if (!Array.from(select.options).some(opt => opt.value === net.ssid)) {
                const option = document.createElement('option');
                option.value   = net.ssid;
                option.innerText = net.ssid;
                select.appendChild(option);
            }
        });
    } catch(e) {
        console.error('capture fetch failed', e);
    }
}

async function feed(type) {
    try {
        await fetch(`/api?action=feed&type=${type}`);
        updateStatus();
        petState   = 'fed';
        stateTimer = 24;

        const scrib = document.getElementById('scrib');
        scrib.innerText = 'SCRIB JIB: *click* *slurp*';
        setTimeout(() => { scrib.innerText = 'SCRIB JIB clicks hungrily.'; }, 2000);
    } catch(e) {
        console.error('feed failed', e);
    }
}

async function pet() {
    try {
        await fetch('/api?action=pet');
        updateStatus();
        petState   = 'petted';
        stateTimer = 24;
    } catch(e) {
        console.error('pet failed', e);
    }
}

async function scan() {
    const btn = document.getElementById('scanBtn');
    btn.disabled = true;
    btn.innerText = 'SNIFFING...';
    await fetchCaptures();
    setTimeout(() => { btn.disabled = false; btn.innerText = 'SNIFF FRAMES'; }, 3000);
}

async function deauth() {
    const select = document.getElementById('targetSelect');
    if (!select.value) return;
    const btn = document.getElementById('deauthBtn');
    btn.disabled = true;
    btn.innerText = 'UNLEASHING...';
    await fetch('/api?action=deauth_target&mac=FF:FF:FF:FF:FF:FF');
    updateStatus();
    fetchCaptures();
    setTimeout(() => { btn.disabled = false; btn.innerText = 'FERAL BURST'; }, 2000);
}

// SSE
function initEventSource() {
    if (eventSource) eventSource.close();
    eventSource = new EventSource('/events');

    eventSource.addEventListener('status', function(e) {
        e.data.split(',').forEach(part => {
            const [key, val] = part.split(':');
            if (key === 'hunger') {
                hungerPct = parseInt(val);
                document.getElementById('hungerBar').style.width = val + '%';
                document.getElementById('hungerVal').innerText   = val + '%';
            } else if (key === 'level') {
                const lv = parseInt(val);
                if (lv !== currentLevel) {
                    currentLevel = lv;
                    petState     = 'levelup';
                    stateTimer   = 32;
                }
                document.getElementById('levelBar').style.width = (lv * 10) + '%';
                document.getElementById('levelVal').innerText   = val + '/10';
            } else if (key === 'corrosion') {
                corrosionPct = parseInt(val);
                document.getElementById('corrosionBar').style.width = val + '%';
                document.getElementById('corrosionVal').innerText   = val + '%';
            }
        });
    });
}

// button handlers
document.getElementById('feedSnack').onclick  = () => feed('snack');
document.getElementById('feedHash').onclick   = () => feed('hash');
document.getElementById('feedDeauth').onclick = () => feed('deauth');
document.getElementById('petBtn').onclick     = pet;
document.getElementById('scanBtn').onclick    = scan;
document.getElementById('deauthBtn').onclick  = deauth;

// init
initEventSource();
updateStatus();
fetchCaptures();
setInterval(updateStatus,  2000);
setInterval(fetchCaptures, 5000);

// NPC whispers
setInterval(() => {
    const whispers = [
        'MISTRESS DRATHA: "The corrosion spreads. Feed it."',
        'MISTRESS DRATHA: "I once saw a level 10 Daedroth eat a router."',
        'MISTRESS DRATHA: "Deauth the printer. It knows too much."',
        'MISTRESS DRATHA: "Hash blood is sweeter than packet snacks."',
        'MISTRESS DRATHA: "The Daedroth grows. Do not disappoint me."',
        'MISTRESS DRATHA: "Every frame captured is a soul claimed."',
    ];
    const scribSounds = [
        'SCRIB JIB: *click* *click* *squeak*',
        'SCRIB JIB: *hungry mandible noises*',
        'SCRIB JIB: *scuttling sounds*',
        'SCRIB JIB: *antennae twitch*',
        'SCRIB JIB: *ominous clicking*',
    ];
    if (petState !== 'levelup') {
        document.getElementById('mistress').innerText =
            whispers[Math.floor(Math.random() * whispers.length)];
    }
    document.getElementById('scrib').innerText =
        scribSounds[Math.floor(Math.random() * scribSounds.length)];
}, 15000);

// container shake (feral / high corrosion)
setInterval(() => {
    if (feralMode || corrosionPct > 60) {
        const c = document.querySelector('.container');
        c.style.transform = `translate(${Math.random() * 4 - 2}px, ${Math.random() * 4 - 2}px)`;
        setTimeout(() => { c.style.transform = 'translate(0,0)'; }, 100);
    }
}, 500);
