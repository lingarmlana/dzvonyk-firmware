#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

const int CALL_LED_PIN = 18;
const int FIND_BUTTON_PIN = 23;
const int FIND_LED_PIN = 26;
const int STATUS_LED_PIN = 2; // Вместо WIFI_LED теперь показывает статус BLE подключения

// Уникальные UUID для BLE сервиса и характеристики (сгенерированы для проекта Dzvonyk)
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "BEB5483E-36E1-4688-B7F5-EA07361B26AA"

bool deviceConnected = false;
BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristic = nullptr;

// Переменные для кнопки и таймеров
bool lastButtonState = HIGH;
unsigned long previousButtonMillis = 0;
const long buttonInterval = 50; 

unsigned long pressStartTime = 0;   
bool isHolding = false;             
bool longPressTriggered = false;    

// Класс для отслеживания подключения/отключения телефона по BLE
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnected(BLEServer* pServer) {
        deviceConnected = true;
        digitalWrite(STATUS_LED_PIN, HIGH);
        Serial.println("[BLE] Телефон підключено!");
    }

    void onDisconnected(BLEServer* pServer) {
        deviceConnected = false;
        digitalWrite(STATUS_LED_PIN, LOW);
        Serial.println("[BLE] Телефон відключено. Перезапуск реклами (вай-фая нет, ищем эфир)...");
        // Перезапускаем рекламный маячок, чтобы телефон мог снова нас найти
        delay(500);
        pServer->getAdvertising()->start();
    }
};

void playRingTone(String status) {
  if (status == "ring") {
    digitalWrite(CALL_LED_PIN, HIGH);
  } 
  else if (status == "stop") {
    digitalWrite(CALL_LED_PIN, LOW);
  }
}

// Класс для обработки входящих данных от приложения (когда телефон присылает статус звонка)
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        String value = pCharacteristic->getValue();

        if (value.length() > 0) {
            Serial.println("[BLE] Отримано дані: " + value);

            // Разбираем формат "status:number" (например, "ring:123456789" или "stop:123456789")
            int separatorIndex = value.indexOf(':');
            if (separatorIndex != -1) {
                String status = value.substring(0, separatorIndex);
                String number = value.substring(separatorIndex + 1);
                
                playRingTone(status);
                Serial.println("-> Дія: Статус = " + status + ", Номер = " + number);
            }
        }
    }
};

// Заглушка для отправки запроса на поиск телефона (пока оставляем в виде логов, потом прикрутим обратную связь по BLE)
void sendPhoneFindRequest(String action) {
  Serial.println("[КНОПКА] Запит на пошук телефону: " + action);
  if (deviceConnected) {
    // Здесь можно будет отправить уведомление на телефон через BLE Characteristic Read/Notify
    pCharacteristic->setValue(("find:" + action).c_str());
    pCharacteristic->notify();
  } else {
    Serial.println("[BLE] Неможливо надіслати: телефон не підключено по Bluetooth.");
  }
}

// --- Неблокирующая обработка кнопки и диода ---
void handleFindPhone(unsigned long currentMillis) {
  if (currentMillis - previousButtonMillis >= buttonInterval) {
    previousButtonMillis = currentMillis;

    int currentButtonState = digitalRead(FIND_BUTTON_PIN);

    if (currentButtonState != lastButtonState) {
      lastButtonState = currentButtonState;

      if (currentButtonState == LOW) {
        pressStartTime = currentMillis;
        isHolding = true;
        longPressTriggered = false;
        Serial.println("[КНОПКА] Натиснуто (LOW)");
      } else {
        if (isHolding) {
          unsigned long duration = currentMillis - pressStartTime;
          Serial.print("[КНОПКА] Відпущено. Час утримання: ");
          Serial.print(duration);
          Serial.println(" мс");

          if (longPressTriggered) {
            Serial.println("-> Дія за кнопкою: start (довге утримання)");
            sendPhoneFindRequest("start");
          } else if (duration >= 50) {
            Serial.println("-> Дія за кнопкою: stop (короткий клік)");
            digitalWrite(FIND_LED_PIN, LOW);
            sendPhoneFindRequest("stop");
          }
          isHolding = false;
        }
      }
    }
  }

  if (isHolding && digitalRead(FIND_BUTTON_PIN) == LOW) {
    unsigned long heldDuration = currentMillis - pressStartTime;
    if (heldDuration >= 2000 && !longPressTriggered) {
      longPressTriggered = true;
      Serial.println("[ДІОД] Поріг у 2 секунди пройдено, починаємо мигання!");
    }
    
    if (heldDuration >= 2000) {
      digitalWrite(FIND_LED_PIN, (currentMillis / 50) % 2);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(FIND_BUTTON_PIN, INPUT_PULLUP);
  pinMode(FIND_LED_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(CALL_LED_PIN, OUTPUT);
  digitalWrite(FIND_LED_PIN, LOW);
  digitalWrite(STATUS_LED_PIN, LOW);

  
  //digitalWrite(CALL_LED_PIN, HIGH);
  //digitalWrite(FIND_LED_PIN, HIGH);
  //digitalWrite(STATUS_LED_PIN, HIGH);

  Serial.println("\nІніціалізація BLE сервера...");

  // 1. Инициализируем BLE устройство с именем "Dzvonyk_ESP32"
  BLEDevice::init("Dzvonyk_ESP32");

  // 2. Создаем сервер
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // 3. Создаем сервис
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // 4. Создаем характеристику для приема данных от телефона (WRITE + READ + NOTIFY)
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_WRITE_NR |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902()); // Дескриптор для нотификаций

  // 5. Запускаем сервис
  pService->start();

  // 6. Настраиваем и запускаем рекламный маячок (Advertising), чтобы айфон видел плату в эфире
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // функции для стабильного коннекта с iOS
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.println("BLE сервер запущено! Очікування підключення телефона...");
}

void loop() {
  unsigned long currentMillis = millis(); 

  handleFindPhone(currentMillis); // Кнопка работает автономно и четко
  
  // Больше нет server.handleClient() и udp.parsePacket() — эфир чист, работает только чистый BLE!
}