#ifndef HEADER_H
#define HEADER_H

const char header_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset='utf-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <title>Дзвоник</title>
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
            background-color: #f4f6f8;
            color: #333;
            text-align: center;
            margin: 0;
            padding: 20px;
        }
        .card {
            background: white;
            max-width: 400px;
            margin: 40px auto;
            padding: 30px;
            border-radius: 12px;
            box-shadow: 0 4px 12px rgba(0,0,0,0.08);
        }
        h1, h2 { color: #1a73e8; }
        input[type='text'], input[type='password'] {
            padding: 12px;
            width: 100%;
            box-sizing: border-box;
            margin: 10px 0;
            border: 1px solid #ddd;
            border-radius: 6px;
            font-size: 16px;
        }
        input[type='submit'] {
            padding: 12px 20px;
            background: #1a73e8;
            color: white;
            border: none;
            border-radius: 6px;
            font-size: 16px;
            cursor: pointer;
            width: 100%;
            margin-top: 10px;
        }
        input[type='submit']:hover { background: #1557b0; }
        .btn-green { background: #34a853 !important; }
        .btn-green:hover { background: #2d8a43 !important; }
        hr { border: none; border-top: 1px solid #eee; margin: 20px 0; }
    </style>
</head>
<body>
    <div class='card'>
)rawliteral";

#endif