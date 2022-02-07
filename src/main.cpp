#include <Arduino.h>
#include <ESP32Encoder.h>
#include <FlexyStepper.h>
#include "ui.h"

ESP32Encoder encoder;
ulong encoder_last_read;
UI ui;
FlexyStepper leadscrew;

int leadscrew_tpi = 16;
float leadscrew_pitch = 1.588;
float metric_pitch_to_leadscrew_rpm(float thread_pitch, float spindle_rpm) {
  return spindle_rpm * thread_pitch / leadscrew_pitch;
}

float read_spindle_rpm() {
  ulong now = millis();
  ulong ms_elapsed = now - encoder_last_read;
  encoder_last_read = now;
  int encoder_count = encoder.getCount();
  encoder.clearCount();
  const int quadrature_mult = 2;
  float deg_turned = encoder_count * 360 / (600 * quadrature_mult);
  float rot_turned = deg_turned / 360;
  float rpm = 1000 * 60 * rot_turned / ms_elapsed; 
  return rpm;
}

void move_leadscrew(void * parameters) {
  for(;;) {
    if(!leadscrew.motionComplete()) {
      leadscrew.processMovement();
    }
  }
}


void read_spindle_rev(void * parameters) {
  for(;;) {
    ulong now = millis();
    ulong ms_elapsed = now - encoder_last_read;
    encoder_last_read = now;
    int encoder_count = encoder.getCount();
    //encoder.clearCount();
    const int quadrature_mult = 2;
    float deg_turned = encoder_count * 360 / (600 * quadrature_mult);
    float rev_turned = deg_turned / 360;
    leadscrew.setTargetPositionInRevolutions(rev_turned);
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);

  leadscrew.connectToPins(2, 13);
  leadscrew.setStepsPerRevolution(1600);
  float max_rps = 100;
  leadscrew.setSpeedInRevolutionsPerSecond(max_rps);
  leadscrew.setAccelerationInRevolutionsPerSecondPerSecond(max_rps*100);
  
  ui.init();
  ui.draw();

  ESP32Encoder::useInternalWeakPullResistors=UP;
  encoder_last_read = millis();
  encoder.attachHalfQuad(21, 22);
  //xTaskCreatePinnedToCore(move_leadscrew, "move_leadscrew", 10000, NULL, 1, NULL, 0);
  xTaskCreate(read_spindle_rev, "read_spindle_rot", 1000, NULL, 2, NULL);
}

void loop() {
  /*char print_str[256];
  float rot = read_spindle_rot();
  sprintf(print_str, "RPM: %f", rot);
  Serial.println(print_str);
  ui.draw_rpm(rot);
  leadscrew.moveToPositionInRevolutions(read_spindle_rot());*/
  //leadscrew.processMovement();
  if(!leadscrew.motionComplete()) {
    leadscrew.processMovement();
  }

}