/*
 * SMART CLASS IOT - HEALTH & SAFETY EDITION (Multi File)
 * Board: ESP32 Dev Module
 * Fitur:
 * - Monitoring: Suhu, Lembab, Gas (MQ135), Asap/Rokok (MQ2), Suara, Cahaya, Jarak
 * - Health Standards: WHO & Kemenkes Compliance Check
 * - School Bell: Auto-Schedule (Entry, Break, End)
 * - Telegram Bot, Web Dashboard, Music Player, AI Logic
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <UniversalTelegramBot.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>

#include "index.h"  // Web Dashboard (Updated with Health Refs)
#include "songs.h"  // 25 Melodies + School Bells

// ================= PIN CONFIGURATION =================
#define PIN_DHT         18
#define PIN_BUZZER      26
#define PIN_FAN         4
#define PIN_LAMP        2
#define PIN_LDR         34
#define PIN_MQ135       35  // Air Quality
#define PIN_MQ2         33  // Smoke / Cigarette (NEW)
#define PIN_SOUND       32
#define PIN_TRIG        5
#define PIN_ECHO        19

// ================= PWM & SENSORS =================
#define PWM_CHANNEL     0
#define PWM_RESOLUTION  8
#define PWM_FREQ        2000
#define DHTTYPE         DHT22

// ================= HEALTH STANDARDS (KEMENKES/WHO) =================
// Permenkes No. 2 Tahun 2023 & Kepmen LH No. 48/1996
const float TEMP_MIN_STD = 18.0;
const float TEMP_MAX_STD = 30.0;
const float HUMID_MIN_STD = 40.0;
const float HUMID_MAX_STD = 60.0;
const float NOISE_MAX_STD = 55.0; // dB for Schools
const int   CO2_MAX_STD   = 1000; // PPM (Good Ventilation)

const long GMT_OFFSET_SEC   = 25200; // UTC+7
const int DAYLIGHT_OFFSET   = 0;

// ================= GLOBALS =================
DHT dht(PIN_DHT, DHTTYPE);
WebServer server(80);
WiFiClientSecure client;
UniversalTelegramBot bot("", client);
Preferences pref;

String ssid_name = "YOUR_SSID";
String ssid_pass = "YOUR_PASS";
String bot_token = "YOUR_BOT_TOKEN";
String chat_id   = "YOUR_CHAT_ID";

// Sensor Data
float t = 0, h = 0, lux = 0, dist = 0, db = 0;
int gas_mq135 = 0; // CO2 / Air Quality
int gas_mq2 = 0;   // Smoke / LPG
long rssi = 0;

// System State
bool st_fan = false, st_lamp = false;
bool mode_auto = false;
bool mode_ai = true;
bool auto_bell = true;  // School Bell Schedule

String mood = "Netral";
String ai_status = "Initializing...";
String health_status = "Checking...";
String time_str = "--:--";
int current_hour = 0;
int current_minute = 0;
bool is_day_time = true;
bool alert_active = false;
bool presence_detected = false;
bool smoke_detected = false;

// Timers
unsigned long last_dht = 0;
unsigned long last_analog = 0;
unsigned long last_bot = 0;
unsigned long last_wifi_check = 0;
unsigned long scan_start_time = 0;
unsigned long last_bell_check = 0;

// Music Engine
bool is_playing = false;
int note_index = 0;
unsigned long last_note_start = 0;
bool note_state = false;
int current_song_id = 0;

// ================= PROTOTYPES =================
void handleSaveSettings();
void readDHT();
void readAnalogSensors();
void logicAI();
void checkBellSchedule();
void handleJson();
void handleCommand();
void handleDownload();
void handleMusic();
void handleTelegram(int numNewMessages);
void checkWiFi();
int smoothAnalog(int pin);
void updateTime();
void handleScanWiFi();
void handleRoot();
void playTone(int freq, int duration);
void playSong(int id);

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  pinMode(PIN_FAN, OUTPUT);
  pinMode(PIN_LAMP, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  pinMode(PIN_MQ135, INPUT);
  pinMode(PIN_MQ2, INPUT);
  analogSetAttenuation(ADC_11db); // 0-3.3V range

  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(PIN_BUZZER, PWM_CHANNEL);
  ledcWriteTone(PWM_CHANNEL, 0);

  dht.begin();

  pref.begin("smartclass", false);
  ssid_name = pref.getString("ssid", ssid_name);
  ssid_pass = pref.getString("pass", ssid_pass);
  bot_token = pref.getString("bot", bot_token);
  chat_id   = pref.getString("id", chat_id);

  bot.updateToken(bot_token);

  Serial.print("Connecting: "); Serial.println(ssid_name);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid_name.c_str(), ssid_pass.c_str());

  int try_count = 0;
  while (WiFi.status() != WL_CONNECTED && try_count < 20) {
    delay(250); Serial.print(".");
    try_count++;
  }

  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi OK!");
    Serial.println(WiFi.localIP());
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET, "pool.ntp.org", "time.nist.gov");
  } else {
    Serial.println("\nWiFi Fail. AP: SmartClass_AP");
    WiFi.softAP("SmartClass_AP");
    Serial.println(WiFi.softAPIP());
  }

  client.setInsecure();
  server.on("/", handleRoot);
  server.on("/data", handleJson);
  server.on("/cmd", handleCommand);
  server.on("/csv", handleDownload);
  server.on("/scan", handleScanWiFi);
  server.on("/save", HTTP_POST, handleSaveSettings);
  server.enableCORS(true);
  server.begin();

  playSong(0); // Startup Sound (Mario)
}

// ================= LOOP =================
void loop() {
  server.handleClient();
  handleMusic();
  checkWiFi();

  unsigned long now = millis();

  // DHT Read (Every 2s)
  if (now - last_dht > 2000) {
    last_dht = now;
    readDHT();
  }

  // Analog & Logic (Every 1s)
  if (now - last_analog > 1000) {
    last_analog = now;
    readAnalogSensors();
    updateTime();
    checkBellSchedule();
    logicAI();
    Serial.print("AI:"); Serial.print(ai_status); Serial.print(" T:"); Serial.println(t);
  }

  // Telegram (Every 3s)
  if (WiFi.status() == WL_CONNECTED && !is_playing && (now - last_bot > 3000)) {
    last_bot = now;
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while(numNewMessages) {
      handleTelegram(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
  }
}

// ================= LOGIC AI & HEALTH =================
void logicAI() {
  // 1. Presence Detection
  presence_detected = ((dist > 0 && dist < 150) || (db > 60));

  // 2. Health & Comfort Status (Based on Standards)
  String health = "OK ✅";
  if (t < TEMP_MIN_STD || t > TEMP_MAX_STD) health = "Suhu Tidak Ideal ⚠️";
  if (h < HUMID_MIN_STD || h > HUMID_MAX_STD) health = "Lembab Tidak Ideal ⚠️";
  if (db > NOISE_MAX_STD) health = "Bising! 🔇";
  if (gas_mq135 > CO2_MAX_STD) health = "Udara Kotor! 💨";
  health_status = health;

  // 3. Smoke Detection (MQ2)
  if (gas_mq2 > 2000) { // Threshold for smoke
    smoke_detected = true;
    ai_status = "ROKOK TERDETEKSI! 🚭";
    bot.sendMessage(chat_id, "🚭 PERINGATAN: Asap Rokok Terdeteksi di Kelas!", "");
  } else {
    smoke_detected = false;
  }

  // 4. Mood Calculation
  int stress = 0;
  if (t > 28) stress += 30; if (db > 60) stress += 30; if (gas_mq135 > 1500) stress += 30;
  if (stress < 30) mood = "Nyaman 😊"; else if (stress < 60) mood = "Fokus 😐"; else mood = "Buruk 😡";

  if (!mode_ai) {
    if (!smoke_detected) ai_status = "AI OFF (Manual)";
    return;
  }

  // 5. AI Actions
  if (presence_detected) {
    if (is_day_time) {
      if (!smoke_detected) ai_status = "Class Active 🟢";
      // Comfort Control
      if (t > 28.0) { st_fan = true; digitalWrite(PIN_FAN, HIGH); }
      else { st_fan = false; digitalWrite(PIN_FAN, LOW); }

      if (lux < 400) { st_lamp = true; digitalWrite(PIN_LAMP, HIGH); }
      else { st_lamp = false; digitalWrite(PIN_LAMP, LOW); }
    } else {
      // Night Intruder
      ai_status = "INTRUDER ALERT! 🔴";
      st_lamp = true; digitalWrite(PIN_LAMP, HIGH);
      st_fan = true; digitalWrite(PIN_FAN, HIGH);
      if (!alert_active) {
        bot.sendMessage(chat_id, "⚠️ PERINGATAN! Gerakan malam hari!", "");
        alert_active = true;
      }
    }
  } else {
    if (!smoke_detected) ai_status = "Class Empty ⚪";
    st_fan = false; digitalWrite(PIN_FAN, LOW);
    st_lamp = false; digitalWrite(PIN_LAMP, LOW);
    alert_active = false;
  }
}

// ================= SCHOOL BELL SCHEDULE =================
void checkBellSchedule() {
  if (!auto_bell) return;
  if (millis() - last_bell_check < 60000) return; // Check every minute
  last_bell_check = millis();

  // Schedule (Mon-Fri assumed)
  if (current_hour == 7 && current_minute == 0) {
    playSong(25); // Entry Bell
    bot.sendMessage(chat_id, "🔔 Bel Masuk (07:00)", "");
  }
  else if (current_hour == 10 && current_minute == 0) {
    playSong(26); // Break Bell
    bot.sendMessage(chat_id, "🔔 Bel Istirahat (10:00)", "");
  }
  else if (current_hour == 14 && current_minute == 0) {
    playSong(27); // End Bell
    bot.sendMessage(chat_id, "🔔 Bel Pulang (14:00)", "");
  }
}

// ================= MUSIC ENGINE =================
void playSong(int id) {
  if (id < 0 || id > 27) return;
  current_song_id = id;
  is_playing = true;
  note_index = 0;
  note_state = false;
  last_note_start = millis();
}

void handleMusic() {
  if (!is_playing) return;
  unsigned long now = millis();

  int note_freq = pgm_read_word(&(SONGS[current_song_id].melody[note_index]));
  int note_tempo = pgm_read_word(&(SONGS[current_song_id].tempo[note_index]));

  int duration = 1000 / note_tempo;
  int pause = duration * 1.30;

  if (!note_state) {
    if (note_freq > 0) ledcWriteTone(PWM_CHANNEL, note_freq);
    else ledcWriteTone(PWM_CHANNEL, 0);
    note_state = true;
    last_note_start = now;
  } else {
    if (now - last_note_start > duration) ledcWriteTone(PWM_CHANNEL, 0);
    if (now - last_note_start > pause) {
      note_state = false;
      note_index++;
      int song_len = SONGS[current_song_id].length;
      if (note_index >= song_len) {
        note_index = 0;
        is_playing = false; // Stop after one loop for bells/songs
      }
    }
  }
}

void playTone(int freq, int duration) {
  is_playing = false;
  ledcWriteTone(PWM_CHANNEL, freq);
  delay(duration);
  ledcWriteTone(PWM_CHANNEL, 0);
}

// ================= HANDLERS =================

void handleCommand() {
  String act = server.arg("do");

  if (act == "fan_toggle") { mode_ai = false; st_fan = !st_fan; digitalWrite(PIN_FAN, st_fan); }
  if (act == "lamp_toggle") { mode_ai = false; st_lamp = !st_lamp; digitalWrite(PIN_LAMP, st_lamp); }
  if (act == "ai_toggle") { mode_ai = !mode_ai; }
  if (act == "bell_toggle") { auto_bell = !auto_bell; }

  if (act == "music_play") {
    if (server.hasArg("id")) playSong(server.arg("id").toInt());
  }

  if (act == "music_stop") { is_playing = false; ledcWriteTone(PWM_CHANNEL, 0); }

  if (act == "tone") {
    int f = server.arg("freq").toInt();
    playTone(f, 200);
  }

  server.send(200, "text/plain", "OK");
}

void handleTelegram(int n) {
  for (int i=0; i<n; i++) {
    String txt = bot.messages[i].text;
    String cid = bot.messages[i].chat_id;

    if (txt == "/start") {
      String msg = "🤖 *Smart Class Bot*\n";
      msg += "/status - Cek Kesehatan Kelas\n";
      msg += "/ai [on/off] - Mode Otomatis\n";
      msg += "/music [0-27] - Putar Lagu/Bel\n";
      msg += "/stop - Matikan Suara";
      bot.sendMessage(cid, msg, "Markdown");
    }
    else if (txt == "/status") {
      String msg = "🏥 *Laporan Kesehatan*\n";
      msg += "🌡 " + String(t) + "C (Std: 18-30)\n";
      msg += "💧 " + String(h) + "% (Std: 40-60)\n";
      msg += "🔊 " + String(db) + "dB (Max: 55)\n";
      msg += "💨 AQI: " + String(gas_mq135) + " (Max: 1000)\n";
      msg += "🚭 Asap: " + String(smoke_detected ? "BAHAYA" : "AMAN") + "\n";
      msg += "📊 Status: " + health_status;
      bot.sendMessage(cid, msg, "");
    }
    else if (txt == "/ai on") { mode_ai=true; bot.sendMessage(cid, "AI ON", ""); }
    else if (txt == "/ai off") { mode_ai=false; bot.sendMessage(cid, "AI OFF", ""); }
    else if (txt.startsWith("/music ")) {
      playSong(txt.substring(7).toInt());
      bot.sendMessage(cid, "Playing...", "");
    }
    else if (txt == "/stop") { is_playing = false; ledcWriteTone(PWM_CHANNEL, 0); bot.sendMessage(cid, "Stopped", ""); }
  }
}

void handleJson() {
  JsonDocument doc;
  doc["t"] = t; doc["h"] = h; doc["gas"] = gas_mq135; doc["mq2"] = gas_mq2; doc["db"] = db;
  doc["rssi"] = rssi; doc["fan"] = st_fan; doc["lamp"] = st_lamp;
  doc["ai"] = mode_ai; doc["bell"] = auto_bell; doc["mood"] = mood;
  doc["time"] = time_str; doc["ai_stat"] = ai_status; doc["health"] = health_status;
  String json; serializeJson(doc, json);
  server.sendHeader("Cache-Control", "no-cache, no-store");
  server.send(200, "application/json", json);
}

// ================= UTILS =================

void readDHT() {
  float _t = dht.readTemperature();
  float _h = dht.readHumidity();
  if (!isnan(_t)) t = _t;
  if (!isnan(_h)) h = _h;
}

void readAnalogSensors() {
  gas_mq135 = smoothAnalog(PIN_MQ135);
  gas_mq2   = smoothAnalog(PIN_MQ2);
  lux = smoothAnalog(PIN_LDR);

  digitalWrite(PIN_TRIG, LOW); delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  long dur = pulseIn(PIN_ECHO, HIGH, 30000);
  dist = (dur == 0) ? 0 : dur * 0.034 / 2;

  db = (smoothAnalog(PIN_SOUND) / 4095.0) * 100.0;
  rssi = WiFi.RSSI();
}

int smoothAnalog(int pin) {
  long total = 0;
  for (int i = 0; i < 5; i++) { total += analogRead(pin); delay(2); }
  return total / 5;
}

void updateTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){ time_str = "--:--"; return; }
  char b[10]; strftime(b, sizeof(b), "%H:%M", &timeinfo);
  time_str = String(b);
  current_hour = timeinfo.tm_hour;
  current_minute = timeinfo.tm_min;
  is_day_time = (current_hour >= 7 && current_hour < 17);
}

void handleScanWiFi() {
  int n = WiFi.scanComplete();
  if (n == -2) {
    WiFi.scanNetworks(true, true);
    scan_start_time = millis();
    server.send(202, "application/json", "{\"status\":\"scanning\"}");
  } else if (n == -1) {
    if (millis() - scan_start_time > 10000) {
      WiFi.scanDelete(); WiFi.scanNetworks(true, true); scan_start_time = millis();
    }
    server.send(202, "application/json", "{\"status\":\"scanning\"}");
  } else {
    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();
    for (int i = 0; i < n; ++i) {
      array.add(WiFi.SSID(i));
      if(i >= 20) break;
    }
    WiFi.scanDelete();
    String json; serializeJson(doc, json);
    server.send(200, "application/json", json);
  }
}

void checkWiFi() {
  if (millis() - last_wifi_check > 30000) {
    last_wifi_check = millis();
    if (WiFi.status() != WL_CONNECTED) { WiFi.disconnect(); WiFi.reconnect(); }
  }
}

void handleDownload() {
  String csv = "Time,T,H,Gas,Smoke,DB\n" + String(millis()) + "," + String(t) + "," + String(h) + "," + String(gas_mq135) + "," + String(gas_mq2) + "," + String(db);
  server.sendHeader("Content-Disposition", "attachment; filename=log.csv");
  server.send(200, "text/csv", csv);
}

void handleSaveSettings() {
  if (server.hasArg("ssid") || server.hasArg("ssid_manual")) {
    String n_ssid = server.arg("ssid");
    if(server.arg("ssid_manual").length() > 0) n_ssid = server.arg("ssid_manual");
    pref.putString("ssid", n_ssid);
    pref.putString("pass", server.arg("pass"));
    if(server.arg("bot").length() > 5) pref.putString("bot", server.arg("bot"));
    if(server.arg("id").length() > 5)  pref.putString("id", server.arg("id"));
    server.send(200, "text/html", "<h1>Saved. Restarting...</h1>");
    delay(1000); ESP.restart();
  } else { server.send(400, "text/plain", "Error"); }
}

void handleRoot() {
  server.sendHeader("Cache-Control", "no-cache");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  server.sendContent(HTML_HEAD);
  server.sendContent(HTML_BODY);
  server.sendContent("");
}
