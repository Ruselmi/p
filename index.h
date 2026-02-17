#ifndef INDEX_H
#define INDEX_H

const char HTML_HEAD[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Smart Class IoT</title>
  <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;600;800&display=swap" rel="stylesheet">
  <style>
    :root { --bg: #0f172a; --card: #1e293b; --primary: #6366f1; --text: #f1f5f9; --ok: #10b981; --warn: #f59e0b; --danger: #ef4444; }
    body { font-family: 'Inter', sans-serif; background: var(--bg); color: var(--text); margin: 0; padding: 20px; }
    .container { max-width: 800px; margin: 0 auto; }
    header { text-align: center; margin-bottom: 30px; }
    h1 { margin: 0; font-size: 1.8rem; background: linear-gradient(to right, #818cf8, #c084fc); -webkit-background-clip: text; -webkit-text-fill-color: transparent; }
    .status-dot { height: 10px; width: 10px; background-color: var(--ok); border-radius: 50%; display: inline-block; margin-right: 5px; }

    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(140px, 1fr)); gap: 15px; }
    .card { background: var(--card); padding: 20px; border-radius: 16px; text-align: center; box-shadow: 0 4px 6px -1px rgba(0,0,0,0.1); border: 1px solid rgba(255,255,255,0.05); }
    .card h3 { margin: 0 0 10px 0; font-size: 0.85rem; color: #94a3b8; text-transform: uppercase; letter-spacing: 1px; }
    .val { font-size: 1.8rem; font-weight: 800; }
    .unit { font-size: 0.9rem; color: #64748b; font-weight: 400; }

    .controls { margin-top: 20px; display: grid; gap: 10px; }
    .btn-group { display: flex; gap: 10px; }
    button { flex: 1; padding: 15px; border: none; border-radius: 12px; font-weight: 600; cursor: pointer; color: white; transition: 0.2s; }
    .btn-on { background: var(--card); border: 1px solid var(--primary); }
    .btn-on:hover { background: var(--primary); }
    .btn-off { background: var(--card); border: 1px solid var(--danger); }
    .btn-off:hover { background: var(--danger); }
    .btn-music { background: linear-gradient(135deg, #ec4899, #8b5cf6); border: none; }

    input[type=text], input[type=password] { width: 100%; padding: 10px; margin: 5px 0 15px 0; box-sizing: border-box; border-radius: 8px; border: 1px solid #475569; background: #334155; color: white; }
    .form-card { text-align: left !important; grid-column: span 2; }

    .footer { margin-top: 30px; text-align: center; font-size: 0.8rem; color: #475569; }
    #wifi-list { max-height: 150px; overflow-y: auto; background: #334155; padding: 10px; border-radius: 8px; display: none; margin-bottom: 10px; }
    .wifi-item { padding: 5px; cursor: pointer; border-bottom: 1px solid #475569; }
    .wifi-item:hover { background: #475569; }
  </style>
</head>
<body>
)=====";

const char HTML_BODY[] PROGMEM = R"=====(
  <div class="container">
    <header>
      <h1>Smart Class Panel</h1>
      <div style="margin-top:5px; color:#94a3b8; font-size:0.9rem;">
        <span class="status-dot"></span>Online | <span id="ip">Loading...</span> | <span id="time">--:--</span>
      </div>
    </header>

    <div class="grid">
      <div class="card">
        <h3>Suhu</h3>
        <div class="val" id="t">--</div><span class="unit">°C</span>
      </div>
      <div class="card">
        <h3>Kelembaban</h3>
        <div class="val" id="h">--</div><span class="unit">%</span>
      </div>
      <div class="card">
        <h3>Udara</h3>
        <div class="val" id="gas">--</div><span class="unit">PPM</span>
      </div>
      <div class="card">
        <h3>Kebisingan</h3>
        <div class="val" id="db">--</div><span class="unit">dB</span>
      </div>
      <div class="card" style="grid-column: span 2;">
        <h3>Mood Kelas</h3>
        <div class="val" id="mood" style="color: var(--primary);">Loading...</div>
      </div>

      <!-- FORM WIFI & TELEGRAM -->
      <div class="card form-card">
        <h3>⚙️ Pengaturan Koneksi</h3>
        <form action="/save" method="POST">
          <label>WiFi SSID:</label>
          <div style="display:flex; gap:5px;">
             <input type="text" name="ssid" id="ssid" placeholder="Pilih WiFi..." readonly required>
             <button type="button" onclick="scanWifi()" style="width: 80px; padding: 10px; background: #6366f1;">SCAN</button>
          </div>
          <div id="wifi-list">Scanning...</div>
          <input type="text" name="ssid_manual" placeholder="Atau ketik manual SSID..." style="margin-top:5px;">

          <label>WiFi Password:</label>
          <input type="password" name="pass" placeholder="Password WiFi">

          <label>Bot Token:</label>
          <input type="text" name="bot" placeholder="Telegram Bot Token">

          <label>Chat ID Admin:</label>
          <input type="text" name="id" placeholder="ID Admin Telegram">

          <button type="submit" class="btn-on" style="background:#10b981; width:100%;">SIMPAN & RESTART ALAT</button>
        </form>
      </div>
    </div>

    <div class="controls">
      <div class="btn-group">
        <button onclick="cmd('fan_toggle')" class="btn-on">KIPAS <span id="s_fan">●</span></button>
        <button onclick="cmd('lamp_toggle')" class="btn-on">LAMPU <span id="s_lamp">●</span></button>
      </div>
      <button onclick="cmd('auto_toggle')" style="background:#334155;">MODE OTOMATIS: <span id="s_auto">ON</span></button>
      <div class="btn-group">
        <button onclick="cmd('music_play')" class="btn-music">♫ PLAY ZELDA</button>
        <button onclick="cmd('music_stop')" class="btn-off">■ STOP</button>
      </div>
    </div>

    <div class="footer">
      <a href="/csv" style="color:var(--primary); text-decoration:none;">📥 Download Laporan Excel (.csv)</a>
    </div>
  </div>

  <script>
    function update() {
      fetch('/data?ts=' + new Date().getTime()).then(r => r.json()).then(d => {
        document.getElementById('t').innerText = d.t.toFixed(1);
        document.getElementById('h').innerText = d.h.toFixed(0);
        document.getElementById('gas').innerText = d.gas;
        document.getElementById('db').innerText = d.db.toFixed(1);
        document.getElementById('mood').innerText = d.mood;
        document.getElementById('time').innerText = d.time;
        document.getElementById('ip').innerText = window.location.hostname;

        document.getElementById('s_fan').style.color = d.fan ? '#10b981' : '#64748b';
        document.getElementById('s_lamp').style.color = d.lamp ? '#10b981' : '#64748b';
        document.getElementById('s_auto').innerText = d.auto ? "ON" : "MANUAL";
      });
    }

    function scanWifi() {
       var list = document.getElementById('wifi-list');
       list.style.display = 'block';
       list.innerHTML = 'Scanning... Please Wait (~5s)';

       fetch('/scan').then(r => r.json()).then(data => {
          if(data.status === "scanning") {
             setTimeout(scanWifi, 1000);
             return;
          }
          list.innerHTML = '';
          data.forEach(ssid => {
             var div = document.createElement('div');
             div.className = 'wifi-item';
             div.innerText = ssid;
             div.onclick = function() {
                document.getElementById('ssid').value = ssid;
                list.style.display = 'none';
             };
             list.appendChild(div);
          });
       }).catch(e => {
          list.innerHTML = 'Error scanning: ' + e;
       });
    }

    function cmd(act) { fetch('/cmd?do='+act).then(update); }
    setInterval(update, 2000); update();
  </script>
</body>
</html>
)=====";

#endif
