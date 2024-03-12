#include <Arduino.h>
#include <ESP32Encoder.h>
#include <FlexyStepper.h>

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
    float leadscrew_rev = rev_turned * thread_pitch / leadscrew_pitch;
    leadscrew.setTargetPositionInRevolutions(leadscrew_rev);
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);

  leadscrew.connectToPins(6, 7);
  leadscrew.setStepsPerRevolution(1600);
  float max_rps = 20;
  leadscrew.setSpeedInRevolutionsPerSecond(max_rps);
  leadscrew.setAccelerationInRevolutionsPerSecondPerSecond(max_rps * 100);

  ESP32Encoder::useInternalWeakPullResistors = puType::up;
  encoder_last_read = millis();
  encoder.attachHalfQuad(17, 18);
  Serial.println("STARTING UP");
  // xTaskCreatePinnedToCore(move_leadscrew, "move_leadscrew", 10000, NULL, 1, NULL, 0);
  xTaskCreate(read_spindle_rev, "read_spindle_rot", 10000, NULL, 1, NULL);
  // xTaskCreate(render_ui, "render_ui", 10000, NULL, 2, NULL);
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