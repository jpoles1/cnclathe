#include <Arduino.h>
#include <ESP32Encoder.h>
#include "ui.h"

ESP32Encoder encoder;
ulong encoder_last_read;
UI ui;

void setup() {
  Serial.begin(115200);

  ui.init();
  ui.draw();

  ESP32Encoder::useInternalWeakPullResistors=UP;
  encoder_last_read = millis();
  encoder.attachHalfQuad(21, 22);
}

void loop() {
  ulong now = millis();
  ulong ms_elapsed = now - encoder_last_read;
  encoder_last_read = now;
  int encoder_count = encoder.getCount();
  encoder.clearCount();
  const int quadrature_mult = 2;
  float deg_turned = encoder_count * 360 / (600 * quadrature_mult);
  float rot_turned = deg_turned / 360;
  float rpm = 1000 * 60 * rot_turned / ms_elapsed; 
  char print_str[256];
  sprintf(print_str, "Current pos: %d", encoder_count);
  Serial.println(print_str);
  sprintf(print_str, "RPM: %f", rpm);
  Serial.println(print_str);
  ui.draw_rpm(rpm);
  delay(250);
}