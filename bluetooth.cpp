#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

// ===== CONFIGURATION =====
// Edit DEVICE_ID to create unique car/remote pairs.
// Example:
//   Set 1: DEVICE_ID = "01" (car.cpp and bluetooth.cpp both use "01")
//   Set 2: DEVICE_ID = "02" (car.cpp and bluetooth.cpp both use "02")
// This prevents interference between multiple car/remote pairs.
const char* DEVICE_ID = "01";
// =======================

// Joystick pins
const int JOY_X = 34;
const int JOY_Y = 35;

// Change these if your joystick works backwards
const bool INVERT_X = false;
const bool INVERT_Y = true;

// Joystick center and dead zone
const int JOY_CENTER = 2048;
const int DEAD_ZONE = 180;

void setup() {
  Serial.begin(115200);

  char remote_name[32];
  char car_name[32];
  snprintf(remote_name, sizeof(remote_name), "ESP32_RC_REMOTE_%s", DEVICE_ID);
  snprintf(car_name, sizeof(car_name), "ESP32_RC_CAR_%s", DEVICE_ID);

  SerialBT.begin(remote_name);
  analogReadResolution(12);

  Serial.printf("Remote ready. Name: %s\n", remote_name);
  Serial.printf("Attempting to connect to %s...\n", car_name);

  if (SerialBT.connect(car_name)) {
    Serial.println("Connected to car.");
  } else {
    Serial.println("Not connected yet. Please retry.");
  }
}

int readJoystick(int pin, bool invert) {
  int value = analogRead(pin);
  int output = value - JOY_CENTER;

  // Dead zone
  if (abs(output) < DEAD_ZONE) {
    output = 0;
  }

  // Convert approximately -2048...2047 to -100...100
  int percent = map(output, -2048, 2047, -100, 100);

  percent = constrain(percent, -100, 100);

  if (invert) {
    percent = -percent;
  }

  return percent;
}

void loop() {
  char car_name[32];
  snprintf(car_name, sizeof(car_name), "ESP32_RC_CAR_%s", DEVICE_ID);

  if (!SerialBT.connected()) {
    SerialBT.connect(car_name);
    delay(1000);
    return;
  }

  int steering = readJoystick(JOY_X, INVERT_X);
  int throttle = readJoystick(JOY_Y, INVERT_Y);

  // Send:
  // T = throttle
  // S = steering
  // Example:
  // T:50,S:20

  SerialBT.printf("T:%d,S:%d\n", throttle, steering);

  // Debug
  Serial.printf("Throttle: %d  Steering: %d\n", throttle, steering);

  delay(30); // ~33 updates/sec
}
