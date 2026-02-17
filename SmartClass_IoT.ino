/*
 * SMART CLASS IOT - AI EDITION (Multi File)
 * Board: ESP32 Dev Module
 * Fitur:
 * - Monitoring Sensor (Suhu, Lembab, Gas, Suara, Cahaya, Jarak)
 * - Telegram Bot (Full Control)
 * - Web Dashboard (Piano, Music Player 25 Songs, AI Mode)
 * - AI Logic (Presence Detection, Security, Comfort)
 * - Music Engine (Pro Melodies from PROGMEM)
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <UniversalTelegramBot.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>

#include "index.h"  // Web Dashboard
#include "songs.h"  // 25 Melodies Database

// ================= PIN CONFIGURATION =================
#define PIN_DHT         18
#define PIN_BUZZER      26
#define PIN_FAN         4
#define PIN_LAMP        2
#define PIN_LDR         34
#define PIN_MQ135       35
#define PIN_SOUND       32
#define PIN_TRIG        5
#define PIN_ECHO        19

// ================= PWM & SENSORS =================
#define PWM_CHANNEL     0
#define PWM_RESOLUTION  8
#define PWM_FREQ        2000
#define DHTTYPE         DHT22

// ================= SETTINGS =================
const int GAS_ALARM_LIMIT   = 2500;
const float TEMP_ALARM_LIMIT = 35.0;
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
int gas = 0;
long rssi = 0;

// System State
bool st_fan = false, st_lamp = false;
bool mode_auto = false; // Manual by default
bool mode_ai = true;    // AI Mode ON by default
String mood = "Netral";
String ai_status = "Initializing...";
String time_str = "--:--";
bool is_day_time = true;
bool alert_active = false;
bool presence_detected = false;

// Timers
unsigned long last_dht = 0;
unsigned long last_analog = 0;
unsigned long last_bot = 0;
unsigned long last_wifi_check = 0;
unsigned long scan_start_time = 0;

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

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  pinMode(PIN_FAN, OUTPUT);
  pinMode(PIN_LAMP, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  pinMode(PIN_MQ135, INPUT);
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

  // Startup Sound (Mario Intro - Short)
  current_song_id = 0; // Mario
  is_playing = true;
  last_note_start = millis();
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

// ================= LOGIC AI =================
void logicAI() {
  // 1. Presence Detection (Ultrasonic < 150cm OR Sound > 60dB)
  if ((dist > 0 && dist < 150) || (db > 60)) {
    presence_detected = true;
  } else {
    presence_detected = false;
  }

  // 2. Determine Mood
  int s = 0;
  if (t > 28) s += 30; if (db > 60) s += 30; if (gas > 2000) s += 30;
  if (s < 30) mood = "Nyaman 😊"; else if (s < 60) mood = "Fokus 😐"; else mood = "Buruk 😡";

  if (!mode_ai) {
    ai_status = "AI OFF (Manual)";
    return;
  }

  // 3. AI Actions
  if (presence_detected) {
    if (is_day_time) {
      ai_status = "Class Active 🟢";
      // Comfort Control
      if (t > 28.0) { st_fan = true; digitalWrite(PIN_FAN, HIGH); }
      else { st_fan = false; digitalWrite(PIN_FAN, LOW); }

      if (lux < 400) { st_lamp = true; digitalWrite(PIN_LAMP, HIGH); }
      else { st_lamp = false; digitalWrite(PIN_LAMP, LOW); }
    } else {
      // Night Time + Presence = INTRUDER
      ai_status = "INTRUDER ALERT! 🔴";
      st_lamp = true; digitalWrite(PIN_LAMP, HIGH); // Flash Lamp
      st_fan = true; digitalWrite(PIN_FAN, HIGH);   // Noise

      if (!alert_active) {
        bot.sendMessage(chat_id, "⚠️ PERINGATAN! Gerakan terdeteksi di kelas saat malam hari!", "");
        alert_active = true;
      }
    }
  } else {
    ai_status = "Class Empty ⚪";
    // Energy Saving
    st_fan = false; digitalWrite(PIN_FAN, LOW);
    st_lamp = false; digitalWrite(PIN_LAMP, LOW);
    alert_active = false;
  }

  // Safety Override (Always Active)
  if (gas > GAS_ALARM_LIMIT || t > TEMP_ALARM_LIMIT) {
     ai_status = "DANGER 🔥";
     bot.sendMessage(chat_id, "⚠️ BAHAYA! Gas/Suhu Tinggi!", "");
  }
}

// ================= MUSIC ENGINE =================
void handleMusic() {
  if (!is_playing) return;

  unsigned long now = millis();

  // Get current note data from PROGMEM
  int note_freq = pgm_read_word(&(SONGS[current_song_id].melody[note_index]));
  int note_tempo = pgm_read_word(&(SONGS[current_song_id].tempo[note_index]));

  int duration = 1000 / note_tempo;
  int pause = duration * 1.30;

  if (!note_state) {
    // Start Note
    if (note_freq > 0) ledcWriteTone(PWM_CHANNEL, note_freq);
    else ledcWriteTone(PWM_CHANNEL, 0); // Rest

    note_state = true;
    last_note_start = now;
  } else {
    // Stop tone after duration
    if (now - last_note_start > duration) {
      ledcWriteTone(PWM_CHANNEL, 0);
    }
    // Next note after pause
    if (now - last_note_start > pause) {
      note_state = false;
      note_index++;
      int song_len = SONGS[current_song_id].length;
      if (note_index >= song_len) {
        note_index = 0;
        // is_playing = false; // Loop song
      }
    }
  }
}

void playTone(int freq, int duration) {
  is_playing = false; // Stop background music
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

  if (act == "music_play") {
    if (server.hasArg("id")) current_song_id = server.arg("id").toInt();
    if (current_song_id < 0) current_song_id = 0;
    if (current_song_id > 24) current_song_id = 24;

    is_playing = true;
    note_index = 0;
    note_state = false;
    last_note_start = millis();
  }

  if (act == "music_stop") {
    is_playing = false;
    ledcWriteTone(PWM_CHANNEL, 0);
  }

  if (act == "tone") {
    int f = server.arg("freq").toInt();
    playTone(f, 200); // 200ms duration for piano key
  }

  server.send(200, "text/plain", "OK");
}

void handleTelegram(int n) {
  for (int i=0; i<n; i++) {
    String txt = bot.messages[i].text;
    String cid = bot.messages[i].chat_id;

    if (txt == "/start") {
      String msg = "🤖 *Smart Class AI Bot*\n\n";
      msg += "/status - Cek kondisi kelas\n";
      msg += "/ai [on/off] - Mode Otomatis\n";
      msg += "/fan [on/off] - Kipas Manual\n";
      msg += "/lamp [on/off] - Lampu Manual\n";
      msg += "/music [0-24] - Putar Lagu\n";
      msg += "/stop - Matikan Musik";
      bot.sendMessage(cid, msg, "Markdown");
    }
    else if (txt == "/status" || txt == "/cek") {
      String msg = "📊 *Status Kelas*\n";
      msg += "🌡 Suhu: " + String(t) + "C\n";
      msg += "💧 Lembab: " + String(h) + "%\n";
      msg += "💨 Gas: " + String(gas) + "\n";
      msg += "🔊 Suara: " + String(db) + "dB\n";
      msg += "👥 Orang: " + String(presence_detected ? "ADA" : "KOSONG") + "\n";
      msg += "🤖 AI Status: " + ai_status;
      bot.sendMessage(cid, msg, "Markdown");
    }
    else if (txt == "/ai on") { mode_ai=true; bot.sendMessage(cid, "AI Mode ON", ""); }
    else if (txt == "/ai off") { mode_ai=false; bot.sendMessage(cid, "AI Mode OFF", ""); }
    else if (txt == "/fan on") { mode_ai=false; digitalWrite(PIN_FAN,1); bot.sendMessage(cid, "Fan ON", ""); }
    else if (txt == "/fan off") { mode_ai=false; digitalWrite(PIN_FAN,0); bot.sendMessage(cid, "Fan OFF", ""); }
    else if (txt == "/lamp on") { mode_ai=false; digitalWrite(PIN_LAMP,1); bot.sendMessage(cid, "Lamp ON", ""); }
    else if (txt == "/lamp off") { mode_ai=false; digitalWrite(PIN_LAMP,0); bot.sendMessage(cid, "Lamp OFF", ""); }
    else if (txt.startsWith("/music ")) {
      String id_str = txt.substring(7);
      current_song_id = id_str.toInt();
      is_playing = true; note_index = 0; note_state = false; last_note_start = millis();
      bot.sendMessage(cid, "Playing Song #" + id_str, "");
    }
    else if (txt == "/stop") { is_playing = false; ledcWriteTone(PWM_CHANNEL, 0); bot.sendMessage(cid, "Music Stopped", ""); }
  }
}

void handleJson() {
  JsonDocument doc;
  doc["t"] = t; doc["h"] = h; doc["gas"] = gas; doc["db"] = db;
  doc["rssi"] = rssi; doc["fan"] = st_fan; doc["lamp"] = st_lamp;
  doc["ai"] = mode_ai; doc["mood"] = mood; doc["time"] = time_str; doc["ai_stat"] = ai_status;
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
  gas = smoothAnalog(PIN_MQ135);
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
  is_day_time = (timeinfo.tm_hour >= 7 && timeinfo.tm_hour < 17);
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
  String csv = "Time,T,H,Gas,DB\n" + String(millis()) + "," + String(t) + "," + String(h) + "," + String(gas) + "," + String(db);
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
