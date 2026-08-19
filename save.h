#ifndef SAVE_H
#define SAVE_H

const char save_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset='utf-8'>
    <meta http-equiv='refresh' content='5;url=http://dzvonyk.local/'>
    <title>Saved</title>
</head>
<body style='font-family: Arial; text-align: center; margin-top: 50px;'>
    <h2>Налаштування збережено!</h2>
    <p>Плата перезавантажується і підключається до мережі...</p>
    <p>Зараз ви будете автоматично перенаправлені на головну сторінку через 5 секунд.</p>
</body>
</html>
)rawliteral";

#endif