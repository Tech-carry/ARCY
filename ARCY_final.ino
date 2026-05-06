#define BLYNK_TEMPLATE_ID "TMPL3J2yptBJY"
#define BLYNK_TEMPLATE_NAME "ARCY ToDo"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <time.h>
#include <BlynkSimpleEsp32.h>

// ===== WIFI =====
const char* ssid = "carry";
const char* password = "123456789";

// ===== BLYNK =====
char auth[] = "tQKQm07fzyBSP0EGLNMg762U4_auTYQT";

// ===== API =====
String apiKey = "ea2a9a137ae2f60258086f9813dfbc5a";
String city = "Pune";

// ===== OLED =====
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

// ===== TOUCH =====
#define TOUCH_PIN 15

// ===== BATTERY =====
#define BATTERY_PIN 34
int batteryPercent = 0;

// ===== WIFI SIGNAL =====
int wifiStrength = 0;

// ===== DATA =====
int temp = 0;
String weather = "Clear";

// ===== AQI =====
int aqi = 0;
String airQuality = "Good";

// ===== TIME =====
const char* ntpServer = "pool.ntp.org";
long gmtOffset_sec = 19800;

// ===== MODE =====
int screenMode = 0;
int prevScreen = 0;
int animationOffset = 0;
bool animating = false;

// ===== FACE =====
bool eyesClosed = false;
unsigned long lastBlink = 0;

// ===== TODO =====
#define MAX_TASKS 5
String tasks[MAX_TASKS];
int taskCount = 0;

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);
  u8g2.begin();

  pinMode(TOUCH_PIN, INPUT_PULLDOWN);

  Blynk.begin(auth, ssid, password);
  configTime(gmtOffset_sec, 0, ntpServer);
}

// ================= LOOP =================
void loop() {
  Blynk.run();
  handleTouch();

  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 10000) {
    getWeather();
    getAQI();
    getBattery();        // 🔋
    getWiFiStrength();   // 📶
    lastUpdate = millis();
  }

  drawScreen();
}

// ================= BLYNK =================
BLYNK_WRITE(V0) {
  String newTask = param.asString();
  newTask.trim();

  if (newTask.length() > 15) {
    newTask = newTask.substring(0, 15);
  }

  if (newTask.length() == 0) return;

  if (taskCount < MAX_TASKS) {
    tasks[taskCount++] = newTask;
  } else {
    for (int i = 0; i < MAX_TASKS - 1; i++) {
      tasks[i] = tasks[i + 1];
    }
    tasks[MAX_TASKS - 1] = newTask;
  }
}

// ================= TOUCH =================
void handleTouch() {
  static bool lastState = LOW;
  static unsigned long lastTouchTime = 0;

  bool currentState = digitalRead(TOUCH_PIN);

  if (millis() - lastTouchTime > 300) {
    if (currentState == HIGH && lastState == LOW) {

      prevScreen = screenMode;
      screenMode++;
      if (screenMode > 5) screenMode = 0;

      animating = true;
      animationOffset = 128;

      lastTouchTime = millis();
    }
  }
  lastState = currentState;
}

// ================= WEATHER =================
void getWeather() {
  HTTPClient http;

  String url = "http://api.openweathermap.org/data/2.5/weather?q=" +
               city + "&appid=" + apiKey + "&units=metric";

  http.begin(url);
  int code = http.GET();

  if (code == 200) {
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, http.getString());

    temp = doc["main"]["temp"];
    weather = doc["weather"][0]["main"].as<String>();
  }

  http.end();
}

// ================= AQI =================
void getAQI() {
  HTTPClient http;

  String url = "http://api.openweathermap.org/data/2.5/air_pollution?lat=18.52&lon=73.85&appid=" + apiKey;

  http.begin(url);
  int code = http.GET();

  if (code == 200) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, http.getString());

    float pm25 = doc["list"][0]["components"]["pm2_5"];
    aqi = (int)pm25;

    if (aqi <= 50) airQuality = "Good";
    else if (aqi <= 100) airQuality = "Moderate";
    else if (aqi <= 150) airQuality = "Unhealthy";
    else airQuality = "Very Bad";
  }

  http.end();
}

// ================= BATTERY =================
void getBattery() {
  int raw = analogRead(BATTERY_PIN);
  float voltage = raw * (3.3 / 4095.0) * 2;

  if (voltage >= 4.2) batteryPercent = 100;
  else if (voltage <= 3.3) batteryPercent = 0;
  else batteryPercent = (voltage - 3.3) * 100 / (4.2 - 3.3);
}

// ================= WIFI =================
void getWiFiStrength() {
  wifiStrength = WiFi.RSSI();
}

int getWiFiBars() {
  if (wifiStrength > -60) return 4;
  else if (wifiStrength > -70) return 3;
  else if (wifiStrength > -80) return 2;
  else return 1;
}

// ================= DRAW =================
void drawFace(int offset) {
  if (millis() - lastBlink > 2000) {
    eyesClosed = !eyesClosed;
    lastBlink = millis();
  }

  if (eyesClosed) {
    u8g2.drawLine(40 - offset, 30, 60 - offset, 30);
    u8g2.drawLine(70 - offset, 30, 90 - offset, 30);
  } else {
    u8g2.drawDisc(50 - offset, 30, 5);
    u8g2.drawDisc(80 - offset, 30, 5);
  }

  u8g2.drawLine(50 - offset, 45, 80 - offset, 45);
}

void drawTime(int offset, struct tm &timeinfo) {
  char timeStr[10];
  sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);

  u8g2.setFont(u8g2_font_logisoso20_tr);
  u8g2.drawStr(10 - offset, 40, timeStr);
}

void drawDate(int offset, struct tm &timeinfo) {
  char dateStr[20];
  strftime(dateStr, sizeof(dateStr), "%d %b %Y", &timeinfo);

  char dayStr[10];
  strftime(dayStr, sizeof(dayStr), "%A", &timeinfo);

  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(10 - offset, 20, dayStr);
  u8g2.drawStr(10 - offset, 40, dateStr);
}

void drawWeather(int offset) {
  String tempStr = String(temp) + "C";

  u8g2.setFont(u8g2_font_logisoso16_tr);
  u8g2.drawStr(10 - offset, 40, tempStr.c_str());

  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(10 - offset, 60, weather.c_str());
}

void drawAQI(int offset) {
  u8g2.setFont(u8g2_font_logisoso16_tr);
  u8g2.drawStr(10 - offset, 35, ("AQI: " + String(aqi)).c_str());

  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(10 - offset, 55, airQuality.c_str());
}

void drawTodo(int offset) {
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(2, 10, "TO-DO:");

  for (int i = 0; i < taskCount; i++) {
    int y = 20 + (i * 14);

    String task = tasks[i];
    String line1 = "", line2 = "";

    if (task.length() > 10) {
      line1 = task.substring(0, 10);
      line2 = task.substring(10);
    } else {
      line1 = task;
    }

    String firstLine = String(i + 1) + ") " + line1;
    u8g2.drawStr(2, y, firstLine.c_str());

    if (line2.length() > 0) {
      u8g2.drawStr(10, y + 10, line2.c_str());
    }
  }
}

// ================= MAIN DISPLAY =================
void drawScreen() {

  struct tm timeinfo;
  getLocalTime(&timeinfo);

  u8g2.clearBuffer();

  switch (screenMode) {
    case 0: drawFace(0); break;
    case 1: drawTime(0, timeinfo); break;
    case 2: drawDate(0, timeinfo); break;
    case 3: drawWeather(0); break;
    case 4: drawTodo(0); break;
    case 5: drawAQI(0); break;
  }

  // ===== STATUS BAR =====
  u8g2.setFont(u8g2_font_5x8_tr);

  String batt = String(batteryPercent) + "%";
  u8g2.drawStr(85, 8, batt.c_str());

  int bars = getWiFiBars();
  for (int i = 0; i < bars; i++) {
    u8g2.drawBox(110 + (i * 3), 8 - (i * 2), 2, (i * 2));
  }

  u8g2.sendBuffer();
}