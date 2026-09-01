import asyncio
from bleak import BleakClient, BleakScanner

# Наши UUID из прошивки ESP32
SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
CHARACTERISTIC_UUID = "BEB5483E-36E1-4688-B7F5-EA07361B26AA"
DEVICE_NAME = "Dzvonyk_ESP32"

async def main():
    print(f"Шукаю пристрій {DEVICE_NAME} в ефірі...")
    device = await BleakScanner.find_device_by_name(DEVICE_NAME, timeout=10.0)
    
    if not device:
        print("❌ Не знайдено пристрій! Перевір, чи ввімкнена плата.")
        return

    print(f"✅ Знайдено! Підключаюся до {device.address}...")

    async with BleakClient(device) as client:
        print("🔗 Зондування та підключення успішні!")
        print("Введи команду для відправки (наприклад: ring:12345, stop:12345) або 'exit' для виходу:")

        while True:
            # Читаем текст прямо из терминала
            command = await asyncio.to_thread(input, "\nКоманда > ")
            
            if command.lower() == 'exit':
                print("Відключення...")
                break
            
            if not command.strip():
                continue

            # Отправляем строку в характеристику ESP32 по BLE
            data_bytes = command.encode('utf-8')
            await client.write_gatt_char(CHARACTERISTIC_UUID, data_bytes, response=False)
            print(f"📤 Надіслано на плату: {command}")

if __name__ == "__main__":
    asyncio.run(main())