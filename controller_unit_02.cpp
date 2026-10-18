#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// =====================================
// SELECT CAR
// =====================================

#define TARGET_CAR_ID 2
#define WIFI_CHANNEL  6

// =====================================
// JOYSTICK
// =====================================

#define JOY_X 34
#define JOY_Y 35

#define JOY_CENTER_X 2048
#define JOY_CENTER_Y 2048

#define DEADZONE 100

// =====================================
// ESP-NOW DATA
// =====================================

typedef struct {
  uint8_t carID;
  int16_t throttle;
  int16_t steering;
} RCCommand;

RCCommand command;

// =====================================
// JOYSTICK READING
// =====================================

int readAxis(int pin, int center, bool invert) {
  int raw = analogRead(pin);
  int delta = raw - center;

  if (abs(delta) < DEADZONE) {
    return 0;
  }

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

// =====================================
// SETUP
// =====================================

void setup() {
  Serial.begin(115200);

  analogReadResolution(12);

  pinMode(JOY_X, INPUT);
  pinMode(JOY_Y, INPUT);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_wifi_set_channel(
        WIFI_CHANNEL,
        WIFI_SECOND_CHAN_NONE
      ) != ESP_OK) {
    Serial.println("Failed to set Wi-Fi channel!");
    while (true) delay(1000);
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed!");
    while (true) delay(1000);
  }

  // Register broadcast address: FF:FF:FF:FF:FF:FF
  esp_now_peer_info_t peer = {};

  uint8_t broadcastAddress[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
  };

  memcpy(peer.peer_addr, broadcastAddress, 6);

  peer.channel = WIFI_CHANNEL;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;

  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("Failed to add broadcast peer!");
    while (true) delay(1000);
  }

  Serial.printf(
    "Controller ready | Car ID: %d | Channel: %d\n",
    TARGET_CAR_ID,
    WIFI_CHANNEL
  );
}

// =====================================
// MAIN LOOP
// =====================================

void loop() {
  command.carID = TARGET_CAR_ID;

  command.throttle = readAxis(
    JOY_Y,
    JOY_CENTER_Y,
    true
  );

  command.steering = readAxis(
    JOY_X,
    JOY_CENTER_X,
    false
  );

  uint8_t broadcastAddress[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
  };

  esp_err_t result = esp_now_send(
    broadcastAddress,
    (uint8_t *)&command,
    sizeof(command)
  );

  Serial.printf(
    "ID:%d | T:%d | S:%d | %s\n",
    command.carID,
    command.throttle,
    command.steering,
    result == ESP_OK ? "Sent" : "Error"
  );

  delay(20);
}
