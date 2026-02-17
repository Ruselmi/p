/*
 * SMART CLASS IOT - MINIMALIST EDITION (Single File)
 * Board: ESP32 Dev Module
 * Fitur: Monitoring Sensor (Realtime), Telegram Bot, Web Dashboard (Stable), Music Player, WiFi Manager
 *
 * Update Fixes (v3.5 - WiFi Scan Fixed):
 * - WIFI SCAN: Diperbaiki dengan mode async/blocking yang lebih robust + Hidden Networks.
 * - LOGGING: Ditambah Serial Print untuk debug scan.
 * - COMPILER: Struktur tetap aman (setup/loop di atas).
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <UniversalTelegramBot.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>

// ================= KONFIGURASI PIN =================
#define PIN_DHT         18
#define PIN_BUZZER      26
#define PIN_FAN         4
#define PIN_LAMP        2
#define PIN_LDR         34
#define PIN_MQ135       35
#define PIN_SOUND       32
#define PIN_TRIG        5
#define PIN_ECHO        19

// ================= KONFIGURASI PWM =================
#define PWM_CHANNEL     0
#define PWM_RESOLUTION  8
#define PWM_FREQ        2000

#define DHTTYPE         DHT22

// ================= KONFIGURASI ALARM & WAKTU =================
const int GAS_ALARM_LIMIT   = 2500;
const float TEMP_ALARM_LIMIT = 35.0;
const long GMT_OFFSET_SEC   = 25200; // WIB (UTC+7)
const int DAYLIGHT_OFFSET   = 0;

// ================= VARIABEL GLOBAL =================
DHT dht(PIN_DHT, DHTTYPE);
WebServer server(80);
WiFiClientSecure client;
UniversalTelegramBot bot("", client);
Preferences pref;

// Default values
String ssid_name = "YOUR_SSID";
String ssid_pass = "YOUR_PASS";
String bot_token = "YOUR_BOT_TOKEN";
String chat_id   = "YOUR_CHAT_ID";

float t = 0, h = 0, lux = 0, dist = 0, db = 0;
int gas = 0;
long rssi = 0;
bool st_fan = false, st_lamp = false, mode_auto = true;
String mood = "Netral";
String time_str = "--:--";
bool is_day_time = true;
bool alert_active = false;

// Timer variables
unsigned long last_dht = 0;
unsigned long last_analog = 0;
unsigned long last_bot = 0;
unsigned long last_wifi_check = 0;

// ================= MUSIK (DEFINISI NADA) =================
#define NOTE_G3  196
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784

// --- ZELDA: SONG OF STORMS ---
int melody_theme[] = {
  NOTE_D4, NOTE_F4, NOTE_D5,
  NOTE_D4, NOTE_F4, NOTE_D5,
  NOTE_E5, NOTE_F5, NOTE_E5, NOTE_F5, NOTE_E5, NOTE_C5, NOTE_A4
};
int durations_theme[] = {
  8, 8, 2,
  8, 8, 2,
  6, 16, 6, 16, 6, 8, 2
};

bool is_playing = false;
int note_index = 0;
unsigned long last_note_start = 0;
bool note_state = false;
int melody_len = sizeof(melody_theme) / sizeof(int);

// ================= FUNCTION PROTOTYPES =================
void handleSaveSettings();
void readDHT();
void readAnalogSensors();
void logicAuto();
void handleJson();
void handleCommand();
void handleDownload();
void handleMusic();
void handleTelegram(int numNewMessages);
void checkWiFi();
int smoothAnalog(int pin);
void updateTime();
void checkAlert();
void handleScanWiFi();
void handleRoot();

// ================= LOGIKA UTAMA =================

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n--- SMART CLASS IOT STARTING ---");

  // 1. Setup Pin & PWM Buzzer
  pinMode(PIN_FAN, OUTPUT);
  pinMode(PIN_LAMP, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  // Setup PWM LEDC untuk Buzzer
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(PIN_BUZZER, PWM_CHANNEL);
  ledcWriteTone(PWM_CHANNEL, 0); // Matikan buzzer di awal

  dht.begin();

  // 2. Load Preferences
  pref.begin("smartclass", false);
  ssid_name = pref.getString("ssid", ssid_name);
  ssid_pass = pref.getString("pass", ssid_pass);
  bot_token = pref.getString("bot", bot_token);
  chat_id   = pref.getString("id", chat_id);

  bot.updateToken(bot_token);
  Serial.println("[SETUP] Preferences Loaded");

  // 3. Koneksi WiFi
  Serial.print("[WIFI] Connecting to: "); Serial.println(ssid_name);

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid_name.c_str(), ssid_pass.c_str());

  int try_count = 0;
  while (WiFi.status() != WL_CONNECTED && try_count < 20) {
    delay(250); Serial.print(".");
    try_count++;
  }

  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WIFI] Connected!");
    Serial.print("[WIFI] IP Address: "); Serial.println(WiFi.localIP());
    // Init NTP Time
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET, "pool.ntp.org", "time.nist.gov");
    Serial.println("[NTP] Time synced");
  } else {
    Serial.println("\n[WIFI] Failed. Starting AP Mode.");
    WiFi.softAP("SmartClass_AP");
    Serial.print("[AP] IP Address: "); Serial.println(WiFi.softAPIP());
  }

  // 4. Setup Server
  client.setInsecure();
  server.on("/", handleRoot);
  server.on("/data", handleJson);
  server.on("/cmd", handleCommand);
  server.on("/csv", handleDownload);
  server.on("/scan", handleScanWiFi);
  server.on("/save", HTTP_POST, handleSaveSettings);
  server.enableCORS(true); // ENABLE CORS
  server.begin();
  Serial.println("[SERVER] Started");

  // --- STARTUP SOUND: MARIO INTRO ---
  int mario_notes[] = {NOTE_E5, NOTE_E5, NOTE_E5, NOTE_C5, NOTE_E5, NOTE_G5, NOTE_G4};
  int mario_durs[]  = {100, 100, 100, 100, 100, 200, 200};

  for(int i=0; i<7; i++){
    ledcWriteTone(PWM_CHANNEL, mario_notes[i]);
    delay(mario_durs[i]);
    ledcWriteTone(PWM_CHANNEL, 0);
    delay(50); // Jeda antar nada
  }
}

void loop() {
  server.handleClient();
  handleMusic();
  checkWiFi();

  unsigned long now = millis();

  // 1. Baca Sensor DHT (Tiap 2 detik)
  if (now - last_dht > 2000) {
    last_dht = now;
    readDHT();
  }

  // 2. Baca Sensor Analog Lain (Tiap 1 detik)
  if (now - last_analog > 1000) {
    last_analog = now;
    readAnalogSensors();
    updateTime();
    logicAuto();
    checkAlert();

    // Debug info
    Serial.print("[SENSOR] T:"); Serial.print(t);
    Serial.print(" H:"); Serial.println(h);
  }

  // 3. Cek Telegram (Setiap 3 detik jika konek & TIDAK SEDANG MUSIK)
  if (!is_playing && WiFi.status() == WL_CONNECTED && now - last_bot > 3000) {
    last_bot = now;
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while(numNewMessages) {
      Serial.println("[BOT] New Message!");
      handleTelegram(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
  }
}

// ================= FUNGSI-FUNGSI PENDUKUNG =================

void handleScanWiFi() {
  Serial.println("[WIFI] Start Scan...");

  // WiFi Scan Sync (Blocking) - Simpel dan Ampuh
  int n = WiFi.scanNetworks(false, true); // (async=false, show_hidden=true)

  Serial.print("[WIFI] Scan Done. Found: ");
  Serial.println(n);

  // JSON v7: JsonDocument
  JsonDocument doc;
  JsonArray array = doc.to<JsonArray>();

  if (n == 0) {
    Serial.println("[WIFI] No networks found");
  } else {
    for (int i = 0; i < n; ++i) {
      array.add(WiFi.SSID(i));
      // Hanya tampilkan di Serial jika perlu debug
      // Serial.print(i + 1); Serial.print(": "); Serial.print(WiFi.SSID(i)); Serial.print(" ("); Serial.print(WiFi.RSSI(i)); Serial.println(")");
      if(i >= 20) break; // Limit 20
    }
  }

  String json;
  serializeJson(doc, json);

  // Header penting agar browser tidak cache hasil scan
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");

  server.send(200, "application/json", json);
}

void updateTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    time_str = "--:--";
    return;
  }
  char timeStringBuff[10];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M", &timeinfo);
  time_str = String(timeStringBuff);

  int hour = timeinfo.tm_hour;
  if (hour >= 7 && hour < 17) {
    is_day_time = true;
  } else {
    is_day_time = false;
  }
}

void checkWiFi() {
  unsigned long now = millis();
  if (now - last_wifi_check > 10000) {
    last_wifi_check = now;
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[WIFI] Lost Connection. Reconnecting...");
      WiFi.reconnect();
    }
  }
}

int smoothAnalog(int pin) {
  long total = 0;
  int samples = 5;
  for (int i = 0; i < samples; i++) {
    total += analogRead(pin);
    delay(2);
  }
  return total / samples;
}

void handleSaveSettings() {
  if (server.hasArg("ssid") || server.hasArg("ssid_manual")) {
    String n_ssid = server.arg("ssid");
    if(server.arg("ssid_manual").length() > 0) {
      n_ssid = server.arg("ssid_manual");
    }

    String n_pass = server.arg("pass");
    String n_bot  = server.arg("bot");
    String n_id   = server.arg("id");

    Serial.println("[SETTINGS] Saving new config...");
    pref.putString("ssid", n_ssid);
    pref.putString("pass", n_pass);
    if(n_bot.length() > 5) pref.putString("bot", n_bot);
    if(n_id.length() > 5)  pref.putString("id", n_id);

    String html = "<html><body><h1>Berhasil Disimpan!</h1><p>Alat akan restart...</p></body></html>";
    server.send(200, "text/html", html);
    delay(2000);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Error: SSID kosong");
  }
}

void readDHT() {
  float _t = dht.readTemperature();
  float _h = dht.readHumidity();
  if (!isnan(_t)) t = _t;
  if (!isnan(_h)) h = _h;
}

void readAnalogSensors() {
  gas = smoothAnalog(PIN_MQ135);
  lux = smoothAnalog(PIN_LDR);

  digitalWrite(PIN_TRIG, LOW); delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  long dur = pulseIn(PIN_ECHO, HIGH, 30000);
  dist = (dur == 0) ? 0 : dur * 0.034 / 2;

  int raw_sound = smoothAnalog(PIN_SOUND);
  db = (raw_sound / 4095.0) * 100.0;

  rssi = WiFi.RSSI();

  int stress = 0;
  if (t > 28) stress += 30;
  if (db > 60) stress += 30;
  if (gas > 2000) stress += 30;

  if (stress < 30) mood = "Nyaman 😊";
  else if (stress < 60) mood = "Fokus 😐";
  else mood = "Tidak Kondusif 😡";
}

void checkAlert() {
  bool current_danger = (gas > GAS_ALARM_LIMIT) || (t > TEMP_ALARM_LIMIT);

  if (current_danger && !alert_active) {
    Serial.println("[ALERT] Danger Detected!");
    String msg = "⚠️ *PERINGATAN BAHAYA!*\n\n";
    if(gas > GAS_ALARM_LIMIT) msg += "🔥 Gas Tinggi: " + String(gas) + " PPM\n";
    if(t > TEMP_ALARM_LIMIT)  msg += "🌡 Suhu Panas: " + String(t) + " °C\n";
    msg += "\nSegera cek lokasi!";
    bot.sendMessage(chat_id, msg, "Markdown");
    alert_active = true;
  }
  else if (!current_danger && alert_active) {
    Serial.println("[ALERT] Condition Normal.");
    bot.sendMessage(chat_id, "✅ *KONDISI AMAN*\nSensor kembali normal.", "Markdown");
    alert_active = false;
  }
}

void logicAuto() {
  if (!mode_auto) return;

  if (!is_day_time) {
    st_fan = false;
    st_lamp = false;
    digitalWrite(PIN_FAN, LOW);
    digitalWrite(PIN_LAMP, LOW);
    return;
  }

  if (t > 27.0) { st_fan = true; digitalWrite(PIN_FAN, HIGH); }
  else { st_fan = false; digitalWrite(PIN_FAN, LOW); }

  if (lux < 500) { st_lamp = true; digitalWrite(PIN_LAMP, HIGH); }
  else { st_lamp = false; digitalWrite(PIN_LAMP, LOW); }
}

void handleJson() {
  // JSON v7: JsonDocument (auto size)
  JsonDocument doc;

  doc["t"] = t;
  doc["h"] = h;
  doc["gas"] = gas;
  doc["db"] = db;
  doc["rssi"] = rssi;
  doc["fan"] = st_fan;
  doc["lamp"] = st_lamp;
  doc["auto"] = mode_auto;
  doc["mood"] = mood;
  doc["time"] = time_str;
  doc["alert"] = alert_active;

  String json;
  serializeJson(doc, json);

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");

  server.send(200, "application/json", json);
}

void handleCommand() {
  String act = server.arg("do");
  Serial.print("[CMD] Received: "); Serial.println(act);

  if (act == "fan_toggle") { mode_auto = false; st_fan = !st_fan; digitalWrite(PIN_FAN, st_fan); }
  if (act == "lamp_toggle") { mode_auto = false; st_lamp = !st_lamp; digitalWrite(PIN_LAMP, st_lamp); }
  if (act == "auto_toggle") { mode_auto = !mode_auto; }
  if (act == "music_play") { is_playing = true; note_index = 0; note_state = false; last_note_start = millis(); }
  if (act == "music_stop") { is_playing = false; ledcWriteTone(PWM_CHANNEL, 0); }

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.send(200, "text/plain", "OK");
}

void handleDownload() {
  String csv = "Waktu,Suhu,Lembab,Gas,Suara,RSSI\n";
  csv += String(millis()) + "," + String(t) + "," + String(h) + "," + String(gas) + "," + String(db) + "," + String(rssi);
  server.sendHeader("Content-Disposition", "attachment; filename=log_kelas.csv");
  server.send(200, "text/csv", csv);
}

void handleMusic() {
  if (!is_playing) return;

  int note_duration = 1000 / durations_theme[note_index];
  int pause_between = note_duration * 1.30;
  unsigned long now = millis();

  if (!note_state) {
    ledcWriteTone(PWM_CHANNEL, melody_theme[note_index]);
    note_state = true;
    last_note_start = now;
  }

  if (note_state && (now - last_note_start > note_duration)) {
    ledcWriteTone(PWM_CHANNEL, 0);
  }

  if (now - last_note_start > pause_between) {
    note_state = false;
    note_index++;
    if (note_index >= melody_len) { note_index = 0; }
  }
}

void handleTelegram(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id_msg = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    Serial.print("[BOT] Msg: "); Serial.println(text);

    if (text == "/start") {
      bot.sendMessage(chat_id_msg, "Halo! Perintah: /cek, /fan_on, /fan_off, /lamp_on, /lamp_off", "");
    }
    else if (text == "/cek") {
      String msg = "Status Kelas (" + time_str + "):\n";
      msg += "🌡 Suhu: " + String(t) + "C\n";
      msg += "💡 Lampu: " + String(st_lamp?"ON":"OFF") + "\n";
      msg += "⚠️ Alert: " + String(alert_active?"BAHAYA":"AMAN");
      bot.sendMessage(chat_id_msg, msg, "");
    }
    else if (text == "/fan_on") { mode_auto=false; st_fan=true; digitalWrite(PIN_FAN, HIGH); bot.sendMessage(chat_id_msg, "Kipas ON", ""); }
    else if (text == "/fan_off") { mode_auto=false; st_fan=false; digitalWrite(PIN_FAN, LOW); bot.sendMessage(chat_id_msg, "Kipas OFF", ""); }
    else if (text == "/lamp_on") { mode_auto=false; st_lamp=true; digitalWrite(PIN_LAMP, HIGH); bot.sendMessage(chat_id_msg, "Lampu ON", ""); }
    else if (text == "/lamp_off") { mode_auto=false; st_lamp=false; digitalWrite(PIN_LAMP, LOW); bot.sendMessage(chat_id_msg, "Lampu OFF", ""); }
  }
}

// ================= HTML SECTION =================
extern const char HTML_PAGE[]; // Forward declaration (sebenarnya tidak dipakai lagi jika split)
extern const char HTML_HEAD[];
extern const char HTML_BODY[];

const char HTML_HEAD[] PROGMEM = R"=====(
<!DOCTYPE html><html lang="id"><head>
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
input[type=text], input[type=password], select { width: 100%; padding: 10px; margin: 5px 0 15px 0; box-sizing: border-box; border-radius: 8px; border: 1px solid #475569; background: #334155; color: white; }
.form-card { text-align: left !important; grid-column: span 2; }
.footer { margin-top: 30px; text-align: center; font-size: 0.8rem; color: #475569; }
.alert-box { background: var(--danger); color: white; padding: 10px; border-radius: 8px; margin-bottom: 20px; display: none; text-align: center; font-weight: bold; }
</style></head>
)=====";

const char HTML_BODY[] PROGMEM = R"=====(
<body>
<div class="container">
<header><h1>Smart Class Panel</h1><div style="margin-top:5px; color:#94a3b8; font-size:0.9rem;"><span class="status-dot" id="dot"></span><span id="status_txt">Online</span> | <span id="ip">Loading...</span> | <span id="time">--:--</span></div></header>
<div id="alert" class="alert-box">⚠️ PERINGATAN: SUHU / GAS BERBAHAYA!</div>
<div class="grid">
<div class="card"><h3>Suhu</h3><div class="val" id="t">--</div><span class="unit">°C</span></div>
<div class="card"><h3>Kelembaban</h3><div class="val" id="h">--</div><span class="unit">%</span></div>
<div class="card"><h3>Udara</h3><div class="val" id="gas">--</div><span class="unit">PPM</span></div>
<div class="card"><h3>Kebisingan</h3><div class="val" id="db">--</div><span class="unit">dB</span></div>
<div class="card" style="grid-column: span 2;"><h3>Mood Kelas</h3><div class="val" id="mood" style="color: var(--primary);">Loading...</div></div>
<div class="card form-card"><h3>⚙️ Pengaturan Koneksi</h3><form action="/save" method="POST">
<label>Pilih WiFi:</label><div style="display:flex; gap:5px;"><select id="ssid_list" name="ssid"><option value="" disabled selected>-- Scan Dulu --</option></select><button type="button" onclick="scanWifi()" style="width:100px; padding:10px; background:#6366f1;">🔄 Scan</button></div>
<input type="text" id="ssid_manual" name="ssid_manual" placeholder="Atau ketik nama WiFi manual..." style="margin-top:-10px;">
<label>WiFi Password:</label><input type="password" name="pass" placeholder="Password WiFi">
<label>Bot Token:</label><input type="text" name="bot" placeholder="Telegram Bot Token">
<label>Chat ID Admin:</label><input type="text" name="id" placeholder="ID Admin Telegram">
<button type="submit" class="btn-on" style="background:#10b981; width:100%;">SIMPAN & RESTART ALAT</button></form></div></div>
<div class="controls"><div class="btn-group"><button onclick="cmd('fan_toggle')" class="btn-on">KIPAS <span id="s_fan">●</span></button><button onclick="cmd('lamp_toggle')" class="btn-on">LAMPU <span id="s_lamp">●</span></button></div>
<button onclick="cmd('auto_toggle')" style="background:#334155;">MODE OTOMATIS: <span id="s_auto">ON</span></button>
<div class="btn-group"><button onclick="cmd('music_play')" class="btn-music">♫ PLAY ZELDA</button><button onclick="cmd('music_stop')" class="btn-off">■ STOP</button></div></div>
<div class="footer"><a href="/csv" style="color:var(--primary); text-decoration:none;">📥 Download Laporan Excel (.csv)</a></div></div>
<script>
function update(){fetch('/data?_='+Date.now()).then(r=>r.json()).then(d=>{
document.getElementById('t').innerText=d.t.toFixed(1);document.getElementById('h').innerText=d.h.toFixed(0);
document.getElementById('gas').innerText=d.gas;document.getElementById('db').innerText=d.db.toFixed(1);
document.getElementById('mood').innerText=d.mood;document.getElementById('ip').innerText=window.location.hostname;
document.getElementById('time').innerText=d.time;
document.getElementById('s_fan').style.color=d.fan?'#10b981':'#64748b';document.getElementById('s_lamp').style.color=d.lamp?'#10b981':'#64748b';
document.getElementById('s_auto').innerText=d.auto?"ON":"MANUAL";
if(d.alert)document.getElementById('alert').style.display='block';else document.getElementById('alert').style.display='none';
document.getElementById('dot').style.backgroundColor='#10b981';document.getElementById('status_txt').innerText="Online";
}).catch(e=>{console.log(e);document.getElementById('dot').style.backgroundColor='#ef4444';document.getElementById('status_txt').innerText="Terputus...";});}
function scanWifi(){var s=document.getElementById('ssid_list');s.innerHTML="<option>Scanning...</option>";fetch('/scan').then(r=>r.json()).then(d=>{s.innerHTML="";if(d.length===0)s.innerHTML="<option>No WiFi</option>";else d.forEach(x=>{var o=document.createElement('option');o.value=x;o.innerText=x;s.appendChild(o);});}).catch(e=>{s.innerHTML="<option>Error</option>";});}
function cmd(a){fetch('/cmd?do='+a).then(update);}
setInterval(update,1000);update();
</script></body></html>
)=====";

void handleRoot() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  server.sendContent(HTML_HEAD);
  server.sendContent(HTML_BODY);
  server.sendContent("");
}
