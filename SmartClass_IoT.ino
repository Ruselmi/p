/*
 * SMART CLASS IOT - MINIMALIST EDITION (Single File)
 * Board: ESP32 Dev Module
 * Fitur: Monitoring Sensor (Smoothed), Telegram Bot, Web Dashboard (Enhanced), Music Player (Non-Blocking), WiFi Manager (Auto-Reconnect)
 *
 * Update:
 * - FIXED: Function Prototypes added.
 * - SYSTEM: WiFi Auto-Reconnect & RSSI Monitoring added.
 * - SENSOR: Averaging filter for analog sensors (Gas, LDR, Sound) for stability.
 * - AUDIO: PWM LEDC system (ledcWriteTone) optimized.
 * - PIN: Buzzer updated to 26.
 * - SOUND: Startup (Mario) & Default (Zelda).
 *
 * NEW FEATURES:
 * - NTP Time: Auto-Sync Waktu (WIB UTC+7).
 * - JADWAL OTOMATIS: Sistem hanya aktif jam 07:00 - 17:00 (Hemat Energi).
 * - NOTIFIKASI BAHAYA: Kirim Telegram jika Gas > 2500 atau Suhu > 35°C.
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <UniversalTelegramBot.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h> // Library Waktu bawaan ESP32

// ================= KONFIGURASI PIN =================
#define PIN_DHT         18
#define PIN_BUZZER      26  // Update ke 26 sesuai request
#define PIN_FAN         4
#define PIN_LAMP        2
#define PIN_LDR         34
#define PIN_MQ135       35
#define PIN_SOUND       32
#define PIN_TRIG        5
#define PIN_ECHO        19

// ================= KONFIGURASI PWM BUZZER =================
#define PWM_CHANNEL     0
#define PWM_RESOLUTION  8
#define PWM_FREQ        2000

#define DHTTYPE         DHT22

// ================= KONFIGURASI ALARM & WAKTU =================
const int GAS_ALARM_LIMIT   = 2500; // Batas Bahaya Gas
const float TEMP_ALARM_LIMIT = 35.0; // Batas Bahaya Suhu
const long GMT_OFFSET_SEC   = 25200; // WIB (UTC+7) = 7 * 3600
const int DAYLIGHT_OFFSET   = 0;

// ================= VARIABEL GLOBAL =================
DHT dht(PIN_DHT, DHTTYPE);
WebServer server(80);
WiFiClientSecure client;
UniversalTelegramBot bot("", client);
Preferences pref;

// Default values
String ssid_name = "ELSON";
String ssid_pass = "elso250129";
String bot_token = "8324067380:AAHfMtWfLdtoYByjnrm2sgy90z3y01V6C-I";
String chat_id   = "6383896382";

float t = 0, h = 0, lux = 0, dist = 0, db = 0;
int gas = 0;
long rssi = 0; // Signal strength
bool st_fan = false, st_lamp = false, mode_auto = true;
String mood = "Netral";
String time_str = "--:--"; // String jam
bool is_day_time = true;   // Status siang/malam
bool alert_active = false; // Status bahaya

// Timer variables
unsigned long last_sensor = 0;
unsigned long last_bot = 0;
unsigned long last_wifi_check = 0;
unsigned long last_alert_time = 0; // Anti-spam notif

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

// --- ZELDA: SONG OF STORMS (untuk tombol Play Melody) ---
int melody_theme[] = {
  NOTE_D4, NOTE_F4, NOTE_D5,
  NOTE_D4, NOTE_F4, NOTE_D5,
  NOTE_E5, NOTE_F5, NOTE_E5, NOTE_F5, NOTE_E5, NOTE_C5, NOTE_A4
};
// Durasi (4 = 1/4 ketukan, 8 = 1/8 ketukan, dst)
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

// ================= FUNCTION PROTOTYPES (PENTING!) =================
void handleSaveSettings();
void readSensors();
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

// ================= WEB DASHBOARD =================
const char HTML_PAGE[] PROGMEM = R"=====(
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
    .alert-box { background: var(--danger); color: white; padding: 10px; border-radius: 8px; margin-bottom: 20px; display: none; text-align: center; font-weight: bold; }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <h1>Smart Class Panel</h1>
      <div style="margin-top:5px; color:#94a3b8; font-size:0.9rem;">
        <span class="status-dot"></span>Online | <span id="ip">Loading...</span> | <span id="time">--:--</span>
      </div>
    </header>

    <div id="alert" class="alert-box">⚠️ PERINGATAN: SUHU / GAS BERBAHAYA!</div>

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
          <input type="text" name="ssid" placeholder="Nama WiFi" required>

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
      fetch('/data').then(r => r.json()).then(d => {
        document.getElementById('t').innerText = d.t.toFixed(1);
        document.getElementById('h').innerText = d.h.toFixed(0);
        document.getElementById('gas').innerText = d.gas;
        document.getElementById('db').innerText = d.db.toFixed(1);
        document.getElementById('mood').innerText = d.mood;
        document.getElementById('ip').innerText = window.location.hostname;
        document.getElementById('time').innerText = d.time;

        document.getElementById('s_fan').style.color = d.fan ? '#10b981' : '#64748b';
        document.getElementById('s_lamp').style.color = d.lamp ? '#10b981' : '#64748b';
        document.getElementById('s_auto').innerText = d.auto ? "ON" : "MANUAL";

        if(d.alert) {
          document.getElementById('alert').style.display = 'block';
        } else {
          document.getElementById('alert').style.display = 'none';
        }
      });
    }
    function cmd(act) { fetch('/cmd?do='+act).then(update); }
    setInterval(update, 2000); update();
  </script>
</body>
</html>
)=====";

// ================= LOGIKA UTAMA =================

void setup() {
  Serial.begin(115200);

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

  // 3. Koneksi WiFi (Non-Blocking Attempt)
  Serial.print("Menghubungkan ke: "); Serial.println(ssid_name);

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid_name.c_str(), ssid_pass.c_str());

  // Wait a bit for connection but don't hang forever
  int try_count = 0;
  while (WiFi.status() != WL_CONNECTED && try_count < 20) {
    delay(250); Serial.print(".");
    try_count++;
  }

  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Terhubung!");
    Serial.print("IP Address: "); Serial.println(WiFi.localIP());
    // Init NTP Time
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET, "pool.ntp.org", "time.nist.gov");
  } else {
    Serial.println("\nGagal Connect. Masuk Mode Setup AP + Auto Reconnect Loop.");
    WiFi.softAP("SmartClass_AP");
    Serial.print("IP AP: "); Serial.println(WiFi.softAPIP());
  }

  // 4. Setup Server
  client.setInsecure();
  server.on("/", [](){ server.send(200, "text/html", HTML_PAGE); });
  server.on("/data", handleJson);
  server.on("/cmd", handleCommand);
  server.on("/csv", handleDownload);
  server.on("/save", HTTP_POST, handleSaveSettings);
  server.begin();

  // --- STARTUP SOUND: MARIO INTRO ---
  int mario_notes[] = {NOTE_E5, NOTE_E5, NOTE_E5, NOTE_C5, NOTE_E5, NOTE_G5, NOTE_G4};
  int mario_durs[]  = {100, 100, 100, 100, 100, 200, 200}; // Durasi ms

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
  checkWiFi(); // Auto reconnect check

  unsigned long now = millis();

  // 1. Baca Sensor (Setiap 2 detik)
  if (now - last_sensor > 2000) {
    last_sensor = now;
    readSensors();
    updateTime(); // Cek Waktu NTP
    logicAuto();  // Logika Otomatis + Jadwal
    checkAlert(); // Cek Bahaya
  }

  // 2. Cek Telegram (Setiap 3 detik jika konek & TIDAK SEDANG MUSIK)
  // Mencegah musik patah-patah karena proses Telegram yang berat
  if (!is_playing && WiFi.status() == WL_CONNECTED && now - last_bot > 3000) {
    last_bot = now;
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while(numNewMessages) {
      handleTelegram(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
  }
}

// ================= FUNGSI-FUNGSI =================

void updateTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    time_str = "--:--";
    return;
  }
  char timeStringBuff[10];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M", &timeinfo);
  time_str = String(timeStringBuff);

  // Tentukan Siang/Malam (07:00 - 17:00 Aktif)
  int hour = timeinfo.tm_hour;
  if (hour >= 7 && hour < 17) {
    is_day_time = true;
  } else {
    is_day_time = false;
  }
}

void checkWiFi() {
  // Cek koneksi setiap 10 detik. Jika putus, coba reconnect.
  unsigned long now = millis();
  if (now - last_wifi_check > 10000) {
    last_wifi_check = now;
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi Putus! Mencoba reconnect...");
      WiFi.disconnect();
      WiFi.reconnect();
    }
  }
}

int smoothAnalog(int pin) {
  long total = 0;
  int samples = 10;
  for (int i = 0; i < samples; i++) {
    total += analogRead(pin);
    delay(2); // delay kecil untuk stabilisasi ADC
  }
  return total / samples;
}

void handleSaveSettings() {
  if (server.hasArg("ssid")) {
    String n_ssid = server.arg("ssid");
    String n_pass = server.arg("pass");
    String n_bot  = server.arg("bot");
    String n_id   = server.arg("id");

    pref.putString("ssid", n_ssid);
    pref.putString("pass", n_pass);
    if(n_bot.length() > 5) pref.putString("bot", n_bot);
    if(n_id.length() > 5)  pref.putString("id", n_id);

    String html = "<html><body><h1>Berhasil Disimpan!</h1><p>Alat akan restart...</p></body></html>";
    server.send(200, "text/html", html);
    delay(2000);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Error");
  }
}

void readSensors() {
  float _t = dht.readTemperature();
  float _h = dht.readHumidity();
  if (!isnan(_t)) t = _t;
  if (!isnan(_h)) h = _h;

  // Baca sensor dengan averaging (smoothing)
  gas = smoothAnalog(PIN_MQ135);
  lux = smoothAnalog(PIN_LDR);

  // Ultrasonik
  digitalWrite(PIN_TRIG, LOW); delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  long dur = pulseIn(PIN_ECHO, HIGH, 30000); // 30ms timeout
  dist = (dur == 0) ? 0 : dur * 0.034 / 2;

  // Suara (Smooth)
  int raw_sound = smoothAnalog(PIN_SOUND);
  db = (raw_sound / 4095.0) * 100.0; // Kalibrasi sederhana

  // RSSI WiFi
  rssi = WiFi.RSSI();

  // Logika Mood
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
    // Bahaya baru terdeteksi -> Kirim Telegram
    String msg = "⚠️ *PERINGATAN BAHAYA!*\n\n";
    if(gas > GAS_ALARM_LIMIT) msg += "🔥 Gas Tinggi: " + String(gas) + " PPM\n";
    if(t > TEMP_ALARM_LIMIT)  msg += "🌡 Suhu Panas: " + String(t) + " °C\n";
    msg += "\nSegera cek lokasi!";

    bot.sendMessage(chat_id, msg, "Markdown");
    alert_active = true;
  }
  else if (!current_danger && alert_active) {
    // Bahaya selesai -> Kirim Telegram
    bot.sendMessage(chat_id, "✅ *KONDISI AMAN*\nSensor kembali normal.", "Markdown");
    alert_active = false;
  }
}

void logicAuto() {
  if (!mode_auto) return;

  // FITUR BARU: JADWAL OTOMATIS (Hanya aktif 07:00 - 17:00)
  // Di luar jam itu, matikan semua untuk hemat energi (kecuali ada bahaya, tp bahaya di handle checkAlert)
  if (!is_day_time) {
    st_fan = false;
    st_lamp = false;
    digitalWrite(PIN_FAN, LOW);
    digitalWrite(PIN_LAMP, LOW);
    return;
  }

  // Logika Normal (Siang Hari)
  if (t > 27.0) { st_fan = true; digitalWrite(PIN_FAN, HIGH); }
  else { st_fan = false; digitalWrite(PIN_FAN, LOW); }

  if (lux < 500) { st_lamp = true; digitalWrite(PIN_LAMP, HIGH); }
  else { st_lamp = false; digitalWrite(PIN_LAMP, LOW); }
}

void handleJson() {
  String json = "{";
  json += "\"t\":" + String(t) + ",";
  json += "\"h\":" + String(h) + ",";
  json += "\"gas\":" + String(gas) + ",";
  json += "\"db\":" + String(db) + ",";
  json += "\"rssi\":" + String(rssi) + ",";
  json += "\"fan\":" + String(st_fan) + ",";
  json += "\"lamp\":" + String(st_lamp) + ",";
  json += "\"auto\":" + String(mode_auto) + ",";
  json += "\"mood\":\"" + mood + "\",";
  json += "\"time\":\"" + time_str + "\",";
  json += "\"alert\":" + String(alert_active ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

void handleCommand() {
  String act = server.arg("do");
  if (act == "fan_toggle") { mode_auto = false; st_fan = !st_fan; digitalWrite(PIN_FAN, st_fan); }
  if (act == "lamp_toggle") { mode_auto = false; st_lamp = !st_lamp; digitalWrite(PIN_LAMP, st_lamp); }
  if (act == "auto_toggle") { mode_auto = !mode_auto; }
  if (act == "music_play") { is_playing = true; note_index = 0; note_state = false; last_note_start = millis(); }
  if (act == "music_stop") { is_playing = false; ledcWriteTone(PWM_CHANNEL, 0); }
  server.send(200, "text/plain", "OK");
}

void handleDownload() {
  String csv = "Waktu,Suhu,Lembab,Gas,Suara,RSSI\n";
  csv += String(millis()) + "," + String(t) + "," + String(h) + "," + String(gas) + "," + String(db) + "," + String(rssi);
  server.sendHeader("Content-Disposition", "attachment; filename=log_kelas.csv");
  server.send(200, "text/csv", csv);
}

// LOGIKA MUSIK NON-BLOCKING DENGAN LEDC
void handleMusic() {
  if (!is_playing) return;

  int note_duration = 1000 / durations_theme[note_index];
  int pause_between = note_duration * 1.30;
  unsigned long now = millis();

  if (!note_state) {
    // Mulai mainkan nada
    ledcWriteTone(PWM_CHANNEL, melody_theme[note_index]);
    note_state = true;
    last_note_start = now;
  }

  // Matikan nada setelah durasi 'note_duration'
  if (note_state && (now - last_note_start > note_duration)) {
    ledcWriteTone(PWM_CHANNEL, 0);
  }

  // Pindah ke nada berikutnya setelah jeda 'pause_between'
  if (now - last_note_start > pause_between) {
    note_state = false; // Reset state untuk nada baru
    note_index++;
    if (note_index >= melody_len) { note_index = 0; }
  }
}

void handleTelegram(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id_msg = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

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
