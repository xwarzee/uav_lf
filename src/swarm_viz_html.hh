#pragma once
#include <string>

// Embedded HTML page for the SwarmVisualizer HTTP server.
// Kept in a separate header to avoid LF parser issues with raw string literals
// and JavaScript single-quoted strings inside {= ... =} blocks.

namespace swarm_viz {

static const std::string HTML = R"HTML(<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="utf-8">
<title>DroneSwarm — Live</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{background:#050510;overflow:hidden;font-family:'Courier New',monospace}
canvas{display:block}
#hud{position:fixed;top:14px;left:14px;color:#80c0ff;font-size:12px;
     pointer-events:none;text-shadow:0 0 6px #4080ff;line-height:1.7}
#hud h2{font-size:13px;color:#fff;letter-spacing:3px;margin-bottom:6px}
#status{position:fixed;bottom:14px;left:14px;font-size:11px;color:#40ff80}
#hint{position:fixed;bottom:14px;right:14px;font-size:10px;color:#445566;text-align:right}
</style>
</head>
<body>
<div id="hud"><h2>&#x2b21; DRONE SWARM</h2><div id="list"></div></div>
<div id="status">&#x25cf; CONNEXION&#x2026;</div>
<div id="hint">Glisser&nbsp;: rotation &nbsp; Scroll&nbsp;: zoom<br>Clic droit&nbsp;: panoramique</div>

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
document.body.appendChild(renderer.domElement);

const controls = new THREE.OrbitControls(camera, renderer.domElement);
controls.enableDamping = true;
controls.dampingFactor = 0.08;
controls.target.set(0, 15, 0);

// ── Ground grid ───────────────────────────────────────────────────────────
scene.add(new THREE.GridHelper(500, 50, 0x0d1f33, 0x071018));
scene.add(new THREE.AxesHelper(25));

// ── Lights ────────────────────────────────────────────────────────────────
scene.add(new THREE.AmbientLight(0x203040, 2.5));
const sun = new THREE.DirectionalLight(0xffffff, 1.2);
sun.position.set(100, 200, 80);
scene.add(sun);

// ── Drone palette ─────────────────────────────────────────────────────────
const COLORS = [0xff3333, 0x33ff77, 0x3377ff, 0xffcc00, 0xff33ff, 0x33ffff,
                0xff8800, 0x88ff00];
const MAX_TRAIL = 150;
const drones = {};

function getOrCreate(id) {
  if (drones[id]) return drones[id];
  const c = COLORS[(id - 1) % COLORS.length];

  const geo = new THREE.SphereGeometry(1.5, 16, 16);
  const mat = new THREE.MeshPhongMaterial({
    color: c,
    emissive: new THREE.Color(c).multiplyScalar(0.35),
    shininess: 90
  });
  const mesh = new THREE.Mesh(geo, mat);
  scene.add(mesh);

  const light = new THREE.PointLight(c, 3, 22);
  mesh.add(light);

  // Label
  const cv = document.createElement("canvas");
  cv.width = 64; cv.height = 32;
  const ctx = cv.getContext("2d");
  ctx.fillStyle = "#" + c.toString(16).padStart(6, "0");
  ctx.font = "bold 20px Courier New";
  ctx.fillText("D" + id, 6, 24);
  const sp = new THREE.Sprite(new THREE.SpriteMaterial(
    {map: new THREE.CanvasTexture(cv), transparent: true, depthTest: false}));
  sp.scale.set(7, 3.5, 1);
  sp.position.set(0, 3.5, 0);
  mesh.add(sp);

  drones[id] = {mesh: mesh, trailPts: [], trailLine: null};
  return drones[id];
}

// UAV: x=east, y=north, z=alt  →  Three.js Y=up: (x, z, -y)
function uavToThree(x, y, z) { return new THREE.Vector3(x, z, -y); }

function moveDrone(id, x, y, z) {
  const d = getOrCreate(id);
  const p = uavToThree(x, y, z);
  d.mesh.position.copy(p);

  d.trailPts.push(p.clone());
  if (d.trailPts.length > MAX_TRAIL) d.trailPts.shift();
  if (d.trailLine) scene.remove(d.trailLine);
  if (d.trailPts.length > 1) {
    const g = new THREE.BufferGeometry().setFromPoints(d.trailPts);
    const m = new THREE.LineBasicMaterial(
      {color: COLORS[(id - 1) % COLORS.length], opacity: 0.45, transparent: true});
    d.trailLine = new THREE.Line(g, m);
    scene.add(d.trailLine);
  }
}

// Formation target marker (ring on the ground plane)
let targetMesh = null;
function setTarget(x, y, z) {
  if (!targetMesh) {
    const g = new THREE.RingGeometry(3.5, 5, 32);
    const m = new THREE.MeshBasicMaterial(
      {color: 0xffffff, side: THREE.DoubleSide, opacity: 0.25, transparent: true});
    targetMesh = new THREE.Mesh(g, m);
    targetMesh.rotation.x = -Math.PI / 2;
    scene.add(targetMesh);
  }
  const p = uavToThree(x, y, z);
  targetMesh.position.set(p.x, 0.1, p.z);
}

// ── Animation ─────────────────────────────────────────────────────────────
const clock = new THREE.Clock();
(function animate() {
  requestAnimationFrame(animate);
  controls.update();
  const t = clock.getElapsedTime();
  Object.entries(drones).forEach(function(entry) {
    entry[1].mesh.position.y += Math.sin(t * 2.1 + Number(entry[0]) * 1.4) * 0.005;
  });
  renderer.render(scene, camera);
})();

window.addEventListener("resize", function() {
  camera.aspect = innerWidth / innerHeight;
  camera.updateProjectionMatrix();
  renderer.setSize(innerWidth, innerHeight);
});

// ── Server-Sent Events ────────────────────────────────────────────────────
const statusEl = document.getElementById("status");
const listEl   = document.getElementById("list");

const es = new EventSource("/events");
es.onopen  = function() { statusEl.style.color = "#40ff80"; statusEl.textContent = "\u25cf LIVE"; };
es.onerror = function() { statusEl.style.color = "#ff4444"; statusEl.textContent = "\u25cf RECONNEXION\u2026"; };
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
