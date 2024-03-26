#include <Arduino.h>
#include <ESP32Encoder.h>
#include <FlexyStepper.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <ArduinoJson.h>

ESP32Encoder encoder;
FlexyStepper leadscrew;

unsigned long encoder_last_read;
unsigned long ws_last_updated;

int ws_update_interval = 1000; // in ms

uint64_t encoder_last_count = 0;
float leadscrew_pitch = 1.5;
float thread_pitch = 3;
float spindle_rpm = 0;

void read_spindle_rev(void *parameters)
{
  for (;;)
  {
    int64_t encoder_count = encoder.getCount();
    leadscrew.setTargetPositionInRevolutions(leadscrew_rev);
    const int quadrature_mult = 2;
    float rev_turned = (float)encoder_count / (float)(600 * quadrature_mult);
    float spindle_rpm = rev_turned / (millis() - encoder_last_read) * 1000 * 60;
    encoder_last_read = millis();
    float leadscrew_rev = rev_turned * thread_pitch / leadscrew_pitch;
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}


// Replace with your network credentials
const char* ssid = "Mole People";
const char* password = "deepEarth!";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Initialize SPIFFS
void init_SPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

void handle_ws_incoming(void *arg, uint8_t *data, size_t len) {
  char *msg = (char *)data;
  Serial.printf("Received data: %s\n", msg);
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, msg);
  // If thread_pitch is in the message, update the leadscrew pitch
  if (doc.containsKey("thread_pitch")) {
    thread_pitch = doc["thread_pitch"];
    encoder.clearCount();
    leadscrew.setCurrentPositionInRevolutions(0);
    Serial.println(thread_pitch);
  }
  
}

void setup() {
  Serial.begin(115200);

  leadscrew.connectToPins(6, 7);
  leadscrew.setStepsPerRevolution(1600);
  float max_rps = 20;
  leadscrew.setSpeedInRevolutionsPerSecond(max_rps);
  leadscrew.setAccelerationInRevolutionsPerSecondPerSecond(max_rps * 100);

  ESP32Encoder::useInternalWeakPullResistors = puType::down;
  encoder_last_read = millis();
  ws_last_updated = millis();
  pinMode(17,INPUT_PULLDOWN );
  pinMode(18, INPUT_PULLDOWN);
  encoder.attachHalfQuad(17, 18);
  Serial.println("STARTING UP");
  // xTaskCreatePinnedToCore(move_leadscrew, "move_leadscrew", 10000, NULL, 1, NULL, 0);
  xTaskCreate(read_spindle_rev, "read_spindle_rot", 10000, NULL, 1, NULL);
  // xTaskCreate(render_ui, "render_ui", 10000, NULL, 2, NULL);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println(WiFi.localIP());

  init_SPIFFS();

  // Handle WebSocket
  ws.onEvent([](AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
      switch (type) {
        case WS_EVT_CONNECT:
          Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
          break;
        case WS_EVT_DISCONNECT:
          Serial.printf("WebSocket client #%u disconnected\n", client->id());
          break;
        case WS_EVT_DATA:
          handle_ws_incoming(arg, data, len);
          break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
          break;
    }
  });
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.serveStatic("/", SPIFFS, "/");

  // Start server
  server.begin();

  // Print IP to serial
  Serial.println(WiFi.localIP());
}

void loop()
{
  if (!leadscrew.motionComplete()) {
    leadscrew.processMovement();
  }
  if (millis() - ws_last_updated > ws_update_interval) {
    ws_last_updated = millis();
    DynamicJsonDocument doc(1024);
    doc["spindle_rpm"] = spindle_rpm;
    doc["leadscrew_rpm"] = spindle_rpm * thread_pitch / leadscrew_pitch;
    String output;
    serializeJson(doc, output);
    ws.textAll(output.c_str());
  }
}