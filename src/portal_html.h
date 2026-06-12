#pragma once
#include <pgmspace.h>

// ============================================================================
// Web Dashboard HTML
// ============================================================================

const char PORTAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>NeoKey Commander</title>
<style>
  @import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600&display=swap');

  :root {
    --bg-base: #09090b;
    --bg-surface: #18181b;
    --bg-surface-hover: #27272a;
    --border: #27272a;
    --text-main: #fafafa;
    --text-muted: #a1a1aa;
    --primary: #38bdf8;
    --primary-hover: #7dd3fc;
    --success: #34d399;
    --danger: #f87171;
    --sidebar-w: 240px;
  }

  * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Inter', sans-serif; }
  
  body {
    background: var(--bg-base);
    color: var(--text-main);
    display: flex;
    height: 100vh;
    overflow: hidden;
  }

  /* Sidebar */
  aside {
    width: var(--sidebar-w);
    background: var(--bg-surface);
    border-right: 1px solid var(--border);
    display: flex;
    flex-direction: column;
    padding: 1.5rem 1rem;
    z-index: 10;
  }
  .brand {
    font-size: 1.25rem;
    font-weight: 600;
    margin-bottom: 2rem;
    padding: 0 0.5rem;
    display: flex;
    align-items: center;
    gap: 0.5rem;
  }
  .brand .dot {
    width: 8px; height: 8px; border-radius: 50%; background: var(--primary);
  }
  .nav-item {
    padding: 0.75rem 1rem;
    border-radius: 8px;
    color: var(--text-muted);
    cursor: pointer;
    margin-bottom: 0.25rem;
    font-weight: 500;
    transition: all 0.2s;
    display: flex;
    align-items: center;
    gap: 0.75rem;
  }
  .nav-item:hover { background: var(--bg-surface-hover); color: var(--text-main); }
  .nav-item.active { background: rgba(56, 189, 248, 0.1); color: var(--primary); }
  
  .spacer { flex-grow: 1; }
  .save-btn {
    background: var(--primary);
    color: #000;
    border: none;
    padding: 0.75rem;
    border-radius: 8px;
    font-weight: 600;
    cursor: pointer;
    transition: 0.2s;
    text-align: center;
  }
  .save-btn:hover { background: var(--primary-hover); }

  /* Main Content */
  main {
    flex-grow: 1;
    overflow-y: auto;
    padding: 2rem;
    position: relative;
  }
  .tab-content { display: none; max-width: 900px; margin: 0 auto; animation: fade 0.3s ease; }
  .tab-content.active { display: block; }
  
  @keyframes fade { from { opacity: 0; transform: translateY(10px); } to { opacity: 1; transform: translateY(0); } }

  h2 { font-size: 1.5rem; margin-bottom: 1.5rem; font-weight: 600; }
  h3 { font-size: 1rem; margin-bottom: 1rem; color: var(--text-muted); font-weight: 500; }

  /* Global Header */
  .global-header {
    display: flex; justify-content: space-between; align-items: center;
    background: var(--bg-surface); padding: 1rem 2rem; border-bottom: 1px solid var(--border);
    margin: -2rem -2rem 2rem -2rem; /* offsets the main padding */
    position: sticky; top: -2rem; z-index: 20;
  }
  .header-title { font-weight: 600; color: var(--text-muted); font-size: 0.9rem; text-transform: uppercase; letter-spacing: 0.05em; }
  
  .monitor-macropad { display: flex; gap: 0.5rem; }
  .monitor-key {
    width: 36px; height: 36px;
    background: #27272a; border-radius: 8px;
    border: 1px solid #3f3f46;
    display: flex; align-items: center; justify-content: center;
    font-size: 0.85rem; font-weight: 600; cursor: pointer;
    box-shadow: 0 4px 0 #18181b;
    transition: 0.1s;
    color: #fff;
    text-shadow: 0 1px 2px rgba(0,0,0,0.8);
  }
  .monitor-key:active { transform: translateY(2px); box-shadow: 0 2px 0 #18181b; }
  .monitor-key.active-preset { border-color: var(--primary); box-shadow: 0 4px 0 rgba(56,189,248,0.5); }

  /* Grid / Cards */
  .grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
    gap: 1.5rem;
    margin-bottom: 2rem;
  }
  .card {
    background: var(--bg-surface);
    border: 1px solid var(--border);
    border-radius: 12px;
    padding: 1.5rem;
  }
  .stat-row { display: flex; justify-content: space-between; margin-bottom: 0.75rem; font-size: 0.9rem; }
  .stat-val { font-weight: 500; }
  .status-dot { display: inline-block; width: 8px; height: 8px; border-radius: 50%; background: var(--success); margin-right: 6px; }
  .status-dot.offline { background: var(--danger); }

  /* Forms & Inputs */
  .form-group { margin-bottom: 1rem; }
  label { display: block; font-size: 0.85rem; color: var(--text-muted); margin-bottom: 0.4rem; }
  input[type="text"], input[type="password"], input[type="number"] {
    width: 100%; padding: 0.75rem;
    background: var(--bg-base); border: 1px solid var(--border);
    color: var(--text-main); border-radius: 6px; outline: none;
    transition: 0.2s;
  }
  input:focus { border-color: var(--primary); }
  
  .inline-btn {
    background: var(--bg-surface-hover); border: 1px solid var(--border);
    color: var(--text-main); padding: 0.5rem 1rem; border-radius: 6px;
    cursor: pointer; font-size: 0.85rem; transition: 0.2s;
  }
  .inline-btn:hover { background: var(--border); }
  
  /* Visual Key Mapper */
  .macropad {
    display: flex; gap: 1rem; justify-content: center; flex-wrap: wrap;
    background: var(--bg-base); padding: 2rem; border-radius: 12px; border: 1px solid var(--border);
  }
  .key {
    width: 64px; height: 64px;
    background: #27272a; border-radius: 12px;
    border: 2px solid #3f3f46;
    display: flex; align-items: center; justify-content: center;
    font-size: 1.25rem; font-weight: 600; cursor: pointer;
    box-shadow: 0 8px 0 #18181b;
    transition: 0.1s;
    position: relative;
  }
  .key:active { transform: translateY(4px); box-shadow: 0 4px 0 #18181b; }
  .key.selected { border-color: var(--primary); box-shadow: 0 8px 0 rgba(56,189,248,0.5); }
  .key-badge {
    position: absolute; top: -8px; right: -8px;
    background: var(--primary); color: #000; font-size: 0.7rem;
    padding: 2px 6px; border-radius: 8px; font-weight: 600;
  }

  /* Sliders & Radio */
  input[type="range"] {
    -webkit-appearance: none; width: 100%; height: 6px; background: var(--border);
    border-radius: 3px; outline: none; margin: 0.5rem 0;
  }
  input[type="range"]::-webkit-slider-thumb {
    -webkit-appearance: none; width: 16px; height: 16px; border-radius: 50%;
    background: var(--text-main); cursor: pointer;
  }
  .radio-group { display: flex; gap: 1.5rem; margin-top: 0.5rem; flex-wrap: wrap; }
  .radio-group label { display: flex; align-items: center; gap: 0.5rem; color: var(--text-main); font-size: 0.9rem; cursor: pointer; }

  /* Mobile */
  @media (max-width: 768px) {
    body { flex-direction: column; }
    aside { width: 100%; border-right: none; border-bottom: 1px solid var(--border); flex-direction: row; align-items: center; padding: 0.75rem; overflow-x: auto; }
    .brand { margin: 0 1rem 0 0; font-size: 1rem; }
    .nav-item { margin: 0 0.5rem 0 0; padding: 0.5rem; white-space: nowrap; font-size: 0.9rem; }
    .spacer { display: none; }
    .save-btn { margin-left: auto; padding: 0.5rem 1rem; font-size: 0.9rem; }
    main { padding: 1rem; }
  }

  /* Toast & Scan results */
  .toast { position: fixed; bottom: 2rem; left: 50%; transform: translateX(-50%); background: var(--success); color: #000; padding: 0.75rem 1.5rem; border-radius: 8px; font-weight: 500; opacity: 0; pointer-events: none; transition: 0.3s; z-index: 50; }
  .toast.show { opacity: 1; }
  .toast.error { background: var(--danger); }
  
  .scan-list { margin-top: 1rem; display: flex; flex-direction: column; gap: 0.5rem; }
  .scan-item { display: flex; justify-content: space-between; align-items: center; padding: 0.75rem; background: var(--bg-base); border: 1px solid var(--border); border-radius: 6px; }
  
  /* OTA Banner */
  .ota-banner {
    display: none;
    background: rgba(56, 189, 248, 0.15);
    border: 1px solid var(--primary);
    color: var(--primary);
    padding: 1rem;
    border-radius: 8px;
    margin-bottom: 1.5rem;
    text-align: center;
    font-weight: 500;
    font-size: 0.95rem;
    cursor: pointer;
    transition: 0.2s;
  }
  .ota-banner:hover { background: rgba(56, 189, 248, 0.25); }
  
  /* OTA Progress */
  #ota-progress-container { display: none; margin-top: 1rem; }
  #ota-bar { height: 8px; background: var(--border); border-radius: 4px; overflow: hidden; }
  #ota-fill { height: 100%; background: var(--primary); width: 0%; transition: width 0.2s; }
</style>
</head>
<body>

<aside>
  <div class="brand"><div class="dot"></div> Commander</div>
  <div class="nav-item active" onclick="nav('dash')">Dashboard</div>
  <div class="nav-item" onclick="nav('mapper')">Key Mapper</div>
  <div class="nav-item" onclick="nav('network')">Network</div>
  <div class="nav-item" onclick="nav('system')">System</div>
  <div class="spacer"></div>
  <button class="save-btn" onclick="saveConfig()">Save All</button>
</aside>

<main>
  <!-- Global Header with Virtual Key Monitor -->
  <header class="global-header">
    <div class="header-title">Virtual Macropad</div>
    <div class="monitor-macropad" id="monitor-macropad">
      <div class="monitor-key" onclick="pressKey(0)" id="mkey0">0</div>
      <div class="monitor-key" onclick="pressKey(1)" id="mkey1">1</div>
      <div class="monitor-key" onclick="pressKey(2)" id="mkey2">2</div>
      <div class="monitor-key" onclick="pressKey(3)" id="mkey3">3</div>
    </div>
  </header>

  <!-- DASHBOARD -->
  <div id="tab-dash" class="tab-content active">
    <div class="ota-banner" id="ota-banner" onclick="nav('system')">
      ✨ A new firmware update is available! Long press Key 0 on the device to install, or click here to flash it manually.
    </div>
    <h2>Dashboard</h2>
    <div class="grid">
      <div class="card">
        <h3>System Status</h3>
        <div class="stat-row"><span>Wi-Fi</span> <span class="stat-val" id="st-wifi">...</span></div>
        <div class="stat-row"><span>IP Address</span> <span class="stat-val" id="st-ip">...</span></div>
        <div class="stat-row"><span>RSSI</span> <span class="stat-val" id="st-rssi">...</span></div>
        <div class="stat-row"><span>I2C NeoKey</span> <span class="stat-val" id="st-i2c">...</span></div>
        <div class="stat-row"><span>Free RAM</span> <span class="stat-val" id="st-ram">...</span></div>
        <div class="stat-row"><span>Uptime</span> <span class="stat-val" id="st-up">...</span></div>
        <div class="stat-row"><span>Firmware</span> <span class="stat-val" id="st-fw">...</span></div>
      </div>
      <div class="card">
        <h3>WLED Targets</h3>
        <div id="dash-wleds" style="font-size:0.9rem; color:var(--text-muted); line-height:1.6;">None configured.</div>
      </div>
    </div>
  </div>

  <!-- KEY MAPPER -->
  <div id="tab-mapper" class="tab-content">
    <h2>Visual Key Mapper</h2>
    <p style="color:var(--text-muted); margin-bottom: 1.5rem;">Click a physical key below to assign its WLED preset ID.</p>
    
    <div style="margin-bottom: 1.5rem; background: var(--bg-surface-hover); padding: 1rem; border-radius: 8px;">
      <label style="display:flex; align-items:center; gap:0.5rem; cursor:pointer;">
        <input type="checkbox" id="autoMap" onchange="updateAutoMap()">
        <strong>Automatically assign the 4 lowest WLED presets</strong>
      </label>
      <p style="color:var(--text-muted); font-size:0.85rem; margin-top:0.5rem; margin-left:1.5rem;">If unchecked, you can manually override the preset IDs. Custom labels are always supported.</p>
    </div>

    <div class="macropad">
      <div class="key" onclick="selectKey(0)">0<div class="key-badge" id="badge0">P1</div></div>
      <div class="key" onclick="selectKey(1)">1<div class="key-badge" id="badge1">P2</div></div>
      <div class="key" onclick="selectKey(2)">2<div class="key-badge" id="badge2">P3</div></div>
      <div class="key" onclick="selectKey(3)">3<div class="key-badge" id="badge3">P4</div></div>
    </div>
    
    <div class="card" style="margin-top:2rem; display:none;" id="key-editor">
      <h3 id="edit-title">Edit Key 0</h3>
      <div class="form-group">
        <label>WLED Preset ID</label>
        <input type="number" id="edit-preset" min="1" max="250" oninput="updatePreset()">
      </div>
      <div class="form-group" style="margin-top:1rem;">
        <label>Custom Label (Optional)</label>
        <input type="text" id="edit-label" maxlength="12" placeholder="e.g. Sunset" oninput="updatePreset()">
      </div>
    </div>
  </div>

  <!-- NETWORK -->
  <div id="tab-network" class="tab-content">
    <h2>Network Configuration</h2>
    <div class="grid">
      <div class="card">
        <h3>Wi-Fi Networks</h3>
        <div id="wifi-list"></div>
        <button class="inline-btn" style="margin-top:1rem;" onclick="addWifi()">+ Add Network</button>
      </div>
      
      <div class="card">
        <h3>WLED Devices</h3>
        <div id="wled-list"></div>
        <button class="inline-btn" style="margin-top:1rem; margin-bottom:1rem;" onclick="addWled()">+ Add IP Manually</button>
        
        <hr style="border:0; border-top:1px solid var(--border); margin:1rem 0;">
        <h3>Auto-Discovery</h3>
        <button class="inline-btn" onclick="scanWled()">Scan Local Network</button>
        <div class="scan-list" id="scan-results"></div>
      </div>
    </div>
  </div>

  <!-- SYSTEM -->
  <div id="tab-system" class="tab-content">
    <h2>System Settings</h2>
    <div class="grid">
      <div class="card">
        <h3>Hardware Configuration</h3>
        <div class="form-group">
          <label>Active Key Brightness (<span id="actBriVal">191</span>)</label>
          <input type="range" id="actBri" min="0" max="255" oninput="document.getElementById('actBriVal').innerText=this.value">
        </div>
        <div class="form-group">
          <label>Inactive Key Brightness (<span id="inaBriVal">64</span>)</label>
          <input type="range" id="inaBri" min="0" max="255" oninput="document.getElementById('inaBriVal').innerText=this.value">
        </div>
        <div class="form-group" style="margin-top:1.5rem;">
          <label>LED Feedback Style</label>
          <div class="radio-group">
            <label><input type="radio" name="fbStyle" value="0"> Solid</label>
            <label><input type="radio" name="fbStyle" value="1"> Fade</label>
            <label><input type="radio" name="fbStyle" value="2"> Flash</label>
          </div>
        </div>
      </div>
      
      <div class="card">
        <h3>OTA Firmware Update</h3>
        <p style="color:var(--text-muted); font-size:0.85rem; margin-bottom:1rem;">Upload a compiled .bin file to flash the ESP32.</p>
        <input type="file" id="ota-file" accept=".bin" style="margin-bottom:1rem; font-size:0.85rem; color:var(--text-muted);">
        <button class="inline-btn" onclick="startOta()">Flash Firmware</button>
        <div id="ota-progress-container">
          <div id="ota-bar"><div id="ota-fill"></div></div>
          <div style="font-size:0.8rem; text-align:center; margin-top:0.5rem;" id="ota-text">0%</div>
        </div>
      </div>
    </div>
  </div>

</main>

<div class="toast" id="toast">Settings Saved!</div>

<script>
// --- State ---
let config = { ssids:[], passwords:[], wleds:[], presets:[1,2,3,4], labels:['','','',''], autoMap:true, actBri:191, inaBri:26, fbStyle:1 };
let selectedKey = -1;

// --- Navigation ---
function nav(tabId) {
  document.querySelectorAll('.nav-item').forEach(el => el.classList.remove('active'));
  document.querySelectorAll('.tab-content').forEach(el => el.classList.remove('active'));
  event.currentTarget.classList.add('active');
  document.getElementById('tab-' + tabId).classList.add('active');
}

function showToast(msg, isErr=false) {
  const t = document.getElementById('toast');
  t.innerText = msg;
  t.className = 'toast show' + (isErr?' error':'');
  setTimeout(() => t.classList.remove('show'), 3000);
}

// --- Data Fetch ---
async function fetchStatus() {
  try {
    let r = await fetch('/status');
    let d = await r.json();
    document.getElementById('st-wifi').innerHTML = d.connected ? `<span class="status-dot"></span>${d.ssid}` : `<span class="status-dot offline"></span>Disconnected`;
    document.getElementById('st-ip').innerText = d.ip;
    document.getElementById('st-rssi').innerText = d.rssi + ' dBm';
    document.getElementById('st-i2c').innerHTML = d.i2c_ok ? `<span style="color:var(--success)">OK</span>` : `<span style="color:var(--danger)">Error</span>`;
    document.getElementById('st-ram').innerText = (d.heap/1024).toFixed(1) + ' KB';
    document.getElementById('st-up').innerText = d.uptime + ' s';
    document.getElementById('st-fw').innerText = 'v' + d.fw;
    
    // OTA banner
    if (d.ota) {
      document.getElementById('ota-banner').style.display = 'block';
    } else {
      document.getElementById('ota-banner').style.display = 'none';
    }
    
    // Update monitor keys
    if(d.keyColors && config.presets) {
      for(let i=0; i<4; i++) {
        let mk = document.getElementById('mkey'+i);
        // hex color
        let hex = '#' + ('000000' + d.keyColors[i].toString(16)).slice(-6);
        // Only override background if it has a color, else fallback to dark grey
        mk.style.backgroundColor = d.keyColors[i] === 0 ? '#27272a' : hex;
        
        // Active highlight
        if(d.activePreset === config.presets[i]) {
          mk.classList.add('active-preset');
        } else {
          mk.classList.remove('active-preset');
        }
      }
    }
  } catch(e){}
}

async function fetchConfig() {
  try {
    let r = await fetch('/config_data');
    let d = await r.json();
    config = Object.assign(config, d);
    renderNetwork();
    renderSystem();
    renderMapper();
  } catch(e){}
}

// --- Renderers ---
function renderNetwork() {
  const wl = document.getElementById('wifi-list');
  wl.innerHTML = '';
  config.ssids.forEach((s, i) => {
    wl.innerHTML += `<div class="form-group">
      <input type="text" id="s_${i}" value="${s}" placeholder="SSID" style="margin-bottom:0.25rem;">
      <input type="password" id="p_${i}" placeholder="Password (leave blank to keep)">
    </div>`;
  });
  
  const dl = document.getElementById('wled-list');
  dl.innerHTML = '';
  const dld = document.getElementById('dash-wleds');
  dld.innerHTML = '';
  config.wleds.forEach((w, i) => {
    dl.innerHTML += `<div class="form-group" style="display:flex;gap:0.5rem;">
      <input type="text" id="w_${i}" value="${w}" placeholder="IP or mDNS">
      <button class="inline-btn" onclick="delWled(${i})">X</button>
    </div>`;
    dld.innerHTML += `<div>&#8226; ${w}</div>`;
  });
  if(config.wleds.length===0) dld.innerHTML = 'None configured.';
}

function renderSystem() {
  document.getElementById('actBri').value = config.actBri;
  document.getElementById('actBriVal').innerText = config.actBri;
  document.getElementById('inaBri').value = config.inaBri;
  document.getElementById('inaBriVal').innerText = config.inaBri;
  let rb = document.querySelector(`input[name="fbStyle"][value="${config.fbStyle}"]`);
  if(rb) rb.checked = true;
}

function renderMapper() {
  document.getElementById('autoMap').checked = config.autoMap;
  for(let i=0; i<4; i++) {
    let lbl = config.labels[i] ? config.labels[i] : 'P' + config.presets[i];
    document.getElementById('badge'+i).innerText = lbl;
  }
}

// --- Interactions ---
function selectKey(k) {
  selectedKey = k;
  document.querySelectorAll('.key').forEach(el=>el.classList.remove('selected'));
  event.currentTarget.classList.add('selected');
  document.getElementById('key-editor').style.display = 'block';
  document.getElementById('edit-title').innerText = 'Edit Key ' + k;
  document.getElementById('edit-preset').value = config.presets[k];
  document.getElementById('edit-label').value = config.labels[k] || '';
  document.getElementById('edit-preset').disabled = config.autoMap;
}

function updatePreset() {
  if(selectedKey < 0) return;
  config.presets[selectedKey] = parseInt(document.getElementById('edit-preset').value) || 1;
  config.labels[selectedKey] = document.getElementById('edit-label').value;
  renderMapper();
}

function updateAutoMap() {
  config.autoMap = document.getElementById('autoMap').checked;
  document.getElementById('edit-preset').disabled = config.autoMap;
}

function addWifi() { config.ssids.push(''); config.passwords.push(''); renderNetwork(); }
function addWled() { config.wleds.push(''); renderNetwork(); }
function delWled(i) { config.wleds.splice(i,1); renderNetwork(); }

async function pressKey(k) {
  let mk = document.getElementById('mkey'+k);
  mk.style.transform = 'translateY(2px)';
  mk.style.boxShadow = '0 2px 0 #18181b';
  setTimeout(() => {
    mk.style.transform = '';
    mk.style.boxShadow = '';
  }, 100);
  
  try {
    await fetch('/api/press?id=' + k);
    setTimeout(fetchStatus, 300);
  } catch(e) {}
}

async function scanWled() {
  const res = document.getElementById('scan-results');
  res.innerHTML = '<div style="color:var(--text-muted);font-size:0.85rem;">Scanning...</div>';
  try {
    let r = await fetch('/scan_wled');
    let ds = await r.json();
    res.innerHTML = '';
    if(ds.length===0) res.innerHTML = '<div style="color:var(--text-muted);font-size:0.85rem;">No devices found.</div>';
    ds.forEach(d => {
      res.innerHTML += `<div class="scan-item">
        <div><strong>${d.name}</strong><br><small style="color:var(--text-muted)">${d.ip}</small></div>
        <button class="inline-btn" onclick="config.wleds.push('${d.ip}');renderNetwork();">Add</button>
      </div>`;
    });
  } catch(e){ res.innerHTML = '<div style="color:var(--danger);font-size:0.85rem;">Scan failed.</div>'; }
}

// --- Save & OTA ---
async function saveConfig() {
  let fd = new URLSearchParams();
  for(let i=0; i<config.ssids.length; i++) {
    let s = document.getElementById('s_'+i);
    let p = document.getElementById('p_'+i);
    if(s && s.value) { fd.append('ssid'+i, s.value); if(p && p.value) fd.append('pass'+i, p.value); }
  }
  for(let i=0; i<config.wleds.length; i++) {
    let w = document.getElementById('w_'+i);
    if(w && w.value) fd.append('addr'+i, w.value);
  }
  fd.append('actBri', document.getElementById('actBri').value);
  fd.append('inaBri', document.getElementById('inaBri').value);
  let fb = document.querySelector('input[name="fbStyle"]:checked');
  if(fb) fd.append('fbStyle', fb.value);
  
  fd.append('autoMap', config.autoMap);
  for(let i=0; i<4; i++) {
    fd.append('preset'+i, config.presets[i]);
    fd.append('lbl'+i, config.labels[i] || '');
  }
  
  try {
    let r = await fetch('/save', { method:'POST', body:fd });
    if(r.ok) showToast('Settings Saved!');
    else showToast('Save failed!', true);
    fetchConfig();
  } catch(e) { showToast('Network error!', true); }
}

function startOta() {
  const file = document.getElementById('ota-file').files[0];
  if(!file) return alert('Select a .bin file first!');
  
  const fd = new FormData();
  fd.append('update', file);
  
  const xhr = new XMLHttpRequest();
  xhr.open('POST', '/update', true);
  
  document.getElementById('ota-progress-container').style.display = 'block';
  const fill = document.getElementById('ota-fill');
  const txt = document.getElementById('ota-text');
  
  xhr.upload.onprogress = function(e) {
    if(e.lengthComputable) {
      let pct = (e.loaded / e.total) * 100;
      fill.style.width = pct + '%';
      txt.innerText = pct.toFixed(0) + '%';
    }
  };
  xhr.onload = function() {
    if(xhr.status === 200) { showToast('Update success! Rebooting...'); setTimeout(()=>location.reload(), 5000); }
    else { showToast('Update failed!', true); }
  };
  xhr.send(fd);
}

// Init
setInterval(fetchStatus, 3000);
fetchStatus();
fetchConfig();
</script>
</body>
</html>
)rawliteral";

const char PORTAL_SUCCESS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>NeoKey Commander</title>
<style>
  @import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600&display=swap');
  body {
    font-family: 'Inter', sans-serif;
    background: #09090b;
    color: #fafafa;
    display: flex;
    align-items: center;
    justify-content: center;
    min-height: 100vh;
    text-align: center;
    margin: 0;
  }
  .msg {
    background: #18181b;
    border: 1px solid #27272a;
    border-radius: 12px;
    padding: 3.5rem 4.5rem;
    box-shadow: 0 4px 24px rgba(0,0,0,0.5);
  }
  h1 { font-size: 1.5rem; margin-bottom: 0.75rem; }
  p { color: #a1a1aa; font-size: 0.95rem; }
</style>
</head>
<body>
  <div class="msg">
    <h1>Configuration Saved</h1>
    <p>The device is rebooting to apply changes.</p>
  </div>
</body>
</html>
)rawliteral";
