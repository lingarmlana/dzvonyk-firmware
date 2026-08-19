#ifndef INDEX_H
#define INDEX_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset='utf-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <title>Dzvonyk</title>
</head>
<body style='font-family: Arial; text-align: center; margin-top: 50px;'>
    <h1>Привіт! Я Дзвоник</h1>
    <p>Статус: <b>Працюю та очікую на дзвінки</b></p>
    <p>Підключено до: <b>%SSID%</b></p>
    <p>Моя IP-адреса: <b>%IP%</b></p>
    <p>Якість сигналу: <b>%RSSI% dBm</b></p>
    <hr>
    <h3>Змінити Wi-Fi мережу:</h3>
    <form action='/save' method='POST'>
        <input type='text' name='ssid' placeholder='Новий SSID' style='padding: 10px; width: 80%; margin: 10px;'><br>
        <input type='password' name='pass' placeholder='Новий пароль' style='padding: 10px; width: 80%; margin: 10px;'><br>
        <input type='submit' value='Зберегти та перезавантажити' style='padding: 10px 20px; background: blue; color: white; border: none;'>
    </form>
</body>
</html>
)rawliteral";

#endif