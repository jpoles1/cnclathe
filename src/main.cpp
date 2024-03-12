#include <Arduino.h>
#include <ESP32Encoder.h>
#include <FlexyStepper.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <ArduinoJson.h>

ESP32Encoder encoder;
ulong encoder_last_read;
FlexyStepper leadscrew;

uint64_t encoder_last_count = 0;

float read_spindle_rpm()
{
  uint64_t encoder_count = encoder.getCount();
  ulong now = millis();
  ulong ms_elapsed = now - encoder_last_read;
  encoder_last_read = now;
  const int quadrature_mult = 2;
  int steps_moved = encoder_count - encoder_last_count;
  encoder_last_count = encoder_count;
  float rev_turned = (float)steps_moved / (float)(600 * quadrature_mult);
  int rpm = round(1000 * 60 * rev_turned / ms_elapsed);
  float rps = 1000 * rev_turned / ms_elapsed;
  if (rps < 10)
    rps = 10;
  // leadscrew.setSpeedInRevolutionsPerSecond(rps);
  Serial.println(rpm);
  Serial.println(rps);
  return rpm;
}

void move_leadscrew(void *parameters)
{
  for (;;)
  {
    if (!leadscrew.motionComplete())
    {
      leadscrew.processMovement();
    }
  }
}

/*int leadscrew_tpi = 16;
float leadscrew_pitch = 1.588;
float metric_pitch_to_leadscrew_rpm(float thread_pitch, float spindle_rpm) {
  return spindle_rpm * thread_pitch / leadscrew_pitch;
}*/
float leadscrew_pitch = 1.5;
float thread_pitch = 3;

void read_spindle_rev(void *parameters)
{
  for (;;)
  {
    int64_t encoder_count = encoder.getCount();
    const int quadrature_mult = 2;
    float rev_turned = (float)encoder_count / (float)(600 * quadrature_mult);
    Serial.println(rev_turned);
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
  if (doc.containsKey("thread_pitch")) {
    thread_pitch = doc["thread_pitch"];
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
  pinMode(17,INPUT_PULLDOWN );
  pinMode(18, INPUT_PULLDOWN);
  //encoder.attachHalfQuad(17, 18);
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
  /*char print_str[256];
  float rot = read_spindle_rot();
  sprintf(print_str, "RPM: %f", rot);
  Serial.println(print_str);
  ui.draw_rpm(rot);
  leadscrew.moveToPositionInRevolutions(read_spindle_rot());*/
  // leadscrew.processMovement();
  if (!leadscrew.motionComplete())
  {
    leadscrew.processMovement();
  } /*else {
    leadscrew.setCurrentPositionInSteps(0);
    encoder.clearCount();
    encoder_last_count = 0;
  }*/
}