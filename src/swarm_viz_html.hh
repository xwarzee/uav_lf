#pragma once
#include <string>

// Embedded HTML page for the SwarmVisualizer HTTP server.
// Kept in a separate header to avoid LF parser issues with raw string literals
// and JavaScript single-quoted strings inside {= ... =} blocks.

namespace swarm_viz {

static const std::string HTML = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>DroneSwarm — Live</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{background:#050510;overflow:hidden;font-family:'Courier New',monospace}
canvas{display:block}
#hud{position:fixed;top:14px;left:14px;color:#80c0ff;font-size:12px;
     pointer-events:none;text-shadow:0 0 6px #4080ff;line-height:1.8}
#hud h2{font-size:13px;color:#fff;letter-spacing:3px;margin-bottom:8px}
#status{position:fixed;bottom:14px;left:14px;font-size:11px;color:#40ff80}
#hint{position:fixed;bottom:14px;right:14px;font-size:10px;color:#445566;text-align:right}
</style>
</head>
<body>
<div id="hud"><h2>&#x2b21; DRONE SWARM</h2><div id="list"></div></div>
<div id="status">&#x25cf; CONNECTING&#x2026;</div>
<div id="hint">Drag: rotate &nbsp; Scroll: zoom<br>Right-click: pan</div>

<script src="https://cdnjs.cloudflare.com/ajax/libs/three.js/r128/three.min.js"></script>
<script src="https://cdn.jsdelivr.net/npm/three@0.128.0/examples/js/controls/OrbitControls.js"></script>
<script>
// ── Scene ─────────────────────────────────────────────────────────────────
const scene = new THREE.Scene();
scene.background = new THREE.Color(0x050510);
scene.fog = new THREE.FogExp2(0x050510, 0.0025);

const camera = new THREE.PerspectiveCamera(55, innerWidth/innerHeight, 0.1, 4000);
camera.position.set(80, 60, 100);
camera.lookAt(0, 15, 0);

const renderer = new THREE.WebGLRenderer({antialias: true});
renderer.setSize(innerWidth, innerHeight);
renderer.setPixelRatio(devicePixelRatio);
renderer.shadowMap.enabled = true;
document.body.appendChild(renderer.domElement);

const controls = new THREE.OrbitControls(camera, renderer.domElement);
controls.enableDamping = true;
controls.dampingFactor = 0.08;
controls.target.set(0, 15, 0);

// ── Ground grid ───────────────────────────────────────────────────────────
const grid = new THREE.GridHelper(500, 50, 0x0d1f33, 0x071018);
scene.add(grid);
scene.add(new THREE.AxesHelper(25));

// ── Lights ────────────────────────────────────────────────────────────────
scene.add(new THREE.AmbientLight(0x304050, 3));
const sun = new THREE.DirectionalLight(0xffffff, 1.5);
sun.position.set(100, 200, 80);
sun.castShadow = true;
scene.add(sun);

// ── Drone palette ─────────────────────────────────────────────────────────
const COLORS = [0xff3333, 0x33ff77, 0x3377ff, 0xffcc00, 0xff33ff, 0x33ffff,
                0xff8800, 0x88ff00];
const MAX_TRAIL = 150;
const drones = {};   // id → {group, rotors, trailPts, trailLine, prevPos}

// Build one quadcopter mesh group
function buildQuadcopter(colorHex) {
  const group = new THREE.Group();
  const rotors = [];
  const mainColor = new THREE.Color(colorHex);

  // ── Central body ──────────────────────────────────────────────────────
  const bodyGeo = new THREE.CylinderGeometry(1.1, 0.9, 0.55, 8);
  const bodyMat = new THREE.MeshPhongMaterial({
    color: colorHex,
    emissive: mainColor.clone().multiplyScalar(0.15),
    shininess: 120
  });
  group.add(new THREE.Mesh(bodyGeo, bodyMat));

  // Top cover dome
  const topGeo = new THREE.SphereGeometry(0.9, 8, 4, 0, Math.PI*2, 0, Math.PI/2);
  const topMat = new THREE.MeshPhongMaterial({
    color: colorHex,
    emissive: mainColor.clone().multiplyScalar(0.1),
    shininess: 160
  });
  const top = new THREE.Mesh(topGeo, topMat);
  top.position.y = 0.28;
  group.add(top);

  // Camera dome (bottom)
  const camGeo = new THREE.SphereGeometry(0.42, 8, 6, 0, Math.PI*2, Math.PI/2, Math.PI/2);
  const camMat = new THREE.MeshPhongMaterial({color: 0x111111, shininess: 220});
  const cam = new THREE.Mesh(camGeo, camMat);
  cam.position.y = -0.28;
  group.add(cam);

  // ── 4 arms + rotors ───────────────────────────────────────────────────
  const ARM_LEN   = 4.2;
  const ARM_DEG   = [45, 135, 225, 315];
  const armMat    = new THREE.MeshPhongMaterial({color: 0x1e1e1e, shininess: 40});
  const motorMat  = new THREE.MeshPhongMaterial({color: 0x0a0a0a, shininess: 80});
  const bladeMat  = new THREE.MeshPhongMaterial({
    color: 0x2a2a2a, side: THREE.DoubleSide, transparent: true, opacity: 0.88
  });

  ARM_DEG.forEach(function(deg, idx) {
    const rad = deg * Math.PI / 180;
    const tx  = Math.cos(rad) * ARM_LEN;
    const tz  = Math.sin(rad) * ARM_LEN;

    // Arm (thin box rotated toward tip)
    const armGeo = new THREE.BoxGeometry(ARM_LEN, 0.2, 0.2);
    const arm = new THREE.Mesh(armGeo, armMat);
    arm.position.set(tx / 2, 0, tz / 2);
    arm.rotation.y = -rad;
    group.add(arm);

    // Motor housing
    const mh = new THREE.Mesh(new THREE.CylinderGeometry(0.32, 0.32, 0.38, 8), motorMat);
    mh.position.set(tx, 0.08, tz);
    group.add(mh);

    // Propeller: 2 crossed blades
    const rotorGroup = new THREE.Group();
    rotorGroup.position.set(tx, 0.38, tz);
    rotorGroup._spinDir = (idx % 2 === 0) ? 1 : -1;

    const b1 = new THREE.Mesh(new THREE.BoxGeometry(2.9, 0.06, 0.28), bladeMat);
    const b2 = new THREE.Mesh(new THREE.BoxGeometry(2.9, 0.06, 0.28), bladeMat);
    b2.rotation.y = Math.PI / 2;
    rotorGroup.add(b1);
    rotorGroup.add(b2);
    group.add(rotorGroup);
    rotors.push(rotorGroup);

    // Rotor LED glow (alternating red/green for navigation lights)
    const ledCol = (idx === 0) ? 0x00ff44 : (idx === 1) ? 0xff2200 : 0xffffff;
    const led = new THREE.PointLight(ledCol, 1.2, 10);
    led.position.set(tx, 0.6, tz);
    group.add(led);
  });

  // ── Landing legs ──────────────────────────────────────────────────────
  const legMat = new THREE.MeshPhongMaterial({color: 0x1a1a1a});
  [[-1.1, -1.1], [1.1, -1.1], [-1.1, 1.1], [1.1, 1.1]].forEach(function(xz) {
    const leg = new THREE.Mesh(new THREE.CylinderGeometry(0.07, 0.07, 0.7, 6), legMat);
    leg.position.set(xz[0], -0.62, xz[1]);
    group.add(leg);
    const foot = new THREE.Mesh(new THREE.BoxGeometry(0.6, 0.07, 0.07), legMat);
    foot.position.set(xz[0], -1.0, xz[1]);
    group.add(foot);
  });

  return {group: group, rotors: rotors};
}

function getOrCreate(id) {
  if (drones[id]) return drones[id];
  const c = COLORS[(id - 1) % COLORS.length];

  const q = buildQuadcopter(c);

  // Label sprite
  const cv = document.createElement("canvas");
  cv.width = 64; cv.height = 32;
  const ctx = cv.getContext("2d");
  ctx.fillStyle = "#" + c.toString(16).padStart(6, "0");
  ctx.font = "bold 20px Courier New";
  ctx.fillText("D" + id, 6, 24);
  const sp = new THREE.Sprite(new THREE.SpriteMaterial(
    {map: new THREE.CanvasTexture(cv), transparent: true, depthTest: false}));
  sp.scale.set(7, 3.5, 1);
  sp.position.set(0, 5.5, 0);
  q.group.add(sp);

  scene.add(q.group);
  drones[id] = {
    group: q.group,
    rotors: q.rotors,
    trailPts: [],
    trailLine: null,
    prevPos: null
  };
  return drones[id];
}

// UAV: x=east, y=north, z=alt  →  Three.js Y=up: (x, z, -y)
function uavToThree(x, y, z) {
  return new THREE.Vector3(x, z, -y);
}

function moveDrone(id, x, y, z) {
  const d = getOrCreate(id);
  const p = uavToThree(x, y, z);

  // Banking: tilt the body toward the direction of motion
  if (d.prevPos) {
    const vel = p.clone().sub(d.prevPos);
    const spd = vel.length();
    const TILT = 0.45;
    if (spd > 0.02) {
      d.group.rotation.z = -vel.x * TILT / Math.max(spd, 0.1);
      d.group.rotation.x =  vel.z * TILT / Math.max(spd, 0.1);
    } else {
      d.group.rotation.z *= 0.88;
      d.group.rotation.x *= 0.88;
    }
  }
  d.prevPos = p.clone();
  d.group.position.copy(p);

  // Trail
  d.trailPts.push(p.clone());
  if (d.trailPts.length > MAX_TRAIL) d.trailPts.shift();
  if (d.trailLine) scene.remove(d.trailLine);
  if (d.trailPts.length > 1) {
    const g = new THREE.BufferGeometry().setFromPoints(d.trailPts);
    const m = new THREE.LineBasicMaterial(
      {color: COLORS[(id - 1) % COLORS.length], opacity: 0.4, transparent: true});
    d.trailLine = new THREE.Line(g, m);
    scene.add(d.trailLine);
  }
}

// Formation target marker (ring on the ground)
let targetMesh = null;
function setTarget(x, y, z) {
  if (!targetMesh) {
    const g = new THREE.RingGeometry(3.5, 5, 32);
    const m = new THREE.MeshBasicMaterial(
      {color: 0xffffff, side: THREE.DoubleSide, opacity: 0.2, transparent: true});
    targetMesh = new THREE.Mesh(g, m);
    targetMesh.rotation.x = -Math.PI / 2;
    scene.add(targetMesh);
  }
  const p = uavToThree(x, y, z);
  targetMesh.position.set(p.x, 0.1, p.z);
}

// ── Animation loop ─────────────────────────────────────────────────────────
const clock = new THREE.Clock();
(function animate() {
  requestAnimationFrame(animate);
  controls.update();

  // Spin propellers + subtle hover
  const t = clock.getElapsedTime();
  Object.entries(drones).forEach(function(entry) {
    const d = entry[1];
    d.rotors.forEach(function(r) {
      r.rotation.y += r._spinDir * 0.35;
    });
    // Subtle vertical hover oscillation
    d.group.position.y += Math.sin(t * 2.2 + Number(entry[0]) * 1.5) * 0.004;
  });

  renderer.render(scene, camera);
})();

window.addEventListener("resize", function() {
  camera.aspect = innerWidth / innerHeight;
  camera.updateProjectionMatrix();
  renderer.setSize(innerWidth, innerHeight);
});

// ── Server-Sent Events ─────────────────────────────────────────────────────
const statusEl = document.getElementById("status");
const listEl   = document.getElementById("list");

const es = new EventSource("/events");
es.onopen  = function() {
  statusEl.style.color = "#40ff80";
  statusEl.textContent = "\u25cf LIVE";
};
es.onerror = function() {
  statusEl.style.color = "#ff4444";
  statusEl.textContent = "\u25cf RECONNECTING\u2026";
};
es.onmessage = function(e) {
  const data = JSON.parse(e.data);
  listEl.innerHTML = "";
  data.drones.forEach(function(dr) {
    moveDrone(dr.id, dr.x, dr.y, dr.z);
    const hex = "#" + COLORS[(dr.id - 1) % COLORS.length].toString(16).padStart(6, "0");
    const row = document.createElement("div");
    row.innerHTML =
      "<span style=\"color:" + hex + "\">&#x25b6; D" + dr.id + "</span>" +
      "&nbsp; x:<b>" + dr.x.toFixed(1) + "</b>" +
      "&nbsp; y:<b>" + dr.y.toFixed(1) + "</b>" +
      "&nbsp; z:<b>" + dr.z.toFixed(1) + "</b>";
    listEl.appendChild(row);
  });
  if (data.target) setTarget(data.target.x, data.target.y, data.target.z);
};
</script>
</body>
</html>
)HTML";

} // namespace swarm_viz
