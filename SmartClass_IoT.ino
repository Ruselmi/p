/*
 * SMART CLASS IOT - MINIMALIST EDITION (Multi File)
 * Board: ESP32 Dev Module
 * Fitur: Monitoring Sensor, Telegram Bot, Web Dashboard, Music Player, WiFi Manager
 *
 * Update Fixes (v3.7 - Split File):
 * - STRUCTURE: Kode HTML dipisah ke "index.h" untuk menghindari error compiler.
 * - WIFI: Async Scan Fix.
 * - BOT: Secure Client.
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <UniversalTelegramBot.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>

#include "index.h" // MEMUAT FILE HTML

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
const long GMT_OFFSET_SEC   = 25200;
const int DAYLIGHT_OFFSET   = 0;

// ================= VARIABEL GLOBAL =================
DHT dht(PIN_DHT, DHTTYPE);
WebServer server(80);
WiFiClientSecure client;
UniversalTelegramBot bot("", client);
Preferences pref;

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

unsigned long last_dht = 0;
unsigned long last_analog = 0;
unsigned long last_bot = 0;
unsigned long last_wifi_check = 0;
unsigned long scan_start_time = 0; // For scan timeout

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

int melody_theme[] = { NOTE_D4, NOTE_F4, NOTE_D5, NOTE_D4, NOTE_F4, NOTE_D5, NOTE_E5, NOTE_F5, NOTE_E5, NOTE_F5, NOTE_E5, NOTE_C5, NOTE_A4 };
int durations_theme[] = { 8, 8, 2, 8, 8, 2, 6, 16, 6, 16, 6, 8, 2 };

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

  pinMode(PIN_FAN, OUTPUT);
  pinMode(PIN_LAMP, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  pinMode(PIN_MQ135, INPUT); // Explicit Input
  analogSetAttenuation(ADC_11db); // Range 0-3.3V

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

  int mario_notes[] = {NOTE_E5, NOTE_E5, NOTE_E5, NOTE_C5, NOTE_E5, NOTE_G5, NOTE_G4};
  int mario_durs[]  = {100, 100, 100, 100, 100, 200, 200};
  for(int i=0; i<7; i++){
    ledcWriteTone(PWM_CHANNEL, mario_notes[i]); delay(mario_durs[i]);
    ledcWriteTone(PWM_CHANNEL, 0); delay(50);
  }
}

void loop() {
  server.handleClient();
  handleMusic();
  checkWiFi();

  unsigned long now = millis();

  if (now - last_dht > 2000) {
    last_dht = now;
    readDHT();
  }

  if (now - last_analog > 1000) {
    last_analog = now;
    readAnalogSensors();
    updateTime();
    logicAuto();
    checkAlert();
    Serial.print("T:"); Serial.print(t); Serial.print(" H:"); Serial.println(h);
  }

  if (WiFi.status() == WL_CONNECTED && !is_playing && (now - last_bot > 3000)) {
    last_bot = now;
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while(numNewMessages) {
      handleTelegram(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
  }
}

void handleScanWiFi() {
  int n = WiFi.scanComplete();
  if (n == -2) {
    WiFi.scanNetworks(true, true);
    scan_start_time = millis();
    server.send(202, "application/json", "{\"status\":\"scanning\"}");
  } else if (n == -1) {
    if (millis() - scan_start_time > 10000) { // Timeout 10s
      WiFi.scanDelete();
      WiFi.scanNetworks(true, true);
      scan_start_time = millis();
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

void updateTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){ time_str = "--:--"; return; }
  char b[10]; strftime(b, sizeof(b), "%H:%M", &timeinfo);
  time_str = String(b);
  is_day_time = (timeinfo.tm_hour >= 7 && timeinfo.tm_hour < 17);
}

void checkWiFi() {
  if (millis() - last_wifi_check > 15000) {
    last_wifi_check = millis();
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.reconnect();
    }
  }
}

int smoothAnalog(int pin) {
  long total = 0;
  for (int i = 0; i < 5; i++) { total += analogRead(pin); delay(2); }
  return total / 5;
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
  } else {
    server.send(400, "text/plain", "Error: SSID Empty");
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
  db = (smoothAnalog(PIN_SOUND) / 4095.0) * 100.0;
  rssi = WiFi.RSSI();

  // Debug output
  Serial.printf("Gas:%d Lux:%0.1f Dist:%0.1f dB:%0.1f\n", gas, lux, dist, db);

  int s = 0;
  if (t > 28) s += 30; if (db > 60) s += 30; if (gas > 2000) s += 30;
  if (s < 30) mood = "Nyaman"; else if (s < 60) mood = "Fokus"; else mood = "Tidak Kondusif";
}

void checkAlert() {
  bool danger = (gas > GAS_ALARM_LIMIT) || (t > TEMP_ALARM_LIMIT);
  if (danger && !alert_active) {
    bot.sendMessage(chat_id, "⚠️ BAHAYA! Cek Kelas!", "");
    alert_active = true;
  } else if (!danger && alert_active) {
    bot.sendMessage(chat_id, "✅ Aman.", "");
    alert_active = false;
  }
}

void logicAuto() {
  if (!mode_auto) return;
  if (!is_day_time) { digitalWrite(PIN_FAN,0); digitalWrite(PIN_LAMP,0); return; }
  digitalWrite(PIN_FAN, t > 27.0);
  digitalWrite(PIN_LAMP, lux < 500);
}

void handleJson() {
  JsonDocument doc;
  doc["t"] = t; doc["h"] = h; doc["gas"] = gas; doc["db"] = db;
  doc["rssi"] = rssi; doc["fan"] = st_fan; doc["lamp"] = st_lamp;
  doc["auto"] = mode_auto; doc["mood"] = mood; doc["time"] = time_str; doc["alert"] = alert_active;
  String json; serializeJson(doc, json);
  server.sendHeader("Cache-Control", "no-cache, no-store");
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
  String csv = "Time,T,H,Gas,DB\n" + String(millis()) + "," + String(t) + "," + String(h) + "," + String(gas) + "," + String(db);
  server.sendHeader("Content-Disposition", "attachment; filename=log.csv");
  server.send(200, "text/csv", csv);
}

void handleMusic() {
  if (!is_playing) return;

  unsigned long now = millis();
  int duration = 1000 / durations_theme[note_index];
  int pause = duration * 1.30;

  if (!note_state) {
    // Start Note
    ledcWriteTone(PWM_CHANNEL, melody_theme[note_index]);
    note_state = true;
    last_note_start = now;
    // Serial.printf("Play Note %d: %d Hz\n", note_index, melody_theme[note_index]);
  } else {
    // Check if note duration passed
    if (now - last_note_start > duration) {
      ledcWriteTone(PWM_CHANNEL, 0); // Stop tone
    }
    // Check if pause duration passed (next note)
    if (now - last_note_start > pause) {
      note_state = false;
      note_index++;
      if (note_index >= melody_len) {
        note_index = 0; // Loop
        // is_playing = false; // Uncomment to play once
      }
    }
  }
}

void handleTelegram(int n) {
  for (int i=0; i<n; i++) {
    String txt = bot.messages[i].text;
    String cid = bot.messages[i].chat_id;
    if (txt == "/start") bot.sendMessage(cid, "Halo! Cmd: /cek /fan_on /fan_off /lamp_on /lamp_off", "");
    else if (txt == "/cek") bot.sendMessage(cid, "Suhu: "+String(t)+"C\nLampu: "+(st_lamp?"ON":"OFF"), "");
    else if (txt == "/fan_on") { digitalWrite(PIN_FAN,1); mode_auto=0; bot.sendMessage(cid, "Fan ON", ""); }
    else if (txt == "/fan_off") { digitalWrite(PIN_FAN,0); mode_auto=0; bot.sendMessage(cid, "Fan OFF", ""); }
    else if (txt == "/lamp_on") { digitalWrite(PIN_LAMP,1); mode_auto=0; bot.sendMessage(cid, "Lamp ON", ""); }
    else if (txt == "/lamp_off") { digitalWrite(PIN_LAMP,0); mode_auto=0; bot.sendMessage(cid, "Lamp OFF", ""); }
  }
}

void handleRoot() {
  server.sendHeader("Cache-Control", "no-cache");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  server.sendContent(HTML_HEAD);
  server.sendContent(HTML_BODY);
  server.sendContent("");
}
