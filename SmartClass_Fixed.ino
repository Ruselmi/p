/*
 * SMART CLASS IOT - PURE MONITORING EDITION (WHO/KEMENKES/SNI STANDARDS)
 * Board: ESP32 Dev Module
 * Fitur: Monitoring Lingkungan Kelas, Telegram Alert, Auto Bell, Web Dashboard Minimalis
 * * --- STANDAR REFERENSI ---
 * 1. Suhu: 18-30°C (Kemenkes/SNI)
 * 2. Kebisingan: < 55 dB (WHO School Guidelines)
 * 3. Pencahayaan: 250 - 300 Lux (SNI Ruang Kelas)
 * 4. Keamanan: Deteksi Api (Flame) & Asap Rokok (MQ-2)
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <UniversalTelegramBot.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "time.h"

// ================= KONFIGURASI PIN (FIXED) =================
#define PIN_DHT         18
#define PIN_BUZZER      26
#define PIN_FAN         4
#define PIN_LAMP        2

// SENSOR CAHAYA (LDR)
#define PIN_LDR         34 // ADC1
#define PIN_LDR_DO      25 // Digital (DO)

// SENSOR GAS (MQ-2) - Mendeteksi Asap Rokok
#define PIN_MQ2         35 // ADC1 - (Sesuai Request)
#define PIN_MQ2_DO      27 // Digital (DO)

// SENSOR API (FLAME) - Mendeteksi Kebakaran
// Dipindah ke 33 karena 35 dipakai MQ2
#define PIN_FLAME       33 // ADC1

#define PIN_SOUND       32 // ADC1
#define PIN_TRIG        5
#define PIN_ECHO        19

// ================= SYSTEM CONFIG =================
#define PWM_CHANNEL     0
#define PWM_RESOLUTION  8
#define PWM_FREQ        2000
#define DHTTYPE         DHT22
#define NTP_SERVER      "pool.ntp.org"
#define GMT_OFFSET_SEC  25200 // UTC+7 (WIB)
#define DAY_LIGHT_OFFSET 0

// ================= VARIABLES =================
DHT dht(PIN_DHT, DHTTYPE);
WebServer server(80);
WiFiClientSecure client;
UniversalTelegramBot bot("", client);
Preferences pref;

// Credentials
String ssid_name = "ELSON";
String ssid_pass = "elson250129";
String bot_token = "8324067380:AAHfMtWfLdtoYByjnrm2sgy90z3y01V6C-I";
String chat_id   = "6383896382";

// Data Sensor
float t = 0, h = 0, db = 0;
int lux = 0, mq2_val = 0, flame_val = 4095;
int ldr_do = 1, mq2_do = 1;
String safety_status = "AMAN"; // Status Gabungan (Api & Asap)
unsigned long last_sensor = 0, last_bot = 0, last_bell_check = 0, last_alert = 0, last_wifi_check = 0;

// Music Player
bool is_playing = false;
int note_index = 0;
unsigned long last_note_start = 0;
bool note_state = false;
const int* current_melody;
const int* current_tempo;
int current_len = 0;

// ================= DATA MUSIK =================
#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_D5 587
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_G5 784
#define REST    0

const int song_bell_m[] = {NOTE_C5, NOTE_E5, NOTE_G5, NOTE_C5};
const int song_bell_t[] = {4, 4, 4, 2};

// ================= FUNCTION PROTOTYPES =================
void handleSaveSettings();
void readSensors();
void handleJson();
void handleCommand();
void handleDownload();
void handleMusic();
void handleTelegram(int numNewMessages);
void playBell();
void checkSchedule();
String getFormattedTime();
void handleScan();

// ================= WEB DASHBOARD (MINIMALIS) =================
const char HTML_PAGE[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Class Monitor</title>
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;600&display=swap" rel="stylesheet">
<style>
:root{--bg:#f8fafc;--card:#ffffff;--text:#1e293b;--acc:#3b82f6;--ok:#22c55e;--warn:#f59e0b;--danger:#ef4444}
body{font-family:'Inter',sans-serif;background:var(--bg);color:var(--text);margin:0;padding:20px}
.container{max-width:900px;margin:0 auto}
header{display:flex;justify-content:space-between;align-items:center;margin-bottom:30px}
h1{font-size:1.5rem;margin:0;font-weight:600}
.time-badge{background:var(--text);color:#fff;padding:5px 15px;border-radius:20px;font-size:0.9rem}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:20px}
.card{background:var(--card);padding:25px;border-radius:16px;box-shadow:0 4px 6px -1px rgba(0,0,0,0.05);border:1px solid #e2e8f0}
.card h3{margin:0 0 10px 0;font-size:0.85rem;color:#64748b;text-transform:uppercase;letter-spacing:1px}
.value-group{display:flex;align-items:baseline;gap:5px}
.value{font-size:2.5rem;font-weight:600;color:var(--text)}
.unit{font-size:1rem;color:#94a3b8}
.standard{margin-top:15px;padding-top:15px;border-top:1px solid #f1f5f9;font-size:0.8rem;color:#64748b;display:flex;justify-content:space-between}
.badge{padding:4px 8px;border-radius:6px;font-size:0.75rem;font-weight:600}
.b-ok{background:#dcfce7;color:#166534}.b-warn{background:#fef3c7;color:#92400e}.b-danger{background:#fee2e2;color:#991b1b}
.status-indicator{height:10px;width:10px;border-radius:50%;display:inline-block;margin-right:5px}
.footer{text-align:center;margin-top:40px;font-size:0.8rem;color:#94a3b8}
.setup-box{margin-top:30px;background:#fff;padding:20px;border-radius:12px;display:none}
input{padding:10px;border:1px solid #cbd5e1;border-radius:8px;width:100%;margin-bottom:10px;box-sizing:border-box}
button{background:var(--text);color:white;border:none;padding:10px 20px;border-radius:8px;cursor:pointer}
</style>
</head>
<body>
<div class="container">
  <header>
    <div>
      <h1>Monitoring Kelas IoT</h1>
      <small style="color:#64748b">Sensor Integration System</small>
    </div>
    <div class="time-badge" id="clock">--:--</div>
  </header>

  <div class="grid">
    <!-- SUHU -->
    <div class="card">
      <h3>Temperatur</h3>
      <div class="value-group"><span class="value" id="t">--</span><span class="unit">°C</span></div>
      <div class="standard">
        <span>Std: 18-30°C</span>
        <span class="badge" id="t_s">-</span>
      </div>
    </div>

    <!-- KELEMBABAN -->
    <div class="card">
      <h3>Kelembaban</h3>
      <div class="value-group"><span class="value" id="h">--</span><span class="unit">%</span></div>
      <div class="standard">
        <span>Std: 45-70%</span>
        <span class="badge" id="h_s">-</span>
      </div>
    </div>

    <!-- CAHAYA -->
    <div class="card">
      <h3>Pencahayaan</h3>
      <div class="value-group"><span class="value" id="lux">--</span><span class="unit">Lux</span></div>
      <div class="standard">
        <span>Std: 250-300 Lux (SNI)</span>
        <span class="badge" id="lux_s">-</span>
      </div>
    </div>

    <!-- SUARA -->
    <div class="card">
      <h3>Kebisingan</h3>
      <div class="value-group"><span class="value" id="db">--</span><span class="unit">dB</span></div>
      <div class="standard">
        <span>Std: < 55 dB (WHO)</span>
        <span class="badge" id="db_s">-</span>
      </div>
    </div>

    <!-- KEAMANAN (API & ASAP) -->
    <div class="card" style="grid-column: 1 / -1; border-left: 5px solid var(--ok);" id="safe_card">
      <div style="display:flex; justify-content:space-between; align-items:center">
        <div>
          <h3>Status Keamanan Kelas</h3>
          <div class="value-group"><span class="value" id="safe_txt">AMAN</span></div>
          <small style="color:#64748b">
            Asap (MQ-2): <span id="mq2_val">0</span> | Api (Flame): <span id="flame_val">0</span>
          </small>
        </div>
        <div style="font-size:3rem" id="safe_icon">🛡️</div>
      </div>
    </div>
  </div>

  <div style="text-align:center; margin-top:20px;">
    <button onclick="document.getElementById('setup').style.display='block'">⚙️ Pengaturan</button>
    <a href="/csv"><button style="background:#fff; color:#1e293b; border:1px solid #cbd5e1">📥 Download Data</button></a>
  </div>

  <div id="setup" class="setup-box">
    <h3>Konfigurasi Sistem</h3>
    <form action="/save" method="POST">
      <input type="text" name="ssid" placeholder="WiFi SSID" id="ssid_in">
      <button type="button" onclick="scanWifi()" style="font-size:0.8rem; margin-bottom:10px">🔍 Scan WiFi</button>
      <div id="scan_res"></div>
      <input type="password" name="pass" placeholder="WiFi Password">
      <input type="text" name="bot" placeholder="Telegram Bot Token">
      <input type="text" name="id" placeholder="Telegram Chat ID">
      <button type="submit">Simpan & Restart</button>
      <button type="button" onclick="this.parentElement.parentElement.style.display='none'" style="background:#ef4444">Tutup</button>
    </form>
  </div>

  <div class="footer">
    Referensi: Permenkes No.7/2019, SNI 6197:2011, WHO Guidelines.
  </div>
</div>

<script>
function eval(id, val, min, max, rev) {
  let el = document.getElementById(id);
  let ok = (val >= min && val <= max);
  if(rev) ok = !ok;
  if(ok) { el.innerText = "Sesuai"; el.className = "badge b-ok"; }
  else { el.innerText = "Tidak Sesuai"; el.className = "badge b-danger"; }
}

function update() {
  fetch('/data').then(r=>r.json()).then(d=>{
    document.getElementById('clock').innerText = d.time;

    // Suhu & Lembab
    document.getElementById('t').innerText = d.t.toFixed(1);
    eval('t_s', d.t, 18, 30, false);
    document.getElementById('h').innerText = d.h.toFixed(0);
    eval('h_s', d.h, 45, 70, false);

    // Lux
    document.getElementById('lux').innerText = d.lux;
    let l_el = document.getElementById('lux_s');
    if(d.lux < 250) { l_el.innerText="Kurang"; l_el.className="badge b-warn"; }
    else if(d.lux > 700) { l_el.innerText="Silau"; l_el.className="badge b-warn"; }
    else { l_el.innerText="Standar SNI"; l_el.className="badge b-ok"; }

    // dB
    document.getElementById('db').innerText = d.db.toFixed(1);
    let db_el = document.getElementById('db_s');
    if(d.db > 55) { db_el.innerText="Bising"; db_el.className="badge b-danger"; }
    else { db_el.innerText="Kondusif"; db_el.className="badge b-ok"; }

    // Safety (Api & Asap)
    document.getElementById('mq2_val').innerText = d.mq2;
    document.getElementById('flame_val').innerText = d.flame;

    let s_txt = document.getElementById('safe_txt');
    let s_card = document.getElementById('safe_card');
    let s_icon = document.getElementById('safe_icon');

    if(d.status == "FIRE") {
      s_txt.innerText = "KEBAKARAN!";
      s_card.style.borderColor = "#ef4444";
      s_card.style.background = "#fee2e2";
      s_txt.style.color = "#ef4444";
      s_icon.innerText = "🔥";
    } else if(d.status == "SMOKE") {
      s_txt.innerText = "ASAP ROKOK!";
      s_card.style.borderColor = "#f59e0b";
      s_card.style.background = "#fef3c7";
      s_txt.style.color = "#b45309";
      s_icon.innerText = "🚬";
    } else {
      s_txt.innerText = "AMAN";
      s_card.style.borderColor = "#22c55e";
      s_card.style.background = "#ffffff";
      s_txt.style.color = "#166534";
      s_icon.innerText = "🛡️";
    }
  });
}

function scanWifi(){
  document.getElementById('scan_res').innerText = "Scanning...";
  fetch('/scan').then(r=>r.json()).then(d=>{
    let html = "";
    d.forEach(s=>{ html += `<div onclick="document.getElementById('ssid_in').value='${s}'" style="padding:5px;cursor:pointer;border-bottom:1px solid #eee">${s}</div>`; });
    document.getElementById('scan_res').innerHTML = html;
  });
}

setInterval(update, 2000);
update();
</script>
</body>
</html>
)=====";

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  // Setup Pin Sensor
  pinMode(PIN_LDR_DO, INPUT_PULLUP);
  pinMode(PIN_MQ2_DO, INPUT_PULLUP);
  pinMode(PIN_FLAME, INPUT); // Analog Read untuk Flame

  // Buzzer Setup
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(PIN_BUZZER, PWM_CHANNEL);
  ledcWriteTone(PWM_CHANNEL, 0);

  dht.begin();

  // Load Prefs
  pref.begin("smartclass", false);
  ssid_name = pref.getString("ssid", ssid_name);
  ssid_pass = pref.getString("pass", ssid_pass);
  bot_token = pref.getString("bot", bot_token);
  chat_id   = pref.getString("id", chat_id);
  bot.updateToken(bot_token);

  // WiFi
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid_name.c_str(), ssid_pass.c_str());

  unsigned long start = millis();
  while(WiFi.status() != WL_CONNECTED && millis() - start < 10000) { delay(100); }

  if(WiFi.status() == WL_CONNECTED) {
    configTime(GMT_OFFSET_SEC, DAY_LIGHT_OFFSET, NTP_SERVER);
    playBell(); // Tanda sistem siap
  } else {
    WiFi.softAP("SmartClass_Setup");
  }

  // Server
  client.setInsecure();
  server.on("/", [](){ server.send(200, "text/html", HTML_PAGE); });
  server.on("/data", handleJson);
  server.on("/save", HTTP_POST, handleSaveSettings);
  server.on("/scan", handleScan);
  server.on("/csv", handleDownload);
  server.begin();
}

// ================= LOOP =================
void loop() {
  server.handleClient();
  handleMusic();

  unsigned long now = millis();

  // 1. Read Sensors (2s)
  if (now - last_sensor > 2000) {
    last_sensor = now;
    readSensors();
  }

  // 2. Schedule (1s)
  if (now - last_bell_check > 1000) {
    last_bell_check = now;
    checkSchedule();
  }

  // 3. Telegram (5s)
  if (WiFi.status() == WL_CONNECTED && now - last_bot > 5000 && !is_playing) {
    last_bot = now;
    int n = bot.getUpdates(bot.last_message_received + 1);
    while(n) {
      handleTelegram(n);
      n = bot.getUpdates(bot.last_message_received + 1);
    }
  }
}

// ================= HANDLERS =================

void readSensors() {
  float t_read = dht.readTemperature();
  float h_read = dht.readHumidity();
  if(!isnan(t_read)) t = t_read;
  if(!isnan(h_read)) h = h_read;

  // LDR (Lux)
  int raw_ldr = analogRead(PIN_LDR);
  lux = map(4095 - raw_ldr, 0, 4095, 0, 1000);

  // Sound (dB)
  unsigned long startMillis = millis();
  float peakToPeak = 0;
  unsigned int signalMax = 0;
  unsigned int signalMin = 4095;
  while (millis() - startMillis < 50) {
    int sample = analogRead(PIN_SOUND);
    if (sample < 1024) {
       if (sample > signalMax) signalMax = sample;
       else if (sample < signalMin) signalMin = sample;
    }
  }
  peakToPeak = signalMax - signalMin;
  db = 20 * log10(peakToPeak / 4095.0 * 3.3) + 50.0;
  if(db < 0) db = 0;

  // MQ-2 (Smoke)
  int mq2_raw = analogRead(PIN_MQ2);
  mq2_do = digitalRead(PIN_MQ2_DO);
  mq2_val = (mq2_val * 0.7) + (mq2_raw * 0.3); // Smooth filter

  // FLAME SENSOR (Fire)
  // Sensor Api biasanya nilai rendah (< 500 atau 1000) jika ada api
  // Nilai 4095 artinya tidak ada api (Gelap IR)
  int flame_raw = analogRead(PIN_FLAME);
  flame_val = (flame_val * 0.7) + (flame_raw * 0.3); // Smooth filter

  // --- SAFETY LOGIC ---
  safety_status = "AMAN";

  // Prioritas 1: API (Bahaya Tinggi)
  // Threshold < 1000 artinya IR kuat terdeteksi (Api)
  if (flame_val < 1000) {
    safety_status = "FIRE";
    if (!is_playing && millis() - last_alert > 10000) {
      // Alarm Kebakaran (Nada Tinggi Cepat)
      ledcWriteTone(PWM_CHANNEL, 2000); delay(100); ledcWriteTone(PWM_CHANNEL, 0); delay(100);
      ledcWriteTone(PWM_CHANNEL, 2000); delay(100); ledcWriteTone(PWM_CHANNEL, 0);

      if (WiFi.status() == WL_CONNECTED && millis() - last_alert > 20000) {
        bot.sendMessage(chat_id, "🔥 KEBAKARAN! Sensor Api mendeteksi nyala api!", "");
        last_alert = millis();
      }
    }
  }
  // Prioritas 2: ASAP (Pelanggaran/Bahaya Ringan)
  else if (mq2_val > 2500 || mq2_do == 0) {
    safety_status = "SMOKE";
    if (!is_playing && millis() - last_alert > 10000) {
      // Alarm Asap (Nada Putus-putus)
      ledcWriteTone(PWM_CHANNEL, 1000); delay(500); ledcWriteTone(PWM_CHANNEL, 0);

      if (WiFi.status() == WL_CONNECTED && millis() - last_alert > 30000) {
        bot.sendMessage(chat_id, "🚬 ASAP ROKOK TERDETEKSI! Level: " + String(mq2_val), "");
        last_alert = millis();
      }
    }
  }
}

void handleJson() {
  String json = "{";
  json += "\"t\":" + String(t, 1) + ",";
  json += "\"h\":" + String(h, 0) + ",";
  json += "\"lux\":" + String(lux) + ",";
  json += "\"db\":" + String(db, 1) + ",";
  json += "\"mq2\":" + String(mq2_val) + ",";
  json += "\"flame\":" + String(flame_val) + ",";
  json += "\"status\":\"" + safety_status + "\",";
  json += "\"time\":\"" + getFormattedTime() + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleCommand() {
  server.send(200, "text/plain", "OK");
}

void handleScan() {
  int n = WiFi.scanNetworks();
  String json = "[";
  for(int i=0; i<n; i++) {
    if(i) json += ",";
    json += "\"" + WiFi.SSID(i) + "\"";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleDownload() {
  String csv = "Waktu,Suhu,Lembab,Lux,dB,MQ2,Flame,Status\n";
  csv += String(millis()) + "," + String(t) + "," + String(h) + "," + String(lux) + "," + String(db) + "," + String(mq2_val) + "," + String(flame_val) + "," + safety_status + "\n";
  server.sendHeader("Content-Disposition", "attachment; filename=data_monitor.csv");
  server.send(200, "text/csv", csv);
}

void handleSaveSettings() {
  if (server.hasArg("ssid")) {
    pref.putString("ssid", server.arg("ssid"));
    pref.putString("pass", server.arg("pass"));
    if(server.arg("bot").length() > 5) pref.putString("bot", server.arg("bot"));
    if(server.arg("id").length() > 5) pref.putString("id", server.arg("id"));
    server.send(200, "text/html", "Tersimpan. Restarting...");
    delay(1000); ESP.restart();
  }
}

// Logic Musik Sederhana (Untuk Bel)
void playBell() {
  if (is_playing) return;
  current_melody = song_bell_m;
  current_tempo = song_bell_t;
  current_len = 4;
  note_index = 0;
  is_playing = true;
  last_note_start = millis();
}

void handleMusic() {
  if (!is_playing) return;
  unsigned long now = millis();
  int note_dur = 1000 / current_tempo[note_index];

  if (!note_state) {
    ledcWriteTone(PWM_CHANNEL, current_melody[note_index]);
    note_state = true;
    last_note_start = now;
  }

  if (now - last_note_start > note_dur) {
    ledcWriteTone(PWM_CHANNEL, 0);
    note_state = false;
    note_index++;
    if (note_index >= current_len) is_playing = false;
  }
}

void checkSchedule() {
  if (WiFi.status() != WL_CONNECTED) return;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  if (timeinfo.tm_sec == 0) {
    if ((timeinfo.tm_hour == 7 && timeinfo.tm_min == 0) ||
        (timeinfo.tm_hour == 10 && timeinfo.tm_min == 0) ||
        (timeinfo.tm_hour == 14 && timeinfo.tm_min == 0)) {
      playBell();
    }
  }
}

void handleTelegram(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id_msg = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    if (text == "/status") {
      String msg = "📊 Status Kelas:\n";
      msg += "🌡️ Suhu: " + String(t, 1) + "°C\n";
      msg += "💡 Cahaya: " + String(lux) + " Lux\n";
      msg += "🔊 Suara: " + String(db, 1) + " dB\n";
      msg += "🔥 Status: " + safety_status;
      bot.sendMessage(chat_id_msg, msg, "");
    }
  }
}

String getFormattedTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) return "--:--";
  char s[10];
  strftime(s, 10, "%H:%M", &timeinfo);
  return String(s);
}
