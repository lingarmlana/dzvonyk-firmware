#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <Preferences.h> // Библиотека для сохранения данных в память платы

#include "index.h"
#include "setup.h"
#include "save.h"

WebServer server(80);      // Веб-сервер на 80 порту
Preferences preferences;   // Объект для работы с энергонезависимой памятью

void handleRoot() {
  // Если плата не подключена к роутеру (режим точки доступа) — показываем форму настройки
  if (WiFi.status() != WL_CONNECTED) {
    server.send(200, "text/html", getSetupPage());
    return;
  }

  // Если плата подключена дома — показываем статус
  String ip = WiFi.localIP().toString();
  String ssid = WiFi.SSID();
  String rssi = String(WiFi.RSSI()); // Рівень сигналу
  
  String html = getIndexPage(ssid, ip, rssi);
    
  server.send(200, "text/html", html);
}

void handleSave() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  // Сохраняем в память платы
  preferences.begin("wifi-config", false);
  preferences.putString("ssid", ssid);
  preferences.putString("pass", pass);
  preferences.end();

  // Відправляємо сторінку редіректу із файлу save.h
  server.send(200, "text/html", save_html);
  
  delay(1000); // Даємо серверу час відправити відповідь
  ESP.restart(); // Перезагружаем плату, чтобы она зашла в домашнюю сеть
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

  // Настраиваем маршруты сервера ВСЕГДА
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  
  // Тестовый маршрут для будущего звонка от Flutter:
  server.on("/call", HTTP_GET, []() {
    String status = server.arg("status");
    String number = server.arg("number");
    Serial.println("Отримано дзвінок від додатка! Статус: " + status + ", Номер: " + number);
    server.send(200, "text/plain", "OK");
  });

  // Если данных нет — запускаем режим Точки Доступа (AP)
  if (savedSsid == "") {
    Serial.println("\nNet sozhranennyh nastroek. Zapuskayu tochku dostupa...");
    WiFi.softAP("Dzvonyk", "12345678");
    
    IPAddress IP = WiFi.softAPIP();
    Serial.print("IP-adres tochki dostupa: ");
    Serial.println(IP);

    // Запускаем mDNS и для точки доступа тоже
    if (MDNS.begin("dzvonyk")) {
      Serial.println("mDNS responder started: http://dzvonyk.local");
    }
  } 
  else {
    // Если настройки есть — пробуем подключиться к домашнему роутеру
    Serial.println("\n Podklyuchenie k sosedney seti: " + savedSsid);
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
      }
    } else {
      Serial.println("\nNe udalos' podklyuchitsya. Sbros nastroek...");
      preferences.begin("wifi-config", false);
      preferences.clear();
      preferences.end();
      ESP.restart();
    }
  }

  // Запускаем сам сервер в самом конце setup()
  server.begin();
}

void loop() {
  server.handleClient();
}
