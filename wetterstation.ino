#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>

// === WLAN ===
const char *ssid = "IOT";
const char *password = "20tgmiot18";

// === DHT11 ===
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// === OLED ===
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// === Webserver ===
WebServer server(80);

// === Zeit / NTP ===
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

// === LED Status ===
#define LED_BUILTIN 2
bool ledEnabled = true;

// === Heartbeat Sensor ===
#define HEARTBEAT_PIN 2

struct Messung {
  float temp;
  float hum;
  int pulse;
  String time;
};

#define MAX_HISTORIE 4
Messung history[MAX_HISTORIE];
int historyIndex = 0;

unsigned long lastUpdate = 0;
const unsigned long interval = 5000;

void addMessung(float temp, float hum, int pulse, String time) {
  history[historyIndex] = { temp, hum, pulse, time };
  historyIndex = (historyIndex + 1) % MAX_HISTORIE;
}

String getFormattedTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Zeitfehler";
  }
  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buffer);
}

void updateDisplay(float temp, float hum, int pulse, const String& time) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("ESP32 Wetterstation");
  display.println("--------------------");
  display.print("Temp: "); display.print(temp); display.println(" C");
  display.print("Feucht: "); display.print(hum); display.println(" %");
  display.print("Puls: "); display.print(pulse); display.println(" /1024");
  display.println(time.substring(11));
  display.display();
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="UTF-8">
  <meta http-equiv="refresh" content="10">
  <title>ESP32 Wetterstation</title>
  <style>
    body {
      font-family: 'Segoe UI', sans-serif;
      background: #eef5f9;
      margin: 0;
      padding: 20px;
      color: #333;
    }
    .container {
      max-width: 600px;
      margin: auto;
      background: #fff;
      padding: 30px;
      border-radius: 15px;
      box-shadow: 0 4px 12px rgba(0,0,0,0.1);
    }
    h1 {
      color: #1976d2;
      margin-bottom: 10px;
    }
    .current {
      font-size: 18px;
      margin-top: 20px;
      background: #e3f2fd;
      padding: 15px;
      border-radius: 10px;
    }
    table {
      width: 100%;
      border-collapse: collapse;
      margin-top: 20px;
      font-size: 15px;
    }
    th, td {
      padding: 10px;
      border-bottom: 1px solid #ccc;
      text-align: center;
    }
    th {
      background-color: #bbdefb;
    }
    tr:nth-child(even) {
      background-color: #f9f9f9;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🌤️ ESP32 Wetterstation</h1>
    <div class="current">
)rawliteral";

  int lastIndex = (historyIndex - 1 + MAX_HISTORIE) % MAX_HISTORIE;
  html += "<strong>Temperatur:</strong> " + String(history[lastIndex].temp, 1) + " °C<br>";
  html += "<strong>Feuchtigkeit:</strong> " + String(history[lastIndex].hum, 1) + " %<br>";
  html += "<strong>Puls:</strong> " + String(history[lastIndex].pulse) + " /1024<br>";
  html += "<strong>Zeit:</strong> " + history[lastIndex].time + "</div>";

  html += R"rawliteral(
    <form method="POST" action="/toggleLed">
      <label><input type="checkbox" name="led" onchange="this.form.submit()" )rawliteral";

  if (ledEnabled) html += "checked";

  html += R"rawliteral(> Status-LED aktiv</label>
    </form>
    <h2>Verlauf</h2>
    <table>
      <tr><th>Zeit</th><th>Temp</th><th>Feucht</th><th>Puls</th></tr>
)rawliteral";

  for (int i = 0; i < MAX_HISTORIE; i++) {
    int idx = (historyIndex - 1 - i + MAX_HISTORIE) % MAX_HISTORIE;
    if (history[idx].time != "") {
      html += "<tr><td>" + history[idx].time + "</td><td>" + String(history[idx].temp, 1) +
              " °C</td><td>" + String(history[idx].hum, 1) + " %</td><td>" + String(history[idx].pulse) + "</td></tr>";
    }
  }

  html += "</table></div></body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Wire.begin();  // Standard I2C Pins (SDA=21, SCL=22)
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED nicht gefunden");
    while (true);
  }

  display.clearDisplay();
  display.display();

  WiFi.begin(ssid, password);
  Serial.print("Verbinde mit WLAN");
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    digitalWrite(LED_BUILTIN, HIGH); delay(200);
    digitalWrite(LED_BUILTIN, LOW); delay(200);
    retry++;
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWLAN-Verbindung fehlgeschlagen!");
    while (true) {
      if (ledEnabled) {
        digitalWrite(LED_BUILTIN, HIGH); delay(100);
        digitalWrite(LED_BUILTIN, LOW); delay(100);
      }
    }
  }

  Serial.println("\nVerbunden!");
  Serial.println(WiFi.localIP());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  server.on("/", handleRoot);
  server.on("/toggleLed", HTTP_POST, []() {
    if (server.hasArg("led")) {
      ledEnabled = true;
    } else {
      ledEnabled = false;
      digitalWrite(LED_BUILTIN, LOW);
    }
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.begin();
  Serial.println("Webserver gestartet");
}

void loop() {
  server.handleClient();

  unsigned long now = millis();
  if (now - lastUpdate >= interval) {
    lastUpdate = now;

    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    int pulse = analogRead(HEARTBEAT_PIN);
    String time = getFormattedTime();

    if (!isnan(temp) && !isnan(hum)) {
      addMessung(temp, hum, pulse, time);
      updateDisplay(temp, hum, pulse, time);

      // LED Status
      if (ledEnabled) {
        if (temp > 28) {
          for (int i = 0; i < 3; i++) {
            digitalWrite(LED_BUILTIN, HIGH); delay(100);
            digitalWrite(LED_BUILTIN, LOW); delay(100);
          }
        } else {
          digitalWrite(LED_BUILTIN, HIGH); delay(100);
          digitalWrite(LED_BUILTIN, LOW);
        }
      }

      Serial.println("Messung aktualisiert");
    } else {
      Serial.println("Sensorfehler");
    }
  }
}