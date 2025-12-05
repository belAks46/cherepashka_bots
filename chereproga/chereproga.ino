#include <TB6612_ESP32.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino_JSON.h>
#include <Adafruit_NeoPixel.h>
#include <ESP32Servo.h>  // Добавляем библиотеку для сервопривода

#define PIN_NEO_PIXEL  16 
#define NUM_PIXELS     16 
#define SERVO_PIN 5      // Пин для сервопривода

Adafruit_NeoPixel NeoPixel(NUM_PIXELS, PIN_NEO_PIXEL, NEO_GRB + NEO_KHZ800);
Servo servo;  // Создаем объект сервопривода

#define AIN1 13 // ESP32 Pin D13 to TB6612FNG Pin AIN1
#define BIN1 12 // ESP32 Pin D12 to TB6612FNG Pin BIN1
#define AIN2 14 // ESP32 Pin D14 to TB6612FNG Pin AIN2
#define BIN2 27 // ESP32 Pin D27 to TB6612FNG Pin BIN2
#define PWMA 26 // ESP32 Pin D26 to TB6612FNG Pin PWMA
#define PWMB 25 // ESP32 Pin D25 to TB6612FNG Pin PWMB
#define STBY 33 // ESP32 Pin D33 to TB6612FNG Pin STBY

const char* ssid = "Ygcvhu";
const char* password = "Li3a2006";

#define RELAY_PIN 15 // pin G15

int speed = 100;
int direction = 0;
int slider = 0;
int fire = 0;
int led = 0;
int servoAngle = 90;  // Начальный угол сервопривода (0-180)

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

JSONVar readings;

// these constants are used to allow you to make your motor configuration
// line up with function names like forward.  Value can be 1 or -1
const int offsetA = 1;
const int offsetB = 1;

Motor motor1 = Motor(AIN1, AIN2, PWMA, offsetA, STBY, 5000, 8, 1);
Motor motor2 = Motor(BIN1, BIN2, PWMB, offsetB, STBY, 5000, 8, 2);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<meta name="viewport" content="width=device-width, initial-scale=1.0">

<style>
    body {height: 100vh;overflow: hidden;margin: 0;}
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
        .circ-btn{border-radius:100%;border:1px solid red;width:120px;height:120px;background:none;margin:0.5rem;}
        .servo-container{margin-top:1rem;}
        .servo-slider{width:80%;}
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
        }
</style>
<div class="parent">
    <div class="div1">
        <div class="parent-j">
            <button class="div-u" id="btn-up">F</button>
            <button class="div-l" id="btn-lt">L</button>
            <button class="div-d" id="btn-dn">B</button>
            <button class="div-r" id="btn-rt">R</button>
            <button class="div-s" id="btn-sp">STOP</button>
        </div>
    </div>
    <div class="div2">
        <div class="analog">
            <div class="analog-data"><div class="analog-value" id="slider-txt">0</div>
                <div class="div-slider"><input id="slider-val" class="slider" type="range" value="0" min="0" max="255"/></div>
            </div> 
            <button class="btn-set" id="btn-set">SET</button>
        </div>
        <div class="servo-container">
            <div>Servo Control: <span id="servo-angle">90</span>°</div>
            <input id="servo-slider" class="servo-slider" type="range" value="90" min="0" max="180"/>
            <button id="btn-servo-left">← Left</button>
            <button id="btn-servo-center">Center</button>
            <button id="btn-servo-right">Right →</button>
        </div>
    </div>
    <div class="div3">
        <button class="circ-btn" id="btn-fire">RELAY</button>
        <button class="circ-btn" id="btn-led">LED</button>
    </div>
</div>

<script>
let gateway = `ws://${window.location.hostname}/ws`;

let sliderTxt = document.querySelector("#slider-txt");
let sliderVal = document.querySelector("#slider-val");
let fireBtn = document.querySelector("#btn-fire");
let ledBtn = document.querySelector("#btn-led");
let servoSlider = document.querySelector("#servo-slider");
let servoAngleDisplay = document.querySelector("#servo-angle");
let ledState = 0;
let fireState = 0;

let websocket;
window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
    initButtons();
}

function initButtons() {
  // Кнопки управления движением
  document.querySelector('#btn-up').addEventListener('touchstart', ()=>{ websocket.send(JSON.stringify({dir:11})) });
  document.querySelector('#btn-up').addEventListener('touchend', ()=>{ websocket.send(JSON.stringify({dir:12})) });

  document.querySelector('#btn-dn').addEventListener('touchstart', ()=>{ websocket.send(JSON.stringify({dir:21})) });
  document.querySelector('#btn-dn').addEventListener('touchend', ()=>{ websocket.send(JSON.stringify({dir:22})) });

  document.querySelector('#btn-lt').addEventListener('touchstart', ()=>{ websocket.send(JSON.stringify({dir:31})) });
  document.querySelector('#btn-lt').addEventListener('touchend', ()=>{ websocket.send(JSON.stringify({dir:32})) });

  document.querySelector('#btn-rt').addEventListener('touchstart', ()=>{ websocket.send(JSON.stringify({dir:41})) });
  document.querySelector('#btn-rt').addEventListener('touchend', ()=>{ websocket.send(JSON.stringify({dir:42})) });

  // Кнопка установки скорости
  document.querySelector('#btn-set').addEventListener('click', ()=>{ 
    websocket.send(JSON.stringify({slider: parseInt(sliderVal.value)})) 
  });
  
  // Кнопка LED
  document.querySelector('#btn-led').addEventListener('click', ()=>{ 
    ledState = !ledState;
    toggleBg(ledBtn, ledState);
    websocket.send(JSON.stringify({led:ledState ? 1 : 0})); 
  });
  
  // Кнопка RELAY
  document.querySelector('#btn-fire').addEventListener('click', ()=>{ 
    fireState = !fireState;
    toggleBg(fireBtn, fireState);
    websocket.send(JSON.stringify({fire:fireState ? 1 : 0})); 
  });

  // Слайдер скорости
  document.querySelector('#slider-val').addEventListener('change', ()=>{
    document.querySelector("#slider-txt").innerHTML = sliderVal.value;
  });

  // Управление сервоприводом
  servoSlider.addEventListener('change', ()=>{
    let angle = servoSlider.value;
    servoAngleDisplay.innerHTML = angle;
    websocket.send(JSON.stringify({servo: parseInt(angle)}));
  });

  // Кнопки быстрого управления сервоприводом
  document.querySelector('#btn-servo-left').addEventListener('click', ()=>{
    websocket.send(JSON.stringify({servo: 0}));
    servoSlider.value = 0;
    servoAngleDisplay.innerHTML = "0";
  });

  document.querySelector('#btn-servo-center').addEventListener('click', ()=>{
    websocket.send(JSON.stringify({servo: 90}));
    servoSlider.value = 90;
    servoAngleDisplay.innerHTML = "90";
  });

  document.querySelector('#btn-servo-right').addEventListener('click', ()=>{
    websocket.send(JSON.stringify({servo: 180}));
    servoSlider.value = 180;
    servoAngleDisplay.innerHTML = "180";
  });
}

function toggleBg(btn, state) {
    if (state) {
        btn.style.background = '#ff0000';
    }
    else {
        btn.style.background = '#ffffff';
    }
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
    websocket.send("getReadings");
}
</script>
)rawliteral";


String getSensorReadings(){
  readings["s"] = String(slider);
  readings["servo"] = String(servoAngle);
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
    if (myObject.hasOwnProperty("slider")) {
      slider = (int)myObject["slider"];
    }
    else if (myObject.hasOwnProperty("fire")) {
      fire = (int)myObject["fire"];      
    }
    else if (myObject.hasOwnProperty("led")) {
      led = (int)myObject["led"];      
    }
    else if (myObject.hasOwnProperty("dir")) {
      direction = (int)myObject["dir"];
      move(direction, slider);  // Используем slider вместо speed
    }
    else if (myObject.hasOwnProperty("servo")) {
      servoAngle = (int)myObject["servo"];
      servo.write(servoAngle);  // Устанавливаем угол сервопривода
    }

    String sensorReadings = getSensorReadings();
    notifyClients(sensorReadings);
  }
}

void move(int direction, int speed) {
  int currentSpeed = slider;  // Используем значение из слайдера

  if (direction == 11) { // Forward
    forward(motor1, motor2, currentSpeed);
  }
  else if (direction == 12) { // Stop forward
    motor1.brake();
    motor2.brake();
  }
  else if (direction == 21) { // Backward
    back(motor1, motor2, currentSpeed);
  }
  else if (direction == 22) { // Stop backward
    motor1.brake();
    motor2.brake();
  }
  else if (direction == 31) { // Left
    motor1.drive(-currentSpeed);
    motor2.drive(currentSpeed);
  }
  else if (direction == 32) { // Stop left
    motor1.brake();
    motor2.brake();
  }
  else if (direction == 41) { // Right
    motor1.drive(currentSpeed);
    motor2.drive(-currentSpeed);
  }
  else if (direction == 42) { // Stop right
    motor1.brake();
    motor2.brake();
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
  pinMode(RELAY_PIN, OUTPUT);  
  
  // Инициализация сервопривода
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  servo.setPeriodHertz(50);      // Стандартная частота для сервоприводов
  servo.attach(SERVO_PIN, 500, 2400); // Диапазон импульсов для большинства сервоприводов
  servo.write(servoAngle);       // Устанавливаем начальное положение

  initWiFi();
  initWebSocket();
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });
  server.begin();
}

void loop()
{
  // Управление LED лентой
  if (led == 1) {
    for (int pixel = 0; pixel < NUM_PIXELS; pixel++) {          
      NeoPixel.setPixelColor(pixel, NeoPixel.Color(255, 255, 255));
    }
    NeoPixel.show();
  }
  else {
    NeoPixel.clear();
    NeoPixel.show();
  }

  // Управление реле
  if (fire == 1) {
    digitalWrite(RELAY_PIN, HIGH);
  }
  else {
    digitalWrite(RELAY_PIN, LOW);
  }
  
  ws.cleanupClients();
}