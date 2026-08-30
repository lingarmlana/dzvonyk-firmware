#ifndef SAVE_H
#define SAVE_H

const char save_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset='utf-8'>
    <meta http-equiv='refresh' content='5;url=http://dzvonyk.local/'>
    <title>Saved</title>
    <script>
        let seconds = 5;
        function countdown() {
            seconds--;
            document.getElementById('timer').innerText = seconds;
        }
        setInterval(countdown, 1000);
    </script>
</head>
<body style='font-family: Arial; text-align: center; margin-top: 50px;'>
    <h2>Налаштування збережено!</h2>
    <p>Плата перезавантажується і підключається до мережі...</p>
    <p>Зараз ви будете автоматично перенаправлені на головну сторінку через <span id="timer">5</span> секунд.</p>
</body>
</html>
)rawliteral";

#endif