#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ========================================
// CHANGE THESE FOR EACH CAR
// ========================================

#define CAR_ID       1
#define WIFI_CHANNEL 1

// Example configurations:
// Car 1: ID 1, channel 1
// Car 2: ID 2, channel 6
// Car 3: ID 3, channel 11

// ========================================
// MOTOR PINS - DRV8833 (REASSIGNED - NO GPIO 16/17)
// ========================================

#define LEFT_IN1  27  // Connect to DRV8833 IN1 (Safe Digital Pin)
#define LEFT_IN2  33  // Connect to DRV8833 IN2 (Safe Digital Pin)

#define RIGHT_IN1 18  // Connect to DRV8833 IN3
#define RIGHT_IN2 19  // Connect to DRV8833 IN4

// ========================================
// ESP-NOW PACKET
// ========================================

typedef struct {
  uint8_t carID;
  int16_t throttle;
  int16_t steering;
} RCCommand;

RCCommand command = {0, 0, 0};

portMUX_TYPE commandMux = portMUX_INITIALIZER_UNLOCKED;

volatile uint32_t lastPacketTime = 0;

// ========================================
// RECEIVE CALLBACK (Arduino-ESP32 3.x)
// ========================================

void onDataRecv(
  const esp_now_recv_info_t *info,
  const uint8_t *data,
  int len
) {
  if (len != sizeof(RCCommand)) return;

  RCCommand incoming;
  memcpy(&incoming, data, sizeof(incoming));

  // Accept only commands addressed to this car
  if (incoming.carID != CAR_ID) return;

  incoming.throttle = constrain(incoming.throttle, -255, 255);
  incoming.steering = constrain(incoming.steering, -255, 255);

  portENTER_CRITICAL(&commandMux);
  command = incoming;
  lastPacketTime = millis();
  portEXIT_CRITICAL(&commandMux);
}

// ========================================
// DRV8833 MOTOR OUTPUT: DIRECT PWM CONTROL
// ========================================

void setMotor(int in1, int in2, int speed) {
  speed = constrain(speed, -255, 255);

  if (speed > 0) {
    // Forward: PWM signal to IN1, keep IN2 fully LOW
    analogWrite(in1, speed);
    analogWrite(in2, 0);
  } else if (speed < 0) {
    // Reverse: Keep IN1 fully LOW, send PWM signal to IN2
    analogWrite(in1, 0);
    analogWrite(in2, abs(speed));
  } else {
    // Coast / Stop: Both pins completely LOW
    analogWrite(in1, 0);
    analogWrite(in2, 0);
  }
}

void stopMotors() {
  setMotor(LEFT_IN1, LEFT_IN2, 0);
  setMotor(RIGHT_IN1, RIGHT_IN2, 0);
}

// ========================================
// SETUP
// ========================================

void setup() {
  Serial.begin(115200);

  // Initialize all 4 directional control lines as outputs safely
  pinMode(LEFT_IN1, OUTPUT);
  pinMode(LEFT_IN2, OUTPUT);
  pinMode(RIGHT_IN1, OUTPUT);
  pinMode(RIGHT_IN2, OUTPUT);

  stopMotors();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Set this car's ESP-NOW channel
  esp_err_t result = esp_wifi_set_channel(
    WIFI_CHANNEL,
    WIFI_SECOND_CHAN_NONE
  );

  if (result != ESP_OK) {
    Serial.println("Failed to set Wi-Fi channel!");
    while (true) {
      stopMotors();
      delay(100);
    }
  }

  Serial.println("\nESP-NOW RC CAR (DRV8833 SAFE MODE)");
  Serial.printf("Car ID: %d\n", CAR_ID);
  Serial.printf("Wi-Fi channel: %d\n", WIFI_CHANNEL);
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed!");

    while (true) {
      stopMotors();
      delay(100);
    }
  }

  esp_now_register_recv_cb(onDataRecv);

  Serial.println("Receiver ready!");
}

// ========================================
// MAIN LOOP
// ========================================

void loop() {
  RCCommand cmd;
  uint32_t packetTime;

  portENTER_CRITICAL(&commandMux);
  cmd = command;
  packetTime = lastPacketTime;
  portEXIT_CRITICAL(&commandMux);

  // Failsafe: stop if no valid command for 500 ms
  if (packetTime == 0 ||
      (uint32_t)(millis() - packetTime) > 500) {
    stopMotors();
    delay(5);
    return;
  }

  int throttle = cmd.throttle;
  int steering = cmd.steering;

  if (abs(throttle) < 8) throttle = 0;
  if (abs(steering) < 8) steering = 0;

  // Differential steering
  int leftSpeed  = throttle + steering;
  int rightSpeed = throttle - steering;

  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);

  // Execute movements via safe remapped pins
  setMotor(LEFT_IN1, LEFT_IN2, leftSpeed);
  setMotor(RIGHT_IN1, RIGHT_IN2, rightSpeed);

  delay(5);
}
