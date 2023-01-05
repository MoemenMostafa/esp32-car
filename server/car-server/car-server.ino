#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#include <analogWrite.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>


#define MOTOR_A 1
#define MOTOR_B 2

#define STOP 0
#define START 1

int ledBlink = false;
int ledOn = false;

// Motor A
int motorAIn1 = 14; // D5
int motorAIn2 = 12; // D6

// Motor B
int motorBIn3 = 13; // D7
int motorBIn4 = 15; // D8

// LED pin
int ledPin = 16; // D0

// Setting PWM properties
const int freq = 30000;


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
  pinMode(motorBIn3, OUTPUT);
  pinMode(motorBIn4, OUTPUT);
  pinMode(ledPin, OUTPUT);

  // Set analogWrite frequency
  analogWriteFreq(freq);


  Serial.begin(115200);

  Serial.println("Program started");

  createAccessPoint();

  // connectToWifi();

  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

  server.begin();
  Serial.println("HTTP server started");


  // trun led on
  analogWrite(ledPin, 255);
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
  if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
    Serial.println("SoftAP Failed to configure");
  };
  if(!WiFi.softAP(ssidAP, passwordAP)){
    Serial.println("SoftAP Failed to start");    
  } else {
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
  }
}

void rotateMotorA(int direction, int dutyCycles) {
  if (direction == 1) {
    Serial.println("Moving Forward");
    analogWrite(motorAIn1, dutyCycles);
    analogWrite(motorAIn2, 0);
  } else if (direction == 2) {
    Serial.println("Moving Backward");
    analogWrite(motorAIn1, 0);
    analogWrite(motorAIn2, dutyCycles);
  } else {
    Serial.println("STOP");
    analogWrite(motorAIn1, 0);
    analogWrite(motorAIn2, 0);
  }
}

void rotateMotorB(int direction, int dutyCycles) {
  if (direction == 1) {
    Serial.println("Moving Forward");
    analogWrite(motorBIn3, dutyCycles);
    analogWrite(motorBIn4, 0);
  } else if (direction == 2) {
    Serial.println("Moving Backward");
    analogWrite(motorBIn3, 0);
    analogWrite(motorBIn4, dutyCycles);
  } else {
    Serial.println("STOP");
    analogWrite(motorBIn3, 0);
    analogWrite(motorBIn4, 0);    
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