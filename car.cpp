#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

// -------------------------
// DRV8833 pins
// -------------------------

// Left motor
const int LEFT_IN1 = 25;
const int LEFT_IN2 = 26;

// Right motor
const int RIGHT_IN1 = 27;
const int RIGHT_IN2 = 14;

// -------------------------
// PWM settings
// -------------------------

const int PWM_FREQ = 20000;
const int PWM_RESOLUTION = 8;

// PWM channels
const int LEFT_CH1 = 0;
const int LEFT_CH2 = 1;

const int RIGHT_CH1 = 2;
const int RIGHT_CH2 = 3;

// -------------------------
// Failsafe
// -------------------------

unsigned long lastPacketTime = 0;

const unsigned long FAILSAFE_TIME = 500;

// -------------------------
// Motor function
// -------------------------

void setMotor(int in1Channel,
              int in2Channel,
              int speed) {

  speed = constrain(speed, -255, 255);

  if (speed > 0) {
    // Forward
    ledcWrite(in1Channel, speed);
    ledcWrite(in2Channel, 0);
  }
  else if (speed < 0) {
    // Reverse
    ledcWrite(in1Channel, 0);
    ledcWrite(in2Channel, -speed);
  }
  else {
    // Stop
    ledcWrite(in1Channel, 0);
    ledcWrite(in2Channel, 0);
  }
}

// -------------------------
// Stop both motors
// -------------------------

void stopMotors() {

  setMotor(LEFT_CH1, LEFT_CH2, 0);
  setMotor(RIGHT_CH1, RIGHT_CH2, 0);
}

// -------------------------
// Setup
// -------------------------

void setup() {

  Serial.begin(115200);

  // PWM setup
  ledcSetup(LEFT_CH1, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(LEFT_CH2, PWM_FREQ, PWM_RESOLUTION);

  ledcSetup(RIGHT_CH1, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(RIGHT_CH2, PWM_FREQ, PWM_RESOLUTION);

  ledcAttachPin(LEFT_IN1, LEFT_CH1);
  ledcAttachPin(LEFT_IN2, LEFT_CH2);

  ledcAttachPin(RIGHT_IN1, RIGHT_CH1);
  ledcAttachPin(RIGHT_IN2, RIGHT_CH2);

  stopMotors();

  // Start Bluetooth
  SerialBT.begin("ESP32_RC_CAR");

  Serial.println("RC Car ready.");
  Serial.println("Bluetooth name: ESP32_RC_CAR");

  lastPacketTime = millis();
}

// -------------------------
// Main loop
// -------------------------

void loop() {

  // -------------------------
  // Bluetooth data
  // -------------------------

  if (SerialBT.available()) {

    String data = SerialBT.readStringUntil('\n');

    data.trim();

    int tIndex = data.indexOf("T:");
    int sIndex = data.indexOf(",S:");

    if (tIndex >= 0 && sIndex >= 0) {

      int throttle =
          data.substring(tIndex + 2, sIndex).toInt();

      int steering =
          data.substring(sIndex + 3).toInt();

      throttle = constrain(throttle, -100, 100);
      steering = constrain(steering, -100, 100);

      // -------------------------
      // Skid steer mixing
      // -------------------------
// Prevent one side from saturating and forcing the other side to "jump"
      
      int left = throttle + steering;
      int right = throttle - steering;

      int maxMotor = max(abs(left), abs(right));
      if (maxMotor > 100) {
        float scale = 100.0 / maxMotor;
        left = (int)(left * scale);
        right = (int)(right * scale);
      }
      
      
      // Keep values within range
      left = constrain(left, -100, 100);
      right = constrain(right, -100, 100);

      // Convert -100...100 to -255...255
      int leftPWM = map(left, -100, 100, -255, 255);
      int rightPWM = map(right, -100, 100, -255, 255);

      setMotor(
        LEFT_CH1,
        LEFT_CH2,
        leftPWM
      );

      setMotor(
        RIGHT_CH1,
        RIGHT_CH2,
        rightPWM
      );

      lastPacketTime = millis();

      Serial.printf(
        "T:%d S:%d | L:%d R:%d\n",
        throttle,
        steering,
        leftPWM,
        rightPWM
      );
    }
  }

  // -------------------------
  // Bluetooth failsafe
  // -------------------------

  if (millis() - lastPacketTime > FAILSAFE_TIME) {
    stopMotors();
  }

  delay(1);
}
