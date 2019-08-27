// Libraries
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClient.h>

// Credentials
#include "./Credentials/GBridge.h"
#include "./Credentials/Wifi.h"

// GPIO pins
const int leftLight = 16;
const int rightLight = 17;

    // setting PWM properties
    const int LightsPwmFreq =
        40000;  // the higher the frequency, the less flickering
const int LightsPwmChannel = 0;  // pwm channel of light pins
const int LightsPwmResolution =
    10;  // 10 Bit = 0-1024 (2^10) as Range for Duty Cycle

WiFiClient wifiClient;
PubSubClient client;

void setup() {
  // GPIO Setup
  ledcSetup(shotChannel, freq, resolution);
  ledcAttachPin(shotSensorPin, shotChannel);
  pinMode(shotSensorPin, INPUT);

  // Serial Setup
  Serial.begin(115200);

  // Wifi Setup
  Serial.println("Waiting for Wifi.");
  WiFi.begin(ssid, wifipw);
  // Wait until wifi is connected. If multiple Wifi AP's should be searched,
  // consider an array and a loop with a timeout
  while (WiFi.status() != WL_CONNECTED) {
  }
  Serial.println("Connected to Wifi: Byte");
  client.setClient(wifiClient);
  client.setServer(mqttServer, mqttPort);

  // MQTT Setup
  client.setCallback(mqttSubscribtionHandler);
  while (!client.connected()) {  // wait until connected to mqtt server
    Serial.println("Connecting to MQTT...");
    if (client.connect(GBRIDGE_MQTT_CLIENTID, GBRIDGE_MQTT_USERNAME,
                       GBRIDGE_MQTT_PASSWORD)) {
      Serial.println("connected to gbridge");
    } else {
      Serial.println("failed with state ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void loop() {
  Serial.println("publishing off");
  client.publish("gBridge/u2379/d8054/onoff", "0");
  delay(5000);
  Serial.println("publishing on");
  client.publish("gBridge/u2379/d8054/onoff", "0");
}

void mqttSubscribtionHandler(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);

  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  Serial.println("-----------------------");
}