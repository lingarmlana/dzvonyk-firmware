#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>

#include "index.h"
#include "setup.h"
#include "save.h"

WebServer server(80);
WiFiUDP udp;
const int udpPort = 4210;
Preferences preferences;

const int CALL_LED_PIN = 18;
const int FIND_BUTTON_PIN = 23;
const int FIND_LED_PIN = 26;
const int WIFI_LED_PIN = 2;

// Переменные для кнопки и таймеров (в стиле прошлого проекта)
bool lastButtonState = HIGH;
unsigned long previousButtonMillis = 0;
const long buttonInterval = 50; // Интервал антидребезга

unsigned long pressStartTime = 0;   // Время начала удержания
bool isHolding = false;             // Флаг: кнопка сейчас зажата
bool longPressTriggered = false;    // Флаг: удержание 2+ сек уже сработало

void handleRoot() {
  if (WiFi.status() != WL_CONNECTED) {
    server.send(200, "text/html", getSetupPage());
    return;
  }

  String ip = WiFi.localIP().toString();
  String ssid = WiFi.SSID();
  String rssi = String(WiFi.RSSI());
  String html = getIndexPage(ssid, ip, rssi);
    
  server.send(200, "text/html", html);
}

void handleWifiSave() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  preferences.begin("wifi-config", false);
  preferences.putString("ssid", ssid);
  preferences.putString("pass", pass);
  preferences.end();

  server.send(200, "text/html", save_html);
  delay(1000);
  ESP.restart();
}

void handlePhoneCall() {
  if (server.hasArg("status") && server.hasArg("number")) {
    String status = server.arg("status");
    String number = server.arg("number");
    
    Serial.println("Отримано дзвінок! Статус: " + status + ", Номер: " + number);
    playRingTone(status);
    
    server.send(200, "text/plain", "OK: Call received for " + number);
  } else {
    server.send(400, "text/plain", "Error: Missing parameters (status or number)");
  }
}

void playRingTone(String status) {
  if (status == "ring") {
    digitalWrite(CALL_LED_PIN, HIGH);
  } 
  else if (status == "stop") {
    digitalWrite(CALL_LED_PIN, LOW);
  }
}

void sendPhoneFindRequest(String action) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String serverPath = "http://dzvonyk.local:8080/find?action=" + action;
    
    http.begin(serverPath.c_str());
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      Serial.println("Запит на пошук телефону (" + action + ") успішно надіслано!");
    } else {
      Serial.print("Помилка надсилання запиту, код: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("Помилка: WiFi не підключено!");
  }
}

// --- Неблокирующая обработка кнопки и диода с логами ---
void handleFindPhone(unsigned long currentMillis) {
  // 1. Опрос состояния кнопки с интервалом антидребезга (50мс)
  if (currentMillis - previousButtonMillis >= buttonInterval) {
    previousButtonMillis = currentMillis;

    int currentButtonState = digitalRead(FIND_BUTTON_PIN);

    // Логика срабатывает ТОЛЬКО при реальном изменении состояния кнопки
    if (currentButtonState != lastButtonState) {
      lastButtonState = currentButtonState;

      if (currentButtonState == LOW) {
        // Кнопку только что нажали
        pressStartTime = currentMillis;
        isHolding = true;
        longPressTriggered = false;
        Serial.println("[КНОПКА] Натиснуто (LOW)");
      } else {
        // Кнопку отпустили
        if (isHolding) {
          unsigned long duration = currentMillis - pressStartTime;
          Serial.print("[КНОПКА] Відпущено. Час утримання: ");
          Serial.print(duration);
          Serial.println(" мс");

          if (longPressTriggered) {
            // Удерживали дольше 2 секунд -> "start"
            Serial.println("-> Дія за кнопкою: start (довге утримання)");
            sendPhoneFindRequest("start");
          } else if (duration >= 50) {
            // Короткий клик -> "stop"
            Serial.println("-> Дія за кнопкою: stop (короткий клік)");
            digitalWrite(FIND_LED_PIN, LOW);
            sendPhoneFindRequest("stop");
          }
          isHolding = false;
        }
      }
    }
  }

  // 2. Динамическая логика мигания ТОЛЬКО пока кнопка реально зажата физически
  if (isHolding && digitalRead(FIND_BUTTON_PIN) == LOW) {
    unsigned long heldDuration = currentMillis - pressStartTime;
    if (heldDuration >= 2000 && !longPressTriggered) {
      longPressTriggered = true;
      Serial.println("[ДІОД] Поріг у 2 секунди пройдено, починаємо мигання!");
    }
    
    if (heldDuration >= 2000) {
      // Мигаем диодом каждые 200мс
      digitalWrite(FIND_LED_PIN, (currentMillis / 50) % 2);
    }
  }
}

void handleUdpPackets() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char incomingPacket[255];
    int len = udp.read(incomingPacket, 255);
    if (len > 0) {
      incomingPacket[len] = '\0'; // Завершуємо рядок
    }
    
    String message = String(incomingPacket);
    Serial.println("Отримано UDP пакет: " + message);

    // Розбираємо формат "status:number" (наприклад, "ring:123456789" або "stop:123456789")
    int separatorIndex = message.indexOf(':');
    if (separatorIndex != -1) {
      String status = message.substring(0, separatorIndex);
      String number = message.substring(separatorIndex + 1);
      
      // Викликаємо твою готову логіку обробки дзвінка!
      playRingTone(status);
      Serial.println("-> UDP дія: Статус = " + status + ", Номер = " + number);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // --- ДОДАЙ ЦІ ДВА РЯДКИ ТИМЧАСОВО ---
  // preferences.begin("wifi-config", false);
  // preferences.clear(); // Повністю очищає всю пам'ять у цьому сховищі
  // preferences.end();
  // ------------------------------------

  // Читаем сохраненные ранее данные из памяти
  preferences.begin("wifi-config", true);
  String savedSsid = preferences.getString("ssid", "");
  String savedPass = preferences.getString("pass", "");
  //String savedSsid = preferences.getString("ssid", "netis_2.4G_A3AE36");
  //String savedPass = preferences.getString("pass", "password");
  preferences.end();

  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleWifiSave);
  server.on("/call", HTTP_GET, handlePhoneCall);
  
  // Тестовый маршрут для будущего звонка от Flutter:
  //server.on("/call", HTTP_GET, []() {
    //String status = server.arg("status");
    //String number = server.arg("number");
    //Serial.println("Отримано дзвінок від додатка! Статус: " + status + ", Номер: " + number);
    //server.send(200, "text/plain", "OK");
  //});

  pinMode(FIND_BUTTON_PIN, INPUT_PULLUP);
  pinMode(FIND_LED_PIN, OUTPUT);
  pinMode(WIFI_LED_PIN, OUTPUT);
  pinMode(CALL_LED_PIN, OUTPUT);
  digitalWrite(FIND_LED_PIN, LOW);

  if (savedSsid == "") {
    Serial.println("\nNet sozhranennyh nastroek. Zapuskayu tochku dostupa...");
    WiFi.softAP("Dzvonyk", "12345678");
    
    IPAddress IP = WiFi.softAPIP();
    Serial.print("IP-adres tochki dostupa: ");
    Serial.println(IP);

    if (MDNS.begin("dzvonyk")) {
      Serial.println("mDNS responder started: http://dzvonyk.local");
    }

    udp.begin(udpPort);
    Serial.print("UDP сервер (AP) запущено на порту: ");
    Serial.println(udpPort);
  } 
  else {
    Serial.println("\nPodklyuchenie k сети: " + savedSsid);
    WiFi.begin(savedSsid.c_str(), savedPass.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nUspeshno! IP: " + WiFi.localIP().toString());

      if (MDNS.begin("dzvonyk")) {
        Serial.println("mDNS responder started: http://dzvonyk.local");
        digitalWrite(WIFI_LED_PIN, HIGH);
      }

        // Запускаємо прослуховування UDP-пакетів
        udp.begin(udpPort);
        Serial.print("UDP сервер запущено на порту: ");
        Serial.println(udpPort);
    } else {
      Serial.println("\nNe udalos' podklyuchitsya. Sbros nastroek...");
      preferences.begin("wifi-config", false);
      preferences.clear();
      preferences.end();
      ESP.restart();
    }
  }

  server.begin();
}

void loop() {
  unsigned long currentMillis = millis(); // Единый источник времени

  server.handleClient();          // Обслуживаем веб-сервер
  handleUdpPackets();
  handleFindPhone(currentMillis); // Неблокирующая проверка кнопки и диода
}