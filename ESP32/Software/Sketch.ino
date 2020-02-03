// Libraries
//--- ESP32
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
//--- C
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Credentials
#include "./Credentials/GBridge.h"
#include "./Credentials/Wifi.h"

// Configuration
#include "./Configuration/GBridge.h"

// GPIO pins
const int rightLightPWM = 16;
const int leftLightPWM = 19;
const int rightLightEnable = 17;
const int leftLightEnable = 18;

// setting PWM properties
const int lightsPwmFrequency = 40000;  // higher frequency -> less flickering
const int lightsPwmChannel = 0;        // pwm channel of light pins
const int lightsPwmResolution = 10;    // 10 Bit = 0-1024 (2^10) for Duty Cycle

// Connectivity
WiFiClient wifiClient;
PubSubClient client;

// Threads
pthread_t connectivityThread;
pthread_t mqttMessagingThread;

int counter;
bool initialConnectionEstablished = false;

// Function Declarations:
void mqttSubscribtionHandler(char* topic, byte* payload, unsigned int length);
void* connectionKeeper(void* param);
void* mqttMessager(void* param);
void WifiConnector();
void MqttConnector();

// ----------------------------------------------------------------------------
// SETUP
// ----------------------------------------------------------------------------

void setup() {
  // Serial Setup
  Serial.begin(115200);

  // GPIO Setup
  ledcSetup(lightsPwmChannel, lightsPwmFrequency, lightsPwmResolution);
  ledcAttachPin(leftLightPWM, lightsPwmChannel);
  ledcAttachPin(rightLightPWM, lightsPwmChannel);
  pinMode(leftLightEnable, OUTPUT);
  pinMode(rightLightEnable, OUTPUT);

  // Initial light (50% Brightness) when turning on
  ledcWrite(lightsPwmChannel, 1024 / 4);

  // MQTT Setup
  client.setClient(wifiClient);
  client.setServer(GBRIDGE_MQTT_SERVER, GBRIDGE_MQTT_PORT);
  client.setCallback(&mqttSubscribtionHandler);

  // Thread Creations
  if (pthread_create(&connectivityThread, NULL, connectionKeeper, NULL) == 0) {
    Serial.printf("Thread Connection Keeper successfully started\n");
  }

  if (pthread_create(&mqttMessagingThread, NULL, mqttMessager, NULL) == 0) {  // necessary to receive messages
    Serial.print("Thread mqtt communication successfully started\n");
  }
}

void loop() {
  // client.loop();  // necessary to receive messages (runs in its own thread now, see mqttMessager)
}

// ----------------------------------------------------------------------------
// FUNCTIONS
// ----------------------------------------------------------------------------

void WifiConnector() {
  // Connecting to Wifi
  counter = 0;
  if (WiFi.status() != WL_CONNECTED) {
    Serial.printf("Connecting to Wifi: %s\n", WIFI_SSID);
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PW);
    while (WiFi.status() != WL_CONNECTED && counter <= 5) {
      Serial.printf("Wifi Connection Try: %d\n", counter++);
      delay(1000);
    }
    if (counter > 5 && WiFi.status() != WL_CONNECTED) {
      Serial.printf("Failed to connect to Wifi: %s\n", WIFI_SSID);
    } else {
      Serial.printf("Connected to Wifi: %s\n", WIFI_SSID);
    }
  }
}

void MqttConnector() {
  // Connecting to MQTT Server
  if (!client.connected()) {
    Serial.printf("Connecting to GBridge via MQTT...\n");
    if (client.connect(GBRIDGE_MQTT_CLIENTID, GBRIDGE_MQTT_USERNAME, GBRIDGE_MQTT_PASSWORD)) {
      // MQTT actions after wifi connection
      client.subscribe(onoff_get);
      client.subscribe(brightness_get);
      Serial.printf("Connected and configured GBridge: %s, %s\n", GBRIDGE_MQTT_CLIENTID, GBRIDGE_MQTT_USERNAME);
    } else {
      Serial.println("Failed with state ");
      Serial.println(client.state());
    }
  }
}

void mqttSubscribtionHandler(char* topic, byte* payload, unsigned int length) {
  Serial.printf("mqttSubscriptionHandler is running now..\n");
  if (client.connected()) {
    int previousValue = (ledcRead(0) / 1024) * 100;
    int value;
    int valueDifference;  // Fidd between old and new value, used for fading
    char responseToPublish[4];

    sprintf(responseToPublish, "%d", value);
    payload[length] = '\0';
    value = atol((char*)payload);
    Serial.println("-----------------------");
    Serial.printf("Topic: %s\n", topic);
    Serial.printf("Message: %d\n", value);
    Serial.println("-----------------------");

    if (strcmp(topic, onoff_get) == 0) {
      digitalWrite(leftLightEnable, value);
      digitalWrite(rightLightEnable, value);
      client.publish(onoff_set, responseToPublish);
    } else if (strcmp(topic, brightness_get) == 0) {
      digitalWrite(leftLightEnable, HIGH);
      digitalWrite(rightLightEnable, HIGH);
      ledcWrite(0, round((1024.0 / 100) * value));  // Writes to the whole channel, so both lights get adjusted
      client.publish(brightness_set, responseToPublish);
    }
  }
}

// ----------------------------------------------------------------------------
// Threaded Functions
// ----------------------------------------------------------------------------

void* connectionKeeper(void* param) {
  while (true) {
    // Connecting to Wifi
    WifiConnector();

    // Connecting to MQTT Server
    MqttConnector();

    initialConnectionEstablished = true;  // Thread joining workaround to start mqttMessager() only after a successfull connection
    delay(1000);                          // To check only every second
  }
}

void* mqttMessager(void* param) {
  while (true) {
    while (initialConnectionEstablished == false) {
      Serial.printf("Thread MqttMessager is waiting for connection..\n");
      delay(1000);
    }
    Serial.printf("Client looping now..\n");
    while (initialConnectionEstablished == true) {
      // while (true) {
      client.loop();
    }
  }
}

// TODO: implement fading function
// PARAMS: old brightness, new brightness, fading duration
// PROTO:
// valueDifference = abs(value - previousValue);

// if (previousValue > value) {
//   for (double fader = previousValue; round(fader) >= value; (double)fader -= (double)valueDifference/100) {
//     usleep();
//   }
// }
// }

// TODO: eventually needs a fix for pwm at 1% brightness (still too bright?!)