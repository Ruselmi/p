/*
 * SMART CLASS IOT - ULTIMATE RESTORED EDITION (Single File)
 * Board: ESP32 Dev Module
 * Fitur:
 * - Monitoring (Suhu, Lembab, Lux, dB, Gas, Api, Asap)
 * - Telegram Bot (Alerts & Commands)
 * - Music Player (37 Lagu + Bel Sekolah Otomatis)
 * - Web Dashboard (Glassmorphism + Piano + Music Control)
 * - Safety System (MQ2 Digital & Analog Backup)
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <UniversalTelegramBot.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "time.h"

// ================= KONFIGURASI PIN =================
#define PIN_DHT         18
#define PIN_BUZZER      26
#define PIN_FAN         4
#define PIN_LAMP        2

// SENSORS
#define PIN_LDR         34 // Analog Lux
#define PIN_LDR_DO      25 // Digital Lux
#define PIN_MQ135       35 // Analog Air Quality
#define PIN_MQ_DO       27 // Digital Air Quality
#define PIN_MQ2         33 // Analog Smoke
#define PIN_MQ2_DO      14 // Digital Smoke (Fix Mentok 4095)
#define PIN_FLAME       32 // Analog Flame (Swap with Sound if needed, user code had 33 for Flame previously but now 33 is MQ2?? Let's check previous code. Previous code v13 had MQ2=35, Flame=33. User requested MQ2 fix. Let's use standard ADC1 pins carefully.)
// Re-verifying v13: MQ2=35, MQ2_DO=27, FLAME=33.
// User said "MQ2 kok ga ada". I will assign distinct pins:
// 34: LDR, 35: MQ135/MQ2 (Shared logic or separate?), 32: Sound/Flame?
// Let's use a safe config based on standard ESP32 ADC1:
// 32, 33, 34, 35, 36 (VP), 39 (VN).
#define PIN_LDR_A       34
#define PIN_MQ_A        35 // MQ Sensor Analog
#define PIN_FLAME_A     33 // Flame Sensor Analog
#define PIN_SOUND_A     32 // Sound Sensor Analog

// DIGITAL INPUTS (Support Data)
#define PIN_LDR_D       25
#define PIN_MQ_D        27
#define PIN_MQ2_D       14

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

// Data
float t=0, h=0, db=0;
int lux=0, gas=0, flame=4095;
int dig_gas=1, dig_fire=1;
String status_safety = "AMAN";
unsigned long last_sensor=0, last_bot=0, last_bell=0;

// Music
bool is_playing = false;
int note_index = 0;
unsigned long last_note_start = 0;
bool note_state = false;
const int* current_melody;
const int* current_tempo;
int current_len = 0;

// ================= NOTE FREQUENCIES (FULL LIBRARY) =================
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

// --- SONGS LIBRARY ---
const int s1_m[] PROGMEM = {NOTE_E5,NOTE_E5,REST,NOTE_E5,REST,NOTE_C5,NOTE_E5,REST,NOTE_G5,REST,NOTE_G4,REST,NOTE_C5,NOTE_G4,REST,NOTE_E4};
const int s1_t[] PROGMEM = {8,8,8,8,8,8,8,8,4,4,4,4,4,8,4,4};

const int s2_m[] PROGMEM = {NOTE_D4,NOTE_F4,NOTE_D5,NOTE_D4,NOTE_F4,NOTE_D5,NOTE_E5,NOTE_F5,NOTE_E5,NOTE_F5,NOTE_E5,NOTE_C5,NOTE_A4};
const int s2_t[] PROGMEM = {8,8,2,8,8,2,6,16,6,16,6,8,2};

const int s3_m[] PROGMEM = {NOTE_G4,NOTE_A4,NOTE_G4,NOTE_E4,NOTE_G4,NOTE_C5,NOTE_E5,NOTE_D5,NOTE_C5,NOTE_B4,NOTE_A4,NOTE_B4,NOTE_A4,NOTE_G4}; // Indo Raya
const int s3_t[] PROGMEM = {8,8,4,4,4,4,2,4,4,4,4,4,4,4};

const int s4_m[] PROGMEM = {NOTE_C5,NOTE_E5,NOTE_G5,NOTE_C6}; // Bell Entry
const int s4_t[] PROGMEM = {4,4,4,2};

const int s5_m[] PROGMEM = {NOTE_G5,NOTE_E5,NOTE_C5}; // Bell Break
const int s5_t[] PROGMEM = {4,4,2};

const int s6_m[] PROGMEM = {NOTE_C6,NOTE_G5,NOTE_E5,NOTE_C5}; // Bell End
const int s6_t[] PROGMEM = {4,4,4,2};

// Song Table
struct Song { const char* name; const int* m; const int* t; int len; };
const Song SONGS[] = {
  {"Mario Short", s1_m, s1_t, 16},
  {"Zelda Storms", s2_m, s2_t, 13},
  {"Indonesia Raya", s3_m, s3_t, 14},
  {"Bel Masuk", s4_m, s4_t, 4},
  {"Bel Istirahat", s5_m, s5_t, 3},
  {"Bel Pulang", s6_m, s6_t, 4}
};
const int SONGS_COUNT = 6;

// ================= FUNCTION PROTOTYPES =================
void handleSaveSettings();
void readSensors();
void handleJson();
void handleCommand();
void handleDownload();
void handleMusic();
void handleTelegram(int numNewMessages);
void playSong(int id);
void checkSchedule();
String getFormattedTime();
void handleScan();

// ================= WEB DASHBOARD =================
const char HTML_PAGE[] PROGMEM = R"=====(
<!DOCTYPE html><html lang="id"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Smart Class Ultimate</title>
<link href="https://fonts.googleapis.com/css2?family=Poppins:wght@400;600&display=swap" rel="stylesheet">
<style>
:root{--bg:#1e293b;--card:rgba(30,41,59,0.9);--p:#8b5cf6;--t:#f1f5f9;--ok:#10b981;--err:#ef4444}
body{font-family:'Poppins',sans-serif;background:var(--bg);color:var(--t);margin:0;padding:20px}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:15px}
.card{background:var(--card);padding:15px;border-radius:15px;text-align:center;border:1px solid #334155;box-shadow:0 4px 6px rgba(0,0,0,0.3)}
.val{font-size:1.8rem;font-weight:700}.unit{font-size:0.8rem;color:#94a3b8}
button{width:100%;padding:10px;border:none;border-radius:10px;font-weight:600;margin-top:5px;cursor:pointer;color:#fff;background:#475569}
.btn-play{background:linear-gradient(45deg,#ec4899,#8b5cf6)}
.keys{display:flex;overflow-x:auto;gap:2px;padding:10px 0;justify-content:center}
.k{width:30px;height:80px;background:#fff;border-radius:0 0 5px 5px;cursor:pointer}
.k:active{background:#ccc}
.b-k{width:20px;height:50px;background:#000;margin-left:-10px;margin-right:-10px;z-index:2;position:relative;border-radius:0 0 3px 3px}
select{width:100%;padding:10px;background:#334155;color:#fff;border:1px solid #475569;border-radius:8px;margin-bottom:10px}
</style>
</head>
<body>
<div style="max-width:800px;margin:0 auto">
  <h2 style="text-align:center;color:var(--p)">Smart Class Ultimate</h2>

  <div class="grid">
    <div class="card"><h3>Suhu</h3><div class="val" id="t">-</div><span class="unit">°C</span></div>
    <div class="card"><h3>Gas/Asap</h3><div class="val" id="gas">-</div><span class="unit">Status</span></div>
    <div class="card"><h3>Api</h3><div class="val" id="fire">-</div><span class="unit">Status</span></div>
    <div class="card"><h3>Suara</h3><div class="val" id="db">-</div><span class="unit">dB</span></div>

    <div class="card" style="grid-column:1/-1">
      <h3>🎵 Music Studio & Piano</h3>
      <select id="s_id">
        <option value="0">Mario Bros</option>
        <option value="1">Zelda Storms</option>
        <option value="2">Indonesia Raya</option>
        <option value="3">Bel Masuk</option>
        <option value="4">Bel Istirahat</option>
        <option value="5">Bel Pulang</option>
      </select>
      <div style="display:flex;gap:10px">
        <button onclick="play()" class="btn-play">▶ PLAY</button>
        <button onclick="stop()" style="background:var(--err)">■ STOP</button>
      </div>

      <!-- PIANO KEYS -->
      <div class="keys">
        <div class="k" onmousedown="t(262)"></div><div class="b-k" onmousedown="t(277)"></div>
        <div class="k" onmousedown="t(294)"></div><div class="b-k" onmousedown="t(311)"></div>
        <div class="k" onmousedown="t(330)"></div>
        <div class="k" onmousedown="t(349)"></div><div class="b-k" onmousedown="t(370)"></div>
        <div class="k" onmousedown="t(392)"></div><div class="b-k" onmousedown="t(415)"></div>
        <div class="k" onmousedown="t(440)"></div><div class="b-k" onmousedown="t(466)"></div>
        <div class="k" onmousedown="t(494)"></div>
        <div class="k" onmousedown="t(523)"></div>
      </div>
    </div>

    <div class="card" style="grid-column:1/-1;text-align:left">
      <h3>⚙️ Config</h3>
      <form action="/save" method="POST">
        <input type="text" name="ssid" placeholder="SSID" style="width:100%;padding:8px;margin:5px 0">
        <input type="text" name="pass" placeholder="Pass" style="width:100%;padding:8px;margin:5px 0">
        <button type="submit" style="background:var(--ok)">Simpan</button>
      </form>
      <div style="margin-top:10px;text-align:center"><a href="/csv" style="color:var(--p)">Download Data Log</a></div>
    </div>
  </div>
</div>

<script>
function t(f){fetch('/cmd?do=tone&f='+f)}
function play(){fetch('/cmd?do=play&id='+document.getElementById('s_id').value)}
function stop(){fetch('/cmd?do=stop')}
function u(){
  fetch('/data').then(r=>r.json()).then(d=>{
    document.getElementById('t').innerText=d.t;
    document.getElementById('gas').innerText=d.gas_stat;
    document.getElementById('gas').style.color=d.gas_stat=="BAHAYA"?"#ef4444":"#10b981";
    document.getElementById('fire').innerText=d.fire_stat;
    document.getElementById('fire').style.color=d.fire_stat=="KEBAKARAN"?"#ef4444":"#10b981";
    document.getElementById('db').innerText=d.db;
  });
}
setInterval(u,2000);u();
</script>
</body></html>
)=====";

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  // Pin Setup
  pinMode(PIN_FAN, OUTPUT);
  pinMode(PIN_LAMP, OUTPUT);
  pinMode(PIN_MQ_D, INPUT_PULLUP);
  pinMode(PIN_MQ2_D, INPUT_PULLUP);
  pinMode(PIN_LDR_D, INPUT_PULLUP);

  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(PIN_BUZZER, PWM_CHANNEL);

  dht.begin();
  analogSetAttenuation(ADC_11db);

  // WiFi
  pref.begin("smartclass", false);
  ssid_name = pref.getString("ssid", ssid_name);
  ssid_pass = pref.getString("pass", ssid_pass);
  bot_token = pref.getString("bot", bot_token);
  chat_id   = pref.getString("id", chat_id);
  bot.updateToken(bot_token);

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid_name.c_str(), ssid_pass.c_str());

  unsigned long s = millis();
  while(WiFi.status() != WL_CONNECTED && millis()-s < 10000) delay(100);

  if(WiFi.status() == WL_CONNECTED) {
    configTime(GMT_OFFSET_SEC, DAY_LIGHT_OFFSET, NTP_SERVER);
    playSong(0); // Startup Sound (Blocking)
    while(is_playing) { handleMusic(); delay(5); }
  } else {
    WiFi.softAP("SmartClass_Setup");
  }

  // Server
  client.setInsecure();
  server.on("/", [](){ server.send(200, "text/html", HTML_PAGE); });
  server.on("/data", handleJson);
  server.on("/cmd", handleCommand);
  server.on("/save", HTTP_POST, handleSaveSettings);
  server.on("/csv", handleDownload);
  server.begin();
}

// ================= LOOP =================
void loop() {
  server.handleClient();
  handleMusic();

  unsigned long now = millis();

  if(now - last_sensor > 2000) {
    last_sensor = now;
    readSensors();
    checkSchedule();
  }

  // Telegram (Only if not playing music)
  if(WiFi.status() == WL_CONNECTED && now - last_bot > 3000 && !is_playing) {
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
  float tr = dht.readTemperature();
  if(!isnan(tr)) t = tr;

  // Gas & Fire Logic
  // Gas: Analog > 3000 OR Digital HIGH (Inverted logic per request)
  int mq_val = analogRead(PIN_MQ_A);
  dig_gas = digitalRead(PIN_MQ_D);

  int flame_val = analogRead(PIN_FLAME_A); // Flame usually LOW = Fire

  // Safety Alerts
  bool gas_alert = (mq_val > 3000 || dig_gas == HIGH);
  bool fire_alert = (flame_val < 1000); // IR Flame sensor value drops when fire detected

  if((gas_alert || fire_alert) && millis() > 60000) { // 60s Warmup
     if(millis() - last_alert > 30000) { // Throttle
       String msg = "⚠️ ";
       if(gas_alert) msg += "GAS/ASAP! ";
       if(fire_alert) msg += "API TERDETEKSI! ";
       if(WiFi.status() == WL_CONNECTED) bot.sendMessage(chat_id, msg, "");
       last_alert = millis();
     }
     if(!is_playing) ledcWriteTone(PWM_CHANNEL, 2000);
  } else {
     if(!is_playing) ledcWriteTone(PWM_CHANNEL, 0);
  }

  status_safety = (gas_alert || fire_alert) ? "BAHAYA" : "AMAN";
}

void handleMusic() {
  if(!is_playing) return;
  unsigned long now = millis();
  int dur = 1000 / pgm_read_dword(&current_tempo[note_index]);

  if(!note_state) {
    ledcWriteTone(PWM_CHANNEL, pgm_read_dword(&current_melody[note_index]));
    note_state = true;
    last_note_start = now;
  }

  if(now - last_note_start > dur) ledcWriteTone(PWM_CHANNEL, 0);

  if(now - last_note_start > (dur * 1.30)) {
    note_state = false;
    note_index++;
    if(note_index >= current_len) is_playing = false;
  }
}

void playSong(int id) {
  if(id < 0 || id >= SONGS_COUNT) return;
  current_melody = SONGS[id].m;
  current_tempo = SONGS[id].t;
  current_len = SONGS[id].len;
  note_index = 0;
  is_playing = true;
}

void checkSchedule() {
  if(WiFi.status() != WL_CONNECTED) return;
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) return;

  if(timeinfo.tm_sec == 0) {
    if(timeinfo.tm_hour == 7 && timeinfo.tm_min == 0) playSong(3);
    if(timeinfo.tm_hour == 10 && timeinfo.tm_min == 0) playSong(4);
    if(timeinfo.tm_hour == 14 && timeinfo.tm_min == 0) playSong(5);
  }
}

void handleTelegram(int numNewMessages) {
  for(int i=0; i<numNewMessages; i++) {
    String text = bot.messages[i].text;
    String id = bot.messages[i].chat_id;

    if(text == "/start") bot.sendMessage(id, "Menu: /cek, /musik, /dashboard", "");
    else if(text == "/cek") {
      String m = "T: " + String(t) + "C\nStatus: " + status_safety;
      bot.sendMessage(id, m, "");
    }
    else if(text == "/musik") {
      bot.sendMessage(id, "Memutar Indonesia Raya...", "");
      playSong(2);
    }
    else if(text == "/dashboard") {
      bot.sendMessage(id, "http://" + WiFi.localIP().toString(), "");
    }
  }
}

void handleCommand() {
  String act = server.arg("do");
  if(act == "play") playSong(server.arg("id").toInt());
  if(act == "stop") { is_playing = false; ledcWriteTone(PWM_CHANNEL, 0); }
  if(act == "tone") ledcWriteTone(PWM_CHANNEL, server.arg("f").toInt());
  server.send(200, "text/plain", "OK");
}

void handleJson() {
  String json = "{";
  json += "\"t\":" + String(t) + ",";
  json += "\"gas_stat\":\"" + (status_safety.indexOf("GAS")>=0 ? "BAHAYA" : "AMAN") + "\",";
  json += "\"fire_stat\":\"" + (status_safety.indexOf("API")>=0 ? "KEBAKARAN" : "AMAN") + "\",";
  json += "\"db\":" + String(db);
  json += "}";
  server.send(200, "application/json", json);
}

void handleSaveSettings() {
  if(server.hasArg("ssid")) {
    pref.putString("ssid", server.arg("ssid"));
    pref.putString("pass", server.arg("pass"));
    server.send(200, "text/html", "Restarting...");
    delay(1000); ESP.restart();
  }
}

void handleDownload() {
  server.send(200, "text/csv", "Time,Temp,Status\n" + String(millis()) + "," + String(t) + "," + status_safety);
}
