#include <TB6612_ESP32.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino_JSON.h>
#include <Adafruit_NeoPixel.h>
#include <ESP32Servo.h>

#define PIN_NEO_PIXEL  16 
#define NUM_PIXELS     16 

Adafruit_NeoPixel NeoPixel(NUM_PIXELS, PIN_NEO_PIXEL, NEO_GRB + NEO_KHZ800);

// Контакты для драйвера моторов
#define AIN1 13
#define AIN2 14
#define BIN1 12
#define BIN2 27
#define PWMA 26
#define PWMB 25
#define STBY 33

// Контакты для управления
#define RELAY_PIN 15    // Реле на G15
#define SERVO_PIN 5     // Сервопривод на G5

const char* ssid = "Ygcvhu";
const char* password = "Li3a2006";

int speed = 100;
int direction = 0;
int slider = 0;
int relayState = 0;      // Состояние реле (0=выкл, 1=вкл)
int servoState = 0;      // Состояние сервы (0=выкл, 1=вкл)
int ledState = 0;
int isMoving = 0;
int servoPos = 90;       // Начальная позиция сервы (90°)

// Создаем объект сервопривода
Servo myServo;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

JSONVar readings;

const int offsetA = 1;
const int offsetB = 1;

Motor motor1 = Motor(AIN1, AIN2, PWMA, offsetA, STBY, 5000, 8, 1);
Motor motor2 = Motor(BIN1, BIN2, PWMB, offsetB, STBY, 5000, 8, 2);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Управление Роботом</title>
</head>
<style>
    body {height: 100vh;overflow: hidden;margin: 0;font-family: Arial, sans-serif;}
    .parent {
        display: grid;
        grid-template-columns: repeat(2, 1fr);
        grid-template-rows: repeat(2, 1fr);
        grid-column-gap: 0px;
        grid-row-gap: 0px;
        height: 100%;
        }
        
        .div1 { grid-area: 1 / 2 / 3 / 3; height: 100vh;}
        .div2 { grid-area: 1 / 1 / 2 / 2; padding: 0.5rem;}
        .div3 { grid-area: 2 / 1 / 3 / 2; }
        .parent-j {
        display: grid;
        grid-template-columns: repeat(3, 1fr);
        grid-template-rows: repeat(3, 1fr);
        grid-column-gap: 0px;
        grid-row-gap: 0px;
        height: 100%;
        }

        .div-u { grid-area: 1 / 2 / 2 / 3; border: 1px solid gray;padding:2rem 3rem; border-radius:100% 100% 0 0;}
        .div-l { grid-area: 2 / 1 / 3 / 2; border: 1px solid gray;padding:2.95rem 2.4rem; border-radius:100% 0 0 100%;}
        .div-d { grid-area: 3 / 2 / 4 / 3; border: 1px solid gray;padding:2rem 3rem; border-radius:0 0 100% 100%;}
        .div-r { grid-area: 2 / 3 / 3 / 4; border: 1px solid gray;padding:2.95rem 2.4rem; border-radius:0 100% 100% 0;}
        .div-s { grid-area: 2 / 2 / 3 / 3; border: 1px solid rgb(233, 80, 80);padding:3rem 2.3rem; }

        .analog {border:1px solid rgb(216, 211, 211);display: flex;border-radius:15px;justify-content: space-between;align-items: stretch;}
        .analog-value{font-size:5rem;}
        .btn-set{width: 35%;border-radius:0 10px 10px 0;border:none;background:#524FF0;color:white;font-size:1.3rem}
        .analog-data{display: flex;flex-wrap: wrap;align-content: stretch;justify-content: flex-start;padding: 0.5rem;}
        .div-slider{width:100%;}
        .slider{width: 90%;}
        .circ-btn{border-radius:100%;border:1px solid red;width:180px;height:180px;background:none;font-size:1.2rem;margin:0.5rem;}
        .servo-controls {
            position: absolute;
            bottom: 10px;
            right: 10px;
            background: rgba(0,0,0,0.7);
            color: white;
            padding: 15px;
            border-radius: 10px;
            display: none;
        }
        .servo-slider {
            width: 200px;
            margin: 10px 0;
        }
        .servo-display {
            font-size: 1.4rem;
            margin: 10px 0;
            text-align: center;
        }
        .servo-btn {
            background: #524FF0;
            color: white;
            border: none;
            padding: 8px 15px;
            border-radius: 5px;
            margin: 5px;
            cursor: pointer;
        }
        .speed-indicator {
            position: absolute;
            top: 10px;
            right: 10px;
            background: rgba(0,0,0,0.7);
            color: white;
            padding: 10px;
            border-radius: 5px;
            font-size: 1.2rem;
        }
        .mode-indicator {
            position: absolute;
            top: 60px;
            right: 10px;
            background: rgba(0,0,0,0.7);
            color: white;
            padding: 10px;
            border-radius: 5px;
            font-size: 1.2rem;
        }
        .servo-toggle-btn {
            position: absolute;
            top: 110px;
            right: 10px;
            background: rgba(0,0,0,0.7);
            color: white;
            padding: 10px;
            border-radius: 5px;
            font-size: 1.2rem;
            border: none;
            cursor: pointer;
        }
        @media screen and (orientation: portrait) {
            .parent {
                display: grid;
                grid-template-columns: 1fr;
                grid-template-rows: repeat(3, 1fr);
                grid-column-gap: 0px;
                grid-row-gap: 0px;
                }

                .div1 { grid-area: 3 / 1 / 4 / 2; height: auto;}
                .div2 { grid-area: 2 / 1 / 3 / 2; }
                .div3 { grid-area: 1 / 1 / 2 / 2; }
                .servo-controls {
                    position: relative;
                    bottom: auto;
                    right: auto;
                    margin-top: 10px;
                }
                .servo-toggle-btn {
                    position: relative;
                    top: auto;
                    right: auto;
                    margin: 10px;
                }
        }
</style>
<body>
<div class="speed-indicator">Скорость: <span id="current-speed">100</span>%</div>
<div class="mode-indicator">
    Реле: <span id="relay-status">ВЫКЛ</span> | 
    Серва: <span id="servo-status">ВЫКЛ</span>
</div>
<button class="servo-toggle-btn" id="servo-toggle-btn">Показать управление сервой</button>
<div class="parent">
    <div class="div1">
        <div class="parent-j">
            <button class="div-u" id="btn-up">ВПЕРЕД</button>
            <button class="div-l" id="btn-lt">ЛЕВО</button>
            <button class="div-d" id="btn-dn">НАЗАД</button>
            <button class="div-r" id="btn-rt">ПРАВО</button>
            <button class="div-s" id="btn-sp">СТОП</button>
        </div>
    </div>
    <div class="div2">
        <div class="analog">
            <div class="analog-data"><div class="analog-value" id="slider-txt">100</div>
                <div class="div-slider"><input id="slider-val" class="slider" type="range" value="100" min="0" max="255"/></div>
            </div> 
            <button class="btn-set" id="btn-set">СКОРОСТЬ</button>
        </div>
    </div>
    <div class="div3">
        <button class="circ-btn" id="btn-relay">РЕЛЕ<br><span id="relay-text">ВЫКЛ</span></button>
        <button class="circ-btn" id="btn-led">СВЕТ<br><span id="led-text">ВЫКЛ</span></button>
        <button class="circ-btn" id="btn-servo">СЕРВА<br><span id="servo-text">ВЫКЛ</span></button>
    </div>
</div>

<!-- Панель управления сервоприводом -->
<div class="servo-controls" id="servo-controls">
    <h3>Управление сервоприводом (GPIO 5)</h3>
    <div class="servo-display">Угол: <span id="servo-angle">90</span>°</div>
    <input type="range" min="0" max="180" value="90" class="servo-slider" id="servo-slider">
    <div>
        <button class="servo-btn" id="servo-0">0°</button>
        <button class="servo-btn" id="servo-90">90°</button>
        <button class="servo-btn" id="servo-180">180°</button>
    </div>
</div>

<script>
let gateway = `ws://${window.location.hostname}/ws`;

let sliderTxt = document.querySelector("#slider-txt");
let sliderVal = document.querySelector("#slider-val");
let relayBtn = document.querySelector("#btn-relay");
let ledBtn = document.querySelector("#btn-led");
let servoBtn = document.querySelector("#btn-servo");
let currentSpeedSpan = document.querySelector("#current-speed");
let relayStatusSpan = document.querySelector("#relay-status");
let servoStatusSpan = document.querySelector("#servo-status");
let relayTextSpan = document.querySelector("#relay-text");
let ledTextSpan = document.querySelector("#led-text");
let servoTextSpan = document.querySelector("#servo-text");
let servoControls = document.querySelector("#servo-controls");
let servoSlider = document.querySelector("#servo-slider");
let servoAngleSpan = document.querySelector("#servo-angle");
let servoToggleBtn = document.querySelector("#servo-toggle-btn");

let motorSpeed = 100;
let currentDirection = 0;
let servoPosition = 90;

let websocket;
window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
    initButtons();
    updateDisplay();
}

function initButtons() {
  // Управление мотором
  document.querySelector('#btn-up').addEventListener('touchstart', ()=>{ 
    currentDirection = 11;
    websocket.send(JSON.stringify({dir:11, speed:motorSpeed}));
    updateSpeedDisplay();
  });
  document.querySelector('#btn-up').addEventListener('touchend', ()=>{ 
    currentDirection = 0;
    websocket.send(JSON.stringify({dir:12}));
  });

  document.querySelector('#btn-dn').addEventListener('touchstart', ()=>{ 
    currentDirection = 21;
    websocket.send(JSON.stringify({dir:21, speed:motorSpeed}));
    updateSpeedDisplay();
  });
  document.querySelector('#btn-dn').addEventListener('touchend', ()=>{ 
    currentDirection = 0;
    websocket.send(JSON.stringify({dir:22}));
  });

  document.querySelector('#btn-lt').addEventListener('touchstart', ()=>{ 
    currentDirection = 31;
    websocket.send(JSON.stringify({dir:31, speed:motorSpeed}));
    updateSpeedDisplay();
  });
  document.querySelector('#btn-lt').addEventListener('touchend', ()=>{ 
    currentDirection = 0;
    websocket.send(JSON.stringify({dir:32}));
  });

  document.querySelector('#btn-rt').addEventListener('touchstart', ()=>{ 
    currentDirection = 41;
    websocket.send(JSON.stringify({dir:41, speed:motorSpeed}));
    updateSpeedDisplay();
  });
  document.querySelector('#btn-rt').addEventListener('touchend', ()=>{ 
    currentDirection = 0;
    websocket.send(JSON.stringify({dir:42}));
  });

  document.querySelector('#btn-sp').addEventListener('click', ()=>{ 
    websocket.send(JSON.stringify({dir:0}));
    currentDirection = 0;
  });

  // Установка скорости
  document.querySelector('#btn-set').addEventListener('click', ()=>{ 
    motorSpeed = parseInt(sliderVal.value);
    updateSpeedDisplay();
    
    if (currentDirection > 0) {
      websocket.send(JSON.stringify({dir:currentDirection, speed:motorSpeed}));
    }
    
    websocket.send(JSON.stringify({slider: motorSpeed}));
  });
  
  // Управление LED
  document.querySelector('#btn-led').addEventListener('click', ()=>{ 
    websocket.send(JSON.stringify({led:1})); 
  });
  
  // Управление РЕЛЕ
  document.querySelector('#btn-relay').addEventListener('click', ()=>{ 
    websocket.send(JSON.stringify({relay:1}));
  });
  
  // Управление СЕРВОЙ
  document.querySelector('#btn-servo').addEventListener('click', ()=>{ 
    websocket.send(JSON.stringify({servo_enable:1}));
  });

  // Показать/скрыть панель управления сервой
  servoToggleBtn.addEventListener('click', ()=>{ 
    if (servoControls.style.display === 'none' || servoControls.style.display === '') {
      servoControls.style.display = 'block';
      servoToggleBtn.textContent = 'Скрыть управление сервой';
    } else {
      servoControls.style.display = 'none';
      servoToggleBtn.textContent = 'Показать управление сервой';
    }
  });

  // Изменение ползунка скорости
  document.querySelector('#slider-val').addEventListener('input', ()=>{
    const value = sliderVal.value;
    document.querySelector("#slider-txt").innerHTML = value;
    currentSpeedSpan.innerHTML = Math.round(value * 100 / 255);
  });

  // Управление сервоприводом
  servoSlider.addEventListener('input', ()=>{
    servoPosition = parseInt(servoSlider.value);
    servoAngleSpan.textContent = servoPosition;
    websocket.send(JSON.stringify({servo: servoPosition}));
  });

  // Кнопки предустановленных позиций сервы
  document.querySelector('#servo-0').addEventListener('click', ()=>{
    servoPosition = 0;
    updateServoSlider();
    websocket.send(JSON.stringify({servo: 0}));
  });

  document.querySelector('#servo-90').addEventListener('click', ()=>{
    servoPosition = 90;
    updateServoSlider();
    websocket.send(JSON.stringify({servo: 90}));
  });

  document.querySelector('#servo-180').addEventListener('click', ()=>{
    servoPosition = 180;
    updateServoSlider();
    websocket.send(JSON.stringify({servo: 180}));
  });
}

function updateSpeedDisplay() {
  currentSpeedSpan.innerHTML = Math.round(motorSpeed * 100 / 255);
  document.querySelector("#slider-txt").innerHTML = motorSpeed;
  sliderVal.value = motorSpeed;
}

function updateDisplay() {
  // Эта функция будет обновляться через данные с сервера
}

function updateServoSlider() {
  servoSlider.value = servoPosition;
  servoAngleSpan.textContent = servoPosition;
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
    getReadings();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function getReadings(){
    websocket.send("getReadings");
}

function onMessage(event) {
    const data = JSON.parse(event.data);
    
    // Обновляем скорость
    if (data.s !== undefined) {
        const newSpeed = parseInt(data.s);
        if (newSpeed !== motorSpeed) {
            motorSpeed = newSpeed;
            updateSpeedDisplay();
        }
    }
    
    // Обновляем статус реле
    if (data.relay !== undefined) {
        const relayState = parseInt(data.relay);
        if (relayState === 1) {
            relayStatusSpan.textContent = 'ВКЛ';
            relayTextSpan.textContent = 'ВКЛ';
            relayBtn.style.background = '#ff0000';
            relayBtn.style.color = '#ffffff';
        } else {
            relayStatusSpan.textContent = 'ВЫКЛ';
            relayTextSpan.textContent = 'ВЫКЛ';
            relayBtn.style.background = '#ffffff';
            relayBtn.style.color = '#000000';
        }
    }
    
    // Обновляем статус LED
    if (data.led !== undefined) {
        const ledState = parseInt(data.led);
        if (ledState === 1) {
            ledTextSpan.textContent = 'ВКЛ';
            ledBtn.style.background = '#ff0000';
            ledBtn.style.color = '#ffffff';
        } else {
            ledTextSpan.textContent = 'ВЫКЛ';
            ledBtn.style.background = '#ffffff';
            ledBtn.style.color = '#000000';
        }
    }
    
    // Обновляем статус сервы
    if (data.servo_state !== undefined) {
        const servoState = parseInt(data.servo_state);
        if (servoState === 1) {
            servoStatusSpan.textContent = 'ВКЛ';
            servoTextSpan.textContent = 'ВКЛ';
            servoBtn.style.background = '#00ff00';
            servoBtn.style.color = '#000000';
        } else {
            servoStatusSpan.textContent = 'ВЫКЛ';
            servoTextSpan.textContent = 'ВЫКЛ';
            servoBtn.style.background = '#ffffff';
            servoBtn.style.color = '#000000';
        }
    }
    
    // Обновляем позицию сервы
    if (data.servo_pos !== undefined) {
        servoPosition = parseInt(data.servo_pos);
        updateServoSlider();
    }
    
    websocket.send("getReadings");
}
</script>
</body>
</html>
)rawliteral";

String getSensorReadings(){
  readings["s"] = String(speed);
  readings["relay"] = String(relayState);
  readings["led"] = String(ledState);
  readings["servo_state"] = String(servoState);
  readings["servo_pos"] = String(servoPos);
  String jsonString = JSON.stringify(readings);
  return jsonString;
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void notifyClients(String sensorReadings) {
  ws.textAll(sensorReadings);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    
    JSONVar myObject = JSON.parse((const char*)data);
    
    // Обработка скорости
    if (myObject.hasOwnProperty("slider")) {
      speed = (int)myObject["slider"];
      Serial.println("Speed set to: " + String(speed));
      
      if (isMoving) {
        move(direction, speed);
      }
    }
    // Обработка направления с указанием скорости
    else if (myObject.hasOwnProperty("dir") && myObject.hasOwnProperty("speed")) {
      direction = (int)myObject["dir"];
      speed = (int)myObject["speed"];
      isMoving = (direction == 11 || direction == 21 || direction == 31 || direction == 41);
      move(direction, speed);
    }
    // Обработка направления без указания скорости
    else if (myObject.hasOwnProperty("dir")) {
      direction = (int)myObject["dir"];
      isMoving = (direction == 11 || direction == 21 || direction == 31 || direction == 41);
      move(direction, speed);
    }
    // Обработка РЕЛЕ (переключатель)
    else if (myObject.hasOwnProperty("relay")) {
      relayState = (relayState == 0) ? 1 : 0; // Переключаем состояние
      
      if (relayState == 1) {
        digitalWrite(RELAY_PIN, HIGH);
        Serial.println("RELAY: ON");
      } else {
        digitalWrite(RELAY_PIN, LOW);
        Serial.println("RELAY: OFF");
      }
    }
    // Обработка LED (переключатель)
    else if (myObject.hasOwnProperty("led")) {
      ledState = (ledState == 0) ? 1 : 0; // Переключаем состояние
      Serial.println("LED: " + String(ledState == 1 ? "ON" : "OFF"));
    }
    // Включение/выключение сервопривода
    else if (myObject.hasOwnProperty("servo_enable")) {
      servoState = (servoState == 0) ? 1 : 0; // Переключаем состояние
      
      if (servoState == 1) {
        if (!myServo.attached()) {
          myServo.attach(SERVO_PIN);
          myServo.write(servoPos);
          Serial.println("SERVO: ON (Pin 5), Position: " + String(servoPos) + "°");
        }
      } else {
        if (myServo.attached()) {
          myServo.detach();
          Serial.println("SERVO: OFF");
        }
      }
    }
    // Обработка позиции сервопривода
    else if (myObject.hasOwnProperty("servo")) {
      if (servoState == 1) { // Только если серва включена
        servoPos = (int)myObject["servo"];
        servoPos = constrain(servoPos, 0, 180);
        
        if (!myServo.attached()) {
          myServo.attach(SERVO_PIN);
        }
        
        myServo.write(servoPos);
        Serial.println("Servo position: " + String(servoPos) + "°");
      }
    }

    String sensorReadings = getSensorReadings();
    notifyClients(sensorReadings);
  }
}

void move(int direction, int speed) {
  speed = constrain(speed, 0, 255);
  
  if (direction == 11) { // Forward
    forward(motor1, motor2, speed);
    Serial.println("Forward at speed: " + String(speed));
  }
  else if (direction == 12) { // Stop forward
    motor1.brake();
    motor2.brake();
    isMoving = 0;
    Serial.println("Stop forward");
  }
  else if (direction == 21) { // Backward
    back(motor1, motor2, speed);
    Serial.println("Backward at speed: " + String(speed));
  }
  else if (direction == 22) { // Stop backward
    motor1.brake();
    motor2.brake();
    isMoving = 0;
    Serial.println("Stop backward");
  }
  else if (direction == 31) { // Left
    motor1.drive(-speed);
    motor2.drive(speed);
    Serial.println("Left at speed: " + String(speed));
  }
  else if (direction == 32) { // Stop left
    motor1.brake();
    motor2.brake();
    isMoving = 0;
    Serial.println("Stop left");
  }
  else if (direction == 41) { // Right
    motor1.drive(speed);
    motor2.drive(-speed);
    Serial.println("Right at speed: " + String(speed));
  }
  else if (direction == 42) { // Stop right
    motor1.brake();
    motor2.brake();
    isMoving = 0;
    Serial.println("Stop right");
  }
  else if (direction == 0) { // Full stop
    motor1.brake();
    motor2.brake();
    isMoving = 0;
    Serial.println("Full stop");
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void setup()
{
  NeoPixel.begin(); 
  Serial.begin(115200);
  
  // Настройка пинов
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  
  // Разрешаем использование всех таймеров для сервопривода
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  
  // Инициализация сервопривода (не подключаем сразу)
  myServo.setPeriodHertz(50); // Стандартная частота для сервоприводов
  
  initWiFi();
  initWebSocket();
  
  // ОБНОВЛЕННЫЙ ОБРАБОТЧИК С ПРАВИЛЬНОЙ КОДИРОВКОЙ
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html; charset=UTF-8", index_html);
  });
  
  // Добавляем обработчик для фавикона (чтобы избежать ошибок 404)
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "image/x-icon", "");
  });
  
  server.begin();
  
  // Инициализация
  speed = 100;
  Serial.println("=== SYSTEM INITIALIZED ===");
  Serial.println("Speed: " + String(speed));
  Serial.println("Servo on pin: " + String(SERVO_PIN));
  Serial.println("Relay on pin: " + String(RELAY_PIN));
  Serial.println("IP address: " + WiFi.localIP().toString());
  Serial.println("Ready! Open browser and go to: http://" + WiFi.localIP().toString());
}

void loop()
{
  // Управление светодиодной лентой
  if (ledState == 1) {
    for (int pixel = 0; pixel < NUM_PIXELS; pixel++) {          
      NeoPixel.setPixelColor(pixel, NeoPixel.Color(255, 255, 255));
    }
    NeoPixel.show();
  }
  else {
    NeoPixel.clear();
    NeoPixel.show();
  }

  // Очистка неактивных клиентов WebSocket
  ws.cleanupClients();
  
  // Отправляем обновления состояния каждые 100мс
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 100) {
    String sensorReadings = getSensorReadings();
    notifyClients(sensorReadings);
    lastUpdate = millis();
  }
  
  delay(10);
}