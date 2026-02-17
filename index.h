#ifndef INDEX_H
#define INDEX_H

const char HTML_HEAD[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Smart Class Health</title>
  <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;600;800&display=swap" rel="stylesheet">
  <style>
    :root { --bg: #0f172a; --card: #1e293b; --primary: #6366f1; --text: #f1f5f9; --ok: #10b981; --warn: #f59e0b; --danger: #ef4444; }
    body { font-family: 'Inter', sans-serif; background: var(--bg); color: var(--text); margin: 0; padding: 20px; }
    .container { max-width: 800px; margin: 0 auto; }
    header { text-align: center; margin-bottom: 20px; }
    h1 { margin: 0; font-size: 1.8rem; background: linear-gradient(to right, #818cf8, #c084fc); -webkit-background-clip: text; -webkit-text-fill-color: transparent; }
    .status-dot { height: 10px; width: 10px; background-color: var(--ok); border-radius: 50%; display: inline-block; margin-right: 5px; }

    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(140px, 1fr)); gap: 15px; }
    .card { background: var(--card); padding: 15px; border-radius: 16px; text-align: center; box-shadow: 0 4px 6px -1px rgba(0,0,0,0.1); border: 1px solid rgba(255,255,255,0.05); }
    .card h3 { margin: 0 0 5px 0; font-size: 0.8rem; color: #94a3b8; text-transform: uppercase; letter-spacing: 1px; }
    .val { font-size: 1.5rem; font-weight: 800; }
    .unit { font-size: 0.8rem; color: #64748b; font-weight: 400; }
    .ref { font-size: 0.7rem; color: #94a3b8; margin-top:5px; display:block; text-decoration: none; border-top: 1px solid #334155; padding-top: 5px; }
    .ref:hover { text-decoration: underline; color: var(--primary); }

    .controls { margin-top: 20px; display: grid; gap: 10px; }
    .btn-group { display: flex; gap: 10px; flex-wrap: wrap; }
    button { flex: 1; padding: 12px; border: none; border-radius: 10px; font-weight: 600; cursor: pointer; color: white; transition: 0.2s; min-width: 100px; }
    .btn-on { background: var(--card); border: 1px solid var(--primary); }
    .btn-on:hover { background: var(--primary); }
    .btn-off { background: var(--card); border: 1px solid var(--danger); }
    .btn-off:hover { background: var(--danger); }

    /* PIANO KEYS */
    .piano-keys { display: flex; gap: 4px; justify-content: center; margin-top: 10px; background: #000; padding: 5px; border-radius: 8px; overflow-x: auto; }
    .key { width: 30px; height: 80px; background: white; border-radius: 0 0 4px 4px; border: 1px solid #ccc; cursor: pointer; position: relative; }
    .key:active { background: #eee; transform: scale(0.95); }
    .key-black { width: 20px; height: 50px; background: black; position: absolute; top: 0; left: -10px; z-index: 2; border-radius: 0 0 3px 3px; }
    .key-label { position: absolute; bottom: 5px; left: 0; right: 0; text-align: center; font-size: 10px; color: #333; font-weight: bold; pointer-events: none; }

    input[type=text], input[type=password], select { width: 100%; padding: 10px; margin: 5px 0 15px 0; box-sizing: border-box; border-radius: 8px; border: 1px solid #475569; background: #334155; color: white; }
    .form-card { text-align: left !important; grid-column: span 2; }
    #wifi-list { max-height: 150px; overflow-y: auto; background: #334155; padding: 10px; border-radius: 8px; display: none; margin-bottom: 10px; }
    .wifi-item { padding: 5px; cursor: pointer; border-bottom: 1px solid #475569; }
  </style>
</head>
<body>
)=====";

const char HTML_BODY[] PROGMEM = R"=====(
  <div class="container">
    <header>
      <h1>Smart Class AI Health</h1>
      <div style="font-size:0.8rem; color:#94a3b8; margin-top:5px;">
        <span class="status-dot"></span><span id="ip">...</span> | <span id="time">--:--</span>
      </div>
    </header>

    <div class="grid">
      <!-- SENSORS WITH HEALTH STANDARDS -->
      <div class="card">
        <h3>Suhu (KEMENKES)</h3>
        <div class="val" id="t">--</div><span class="unit">°C</span>
        <a href="https://peraturan.bpk.go.id/Details/258908/permenkes-no-2-tahun-2023" target="_blank" class="ref">Std: 18-30°C<br>(PMK No.2/2023)</a>
      </div>
      <div class="card">
        <h3>Lembab (KEMENKES)</h3>
        <div class="val" id="h">--</div><span class="unit">%</span>
        <a href="https://peraturan.bpk.go.id/Details/258908/permenkes-no-2-tahun-2023" target="_blank" class="ref">Std: 40-60%<br>(PMK No.2/2023)</a>
      </div>
      <div class="card">
        <h3>Kebisingan (KEPMEN LH)</h3>
        <div class="val" id="db">--</div><span class="unit">dB</span>
        <a href="https://peraturan.go.id/id/kepmen-lh-no-48-tahun-1996" target="_blank" class="ref">Max: 55 dB<br>(KepmenLH 48/1996)</a>
      </div>
      <div class="card">
        <h3>Kualitas Udara</h3>
        <div class="val" id="gas">--</div><span class="unit">PPM</span>
        <span class="ref">Std: < 1000 PPM<br>(Ventilasi Sehat)</span>
      </div>

      <!-- NEW SENSOR: SMOKE -->
      <div class="card" style="grid-column: span 2; border-color: var(--danger);">
        <h3>🚭 Detektor Rokok (MQ-2)</h3>
        <div class="val" id="mq2" style="color:#f59e0b;">--</div>
        <div id="smoke_status" style="font-size:0.9rem; font-weight:bold; margin-top:5px;">Scanning...</div>
      </div>

      <div class="card" style="grid-column: span 2;">
        <h3>AI Health Status</h3>
        <div class="val" id="health" style="color: var(--ok); font-size:1.2rem;">Checking...</div>
        <div style="font-size:0.8rem; margin-top:5px;" id="ai_status">AI System Active</div>
      </div>
    </div>

    <div class="controls">
      <!-- SCHOOL BELL & AI -->
      <div class="btn-group">
        <button onclick="cmd('ai_toggle')" style="background:#334155;">🤖 AI: <span id="s_ai">ON</span></button>
        <button onclick="cmd('bell_toggle')" style="background:#334155;">🔔 BEL: <span id="s_bell">ON</span></button>
      </div>

      <!-- MANUAL CONTROLS -->
      <div class="btn-group">
        <button onclick="cmd('fan_toggle')" class="btn-on">KIPAS <span id="s_fan">●</span></button>
        <button onclick="cmd('lamp_toggle')" class="btn-on">LAMPU <span id="s_lamp">●</span></button>
      </div>

      <!-- MUSIC PLAYER -->
      <div class="card" style="text-align:left;">
        <h3>🎵 Music & Bells</h3>
        <select id="song_select">
          <optgroup label="🔔 Bel Sekolah">
            <option value="25">Bel Masuk (07:00)</option>
            <option value="26">Bel Istirahat (10:00)</option>
            <option value="27">Bel Pulang (14:00)</option>
          </optgroup>
          <optgroup label="🎵 Lagu Hiburan">
            <option value="0">1. Super Mario Bros</option>
            <option value="1">2. Zelda: Song of Storms</option>
            <option value="2">3. Star Wars: Imperial</option>
            <option value="3">4. Happy Birthday</option>
            <option value="4">5. Tetris Theme</option>
            <option value="24">25. Rick Roll</option>
          </optgroup>
        </select>
        <div class="btn-group">
          <button onclick="playSong()" class="btn-on" style="background:#8b5cf6;">PLAY ▶</button>
          <button onclick="cmd('music_stop')" class="btn-off">STOP ■</button>
        </div>
      </div>

      <!-- PIANO -->
      <div class="card">
        <h3>🎹 Mini Piano</h3>
        <div class="piano-keys">
          <div class="key" onmousedown="playTone(262)"> <span class="key-label">C</span> </div>
          <div class="key" onmousedown="playTone(294)"> <span class="key-label">D</span> <div class="key-black" style="left:18px;"></div> </div>
          <div class="key" onmousedown="playTone(330)"> <span class="key-label">E</span> <div class="key-black" style="left:18px;"></div> </div>
          <div class="key" onmousedown="playTone(349)"> <span class="key-label">F</span> </div>
          <div class="key" onmousedown="playTone(392)"> <span class="key-label">G</span> <div class="key-black" style="left:18px;"></div> </div>
          <div class="key" onmousedown="playTone(440)"> <span class="key-label">A</span> <div class="key-black" style="left:18px;"></div> </div>
          <div class="key" onmousedown="playTone(494)"> <span class="key-label">B</span> <div class="key-black" style="left:18px;"></div> </div>
          <div class="key" onmousedown="playTone(523)"> <span class="key-label">C'</span> </div>
        </div>
      </div>

      <!-- SETTINGS -->
      <div class="card form-card">
        <h3>⚙️ Settings</h3>
        <form action="/save" method="POST">
          <div style="display:flex; gap:5px;">
             <input type="text" name="ssid" id="ssid" placeholder="WiFi SSID" readonly required>
             <button type="button" onclick="scanWifi()" style="width:80px; background:#6366f1;">SCAN</button>
          </div>
          <div id="wifi-list">Scanning...</div>
          <input type="text" name="ssid_manual" placeholder="Manual SSID">
          <input type="password" name="pass" placeholder="WiFi Password">
          <input type="text" name="bot" placeholder="Telegram Bot Token">
          <input type="text" name="id" placeholder="Chat ID Admin">
          <button type="submit" class="btn-on" style="background:#10b981; width:100%;">SAVE & RESTART</button>
        </form>
      </div>
    </div>
  </div>

  <script>
    function update() {
      fetch('/data?ts=' + new Date().getTime()).then(r => r.json()).then(d => {
        // Update Values
        document.getElementById('t').innerText = d.t.toFixed(1);
        document.getElementById('h').innerText = d.h.toFixed(0);
        document.getElementById('gas').innerText = d.gas;
        document.getElementById('mq2').innerText = d.mq2;
        document.getElementById('db').innerText = d.db.toFixed(1);
        document.getElementById('health').innerText = d.health;
        document.getElementById('ai_status').innerText = d.ai_stat;
        document.getElementById('time').innerText = d.time;
        document.getElementById('ip').innerText = window.location.hostname;

        // Colors & Status
        document.getElementById('s_fan').style.color = d.fan ? '#10b981' : '#64748b';
        document.getElementById('s_lamp').style.color = d.lamp ? '#10b981' : '#64748b';
        document.getElementById('s_ai').innerText = d.ai ? "ON" : "OFF";
        document.getElementById('s_bell').innerText = d.bell ? "ON" : "OFF";

        // Smoke Alert Visual
        var smokeEl = document.getElementById('mq2');
        var smokeStat = document.getElementById('smoke_status');
        if(d.mq2 > 2000) {
            smokeEl.style.color = '#ef4444';
            smokeStat.innerText = "BAHAYA: ADA ASAP!";
            smokeStat.style.color = '#ef4444';
        } else {
            smokeEl.style.color = '#f59e0b';
            smokeStat.innerText = "Udara Bersih";
            smokeStat.style.color = '#10b981';
        }
      });
    }

    function playSong() {
      var id = document.getElementById('song_select').value;
      cmd('music_play&id=' + id);
    }

    function playTone(freq) { fetch('/cmd?do=tone&freq=' + freq); }

    function scanWifi() {
       var list = document.getElementById('wifi-list');
       list.style.display = 'block';
       list.innerHTML = 'Scanning...';
       fetch('/scan').then(r => r.json()).then(data => {
          if(data.status === "scanning") { setTimeout(scanWifi, 1000); return; }
          list.innerHTML = '';
          data.forEach(ssid => {
             var div = document.createElement('div');
             div.className = 'wifi-item';
             div.innerText = ssid;
             div.onclick = function() { document.getElementById('ssid').value = ssid; list.style.display='none'; };
             list.appendChild(div);
          });
       }).catch(e => { list.innerHTML = 'Error'; });
    }

    function cmd(act) { fetch('/cmd?do='+act).then(update); }
    setInterval(update, 2000); update();
  </script>
</body>
</html>
)=====";

#endif
