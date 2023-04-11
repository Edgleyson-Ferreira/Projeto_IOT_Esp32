#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <HCSR04.h>
HCSR04 hc(15, 2); //initialisation class HCSR04 (trig pin , echo pin)
#define TRIGGER_PIN   15
#define ECHO_PIN   2 //Pino Trigger Pino do Echo 
#include "FS.h"
// NewPing setup of pins and maximum distance. 

// Constantes
const char *ssid ="Marcelo Casa "  ; // COLOCAR UM ESPAÇO APÓS INSERIR O SSID 
const char *password = "1710rosa";
const char *msg_get_led = "getLEDState";
const int dns_port = 53;
const int http_port = 80;
const int ws_port =1337;
float duration; // tempo que o pulso leva para retornar
float distance; // distância percorrida pelo pulso
float obj_distance = 20; // distância limite para detectar um objeto
int timeout = 5000; // intervalo
bool isOccupied = false; // status da vaga
unsigned long occupiedStartTime = 0; // inicia o tempo de ocupação 
unsigned int pingSpeed = 50; // How frequently are we going to send out a ping (in milliseconds). 50ms would be 20 times a second.
unsigned long pingTimer;     // Holds the next ping time
// Globals
AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(1337);
char msg_buf[10];


/***********************************************************
 * Funções
 */
// Callback: send homepage
void onIndexRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/index.html", "text/html");
}
void facepe (AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/facepe.png", "image/png");
  
}
void iot (AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/iot.png", "image/png");
  
}

void upe (AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/upe.png", "image/png");
}
// Callback: send style sheet
void onCSSRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/style.css", "text/css");
}

// Callback: send distance
void onDistanceRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
String results = get_distance_in_cm();
request->send(200, "text/plain",results.c_str());
}


// Callback: send status
void onStatusRequest(AsyncWebServerRequest *request) {
  //Deve executar a função de get_status_string e retornar o status
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  String results = get_status_string();
  request->send(200, "text/plain", results.c_str());
}

// Callback: send 404 if requested file does not exist
void onPageNotFound(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(404, "text/plain", "Not found");
}

/***********************************************************
 * Main
 */

void setup() {
  // Init LED and turn off
  pingTimer = millis(); // Start now.

  // Start Serial port
  Serial.begin(115200);

  // Make sure we can read the file system
  if( !SPIFFS.begin(true)){
    Serial.println("Error mounting SPIFFS");
    while(1);
  }
  conecta_wifi();

  // On HTTP request for root, provide index.html file
  server.on("/facepe.png", HTTP_GET, facepe);

  // On HTTP request for root, provide index.html file
  server.on("/iot.png", HTTP_GET, iot);

  // On HTTP request for root, provide index.html file
  server.on("/upe.png", HTTP_GET, upe);

  // On HTTP request for root, provide index.html file
  server.on("/", HTTP_GET, onIndexRequest);

  // On HTTP request for distance, provide distance.html file
  server.on("/distance", HTTP_GET, onDistanceRequest);

  // On HTTP request for status, provide status.html file
  server.on("/status", HTTP_GET, onStatusRequest);

  // On HTTP request for style sheet, provide style.css
  server.on("/style.css", HTTP_GET, onCSSRequest);

  // Handle requests for pages that do not exist
  server.onNotFound(onPageNotFound);

  // Start web server
  server.begin();
}

void conecta_wifi(){
   // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());
}

void loop() {
  
  // Look for and handle WebSocket data
   time_Measurement();
   distance = duration * (0.0343) / 2;// calcula a distância de sentido único percorrida pelo pulso
   get_vaga_status();
   get_status_string();
   if(WiFi.status() != WL_CONNECTED){
    Serial.println("Wifi desconectou. Reconectando novamente");
    conecta_wifi();
   }
   
}
void time_Measurement()
{ //função para medir o tempo que o pulso leva para retornar
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH);
}
String get_distance_in_cm() {
  float distance_cm = hc.dist();
  uint8_t distancia = (uint8_t) (distance_cm);
  return String(distancia);
}

bool get_vaga_status() {
  bool isOccupied = false;

  if (hc.dist() < obj_distance ) {
    if (occupiedStartTime == 0) {
      occupiedStartTime = millis();
    } else if (millis() - occupiedStartTime > timeout) {
      isOccupied = true;
    }
  } else {
    isOccupied = false;
    occupiedStartTime = 0;
  }
  return isOccupied;
}

String get_status_string() {
  bool status = get_vaga_status();
  if(status) {
    return "Ocupada!";
  } else {
    return "Livre!";
  }
}
