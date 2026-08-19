#ifndef SETUP_H
#define SETUP_H

const char setup_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset='utf-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <title>WiFi Setup</title>
</head>
<body style='font-family: Arial; text-align: center; margin-top: 50px;'>
    <h2>Налаштування Wi-Fi для Дзвоника</h2>
    <form action='/save' method='POST'>
        SSID (Ім'я мережі):<br>
        <input type='text' name='ssid' style='padding: 10px; width: 80%; margin: 10px;'><br>
        Password (Пароль):<br>
        <input type='password' name='pass' style='padding: 10px; width: 80%; margin: 10px;'><br><br>
        <input type='submit' value='Зберегти і Підключитися' style='padding: 10px 20px; background: green; color: white; border: none;'>
    </form>
</body>
</html>
)rawliteral";

#endif