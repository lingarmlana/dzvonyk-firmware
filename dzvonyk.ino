#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>

const int CALL_GREEN_LED_PIN = 5;
const int FIND_RED_LED_PIN = 6;
const int STATUS_YELLOW_LED_PIN = 7;
const int FIND_BUTTON_PIN = 8;
const int DF_TX_PIN = 3; // До RX (через резистор 1кОм)
const int DF_RX_PIN = 4; // До TX

HardwareSerial dfSerial(1);
DFRobotDFPlayerMini myDFPlayer;

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "BEB5483E-36E1-4688-B7F5-EA07361B26AA"

bool deviceConnected = false;
BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristic = nullptr;

volatile bool newCommandAvailable = false;
String globalStatus = "";
int globalNumber = 0;

bool lastButtonState = HIGH;
unsigned long previousButtonMillis = 0;
const long buttonInterval = 50; 

unsigned long pressStartTime = 0;   
bool isHolding = false;             
bool longPressTriggered = false;    

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        digitalWrite(STATUS_YELLOW_LED_PIN, HIGH);
        Serial.println("[BLE] Телефон підключено!");
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        digitalWrite(STATUS_YELLOW_LED_PIN, LOW);
        Serial.println("[BLE] Телефон відключено. Перезапуск реклами...");
        delay(500);
        pServer->getAdvertising()->start();
    }
};

void playRingTone(String status, int number) {
  if (status == "ring") {
    digitalWrite(CALL_GREEN_LED_PIN, HIGH);
    myDFPlayer.playMp3Folder(number);
    Serial.println("[DFPlayer] Запуск треку " + String(number) + " (дзвінок)");
  } 
  else if (status == "stop") {
    digitalWrite(CALL_GREEN_LED_PIN, LOW);
    myDFPlayer.stop();
    Serial.println("[DFPlayer] Зупинка треку");
  }
}

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        String value = pCharacteristic->getValue();

        if (value.length() > 0) {
            Serial.println("[BLE] Отримано дані: " + value);

            int separatorIndex = value.indexOf(':');
            if (separatorIndex != -1) {
                String status = value.substring(0, separatorIndex);
                String number = value.substring(separatorIndex + 1);
                
                globalStatus = status;
                globalNumber = number.toInt();
                newCommandAvailable = true;

                if (status == "ring" || status == "stop") {
                    String ackMessage = "ack:" + status;
                    pCharacteristic->setValue(ackMessage.c_str());
                    pCharacteristic->notify();
                    Serial.println("[BLE] Надіслано підтвердження: " + ackMessage);
                }

                Serial.println("-> Черга: Статус = " + status + ", Номер = " + number);
            }
        }
    }
};

void sendPhoneFindRequest(String action) {
  Serial.println("[КНОПКА] Запит на пошук телефону: " + action);
  if (deviceConnected) {
    pCharacteristic->setValue(("find:" + action).c_str());
    pCharacteristic->notify();
  } else {
    Serial.println("[BLE] Неможливо надіслати: телефон не підключено по Bluetooth.");
  }
}

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
            digitalWrite(FIND_RED_LED_PIN, LOW);
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
      digitalWrite(FIND_RED_LED_PIN, (currentMillis / 50) % 2);

      // === Скид BLE ===
      // if (pServer->getConnectedCount() > 0) {
      //     pServer->disconnect(pServer->getConnId());
      //     Serial.println("[BLE] Поточне з'єднання примусово розірвано кнопкою!");
      // }
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(FIND_BUTTON_PIN, INPUT_PULLUP);
  pinMode(FIND_RED_LED_PIN, OUTPUT);
  pinMode(STATUS_YELLOW_LED_PIN, OUTPUT);
  pinMode(CALL_GREEN_LED_PIN, OUTPUT);
  digitalWrite(FIND_RED_LED_PIN, LOW);
  digitalWrite(STATUS_YELLOW_LED_PIN, LOW);
  digitalWrite(CALL_GREEN_LED_PIN, LOW);

  dfSerial.begin(9600, SERIAL_8N1, DF_RX_PIN, DF_TX_PIN);
  Serial.println("Ініціалізація DFPlayer Mini...");

  if (!myDFPlayer.begin(dfSerial, true, false)) {
    Serial.println("Помилка: DFPlayer Mini не знайдено!");
  } else {
    Serial.println("DFPlayer Mini успішно запущено!");
    myDFPlayer.volume(25);
  }

  Serial.println("\nІніціалізація BLE сервера...");
  BLEDevice::init("Dzvonyk");

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_WRITE_NR |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setName("Dzvonyk");
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // для стабильного коннекту з iOS
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.println("BLE сервер запущено!");
}

void loop() {
  unsigned long currentMillis = millis(); 

  handleFindPhone(currentMillis);

  // Безопасное выполнение тяжелых команд вне BLE прерывания
  if (newCommandAvailable) {
    newCommandAvailable = false;
    playRingTone(globalStatus, globalNumber);
  }
}
