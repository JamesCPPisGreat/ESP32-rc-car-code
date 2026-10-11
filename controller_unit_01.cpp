#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ========================================
// CAR CONFIGURATION
// ========================================

#define TARGET_CAR_ID 1
#define WIFI_CHANNEL  1

// MAC address of the selected car ESP32
uint8_t carAddress[] = {
  0x24, 0x6F, 0x28, 0xAB, 0xCD, 0xEF
};

// ========================================
// JOYSTICK CONFIGURATION
// ========================================

#define JOY_X 34
#define JOY_Y 35
#define JOY_SW 27

// Adjust if the joystick does not center at 2048
#define JOY_CENTER_X 2048
#define JOY_CENTER_Y 2048

// Ignore small movements around the center
#define DEADZONE 100

// ========================================
// ESP-NOW PACKET
// Must match the car receiver
// ========================================

typedef struct {
  uint8_t carID;
  int16_t throttle;
  int16_t steering;
} RCCommand;

RCCommand command;

// ========================================
// READ JOYSTICK AXIS
// Output range: -255 to +255
// ========================================

int readAxis(int pin, int center, bool invert) {
  int raw = analogRead(pin);
  int delta = raw - center;

  // Deadzone around the center
  if (abs(delta) < DEADZONE) {
    return 0;
  }

  // Scale both sides independently
  int output;

  if (delta > 0) {
    output = map(delta, DEADZONE, 2047, 0, 255);
  } else {
    output = map(delta, -2048, -DEADZONE, -255, 0);
  }

  output = constrain(output, -255, 255);

  if (invert) {
    output = -output;
  }

  return output;
}

// ========================================
// SETUP
// ========================================

void setup() {
  Serial.begin(115200);

  analogReadResolution(12);

  pinMode(JOY_X, INPUT);
  pinMode(JOY_Y, INPUT);

  pinMode(JOY_SW, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Set the ESP-NOW channel
  if (esp_wifi_set_channel(
        WIFI_CHANNEL,
        WIFI_SECOND_CHAN_NONE
      ) != ESP_OK) {
    Serial.println("Failed to set Wi-Fi channel!");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("\nESP-NOW RC CONTROLLER");

  Serial.print("Controller MAC: ");
  Serial.println(WiFi.macAddress());

  Serial.printf("Target car ID: %d\n", TARGET_CAR_ID);
  Serial.printf("Wi-Fi channel: %d\n", WIFI_CHANNEL);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed!");
    while (true) {
      delay(1000);
    }
  }

  // Register the selected car as a peer
  esp_now_peer_info_t peer = {};

  memcpy(peer.peer_addr, carAddress, 6);

  peer.channel = WIFI_CHANNEL;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;

  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("Failed to add car peer!");
    while (true) {
      delay(1000);
    }
  }

  command.carID = TARGET_CAR_ID;

  Serial.println("Controller ready!");
}

// ========================================
// MAIN LOOP
// ========================================

void loop() {
  command.carID = TARGET_CAR_ID;

  // Y-axis = throttle
  // Invert so pushing forward produces positive throttle
  command.throttle = readAxis(
    JOY_Y,
    JOY_CENTER_Y,
    true
  );

  // X-axis = steering
  command.steering = readAxis(
    JOY_X,
    JOY_CENTER_X,
    false
  );

  // Send the command
  esp_err_t result = esp_now_send(
    carAddress,
    (uint8_t *)&command,
    sizeof(command)
  );

  // Debug information
  Serial.printf(
    "Car:%d | Throttle:%d | Steering:%d | Send:%s\n",
    command.carID,
    command.throttle,
    command.steering,
    result == ESP_OK ? "OK" : "ERROR"
  );

  delay(20);
}
