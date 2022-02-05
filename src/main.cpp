#include <Arduino.h>
#include <ESP32Encoder.h>

ESP32Encoder encoder;

void setup() {
  Serial.begin(115200);

  ESP32Encoder::useInternalWeakPullResistors=UP;
  encoder.attachHalfQuad(21, 22);
  
}

void loop() {
  char print_str[256];
  sprintf(print_str, "Current pos: %d", (int32_t)encoder.getCount());
  Serial.println(print_str);
  encoder.clearCount();
  delay(250);
}