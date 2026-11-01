// Capture timestamped screenshots of the built local workbench for submission.
// Development-only: uses installed Windows Edge and the already-built package assets.
import { spawn } from 'node:child_process';
import { createServer } from 'node:net';
import { mkdir, readFile, writeFile } from 'node:fs/promises';
import { existsSync } from 'node:fs';
import { resolve, join } from 'node:path';

const root = resolve(import.meta.dirname, '..');
const edgeCandidates = [
  process.env.UEW_EDGE,
  'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe',
  'C:/Program Files/Microsoft/Edge/Application/msedge.exe',
].filter(Boolean);
const edge = edgeCandidates.find(existsSync);
if (!edge) throw new Error('Microsoft Edge was not found. Set UEW_EDGE to its executable path.');
const backendPath = join(root, 'build/package-vs18-x64/Release/workbench_server.exe');
const webRoot = join(root, 'frontend/dist');
const stlPath = join(root, 'data/examples/unit_tetrahedron_ascii.stl');
const csvPath = join(root, 'data/examples/synthetic_temperature_points.csv');
const outDir = join(root, 'docs/demo/screenshots');
const demoPort = 18122;
const logs = [];
let server;
let browser;
let socket;
let sequence = 0;
let requestId = 0;
const pending = new Map();

function timestamp() { return new Date().toISOString(); }
function log(message) { logs.push(`${timestamp()}  ${message}`); }
function delay(ms) { return new Promise((done) => setTimeout(done, ms)); }
function walkthroughText() {
  return [
    '# Timestamped demonstration walkthrough', '',
    'Captured with the bundled Windows x64 Release backend, the production Vite build, and local Microsoft Edge headless rendering on 2026-11-01. Timestamp strings are UTC. Example STL and temperature CSV are the repository self-created examples; the CSV is synthetic, not ANSYS output. No hardware was connected.',
    '', ...logs.map((line) => `- ${line}`), '',
  ].join('\n');
}

async function getFreePort() {
  const listener = createServer();
  await new Promise((resolveListen, reject) => listener.listen(0, '127.0.0.1', resolveListen).once('error', reject));
  const port = listener.address().port;
  await new Promise((resolveClose) => listener.close(resolveClose));
  return port;
}

async function waitUntil(description, condition, timeout = 20000) {
  const start = Date.now();
  while (Date.now() - start < timeout) {
    if (await condition()) return;
    await delay(250);
  }
  const visible = socket ? await evaluate('document.body.innerText').catch(() => '(unable to read page)') : '';
  throw new Error(`Timed out waiting for ${description}${visible ? `; current page text: ${visible.replaceAll(/\s+/g, ' ').slice(0, 500)}` : ''}`);
}

async function cdp(method, params = {}) {
  const id = ++requestId;
  return new Promise((resolveReply, rejectReply) => {
    const timer = setTimeout(() => { pending.delete(id); rejectReply(new Error(`CDP timeout: ${method}`)); }, 15000);
    pending.set(id, { resolve: resolveReply, reject: rejectReply, timer });
    socket.send(JSON.stringify({ id, method, params }));
  });
}

async function evaluate(expression) {
  const result = await cdp('Runtime.evaluate', {
    expression,
    awaitPromise: true,
    returnByValue: true,
    userGesture: true,
  });
  if (result.exceptionDetails) throw new Error(result.exceptionDetails.text ?? 'Browser script failed');
  return result.result?.value;
}

async function clickButton(label) {
  const found = await evaluate(`(() => {
    const button = [...document.querySelectorAll('button')].find((item) => item.textContent.trim() === ${JSON.stringify(label)});
    if (!button) return false;
    button.click(); return true;
  })()`);
  if (!found) throw new Error(`UI button was not found: ${label}`);
  await delay(250);
}

async function chooseFile(path) {
  const documentTree = await cdp('DOM.getDocument', { depth: -1 });
  const rootNode = documentTree.root.nodeId;
  const query = await cdp('DOM.querySelector', { nodeId: rootNode, selector: 'input[type="file"]' });
  if (!query.nodeId) throw new Error('File input not present on Geometry page');
  await cdp('DOM.setFileInputFiles', { files: [path], nodeId: query.nodeId });
}

async function capture(label, evidence) {
  const stamp = timestamp();
  const name = `${String(++sequence).padStart(2, '0')}_${label.replaceAll(/[^A-Za-z0-9_-]/g, '_')}.png`;
  const shot = await cdp('Page.captureScreenshot', { format: 'png', captureBeyondViewport: false, fromSurface: true });
  await writeFile(join(outDir, name), Buffer.from(shot.data, 'base64'));
  const pageText = await evaluate('document.body.innerText');
  log(`${stamp} | ${name} | ${evidence}; visible text sample: ${pageText.replaceAll(/\s+/g, ' ').slice(0, 230)}`);
}

try {
  await mkdir(outDir, { recursive: true });
  const apiPort = await getFreePort();
  server = spawn(backendPath, [], {
    cwd: root,
    windowsHide: true,
    stdio: 'ignore',
    env: { ...process.env, WORKBENCH_PORT: String(apiPort), WORKBENCH_WEB_ROOT: webRoot },
  });
  const baseUrl = `http://127.0.0.1:${apiPort}`;
  await waitUntil('C++ health endpoint', async () => {
    try { return (await (await fetch(`${baseUrl}/api/health`)).json()).status === 'ok'; }
    catch { return false; }
  });
  log(`API health verified at ${baseUrl}/api/health (HTTP 200)`);

  const requestJson = async (path, options = {}) => {
    const response = await fetch(`${baseUrl}${path}`, options);
    const body = await response.json();
    if (!response.ok) throw new Error(`${path} failed with HTTP ${response.status}: ${body.error ?? ''}`);
    return body;
  };
  const health = await requestJson('/api/health');
  const status = await requestJson('/api/status');
  const hardware = await requestJson('/api/hardware/devices');
  const geometry = await requestJson('/api/geometry/stl', {
    method: 'POST', headers: { 'Content-Type': 'application/octet-stream' }, body: await readFile(stlPath),
  });
  const pointCloud = await requestJson('/api/geometry/temperature', {
    method: 'POST', headers: { 'Content-Type': 'application/octet-stream' }, body: await readFile(csvPath),
  });
  const thermal = await requestJson('/api/thermal/solve', {
    method: 'POST', headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ parameters: { length_m: 1, area_m2: 1e-4, conductivity_w_per_m_k: 10,
      heat_generation_w_per_m3: 10000, left_temperature_c: 100, right_temperature_c: 20 }, meshes: [10, 20, 40, 80] }),
  });
  const benchmark = await requestJson('/api/performance/benchmark', {
    method: 'POST', headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ case_count: 40, mesh_size: 80, workers: 3, seed: 20261101 }),
  });
  const model = await requestJson('/api/ml/backprop');
  const smokeRows = [
    '# Package API smoke log', '', `Timestamp UTC: ${timestamp()}`,
    'Endpoint responses came from the locally packaged C++ backend. This log is from one host and contains no hardware measurements.', '',
    `GET /api/health -> ${JSON.stringify(health)}`,
    `GET /api/status -> ${JSON.stringify(status)}`,
    `GET /api/hardware/devices -> simulated=${hardware.simulated}, device_count=${hardware.devices.length}`,
    `POST /api/geometry/stl (data/examples/unit_tetrahedron_ascii.stl) -> triangles=${geometry.triangle_count}, solid=${geometry.solid_name}`,
    `POST /api/geometry/temperature (synthetic example CSV) -> nodes=${pointCloud.node_count}, range_C=${pointCloud.temperature_range_C.join('..')}`,
    `POST /api/thermal/solve -> meshes=${thermal.meshes.map((mesh) => `${mesh.element_count}:${mesh.maximum_midpoint_error_C}C`).join(', ')}`,
    `POST /api/performance/benchmark -> cases=${benchmark.case_count}, mesh=${benchmark.mesh_size}, workers=${benchmark.workers}, reps=${benchmark.measured_repetitions}, median_serial_ms=${benchmark.median_serial_ms}, median_parallel_ms=${benchmark.median_parallel_ms}, max_delta_C=${benchmark.max_temperature_difference_C}`,
    `GET /api/ml/backprop -> loss_rows=${model.loss_history.length}, final_loss=${model.final_loss}, max_gradient_error=${Math.max(...Object.values(model.gradient_check.absolute_error))}`,
    '',
  ].join('\n');
  if (geometry.triangle_count !== 4 || pointCloud.node_count !== 8 || thermal.meshes.length !== 4 ||
      benchmark.measured_repetitions !== 5 || hardware.devices.length !== 3 || model.loss_history.length !== 102) {
    throw new Error('Packaged API smoke response did not match expected module result counts.');
  }
  await writeFile(join(root, 'docs/demo/package-smoke.log'), `${smokeRows}\n`, 'utf8');

  const debugPort = await getFreePort();
  const profileDir = join(root, 'build/demo-edge-profile');
  browser = spawn(edge, [
    '--headless=new', '--enable-webgl', '--ignore-gpu-blocklist', '--use-gl=angle',
    '--use-angle=swiftshader', '--no-first-run', '--no-default-browser-check',
    '--disable-extensions', '--remote-allow-origins=*', `--user-data-dir=${profileDir}`,
    `--remote-debugging-port=${debugPort}`, '--window-size=1680,1060', 'about:blank',
  ], { windowsHide: true, stdio: 'ignore' });
  const debuggerUrl = `http://127.0.0.1:${debugPort}`;
  await waitUntil('Edge DevTools endpoint', async () => {
    try { return (await fetch(`${debuggerUrl}/json/version`)).ok; } catch { return false; }
  });
  const tab = await fetch(`${debuggerUrl}/json/new?${encodeURIComponent(baseUrl)}`, { method: 'PUT' }).then((response) => response.json());
  socket = new WebSocket(tab.webSocketDebuggerUrl);
  await new Promise((resolveOpen, rejectOpen) => {
    socket.addEventListener('open', resolveOpen, { once: true });
    socket.addEventListener('error', rejectOpen, { once: true });
  });
  socket.addEventListener('message', (event) => {
    const message = JSON.parse(event.data);
    if (message.method === 'Runtime.exceptionThrown') {
      log(`Browser exception: ${message.params.exceptionDetails.exception?.description ?? message.params.exceptionDetails.text}`);
    }
    if (message.method === 'Runtime.consoleAPICalled') {
      const values = (message.params.args ?? []).map((argument) => argument.value ?? argument.description ?? '').join(' ');
      if (values) log(`Browser console ${message.params.type}: ${values.slice(0, 300)}`);
    }
    if (message.id && pending.has(message.id)) {
      const entry = pending.get(message.id);
      clearTimeout(entry.timer);
      pending.delete(message.id);
      if (message.error) entry.reject(new Error(message.error.message));
      else entry.resolve(message.result);
    }
  });
  await cdp('Page.enable');
  await cdp('Runtime.enable');
  await cdp('DOM.enable');
  await cdp('Emulation.setDeviceMetricsOverride', { width: 1680, height: 1060, deviceScaleFactor: 1, mobile: false });
  await waitUntil('connected dashboard', async () => (await evaluate('document.body.innerText')).includes('CONNECTED'));
  await delay(700);
  await capture('Overview', 'Dashboard startup and connected backend status');

  await clickButton('Geometry Viewer');
  await chooseFile(stlPath);
  log('Selected data/examples/unit_tetrahedron_ascii.stl through the browser file input');
  await waitUntil('ASCII STL model information', async () => (await evaluate('document.body.innerText')).includes('Triangles'));
  await delay(900);
  await capture('ASCII_STL', 'C++ parsed the self-created ASCII STL and Three.js displayed its triangles');

  await clickButton('Temperature CSV');
  await chooseFile(csvPath);
  await waitUntil('temperature CSV point cloud', async () => (await evaluate('document.body.innerText')).includes('Temperature point cloud'));
  await delay(900);
  await capture('Temperature_CSV', 'C++ parsed synthetic CSV nodes; point cloud, Celsius range and legend are displayed');

  await clickButton('Thermal Analysis');
  await clickButton('Run thermal analysis');
  await waitUntil('completed thermal result', async () => (await evaluate('document.body.innerText')).includes('Analysis complete'));
  await delay(400);
  await capture('Thermal_FEA', 'C++ solver returned selected mesh profiles and midpoint errors');

  await clickButton('Performance');
  await waitUntil('benchmark hardware metadata', async () => (await evaluate('document.body.innerText')).includes('Hardware detected'));
  await evaluate(`(() => { const select = document.querySelector('.config-fields select'); if (select) { select.value = '3'; select.dispatchEvent(new Event('change', { bubbles: true })); } })()`);
  await clickButton('Start benchmark');
  await waitUntil('completed performance benchmark', async () => (await evaluate('document.body.innerText')).includes('SPEEDUP'));
  await capture('Performance', 'C++ serial and worker-pool benchmark returned measured timing and efficiency');

  await clickButton('Hardware');
  await waitUntil('three simulated hardware devices', async () => (await evaluate('document.body.innerText')).includes('SIMULATION ONLY'));
  await capture('Hardware_Mocks', 'Three device adapters and simulated telemetry displayed');
  await evaluate(`(() => { const box = document.querySelector('.failure-toggle input'); if (!box) return false; box.click(); return true; })()`);
  await clickButton('Connect with failure');
  await waitUntil('recoverable simulated timeout', async () => (await evaluate('document.body.innerText')).includes('Recovery:'));
  await capture('Hardware_Timeout', 'Injected timeout shows error and recovery action');
  await clickButton('Retry connection');
  await waitUntil('successful mock reconnection', async () => (await evaluate('document.body.innerText')).includes('CONNECTED'));
  await capture('Hardware_Reconnected', 'Retry recovered the simulated adapter connection');

  await clickButton('Backpropagation');
  await waitUntil('backpropagation result chart', async () => (await evaluate('document.body.innerText')).includes('FINITE-DIFFERENCE CHECK'));
  await capture('Backpropagation', 'C++ forward pass, gradients, finite-difference check, updated parameters and loss chart');

  log(`Backend: ${baseUrl} (loopback only)`);
  log('Screenshots are stored relative to the repository in docs/demo/screenshots/');
  log('This browser walkthrough exercises the packaged backend and frontend bundle. The separate Launch.exe startup path is verified independently.');
  await writeFile(join(root, 'docs/demo/WALKTHROUGH.md'), `${walkthroughText()}\n`, 'utf8');
  console.log(`Captured ${sequence} screenshots under ${outDir}`);
} catch (error) {
  log(`RUN FAILED: ${error instanceof Error ? error.stack : String(error)}`);
  await writeFile(join(root, 'docs/demo/WALKTHROUGH.md'), `${walkthroughText()}\n`, 'utf8');
  throw error;
} finally {
  try { socket?.close(); } catch {}
  if (browser?.pid) {
    const { spawnSync } = await import('node:child_process');
    spawnSync('taskkill.exe', ['/PID', String(browser.pid), '/T', '/F'], { windowsHide: true, stdio: 'ignore' });
  }
  if (server?.pid) server.kill();
}
