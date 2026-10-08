/**    DRV8833 AOUT1/AOUT2 → Left motor
    DRV8833 BOUT1/BOUT2 → Right motor
    DRV8833 AIN1 → GPIO 25
    DRV8833 AIN2 → GPIO 26
    DRV8833 BIN1 → GPIO 27
    DRV8833 BIN2 → GPIO 14
    DRV8833 GND → ESP32 GND
    Motor battery → DRV8833 motor supply
    Do not power the motors from the ESP32 3.3V pin.
    Connect the motor battery negative, DRV8833 GND, and ESP32 GND together.

The following code uses LEDC PWM, which is appropriate for the ESP32.
1. Remote ESP32 — joystick transmitter
**/
#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

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

  // Bluetooth device name
  SerialBT.begin("ESP32_RC_REMOTE");

  analogReadResolution(12);

  Serial.println("Remote ready.");
  Serial.println("Bluetooth name: ESP32_RC_REMOTE");
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
  int steering = readJoystick(JOY_X, INVERT_X);
  int throttle = readJoystick(JOY_Y, INVERT_Y);

  // Send:
  // T = throttle
  // S = steering
  //
  // Example:
  // T:50,S:20

  SerialBT.printf("T:%d,S:%d\n", throttle, steering);

  // Debug
  Serial.printf("Throttle: %d  Steering: %d\n",
                throttle, steering);

  delay(30); // ~33 updates/sec
}
