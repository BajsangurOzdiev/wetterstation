# 🌦️ Wetterstation mit OLED & Webinterface

**Verfasser:** Ozdiev, Petrovic  
**Datum:** 05.06.2025  
**Projekt:** ITP/SYT Schulprojekt

---

## 1. Einführung

In diesem Projekt wurde mit dem Mikrocontroller **ESP32-C3** eine smarte Wetterstation gebaut, die Temperatur, Luftfeuchtigkeit und Puls misst. Die Werte werden auf einem **OLED-Display** angezeigt und über ein modernes **Webinterface** bereitgestellt. Zusätzlich gibt es eine **eingebaute Status-LED**, die visuell über WLAN-Status und Temperatur-Alarm informiert.

---

## 2. Verwendete Komponenten

| Komponente           | Funktion                                    |
|----------------------|---------------------------------------------|
| ESP32-C3             | Zentrale Steuerung, Webserver, Sensoren     |
| DHT11                | Temperatur- und Luftfeuchtigkeitssensor     |
| OLED (SSD1306, I²C)  | Anzeige der Messdaten direkt am Gerät       |
| Heartbeat-Sensor B29 | Misst Herzfrequenz per Fingerkontakt        |
| WLAN                 | Verbindung zu Netzwerk und Webanzeige       |
| Eingebaute LED       | Zeigt WLAN- und Temperaturstatus            |

---

## 1. Mikrocontroller: ESP32-C3

Der **ESP32-C3** ist ein moderner Mikrocontroller mit integriertem **WLAN** und **Bluetooth**, ideal für IoT-Projekte. Er bietet genügend Leistung, um Sensoren auszulesen, Daten zu verarbeiten, ein Display zu steuern und gleichzeitig einen eigenen Webserver zu betreiben.

---
## 2. DHT11


Der DHT11 ist ein digitaler Sensor zur Erfassung von Temperatur und Luftfeuchtigkeit. Er eignet sich besonders gut für Einsteiger


---

## 3. Heartbeat b29

Der Heartbeat b29 ist ein Pulssensor aus dem "40 in 1 SensorKit for 4duino" der den rohwert des Pulses misst indem man den finger dranhält

## 4. OLED-Display:
Das OLED-Display ist die uns zugeteilte Variante(5), auf diesem sollen die Daten veranschaulicht werden (auf dem webinterface ebenfalls). Wir haben ein 0,96 zoll OLED-Display bekommen.

angezigt wird es wie folgt:
- ESP32 Wetterstation
- Temperatur
- Luftfeuchtigkeit
- Puls


## 5. LED-Statusanzeige 

Die eingebaute LED dient als einfache Statusanzeige:

- 🔄 **WLAN-Verbindung aktivieren:**  
  → LED blinkt **schnell** während Verbindungsaufbau 

- ❌ **WLAN fehlgeschlagen:**  
  → LED blinkt **dauerhaft schnell**

- ✅ **Normale Messung bei < 28 °C:**  
  → LED **blinkt einmal kurz** bei jeder Messung 

- 🔥 **Überhitzung (> 28 °C):**  
  → LED **blinkt dreimal kurz hintereinander**

- 📴 **LED ausgeschaltet im Webinterface:**  
  → LED bleibt **aus**, alle Signale deaktiviert

---

## 6. Webinterface

Das Webinterface ist über die IP des ESP32 im Netzwerk aufrufbar (z. B. `http://192.168.0.X`). Es zeigt:

- Die aktuellen Messdaten (Temperatur, Feuchtigkeit, Puls)
- Eine Verlaufstabelle der letzten 4 Messungen
- Zeitstempel bei jeder Messung
- Eine Checkbox zur Steuerung der Status-LED


---



---
## 3. Pinbelegung

| Sensor / Modul       | ESP32-C3 Pin |
|----------------------|--------------|
| DHT11                | GPIO 4       |
| Heartbeat B29        | GPIO 2 (AOUT)  GPIO 3 (DOUT)|
| OLED (SDA / SCL)     | SDA = GPIO 21, SCL = GPIO 22 *(Standard I²C)* |


---



## 4. Arbeitsschritte

1. **ESP32-C3 einrichten**
   - Board-Manager in Arduino IDE konfiguriert
   - Bibliotheken installiert: `WiFi`, `WebServer`, `DHT`, `Adafruit_SSD1306`, `Adafruit_GFX`

2. **Sensoren und Display verbinden**
   - **DHT11** an GPIO 4
   - **Heartbeat B29** AOUT an GPIO 2 und DOUT an GPIO 3
   - **OLED Display** via I2C (SCL, SDA)
   
3. **Code schreiben**
   - den code schreiben 

4. **Webinterface programmieren**
   - HTML/CSS direkt im ESP-Code eingebettet
    - abrufbar im lokalen Netzwerk mit IP



7. **Testphase**
- WLAN-Verbindung geprüft
- Sensorwerte validiert
- Display- und 
- Webinterface angeschaut

---

## 5. Schaltplan 
![schaltplan](https://github.com/user-attachments/assets/b40e78cf-5469-4f31-8f15-e0fdbd629582)


---
## 6. Schaltung
![schaltung](https://github.com/user-attachments/assets/8835b4ce-4a33-443d-8260-bcdfec1fb24d)


## 7. Webinterface 
![webinterface](https://github.com/user-attachments/assets/0358e1e6-84f3-47a2-83b1-69ba7d7c8d9e)

## 8. Quellcode

```cpp
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
```
## 9. Quellen

1. ChatGPT. *ChatGPT*. Verfügbar unter: [https://chatgpt.com/](https://chatgpt.com/) (Zugriff am: 20. Mai 2025).

2. Random Nerd Tutorials. *(o. D.)* *ESP32 Web Server Tutorial*. Verfügbar unter: [https://randomnerdtutorials.com/esp32-web-server-arduino-ide/](https://randomnerdtutorials.com/esp32-web-server-arduino-ide/) (Zugriff am: 20. Mai 2025).

3. Arduino. *(o. D.)* *DHT11 Temperature Sensor Tutorial*. Verfügbar unter: [https://arduinogetstarted.com/tutorials/arduino-dht11](https://arduinogetstarted.com/tutorials/arduino-dht11) (Zugriff am: 20. Mai 2025).

4. ESPressif. *(o. D.)* *ESP32-C3 DevKitM-1 User Guide*. Verfügbar unter: [https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32c3/hw-reference/esp32c3/user-guide-devkitm-1.html](https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32c3/hw-reference/esp32c3/user-guide-devkitm-1.html) (Zugriff am: 20. Mai 2025).
5. Domenico et al. (2020) ESP32 NTP client-server: Get date and Time (arduino IDE), Random Nerd Tutorials. Available at: https://randomnerdtutorials.com/esp32-date-time-ntp-client-server-arduino/ (Accessed: 20 May 2025).
