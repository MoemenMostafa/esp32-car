#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <analogWrite.h>

#define MOTOR_A 1
#define MOTOR_B 2

#define STOP 0
#define START 1

// Motor A
int motorAIn1 = 27;
int motorAIn2 = 26;
// int enable1Pin = 14;

// Motor B
int motorBIn3 = 16;
int motorBIn4 = 17;
// int enable2Pin = 4;

// Setting PWM properties
const int freq = 30000;
const int pwmChannelIn1 = 1;
const int pwmChannelIn2 = 2;
const int pwmChannelIn3 = 3;
const int pwmChannelIn4 = 4;
const int resolution = 8;

const char* ssidAP     = "MyDevelopmentBoard Car";
const char* passwordAP = "12345678";

const char* ssid     = "MagentaWLAN-F9ON";
const char* password = "03291604044512253024";



AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
// Set your Static IP address
IPAddress local_IP(192, 168, 2, 184);
IPAddress gateway(192, 168, 2, 1);
IPAddress subnet(255, 255, 255, 0);

void setup(void)
{
  // sets the pins as outputs:
  pinMode(motorAIn1, OUTPUT);
  pinMode(motorAIn2, OUTPUT);

  // sets the pins as outputs:
  pinMode(motorBIn3, OUTPUT);
  pinMode(motorBIn4, OUTPUT);

  // configure LED PWM functionalitites
  ledcSetup(pwmChannelIn1, freq, resolution);
  ledcSetup(pwmChannelIn2, freq, resolution);
  ledcSetup(pwmChannelIn3, freq, resolution);
  ledcSetup(pwmChannelIn4, freq, resolution);

  // attach the channel to the GPIO to be controlled
  ledcAttachPin(motorAIn1, pwmChannelIn1);
  ledcAttachPin(motorAIn2, pwmChannelIn2);
  ledcAttachPin(motorBIn3, pwmChannelIn3);
  ledcAttachPin(motorBIn4, pwmChannelIn4);

  Serial.begin(115200);

  Serial.println("Program started");

  createAccessPoint();

  connectToWifi();  

  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

  server.begin();
  Serial.println("HTTP server started");

}

void loop()
{
  ws.cleanupClients();
}

void connectToWifi() {
  // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void createAccessPoint() {
  WiFi.softAP(ssidAP, passwordAP);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
}

void rotateMotorA(int direction, int dutyCycles) {
  if (direction == 1) {
    Serial.println("Moving Forward");
    ledcWrite(pwmChannelIn1, dutyCycles);
    ledcWrite(pwmChannelIn2, 0);
  } else if (direction == 2) {
    Serial.println("Moving Backward");
    ledcWrite(pwmChannelIn1, 0);
    ledcWrite(pwmChannelIn2, dutyCycles);
  } else {
    Serial.println("STOP");
    ledcWrite(pwmChannelIn1, 0);
    ledcWrite(pwmChannelIn2, 0);
  }
}

void rotateMotorB(int direction, int dutyCycles) {
  if (direction == 1) {
    Serial.println("Moving Forward");
    ledcWrite(pwmChannelIn3, dutyCycles);
    ledcWrite(pwmChannelIn4, 0);
  } else if (direction == 2) {
    Serial.println("Moving Backward");
    ledcWrite(pwmChannelIn3, 0);
    ledcWrite(pwmChannelIn4, dutyCycles);
  } else {
    Serial.println("STOP");
    ledcWrite(pwmChannelIn3, 0);
    ledcWrite(pwmChannelIn4, 0);
  }
}


void processCarMovement(String inputValue)
{
  int motorGroup = 0;
  int direction = 0;
  int dutyCycles = 0;

  Serial.printf("Got value as %s %d\n", inputValue.c_str(), inputValue.toInt());
  // Serial.printf("MotorGroup %d \n", inputValue.substring(0,1).toInt());
  // Serial.printf("Direction %d \n", inputValue.substring(1,2).toInt());
  // Serial.printf("DutyCycles %d \n", inputValue.substring(2,5).toInt());
  motorGroup = (int) inputValue.substring(0,1).toInt();
  direction = (int) inputValue.substring(1,2).toInt();
  dutyCycles = (int) inputValue.substring(2,5).toInt();


  switch (motorGroup)
  {
    case MOTOR_A:
      rotateMotorA(direction, dutyCycles);
      break;

    case MOTOR_B:
      rotateMotorB(direction, dutyCycles);
      break;
  }
}

void onWebSocketEvent(AsyncWebSocket *server,
                      AsyncWebSocketClient *client,
                      AwsEventType type,
                      void *arg,
                      uint8_t *data,
                      size_t len)
{
  switch (type)
  {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      ws.text(client->id(), String('{"status":"connected"}'));
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      processCarMovement("10000");
      processCarMovement("20000");
      break;
    case WS_EVT_DATA:
      AwsFrameInfo *info;
      info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
      {
        std::string myData = "";
        myData.assign((char *)data, len);
        processCarMovement(myData.c_str());
      }
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
    default:
      break;
  }
}