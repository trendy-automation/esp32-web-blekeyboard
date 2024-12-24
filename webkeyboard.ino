#include <BleKeyboard.h>
#include <BLEDevice.h>
#include <WiFi.h>
#include <WebServer.h>

BleKeyboard bleKeyboard("ESP32KB");

// Список Wi-Fi сетей
const char* wifiNetworks[][2] = {
    {"ssid", "pass"}
};

const int wifiNetworkCount = sizeof(wifiNetworks) / sizeof(wifiNetworks[0]);

// Создаем веб-сервер на порту 80
WebServer server(80);

#define DELAY_BETWEEN_CHARS 10 // Задержка между отправкой строк в миллисекундах
#define DELAY_BETWEEN_LINES 100 // Задержка между отправкой строк в миллисекундах
// Флаг для предотвращения повторной отправки
bool textSent = false;
bool welcomeMsg = false;
int signalLevel = 0;
bool isRussian = false; // Текущая раскладка: false - латиница, true - русская


// Функция сканирования Wi-Fi сетей
int scanWiFiNetworks(const char* ssid) {
    Serial.println("Scanning Wi-Fi networks...");
    int n = WiFi.scanNetworks();
    if (n == 0) {
        Serial.println("No networks found.");
        return -100;
    }

    for (int i = 0; i < n; ++i) {
        if (WiFi.SSID(i) == ssid) {
            Serial.printf("Found network: %s, Signal: %d dBm\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
            WiFi.scanDelete(); // Освобождаем память после сканирования
            return WiFi.RSSI(i);
        }
    }

    WiFi.scanDelete(); // Освобождаем память, если сеть не найдена
    Serial.println("Network not found.");
    return -100;
}

// Функция подключения к Wi-Fi
bool connectToWiFi() {
    for (int i = 0; i < wifiNetworkCount; i++) {
        const char* ssid = wifiNetworks[i][0];
        const char* password = wifiNetworks[i][1];
        signalLevel = scanWiFiNetworks(ssid);

        if (signalLevel < -70) { // Проверяем уровень сигнала
            Serial.printf("Weak signal for %s: %d dBm. Skipping...\n", ssid, signalLevel);
            continue;
        }

        Serial.printf("Connecting to Wi-Fi: %s\n", ssid);
        WiFi.begin(ssid, password);
        int retryCount = 0;

        // Ждем подключения
        while (WiFi.status() != WL_CONNECTED && retryCount < 60) { // Уменьшили количество попыток
            delay(1000);
            Serial.print(".");
            retryCount++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nConnected!");
            Serial.printf("IP Address: http://%s\n", WiFi.localIP().toString().c_str());
            return true;
        }

        Serial.printf("\nFailed to connect to %s. Trying next network...\n", ssid);
    }

    Serial.println("Failed to connect to any Wi-Fi network.");
    return false;
}

// Веб-интерфейс
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Virtual keyboard web interface. Powered by ESP32-S3</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            height: 100vh;
            margin: 0;
            background-color: #f9f9f9;
        }
        .container {
            width: 90%;
            max-width: 600px;
            margin: 0 auto;
            text-align: center;
            background: white;
            border-radius: 10px;
            padding: 20px;
            box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
        }
        textarea {
            width: 100%;
            height: 200px;
            font-size: 16px;
            padding: 10px;
            margin-bottom: 20px;
            border: 1px solid #ddd;
            border-radius: 5px;
        }
        button {
            width: 100%;
            padding: 10px;
            font-size: 16px;
            background-color: #4CAF50;
            color: white;
            border: none;
            border-radius: 5px;
            cursor: pointer;
        }
        textarea:focus {
            outline: 2px solid #4CAF50;
        }
        button:active {
            background-color: #3e8e41;
        }        
        button:hover {
            background-color: #45a049;
        }
        button:disabled {
            background-color: #aaa;
        }
        .status {
            margin-top: 20px;
            font-size: 16px;
        }
    </style>
</head>
<body onload="updateKeyboardStatus()">
    <div class="container">
        <h1>ESP32 Keyboard</h1>
        <textarea id="text-input" placeholder="Enter your text here..."></textarea>
        <button id="send-button">Send</button>
        <div class="status" id="keyboard-status">Keyboard Status: Unknown</div>
        <div class="status" id="response-status">Response Status: None</div>
    </div>

    <script>
        const button = document.getElementById('send-button');
        button.addEventListener('click', sendText);
        async function sendText() {
            button.disabled = true;
            const text = document.getElementById('text-input').value.trim();
            const responseStatus = document.getElementById('response-status');

            if (!text) {
                responseStatus.textContent = 'Response Status: Error (Empty text)';
                return;
            }
            try {
                updateKeyboardStatus();
                const response = await fetch('/send-text', {
                    method: 'POST',
                    headers: { 'Content-Type': 'text/plain' },
                    body: text
                });
                if (response.ok) {
                    const data = await response.text();
                    responseStatus.textContent = `Response Status: OK (${data})`;
                } else {
                    responseStatus.textContent = `Response Status: NOK (Code: ${response.status})`;
                }
            } catch (error) { 
                responseStatus.textContent = `Response Status: Error (${error.message})`;
                button.disabled = false;
            } finally {
                button.disabled = false;
            }
        }

        async function updateKeyboardStatus() {
            try {
                const response = await fetch('/keyboard-status');
                const status = await response.text();
                document.getElementById('keyboard-status').textContent = `Keyboard Status: ${status}`;
            } catch (error) {
                document.getElementById('keyboard-status').textContent = 'Keyboard Status: Unknown';
            }
        }

        setInterval(updateKeyboardStatus, 1000000); // Обновляем статус клавиатуры каждые 1000 сек
    </script>
</body>
</html>
)rawliteral";

void processUTF8Line(const String& line) {
    int startIndex = 0;
    

    // Проверяем наличие BOM в начале строки
    if (line.length() >= 3 &&
        static_cast<uint8_t>(line.charAt(0)) == 0xEF &&
        static_cast<uint8_t>(line.charAt(1)) == 0xBB &&
        static_cast<uint8_t>(line.charAt(2)) == 0xBF) {
        startIndex = 3; // Пропускаем BOM
    }

    for (int i = startIndex; i < line.length(); i++) {
        uint8_t byte1 = static_cast<uint8_t>(line.charAt(i));

		// Проверка начала двухбайтового символа UTF-8
        if (byte1 == 0xD0 || byte1 == 0xD1) { // Русский символ
            if (!isRussian) {
                switchLayout(true);
                isRussian = true;
            }
            if (i + 1 < line.length()) {
                uint8_t byte2 = static_cast<uint8_t>(line.charAt(i + 1));
                uint16_t utf8Char = (byte1 << 8) | byte2;
                if (i == startIndex) {
                  Serial.print("UTF-8 Char ");
                  Serial.println(utf8Char);
                }
                sendChar(utf8Char);
                i++; // Пропускаем второй байт
            }
        } else { // Латинский символ или ASCII
            if (byte1 != ' ') {
              if (isRussian) {
                  switchLayout(false);
                  isRussian = false;
              }
              if (i == startIndex) {
                Serial.print("ASCII Char ");
                Serial.println(byte1);
              }
            }
            sendChar(byte1);
        }
        delay(DELAY_BETWEEN_CHARS); // Задержка между символами
    }
}

void switchLayout(bool toRussian) {
    delay(500);
    bleKeyboard.press(KEY_LEFT_ALT);
    bleKeyboard.press(KEY_LEFT_SHIFT);
    delay(500); // Небольшая задержка для надежности
    bleKeyboard.releaseAll();
    delay(500);
}


void sendChar(uint16_t c) {
    // Таблицы соответствия UTF-8 символов
    static const char russianToEnglishUpper[] = {
        'F', '<', 'D', 'U', 'L', 'T',  ':', 'P', 'B', 'Q', 'R', 'K', 'V', 'Y',
        'J', 'G', 'H', 'C', 'N', 'E', 'A', '[', 'W', 'X', 'I', 'O', ']', 'S', 'M',
        '"', '>', 'Z','?','~' // Заглавные буквы А-Я
    };
    static const char russianToEnglishLower[] = {
      'f',',','d','u','l','t',';','p','b','q','r','k','v','y',
      'j','g','h','c','n','e','a','[','w','x','i','o',']','s','m',
      '\'','.','z','/','`' // строчные буквы а-я
    };

    // Заглавные буквы (А - Я)
    if (c >= 0xD090 && c <= 0xD0AF) {
        uint8_t index = c - 0xD090; // Вычисляем индекс
        bleKeyboard.write(russianToEnglishUpper[index]);
   // Заглавные буква (Ё)
    } else if (c == 0xD081) {
        uint8_t index = c - 0xD080 + 32; // Вычисляем индекс
        bleKeyboard.write(russianToEnglishUpper[index]);
    } 
    // Строчные буквы (а - п)
    else if (c >= 0xD0B0 && c <= 0xD0BF) {
        uint8_t index = c - 0xD0B0; // Вычисляем индекс
        bleKeyboard.write(russianToEnglishLower[index]);
    } 
    // Строчные буквы (продолжение) (р - я, ., ё)
    else if (c >= 0xD180 && c <= 0xD191) {
        uint8_t index = c - 0xD180 + 16; // Вычисляем индекс
        bleKeyboard.write(russianToEnglishLower[index]);
    } else {
        // ASCII символы
        bleKeyboard.write(static_cast<char>(c));
    }
}

// Обработчик для получения текста
void handleTextRequest() {
    if (server.hasArg("plain")) {
        if (bleKeyboard.isConnected()) {
            if (!textSent) {
                textSent = true;
                String receivedText = server.arg("plain");
                Serial.println("Received text for sending:");
                Serial.println(receivedText);

                // Экранируем входной текст
                //String sanitizedText = sanitizeInput(receivedText);

                // Разбиваем текст на строки
                int startIndex = 0;
                int endIndex = receivedText.indexOf('\n');
                
                while (startIndex != -1) {
                    // Получаем строку
                    String line = receivedText.substring(startIndex, (endIndex != -1) ? endIndex : receivedText.length());
                    // Отправляем строку символ за символом
                    
                    processUTF8Line(line);
                  
                    // Отправляем Enter после строки
                    Serial.println("Sent line: " + line);

                    // Задержка между строками
                    delay(DELAY_BETWEEN_LINES);

                    // Переход к следующей строке
                    if (endIndex == -1) break;
                      else bleKeyboard.write(KEY_RETURN);
                    startIndex = endIndex + 1;
                    endIndex = receivedText.indexOf('\n', startIndex);
                }
                // Возвращаем исходную раскладку (латиница)
                if (isRussian) {
                    switchLayout(false);
                }
                // Завершаем отправку текста
                //bleKeyboard.print("");
                server.send(200, "text/plain", "Text sent line by line");
                Serial.println("Response sent with code 200");
                resetTextSentFlag();
            } else {
                server.send(400, "text/plain", "Text already sent");
                Serial.println("Error: Text already sent.");
            }
        } else {
            server.send(400, "text/plain", "No device connected");
            Serial.println("Error: No device connected.");
        }
    } else {
        server.send(400, "text/plain", "No text received");
        Serial.println("Error: No text received.");
    }
}

// Обработчик для сброса флага
void resetTextSentFlag() {
    textSent = false;
}

// Обработчик для статуса клавиатуры
void handleKeyboardStatus() {
    if (bleKeyboard.isConnected()) {
        server.send(200, "text/plain", "Connected");
        Serial.println("Keyboard Status: Connected");
    } else {
        textSent = false;
        server.send(200, "text/plain", "Not Connected");
        Serial.println("Keyboard Status: Not Connected");
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println();

    // Подключение к Wi-Fi
    if (!connectToWiFi()) {
        Serial.println("Starting as an Access Point...");
        WiFi.softAP("ESP32-AP", "12345678");
        Serial.print("Access Point ESP32-AP IP Address: ");
        Serial.println(WiFi.softAPIP());
    }

    BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_MITM);
    bleKeyboard.begin();
    Serial.println("BLE Keyboard started.");

    // Настраиваем обработчики для запросов
    server.on("/", HTTP_GET, []() { server.send(200, "text/html", htmlPage); });
    server.on("/send-text", HTTP_POST, handleTextRequest);
    server.on("/keyboard-status", HTTP_GET, handleKeyboardStatus);
    server.onNotFound([]() { server.send(404, "text/plain", "Not Found"); });

    // Запускаем сервер
    server.begin();
    Serial.println("Web server started.");
}

void loop() {
    static unsigned long lastClientCheck = 0;
    const unsigned long clientCheckInterval = 100; // Интервал проверки клиентов (мс)
    
    // Обработка запросов веб-сервера
    if (millis() - lastClientCheck > clientCheckInterval) {
        lastClientCheck = millis();
        server.handleClient();
    }

    // Разовая отправка приветственного сообщения
    if (!welcomeMsg && WiFi.status() == WL_CONNECTED && bleKeyboard.isConnected()) {
        sendWelcomeMessage();
        welcomeMsg = true; // Устанавливаем флаг, чтобы сообщение не отправлялось повторно
    }
}

void sendWelcomeMessage() {
    String ipAddress = WiFi.localIP().toString();
    String message = "IP Address BLE ESP32 Keyboard: http://" + ipAddress;
    processUTF8Line(message);
    Serial.println("Welcome message sent via BLE.");
}
