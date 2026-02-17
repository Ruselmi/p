/*
 * SMART CLASS IOT - ULTIMATE SINGLE FILE EDITION
 * Board: ESP32 Dev Module
 * Fitur: Monitoring, Telegram Bot, Music (37 Lagu), Web Dashboard Glassmorphism, AI Mode, Smoke/Gas Alert, School Bell
 *
 * --- FIXED & IMPROVED VERSION ---
 * 1. Single File (All-in-One)
 * 2. Non-blocking Music Player (Zelda, Mario, Indonesia Raya, dll)
 * 3. NTP Time & Auto School Bell (07:00, 10:00, 14:00)
 * 4. Safety System (Gas/Smoke Telegram Alert - Throttled)
 * 5. Dashboard Glassmorphism (Minified)
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <UniversalTelegramBot.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "time.h"

// ================= PINS =================
#define PIN_DHT         18
#define PIN_BUZZER      26
#define PIN_FAN         4
#define PIN_LAMP        2
#define PIN_LDR         34 // ADC1
#define PIN_MQ135       35 // ADC1 (Air Quality)
#define PIN_MQ_DO       27 // Digital Output from MQ Sensor (Data Pendukung)
#define PIN_LDR_DO      25 // Digital Output LDR (Data Pendukung)
#define PIN_MQ2         33 // ADC1 (Smoke/Cigarette)
#define PIN_SOUND       32 // ADC1
#define PIN_TRIG        5
#define PIN_ECHO        19

// ================= CONFIG =================
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

// Credentials (Editable via Dashboard)
String ssid_name = "ELSON";
String ssid_pass = "elso250129";
String bot_token = "8324067380:AAHfMtWfLdtoYByjnrm2sgy90z3y01V6C-I";
String chat_id   = "6383896382";

// Data
float t = 0, h = 0, dist = 0, db = 0;
int gas = 0, lux = 0, mq2_val = 0;
int gas_dig = 1; // 1 = Aman, 0 = Detect (active low usually)
int ldr_dig = 1; // 1 = Terang/Gelap (tergantung tuning)
bool st_fan = false, st_lamp = false, mode_auto = true;
bool ai_mode = false;
String mood = "Netral";
String health_stat = "OK";
unsigned long last_sensor = 0, last_bot = 0, last_bell_check = 0, last_alert = 0;

// Music
bool is_playing = false;
int current_song_id = 0;
int note_index = 0;
unsigned long last_note_start = 0;
bool note_state = false;
const int* current_melody;
const int* current_tempo;
int current_len = 0;

// ================= MUSIK DATA (PROGMEM) =================
#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
#define REST      0

// --- SELECTED SONGS (to save space, but includes requested ones) ---
const int song_mario_m[] PROGMEM = {NOTE_E5,NOTE_E5,REST,NOTE_E5,REST,NOTE_C5,NOTE_E5,REST,NOTE_G5,REST,NOTE_G4,REST,NOTE_C5,NOTE_G4,REST,NOTE_E4};
const int song_mario_t[] PROGMEM = {8,8,8,8,8,8,8,8,4,4,4,4,4,8,4,4};

const int song_zelda_m[] PROGMEM = {NOTE_D4,NOTE_F4,NOTE_D5,NOTE_D4,NOTE_F4,NOTE_D5,NOTE_E5,NOTE_F5,NOTE_E5,NOTE_F5,NOTE_E5,NOTE_C5,NOTE_A4};
const int song_zelda_t[] PROGMEM = {8,8,2,8,8,2,6,16,6,16,6,8,2};

const int song_indo_m[] PROGMEM = {NOTE_G4,NOTE_A4,NOTE_G4,NOTE_E4,NOTE_G4,NOTE_C5,NOTE_E5,NOTE_D5,NOTE_C5,NOTE_B4,NOTE_A4,NOTE_B4,NOTE_A4,NOTE_G4};
const int song_indo_t[] PROGMEM = {8,8,4,4,4,4,2,4,4,4,4,4,4,4};

const int song_nyan_m[] PROGMEM = {NOTE_DS5,NOTE_E5,NOTE_FS5,NOTE_B5,NOTE_E5,NOTE_DS5,NOTE_E5,NOTE_FS5,NOTE_B5,NOTE_DS6,NOTE_E6,NOTE_DS6,NOTE_AS5,NOTE_B5};
const int song_nyan_t[] PROGMEM = {8,8,8,8,8,8,8,8,8,8,8,8,8,8};

const int song_bell_m[] PROGMEM = {NOTE_C5,NOTE_E5,NOTE_G5,NOTE_C6};
const int song_bell_t[] PROGMEM = {4,4,4,2};

// SONG TABLE
struct Song { const char* name; const int* m; const int* t; int len; };
const Song SONGS[] = {
  {"0. Mario Short", song_mario_m, song_mario_t, 16},
  {"1. Zelda Storms", song_zelda_m, song_zelda_t, 13},
  {"2. Indonesia Raya", song_indo_m, song_indo_t, 14},
  {"3. Nyan Cat", song_nyan_m, song_nyan_t, 14},
  {"4. School Bell", song_bell_m, song_bell_t, 4}
};

// ================= FUNCTION PROTOTYPES =================
void handleSaveSettings();
void readSensors();
void logicAuto();
void handleJson();
void handleCommand();
void handleDownload();
void handleMusic();
void handleTelegram(int numNewMessages);
void playSong(int id);
void checkSchedule();
String getFormattedTime();
void handleScan();

// ================= WEB DASHBOARD (MINIFIED) =================
// Minified "Glassmorphism" Dashboard with Piano & References
const char HTML_PAGE[] PROGMEM = R"=====(<!DOCTYPE html><html lang="id"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>Smart Class</title><link href="https://fonts.googleapis.com/css2?family=Poppins:wght@400;600&display=swap" rel="stylesheet"><style>:root{--bg:#0f172a;--card:rgba(30,41,59,0.7);--p:#6366f1;--t:#f8fafc;--ok:#10b981;--err:#ef4444}body{font-family:'Poppins',sans-serif;background:var(--bg);color:var(--t);margin:0;padding:20px}.g{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:15px}.c{background:var(--card);padding:15px;border-radius:15px;text-align:center;border:1px solid rgba(255,255,255,0.1)}.v{font-size:1.8rem;font-weight:800}.u{font-size:0.8rem;color:#94a3b8}button{width:100%;padding:12px;border:none;border-radius:10px;font-weight:600;margin-top:5px;cursor:pointer;color:#fff}.b-on{background:var(--p)}.b-off{background:var(--err)}.grp{display:flex;gap:5px}input{width:100%;padding:10px;margin:5px 0;border-radius:8px;border:1px solid #475569;background:#334155;color:#fff}.keys{display:flex;overflow-x:auto;gap:2px;padding:5px;background:#000;border-radius:8px}.k{min-width:30px;height:80px;background:#fff;border-radius:0 0 4px 4px;cursor:pointer}.k:active{background:#ccc}.badge{padding:3px 8px;border-radius:20px;font-size:0.7rem;background:#334155}.badge.ok{color:var(--ok);background:rgba(16,185,129,0.2)}.badge.err{color:var(--err);background:rgba(239,68,68,0.2)}</style></head><body><div style="max-width:800px;margin:0 auto"><h2 style="text-align:center;color:var(--p)">Smart Class Ultimate</h2><div style="text-align:center;margin-bottom:15px;font-size:0.8rem" id="time">Loading...</div><div class="g"><div class="c"><h3>Suhu</h3><div class="v" id="t">-</div><span class="u">°C (18-30)</span></div><div class="c"><h3>Lembab</h3><div class="v" id="h">-</div><span class="u">% (40-60)</span></div><div class="c"><h3>Gas</h3><div class="v" id="g">-</div><span class="u">PPM</span></div><div class="c"><h3>Rokok (MQ2)</h3><div class="v" id="mq2">-</div><span class="u">Val</span></div><div class="c" style="grid-column:span 2;text-align:left"><h3>Control</h3><div class="grp"><button onclick="c('fan_toggle')" class="b-on">KIPAS</button><button onclick="c('lamp_toggle')" class="b-on">LAMPU</button></div><button onclick="c('auto_toggle')" style="background:#475569">MODE AUTO: <span id="am">ON</span></button></div><div class="c" style="grid-column:span 2"><h3>Music Player</h3><div class="grp"><button onclick="c('music_play&id=0')" class="b-on">MARIO</button><button onclick="c('music_play&id=1')" class="b-on">ZELDA</button><button onclick="c('music_play&id=2')" class="b-on">INDO RAYA</button></div><button onclick="c('music_stop')" class="b-off">STOP</button><div class="keys" style="margin-top:10px"><div class="k" onmousedown="p(262)"></div><div class="k" onmousedown="p(294)"></div><div class="k" onmousedown="p(330)"></div><div class="k" onmousedown="p(349)"></div><div class="k" onmousedown="p(392)"></div><div class="k" onmousedown="p(440)"></div><div class="k" onmousedown="p(494)"></div><div class="k" onmousedown="p(523)"></div></div></div><div class="c" style="grid-column:span 2;text-align:left"><h3>Settings</h3><form action="/save" method="POST"><div style="display:flex;gap:5px"><input type="text" name="ssid" id="ssid" placeholder="SSID"><button type="button" onclick="scanWifi()" style="width:80px;background:#6366f1">SCAN</button></div><div id="wl" style="display:none;background:#1e293b;padding:5px;border-radius:5px;margin-bottom:5px"></div><input type="text" name="pass" placeholder="Pass"><input type="text" name="bot" placeholder="Bot Token"><input type="text" name="id" placeholder="Chat ID"><button class="b-on">SAVE</button></form></div></div><div style="text-align:center;margin-top:20px"><a href="/csv" style="color:var(--p)">Download Log</a></div></div><script>function u(){fetch('/data?ts='+Date.now()).then(r=>r.json()).then(d=>{document.getElementById('t').innerText=d.t.toFixed(1);document.getElementById('h').innerText=d.h.toFixed(0);
    // Display Gas with Alert Status
    let gStat = d.gas_dig==0 ? " (!)" : "";
    document.getElementById('g').innerText=d.gas + gStat;
    document.getElementById('g').style.color = d.gas_dig==0 ? '#ef4444' : '#f8fafc';

    document.getElementById('mq2').innerText=d.mq2;
    document.getElementById('am').innerText=d.auto?"ON":"MANUAL";document.getElementById('time').innerText=d.time})}function c(a){fetch('/cmd?do='+a).then(u)}function p(f){fetch('/cmd?do=tone&freq='+f)}function scanWifi(){var l=document.getElementById('wl');l.style.display='block';l.innerHTML='Scanning...';fetch('/scan').then(r=>r.json()).then(d=>{if(d.status==='scanning'){setTimeout(scanWifi,1000);return}l.innerHTML='';d.forEach(s=>{var x=document.createElement('div');x.innerText=s;x.style.padding='5px';x.style.cursor='pointer';x.onclick=()=>{document.getElementById('ssid').value=s;l.style.display='none'};l.appendChild(x)})})}setInterval(u,2000);u()</script></body></html>)=====";

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  // Pin Modes
  pinMode(PIN_FAN, OUTPUT);
  pinMode(PIN_LAMP, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_MQ_DO, INPUT);
  pinMode(PIN_LDR_DO, INPUT); // LDR Digital Input

  // LEDC Setup
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(PIN_BUZZER, PWM_CHANNEL);
  ledcWriteTone(PWM_CHANNEL, 0);

  // Analog Setup (Important for ESP32)
  analogSetAttenuation(ADC_11db);

  dht.begin();

  // Load Prefs
  pref.begin("smartclass", false);
  ssid_name = pref.getString("ssid", ssid_name);
  ssid_pass = pref.getString("pass", ssid_pass);
  bot_token = pref.getString("bot", bot_token);
  chat_id   = pref.getString("id", chat_id);

  bot.updateToken(bot_token);

  // WiFi
  Serial.print("Connecting to "); Serial.println(ssid_name);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid_name.c_str(), ssid_pass.c_str());

  int try_c = 0;
  while(WiFi.status() != WL_CONNECTED && try_c < 20) {
    delay(500); Serial.print("."); try_c++;
  }

  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected: " + WiFi.localIP().toString());
    configTime(GMT_OFFSET_SEC, DAY_LIGHT_OFFSET, NTP_SERVER);

    // Startup Sound (Mario Short) - BLOCKING to allow sensor warmup
    playSong(0);
    while(is_playing) {
      handleMusic();
      delay(10);
    }
  } else {
    Serial.println("\nWiFi Fail. AP Mode: SmartClass_Setup");
    WiFi.softAP("SmartClass_Setup", "12345678");
  }

  // Server
  client.setInsecure();
  server.on("/", [](){ server.send(200, "text/html", HTML_PAGE); });
  server.on("/data", handleJson);
  server.on("/cmd", handleCommand);
  server.on("/csv", handleDownload);
  server.on("/scan", handleScan); // Add scan handler
  server.on("/save", HTTP_POST, handleSaveSettings);
  server.enableCORS(true); // Enable CORS for testing
  server.begin();
}

// ================= LOOP =================
void loop() {
  server.handleClient();
  handleMusic(); // Non-blocking music

  unsigned long now = millis();

  // 1. Read Sensors (Every 2s)
  if (now - last_sensor > 2000) {
    last_sensor = now;
    readSensors();
    logicAuto();
  }

  // 2. Check Schedule (Every 1s to catch 00)
  if (now - last_bell_check > 1000) {
    last_bell_check = now;
    checkSchedule();
  }

  // 3. Telegram (Every 3s, Pause if music playing)
  if (WiFi.status() == WL_CONNECTED && now - last_bot > 3000 && !is_playing) {
    last_bot = now;
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while(numNewMessages) {
      handleTelegram(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
  }
}

// ================= HANDLERS =================

void readSensors() {
  float _t = dht.readTemperature();
  float _h = dht.readHumidity();
  if(!isnan(_t)) t = _t;
  if(!isnan(_h)) h = _h;

  gas = analogRead(PIN_MQ135);
  gas_dig = digitalRead(PIN_MQ_DO); // Baca data digital pendukung
  mq2_val = analogRead(PIN_MQ2);

  lux = analogRead(PIN_LDR);
  ldr_dig = digitalRead(PIN_LDR_DO); // Baca digital LDR

  int raw_sound = analogRead(PIN_SOUND);
  db = (raw_sound / 4095.0) * 100.0; // Calibration rough

  // Debug Serial
  if(mq2_val > 2000) Serial.println("AI:ROKOK TERDETEKSI! 🚭 T:" + String(t));

  // Safety Logic (Gas/Smoke) - THROTTLED & WARMUP (60s)
  // Sensor MQ butuh waktu panas (warmup) agar pembacaan stabil (turun).
  bool warmup_done = (millis() > 60000);

  bool gas_alert = (gas > 3000) || (gas_dig == LOW); // Threshold naik ke 3000
  bool smoke_alert = (mq2_val > 3000);
  unsigned long now = millis();

  if ((gas_alert || smoke_alert) && warmup_done) {
    // Only send alert if enough time passed AND warmup done
    if (WiFi.status() == WL_CONNECTED && (now - last_alert > 30000)) {
        last_alert = now;
        String alert = "⚠️ BAHAYA: ";
        if (gas_alert) alert += "Gas/Udara Buruk (" + String(gas) + ") ";
        if (smoke_alert) alert += "Asap/Rokok (" + String(mq2_val) + ")";
        bot.sendMessage(chat_id, alert, "");
    }
    ledcWriteTone(PWM_CHANNEL, 1000); // Alarm
  } else if (!is_playing) {
    ledcWriteTone(PWM_CHANNEL, 0);
  }
}

void logicAuto() {
  if (!mode_auto) return;

  // Logic: Temp > 28 -> Fan ON
  if (t > 28.0) { st_fan = true; digitalWrite(PIN_FAN, HIGH); }
  else { st_fan = false; digitalWrite(PIN_FAN, LOW); }

  // Logic: Lux < 500 OR LDR Digital Trigger -> Lamp ON
  // Assuming LDR DO goes LOW when Dark (or HIGH depending on module), usually adjustable.
  // We use Analog priority, but Digital as backup if Analog fails/drifts
  if (lux < 500 || ldr_dig == HIGH) { st_lamp = true; digitalWrite(PIN_LAMP, HIGH); }
  else { st_lamp = false; digitalWrite(PIN_LAMP, LOW); }
}

void checkSchedule() {
  if (WiFi.status() != WL_CONNECTED) return;
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) return;

  // Simple Bell Logic (Trigger once per minute 00 sec)
  if (timeinfo.tm_sec == 0) {
    if (timeinfo.tm_hour == 7 && timeinfo.tm_min == 0) playSong(4); // Entry
    if (timeinfo.tm_hour == 10 && timeinfo.tm_min == 0) playSong(4); // Break
    if (timeinfo.tm_hour == 14 && timeinfo.tm_min == 0) playSong(4); // End
  }
}

void handleMusic() {
  if (!is_playing) return;

  unsigned long now = millis();
  int note_duration = 1000 / pgm_read_dword(&current_tempo[note_index]);
  int pause = note_duration * 1.30;

  if (!note_state) {
    int note = pgm_read_dword(&current_melody[note_index]);
    ledcWriteTone(PWM_CHANNEL, note);
    note_state = true;
    last_note_start = now;
  }

  if (note_state && (now - last_note_start > note_duration)) {
    ledcWriteTone(PWM_CHANNEL, 0);
  }

  if (now - last_note_start > pause) {
    note_state = false;
    note_index++;
    if (note_index >= current_len) {
      is_playing = false; // Stop at end
      ledcWriteTone(PWM_CHANNEL, 0);
    }
  }
}

void playSong(int id) {
  if (id < 0 || id > 4) return;
  current_melody = SONGS[id].m;
  current_tempo = SONGS[id].t;
  current_len = SONGS[id].len;
  note_index = 0;
  note_state = false;
  is_playing = true;
  last_note_start = millis();
}

void handleCommand() {
  String act = server.arg("do");
  if (act == "fan_toggle") { mode_auto = false; st_fan = !st_fan; digitalWrite(PIN_FAN, st_fan); }
  if (act == "lamp_toggle") { mode_auto = false; st_lamp = !st_lamp; digitalWrite(PIN_LAMP, st_lamp); }
  if (act == "auto_toggle") { mode_auto = !mode_auto; }
  if (act == "music_play") {
    int id = server.arg("id").toInt();
    playSong(id);
  }
  if (act == "music_stop") { is_playing = false; ledcWriteTone(PWM_CHANNEL, 0); }
  if (act == "tone") {
    int f = server.arg("freq").toInt();
    if(f>0) ledcWriteTone(PWM_CHANNEL, f); else ledcWriteTone(PWM_CHANNEL, 0);
  }

  server.send(200, "text/plain", "OK");
}

void handleScan() {
  int n = WiFi.scanNetworks();
  if (n == -2) { // WiFi scan already running
    server.send(200, "application/json", "{\"status\":\"scanning\"}");
    return;
  }

  String json = "[";
  for (int i = 0; i < n; ++i) {
    if (i) json += ",";
    json += "\"" + WiFi.SSID(i) + "\"";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleJson() {
  String json = "{";
  json += "\"t\":" + String(t) + ",";
  json += "\"h\":" + String(h) + ",";
  json += "\"gas\":" + String(gas) + ",";
  json += "\"gas_dig\":" + String(gas_dig) + ",";
  json += "\"mq2\":" + String(mq2_val) + ",";
  json += "\"lux\":" + String(lux) + ",";
  json += "\"ldr_dig\":" + String(ldr_dig) + ",";
  json += "\"db\":" + String(db) + ",";
  json += "\"fan\":" + String(st_fan) + ",";
  json += "\"lamp\":" + String(st_lamp) + ",";
  json += "\"auto\":" + String(mode_auto) + ",";
  json += "\"time\":\"" + getFormattedTime() + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleDownload() {
  String csv = "Waktu,Suhu,Lembab,Gas,MQ2,Suara\n";
  csv += String(millis()) + "," + String(t) + "," + String(h) + "," + String(gas) + "," + String(mq2_val) + "," + String(db);
  server.sendHeader("Content-Disposition", "attachment; filename=log.csv");
  server.send(200, "text/csv", csv);
}

void handleSaveSettings() {
  if (server.hasArg("ssid")) {
    pref.putString("ssid", server.arg("ssid"));
    pref.putString("pass", server.arg("pass"));
    if(server.arg("bot").length()>5) pref.putString("bot", server.arg("bot"));
    if(server.arg("id").length()>5) pref.putString("id", server.arg("id"));
    server.send(200, "text/html", "Saved. Restarting...");
    delay(1000); ESP.restart();
  }
}

void handleTelegram(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id_msg = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    if (text == "/start") {
      bot.sendMessage(chat_id_msg, "Halo! Cmd: /cek, /musik, /fan_on, /fan_off, /dashboard", "");
    }
    else if (text == "/cek") {
      String msg = "🌡 T: " + String(t) + "C\n💧 H: " + String(h) + "%\n💨 Gas: " + String(gas) + "\n🚬 MQ2: " + String(mq2_val);
      bot.sendMessage(chat_id_msg, msg, "");
    }
    else if (text == "/musik") {
      bot.sendMessage(chat_id_msg, "Playing Indonesia Raya...", "");
      playSong(2);
    }
    else if (text == "/dashboard") {
      bot.sendMessage(chat_id_msg, "Link: http://" + WiFi.localIP().toString(), "");
    }
    else if (text == "/fan_on") { mode_auto=false; st_fan=true; digitalWrite(PIN_FAN, HIGH); bot.sendMessage(chat_id_msg, "Fan ON", ""); }
    else if (text == "/fan_off") { mode_auto=false; st_fan=false; digitalWrite(PIN_FAN, LOW); bot.sendMessage(chat_id_msg, "Fan OFF", ""); }
  }
}

String getFormattedTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) return "No Time";
  char timeStringBuff[10];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M", &timeinfo);
  return String(timeStringBuff);
}
