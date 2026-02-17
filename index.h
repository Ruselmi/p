#ifndef INDEX_H
#define INDEX_H

const char HTML_HEAD[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Smart Class Ultimate</title>
  <link href="https://fonts.googleapis.com/css2?family=Poppins:wght@400;600;800&display=swap" rel="stylesheet">
  <style>
    /* Modern Glassmorphism & Minimalist Design */
    :root {
      --bg: #0f172a; --card: rgba(30, 41, 59, 0.7);
      --primary: #6366f1; --text: #f8fafc; --ok: #10b981; --warn: #f59e0b; --danger: #ef4444;
    }
    body { font-family: 'Poppins', sans-serif; background: var(--bg); color: var(--text); margin: 0; padding: 20px; -webkit-font-smoothing: antialiased; }
    .container { max-width: 800px; margin: 0 auto; }

    header { text-align: center; margin-bottom: 25px; }
    h1 { margin: 0; font-size: 2rem; background: linear-gradient(135deg, #a855f7, #6366f1); -webkit-background-clip: text; -webkit-text-fill-color: transparent; }
    .status-bar { font-size: 0.8rem; color: #94a3b8; margin-top: 5px; display: flex; justify-content: center; gap: 10px; }

    /* Grid Layout */
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(140px, 1fr)); gap: 15px; }

    /* Card Style */
    .card {
      background: var(--card); backdrop-filter: blur(10px);
      padding: 20px; border-radius: 20px; text-align: center;
      border: 1px solid rgba(255,255,255,0.08);
      transition: transform 0.2s;
    }
    .card:hover { transform: translateY(-3px); border-color: rgba(255,255,255,0.2); }

    .card h3 { margin: 0 0 5px 0; font-size: 0.75rem; color: #94a3b8; text-transform: uppercase; letter-spacing: 1px; }
    .val { font-size: 1.8rem; font-weight: 800; }
    .unit { font-size: 0.8rem; color: #64748b; }

    .ref { font-size: 0.65rem; color: #64748b; display: block; margin-top: 5px; text-decoration: none; border-top: 1px solid rgba(255,255,255,0.05); padding-top: 5px; }
    .ref:hover { color: var(--primary); }

    /* Buttons */
    button {
      padding: 14px; border: none; border-radius: 14px; font-weight: 600;
      cursor: pointer; color: white; transition: 0.2s; width: 100%; font-family: inherit;
    }
    .btn-on { background: rgba(99, 102, 241, 0.2); border: 1px solid var(--primary); color: var(--primary); }
    .btn-on:hover, .btn-on.active { background: var(--primary); color: white; }

    .btn-off { background: rgba(239, 68, 68, 0.2); border: 1px solid var(--danger); color: var(--danger); }
    .btn-off:hover { background: var(--danger); color: white; }

    .btn-group { display: flex; gap: 10px; margin-top: 10px; }

    /* Piano Scrollable */
    .piano-container { overflow-x: auto; padding-bottom: 10px; margin-top: 10px; }
    .piano-keys { display: flex; width: max-content; gap: 2px; margin: 0 auto; background: #000; padding: 5px; border-radius: 10px; }
    .key { width: 34px; height: 100px; background: white; border-radius: 0 0 5px 5px; position: relative; cursor: pointer; }
    .key:active { background: #e2e8f0; transform: scale(0.98); }
    .key-black { width: 20px; height: 60px; background: #1e293b; position: absolute; top: 0; left: -10px; z-index: 2; border-radius: 0 0 3px 3px; border: 1px solid #334155; }
    .key-label { position: absolute; bottom: 5px; width: 100%; text-align: center; color: #333; font-size: 9px; font-weight: bold; pointer-events: none; }

    /* Select Input */
    select {
      width: 100%; padding: 12px; border-radius: 12px; background: #334155;
      color: white; border: 1px solid #475569; font-family: inherit; margin-bottom: 10px;
    }

    /* Modal / Hidden */
    #wifi-list { max-height: 150px; overflow-y: auto; background: #0f172a; padding: 10px; border-radius: 10px; display: none; border: 1px solid #334155; margin-bottom: 10px; }
    .wifi-item { padding: 8px; border-bottom: 1px solid #1e293b; cursor: pointer; font-size: 0.9rem; }
    input { width: 100%; padding: 12px; margin: 5px 0 10px 0; background: #1e293b; border: 1px solid #475569; color: white; border-radius: 10px; box-sizing: border-box; }

    .badge { padding: 3px 8px; border-radius: 20px; font-size: 0.7rem; background: #334155; }
    .badge.ok { background: rgba(16, 185, 129, 0.2); color: #10b981; }
    .badge.warn { background: rgba(245, 158, 11, 0.2); color: #f59e0b; }
    .badge.err { background: rgba(239, 68, 68, 0.2); color: #ef4444; }

    /* References List */
    .ref-list { text-align: left; max-height: 200px; overflow-y: auto; font-size: 0.75rem; color: #cbd5e1; }
    .ref-item { padding: 8px 0; border-bottom: 1px solid rgba(255,255,255,0.05); }
    .ref-item a { color: var(--primary); text-decoration: none; }
    .ref-item span { display: block; color: #64748b; font-size: 0.65rem; }
  </style>
</head>
<body>
)=====";

const char HTML_BODY[] PROGMEM = R"=====(
  <div class="container">
    <header>
      <h1>Smart Class Ultimate</h1>
      <div class="status-bar">
        <span><span class="status-dot" style="color:var(--ok);">●</span> Online</span>
        <span id="ip">...</span>
        <span id="time">--:--</span>
      </div>
    </header>

    <div class="grid">
      <!-- MAIN SENSORS -->
      <div class="card">
        <h3>Suhu (KEMENKES)</h3>
        <div class="val" id="t">--</div><span class="unit">°C</span>
        <a href="https://peraturan.bpk.go.id/Details/258908/permenkes-no-2-tahun-2023" target="_blank" class="ref">Std: 18-30°C (PMK 2/2023)</a>
      </div>
      <div class="card">
        <h3>Lembab (KEMENKES)</h3>
        <div class="val" id="h">--</div><span class="unit">%</span>
        <a href="https://peraturan.bpk.go.id/Details/258908/permenkes-no-2-tahun-2023" target="_blank" class="ref">Std: 40-60% (PMK 2/2023)</a>
      </div>
      <div class="card">
        <h3>Suara (KEPMEN LH)</h3>
        <div class="val" id="db">--</div><span class="unit">dB</span>
        <a href="https://peraturan.go.id/id/kepmen-lh-no-48-tahun-1996" target="_blank" class="ref">Max: 55dB (No.48/1996)</a>
      </div>
      <div class="card">
        <h3>Udara (WHO)</h3>
        <div class="val" id="gas">--</div><span class="unit">PPM</span>
        <a href="https://www.ncbi.nlm.nih.gov/books/NBK138705/" target="_blank" class="ref">CO2 &lt; 1000 (Ventilasi)</a>
      </div>

      <!-- STATUS PANEL -->
      <div class="card" style="grid-column: span 2;">
        <h3>Status Kelas</h3>
        <div style="display:flex; justify-content:space-around; align-items:center; margin-top:10px;">
           <div>
             <div style="font-size:0.8rem; color:#94a3b8;">AI SYSTEM</div>
             <div id="ai_status" class="badge ok">Active</div>
           </div>
           <div>
             <div style="font-size:0.8rem; color:#94a3b8;">KESEHATAN</div>
             <div id="health" class="badge ok">Normal</div>
           </div>
           <div>
             <div style="font-size:0.8rem; color:#94a3b8;">ROKOK (MQ2)</div>
             <div id="mq2_stat" class="badge ok">Aman</div>
           </div>
        </div>
      </div>

      <!-- CONTROLS -->
      <div class="card" style="grid-column: span 2; text-align: left;">
        <h3>🎛️ Quick Control</h3>
        <div class="btn-group">
           <button onclick="cmd('ai_toggle')" id="btn_ai" class="btn-on">🤖 AI AUTO</button>
           <button onclick="cmd('bell_toggle')" id="btn_bell" class="btn-on">🔔 AUTO BEL</button>
        </div>
        <div class="btn-group">
           <button onclick="cmd('fan_toggle')" id="btn_fan" class="btn-on">KIPAS</button>
           <button onclick="cmd('lamp_toggle')" id="btn_lamp" class="btn-on">LAMPU</button>
        </div>
      </div>

      <!-- MUSIC & PIANO -->
      <div class="card" style="grid-column: span 2; text-align: left;">
        <h3>🎹 Music Studio (37 Songs)</h3>

        <!-- QUICK SFX -->
        <div style="display:flex; gap:5px; margin-bottom:10px;">
           <button onclick="playSong(33)" style="background:#f59e0b; padding:8px; font-size:0.8rem;">⚡ TOKEN</button>
           <button onclick="playSong(34)" style="background:#ef4444; padding:8px; font-size:0.8rem;">🚑 AMBULANCE</button>
           <button onclick="playSong(35)" style="background:#3b82f6; padding:8px; font-size:0.8rem;">🚓 POLISI</button>
        </div>

        <select id="song_select">
          <optgroup label="Lagu Nasional & Daerah">
            <option value="28">Indonesia Raya</option>
            <option value="29">Halo Halo Bandung</option>
            <option value="30">Gundul Pacul</option>
          </optgroup>
          <optgroup label="Lagu Populer">
            <option value="31">Butterfly (Ayayaya)</option>
            <option value="32">Super Mario Long</option>
            <option value="36">Nyan Cat</option>
            <option value="0">Super Mario Short</option>
            <option value="1">Zelda Storms</option>
            <option value="2">Star Wars Imperial</option>
            <option value="24">Rick Roll</option>
            <option value="23">Coffin Dance</option>
          </optgroup>
          <optgroup label="Bel Sekolah">
            <option value="25">Bel Masuk (07:00)</option>
            <option value="26">Bel Istirahat (10:00)</option>
            <option value="27">Bel Pulang (14:00)</option>
          </optgroup>
        </select>

        <div class="btn-group">
          <button onclick="playSong()" class="btn-on" style="background:#8b5cf6;">▶ PLAY</button>
          <button onclick="cmd('music_stop')" class="btn-off">■ STOP</button>
        </div>

        <!-- FULL PIANO (C4-C5) -->
        <div class="piano-container">
          <div class="piano-keys">
            <!-- C4 - B4 -->
            <div class="key" onmousedown="pt(262)"><span class="key-label">C4</span></div>
            <div class="key" onmousedown="pt(294)"><span class="key-label">D4</span><div class="key-black"></div></div>
            <div class="key" onmousedown="pt(330)"><span class="key-label">E4</span><div class="key-black"></div></div>
            <div class="key" onmousedown="pt(349)"><span class="key-label">F4</span></div>
            <div class="key" onmousedown="pt(392)"><span class="key-label">G4</span><div class="key-black"></div></div>
            <div class="key" onmousedown="pt(440)"><span class="key-label">A4</span><div class="key-black"></div></div>
            <div class="key" onmousedown="pt(494)"><span class="key-label">B4</span><div class="key-black"></div></div>
            <!-- C5 - B5 -->
            <div class="key" onmousedown="pt(523)"><span class="key-label">C5</span></div>
            <div class="key" onmousedown="pt(587)"><span class="key-label">D5</span><div class="key-black"></div></div>
            <div class="key" onmousedown="pt(659)"><span class="key-label">E5</span><div class="key-black"></div></div>
            <div class="key" onmousedown="pt(698)"><span class="key-label">F5</span></div>
            <div class="key" onmousedown="pt(784)"><span class="key-label">G5</span><div class="key-black"></div></div>
            <div class="key" onmousedown="pt(880)"><span class="key-label">A5</span><div class="key-black"></div></div>
            <div class="key" onmousedown="pt(988)"><span class="key-label">B5</span><div class="key-black"></div></div>
            <div class="key" onmousedown="pt(1047)"><span class="key-label">C6</span></div>
          </div>
        </div>
      </div>

      <!-- REFERENCES & JOURNALS -->
      <div class="card" style="grid-column: span 2;">
        <h3 onclick="document.getElementById('ref_list').style.display = document.getElementById('ref_list').style.display == 'none' ? 'block' : 'none'" style="cursor:pointer;">📚 Referensi Jurnal & Standar (Klik)</h3>
        <div id="ref_list" class="ref-list" style="display:none;">
          <div class="ref-item">
            <a href="https://peraturan.bpk.go.id/Details/258908/permenkes-no-2-tahun-2023" target="_blank">Permenkes No. 2 Tahun 2023</a>
            <span>Standar Baku Mutu Kesehatan Lingkungan (Suhu, Kelembaban)</span>
          </div>
          <div class="ref-item">
            <a href="https://peraturan.go.id/id/kepmen-lh-no-48-tahun-1996" target="_blank">Kepmen LH No. 48 Tahun 1996</a>
            <span>Baku Tingkat Kebisingan (Max 55dB Sekolah)</span>
          </div>
          <div class="ref-item">
            <a href="https://www.ncbi.nlm.nih.gov/books/NBK138705/" target="_blank">WHO Guidelines for Indoor Air Quality</a>
            <span>Selected Pollutants (CO, NO2, Benzene)</span>
          </div>
          <div class="ref-item">
            <a href="https://jtiik.ub.ac.id/index.php/jtiik/article/download/9525/1503" target="_blank">Sistem Pemantau Kenyamanan Ruang Kelas (Fikhri et al.)</a>
            <span>Jurnal Teknologi Informasi dan Ilmu Komputer</span>
          </div>
          <div class="ref-item">
            <a href="https://ejurnal.umpri.ac.id/index.php/JIK/article/view/1057" target="_blank">Hubungan Kecerdasan Emosional & Stress (Hastuti & Baiti)</a>
            <span>Jurnal Ilmiah Kesehatan</span>
          </div>
          <div class="ref-item">
            <a href="https://ejurnal.ung.ac.id/index.php/jnj/article/view/9822/0" target="_blank">Tingkat Stres Siswa Full Day School (Soeli et al.)</a>
            <span>Jambura Nursing Journal</span>
          </div>
          <div class="ref-item">
            <a href="https://iai-jatim.com/wp-content/uploads/2023/11/SNI-6197_2020-Konservasi-Energi-Pada-Sistem-Pencahayaan.pdf" target="_blank">SNI 6197:2020 (Pencahayaan)</a>
            <span>Konservasi Energi pada Sistem Pencahayaan</span>
          </div>
          <div class="ref-item">
            <a href="https://jahe.or.id/index.php/jahe/article/view/1348" target="_blank">Mengenal Internet of Things (Baharuddin)</a>
            <span>Journal of Human And Education</span>
          </div>
        </div>
      </div>

      <!-- SETTINGS -->
      <div class="card" style="grid-column: span 2; text-align:left;">
        <h3 onclick="document.getElementById('set_form').style.display='block'" style="cursor:pointer;">⚙️ Settings (Click to Expand)</h3>
        <div id="set_form" style="display:none; margin-top:10px;">
          <form action="/save" method="POST">
             <div style="display:flex; gap:5px;">
                <input type="text" name="ssid" id="ssid" placeholder="WiFi SSID" readonly>
                <button type="button" onclick="scanWifi()" style="width:80px; background:#6366f1;">SCAN</button>
             </div>
             <div id="wifi-list">Scanning...</div>
             <input type="text" name="ssid_manual" placeholder="Manual SSID">
             <input type="password" name="pass" placeholder="Password">
             <input type="text" name="bot" placeholder="Bot Token">
             <input type="text" name="id" placeholder="Chat ID">
             <button type="submit" class="btn-on" style="background:#10b981;">SAVE & RESTART</button>
          </form>
        </div>
      </div>
    </div>

    <div style="text-align:center; margin-top:30px; color:#64748b; font-size:0.7rem;">
      Smart Class Ultimate v4.3 | <a href="/csv" style="color:var(--primary);">Download Log</a>
    </div>
  </div>

  <script>
    function pt(f) { fetch('/cmd?do=tone&freq='+f); }
    function playSong(id) {
        var s = id !== undefined ? id : document.getElementById('song_select').value;
        fetch('/cmd?do=music_play&id='+s);
    }
    function cmd(a) { fetch('/cmd?do='+a).then(update); }

    function update() {
      fetch('/data?ts=' + Date.now()).then(r => r.json()).then(d => {
        document.getElementById('t').innerText = d.t.toFixed(1);
        document.getElementById('h').innerText = d.h.toFixed(0);
        document.getElementById('gas').innerText = d.gas;
        document.getElementById('db').innerText = d.db.toFixed(0);
        document.getElementById('time').innerText = d.time;
        document.getElementById('ip').innerText = window.location.hostname;

        // Status Badges
        document.getElementById('ai_status').innerText = d.ai ? "Active" : "Manual";
        document.getElementById('ai_status').className = d.ai ? "badge ok" : "badge warn";

        document.getElementById('health').innerText = d.health.includes("OK") ? "Normal" : "Warning";
        document.getElementById('health').className = d.health.includes("OK") ? "badge ok" : "badge err";

        document.getElementById('mq2_stat').innerText = d.mq2 > 2000 ? "ASAP!" : "Aman";
        document.getElementById('mq2_stat').className = d.mq2 > 2000 ? "badge err" : "badge ok";

        // Buttons
        document.getElementById('btn_fan').className = d.fan ? "btn-on active" : "btn-on";
        document.getElementById('btn_lamp').className = d.lamp ? "btn-on active" : "btn-on";
        document.getElementById('btn_ai').className = d.ai ? "btn-on active" : "btn-on";
        document.getElementById('btn_bell').className = d.bell ? "btn-on active" : "btn-on";
      });
    }

    function scanWifi() {
       var l = document.getElementById('wifi-list'); l.style.display='block'; l.innerHTML='Scanning...';
       fetch('/scan').then(r=>r.json()).then(d=>{
          if(d.status==="scanning"){ setTimeout(scanWifi,1000); return; }
          l.innerHTML='';
          d.forEach(s=>{
             var x=document.createElement('div'); x.className='wifi-item'; x.innerText=s;
             x.onclick=()=>{document.getElementById('ssid').value=s; l.style.display='none';};
             l.appendChild(x);
          });
       });
    }
    setInterval(update, 2000); update();
  </script>
</body>
</html>
)=====";

#endif
