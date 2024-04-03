#include <Arduino.h>
#include <ESP32Encoder.h>
#include <FlexyStepper.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <ArduinoJson.h>

ESP32Encoder encoder;
FlexyStepper leadscrew;
FlexyStepper xaxis;

const int quadrature_mult = 2;
const float leadscrew_pitch = 1.5;

unsigned long encoder_last_rpm_time;
unsigned long ws_last_updated;

int ws_update_interval = 1000; // in ms

bool leadscrew_direction_invert = true; // Set to true if the leadscrew moves in the opposite direction of the spindle

int64_t encoder_last_rpm_count = 0;
float thread_pitch = 1;
float spindle_rpm = 0;

void calc_spindle_rpm() {
    int64_t encoder_count = encoder.getCount();
    float rev_turned_since_last = (float)(encoder_count - encoder_last_rpm_count) / (float)(600 * quadrature_mult);
    spindle_rpm = rev_turned_since_last  * 1000 * 60 / (millis() - encoder_last_rpm_time);
    encoder_last_rpm_time = millis();
    encoder_last_rpm_count = encoder_count;
}

void read_spindle_rev(void *parameters)
{
  for (;;)
  {
    int64_t encoder_count = encoder.getCount()  * (leadscrew_direction_invert ? -1 : 1);
    float rev_turned = (float)encoder_count / (float)(600 * quadrature_mult);
    float leadscrew_rev = rev_turned * thread_pitch / leadscrew_pitch;
    leadscrew.setTargetPositionInRevolutions(leadscrew_rev);
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
  if (doc.containsKey("halt")) {
    xaxis.setTargetPositionRelativeInMillimeters(0);
    return;
  }
  if (doc.containsKey("thread_pitch")) {
    thread_pitch = doc["thread_pitch"];
    encoder.clearCount();
    leadscrew.setCurrentPositionInRevolutions(0);
    Serial.println(thread_pitch);
  }
  if (doc.containsKey("xjog")) {
    float xjog = doc["xjog"];
    xaxis.setTargetPositionRelativeInMillimeters(xjog);
  }
}

void send_ws_update() {
    calc_spindle_rpm();
    ws_last_updated = millis();
    DynamicJsonDocument doc(1024);
    doc["spindle_rpm"] = round(spindle_rpm * 100) / 100.0;
    doc["leadscrew_rpm"] = round(spindle_rpm * thread_pitch / leadscrew_pitch * 100) / 100.0;
    doc["thread_pitch"] = thread_pitch;
    String output;
    serializeJson(doc, output);
    ws.textAll(output.c_str());
    //Serial.println(output);
}

void setup() {
  Serial.begin(115200);

  //Leadscrew stepper setup
  leadscrew.connectToPins(6, 7);
  leadscrew.setStepsPerRevolution(1600);
  float leadscrew_max_rps = 20;
  leadscrew.setSpeedInRevolutionsPerSecond(leadscrew_max_rps);
  leadscrew.setAccelerationInRevolutionsPerSecondPerSecond(leadscrew_max_rps * 100);

  //X-axis stepper setup
  xaxis.connectToPins(2,21);
  xaxis.setStepsPerRevolution(360 / 0.35);
  xaxis.setStepsPerMillimeter(360 / 0.35);
  float xaxis_max_rpm = 15;
  float xaxis_max_rps = xaxis_max_rpm / 60;
  xaxis.setSpeedInRevolutionsPerSecond(xaxis_max_rps);
  xaxis.setAccelerationInRevolutionsPerSecondPerSecond(xaxis_max_rps * 100);

  ESP32Encoder::useInternalWeakPullResistors = puType::down;
  encoder_last_rpm_time = millis();
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
          send_ws_update();
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
  if (!xaxis.motionComplete()) {
    xaxis.processMovement();
  }
  if (millis() - ws_last_updated > ws_update_interval) {
    send_ws_update();
  }
}