// Credentials
#include "./Credentials/Blynk.h"
#include "./Credentials/Wifi.h"

// Configurations
#include "./Configuration/Blynk.h"

// Libraries
#include <BlynkSimpleEsp32.h>
#include <FreeRTOS/task.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <math.h>

#include <string>

// GPIO pins
const unsigned short int rightLightPWM = 19;
const unsigned short int leftLightPWM = 16;
const unsigned short int rightLightEnable = 18;
const unsigned short int leftLightEnable = 17;

// setting PWM properties
const unsigned short int leftLightPwmChannel = 3;
const unsigned short int rightLightPwmChannel = 6;
const unsigned short int lightsPwmFrequency = 40000;  // higher frequency -> less flickering
const unsigned short int lightsPwmResolution = 10;    // 10 Bit = 1024 (2^10) for Duty Cycle (0 to 1023)

// Object State (Model)
int leftLightState = 1;
int rightLightState = 1;
int leftLightBrightness = 512;
int rightLightBrightness = 512;
int mainLightsBrightness = 512;

// Storage vars
uint32_t mainBrightnessLastWriteTime = 0;

// Thread Handles
TaskHandle_t xTaskMainBrightnessHandle = NULL;

// Settings
int updateIntervalFromMainToAll = 1000;  // E.g. when the last value change of slider V1 happened 500ms ago, it should update all other sliders

// Storage variables
bool writtenFlag = false;

// Connection State
String IpAddress = "";
String MacAddress = "";

// ----------------------------------------------------------------------------
// SETUP
// ----------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);

  SetupGpio(leftLightEnable, rightLightEnable, leftLightPWM, rightLightPWM, leftLightPwmChannel, rightLightPwmChannel, lightsPwmFrequency, lightsPwmResolution);
  initializeOnBoot();

  ConnectToWifi(WIFI_SSID, WIFI_PW);
  if (BLYNK_USE_LOCAL_SERVER)
    Blynk.begin(BLYNK_AUTH, WIFI_SSID, WIFI_PW, BLYNK_SERVER, BLYNK_PORT);
  else
    Blynk.begin(BLYNK_AUTH, WIFI_SSID, WIFI_PW);

  // xTaskCreatePinnedToCore(virtualPinTwoToAllUpdater, "v1SliderUpdaterTask", 1000000, NULL, 0, &xTaskMainBrightnessHandle, 1);
}

void virtualPinTwoToAllUpdater(void* params) {
  Serial.printf("Task running on core %d\n", xPortGetCoreID());
  const TickType_t xDelay = updateIntervalFromMainToAll / portTICK_PERIOD_MS;

  vTaskDelay(xDelay);
  // while (true) {
  //   while ((esp_timer_get_time() / 1000LL) htnessLastWriteTime < updateIntervalFromMainToAll) {
  //     // Block until defined delay before updating all pins to main slider has passed
  //   }
  Serial.printf("Writing main Brightness to V4, V6, V7, V8, V9, diff on update: %d\n");
  Blynk.virtualWrite(V4, mainLightsBrightness);
  Blynk.virtualWrite(V6, mainLightsBrightness);
  Blynk.virtualWrite(V7, mainLightsBrightness);
  Blynk.virtualWrite(V8, mainLightsBrightness);
  Blynk.virtualWrite(V9, mainLightsBrightness);
  Serial.println("Suspending updater thread");
  //   vTaskSuspend(NULL);  // Suspend this task until woken up again
  // }
}

// void updateVirtualPinsAsync(const TaskHandle_t handle, ) {}

// ----------------------------------------------------------------------------
// MAIN LOOP
// ----------------------------------------------------------------------------

void loop() {
  ConnectToWifi(WIFI_SSID, WIFI_PW);
  ConnectToBlynk();
  UpdateIpAddressInBlynk();
  UpdateMacAddressInBlynk();
  Blynk.run();
}

// ----------------------------------------------------------------------------
// FUNCTIONS
// ----------------------------------------------------------------------------

// Blynk Functions

BLYNK_CONNECTED() {  // Restore hardware pins according to current UI config
  Blynk.syncAll();
}

// State Buttons--------------------------------------------------

BLYNK_WRITE(V1) {  // Both lights state
  int pinValue = param.asInt();
  leftLightState = pinValue;
  rightLightState = pinValue;
  digitalWrite(leftLightEnable, pinValue == 0 ? LOW : HIGH);
  Blynk.virtualWrite(V3, pinValue == 0 ? 0 : 1);
  Blynk.virtualWrite(V5, pinValue == 0 ? 0 : 1);
  Blynk.syncVirtual(V3, V5);

  BLYNK_WRITE(V3) {  //  Left light state
    int pinValue = param.asInt();
    leftLightState = pinValue;  // Model
    digitalWrite(leftLightEnable, pinValue == 0 ? LOW : HIGH);
    Blynk.virtualWrite(V1, leftLightState == 1 || rightLightState == 1 ? 1 : 0);
  }

  BLYNK_WRITE(V5) {  // Right light state
    int pinValue = param.asInt();
    rightLightState = pinValue;  // Model
    digitalWrite(rightLightEnable, pinValue == 0 ? LOW : HIGH);
    Blynk.virtualWrite(V1, leftLightState == 1 || rightLightState == 1 ? 1 : 0);
  }

  // Brightness Sliders --------------------------------------------------

  BLYNK_WRITE(V2) {  // Both lights brightness (slider)
    int pinValue = param.asInt();

    mainLightsBrightness = pinValue;  // Model
    leftLightBrightness = pinValue;   // Model
    rightLightBrightness = pinValue;  // Model

    ledcWrite(leftLightPwmChannel, percentToValue(pinValue, 1023));
    ledcWrite(rightLightPwmChannel, percentToValue(pinValue, 1023));

    xTaskCreatePinnedToCore(virtualPinTwoToAllUpdater, "v1SliderUpdaterTask", 1000000, NULL, 0, &xTaskMainBrightnessHandle, 1);

    // mainBrightnessLastWriteTime = esp_timer_get_time / 1000LL;
    // vTaskResume(xTaskMainBrightnessHandle);
  }

  BLYNK_WRITE(V4) {  // Left light brightness (slider)
    int pinValue = param.asInt();
    leftLightBrightness = pinValue;  // Model
    ledcWrite(leftLightPwmChannel, percentToValue(pinValue, 1023));
    Blynk.virtualWrite(V8, pinValue);
  }

  BLYNK_WRITE(V6) {  // Right light brightness (slider)
    int pinValue = param.asInt();
    rightLightBrightness = pinValue;  // Model
    ledcWrite(rightLightPwmChannel, percentToValue(pinValue, 1023));
    Blynk.virtualWrite(V9, pinValue);
  }

  // Brightness Steppers --------------------------------------------------

  BLYNK_WRITE(V7) {  // Both lights brightness (stepper)
    int pinValue = param.asInt();
    leftLightBrightness = pinValue;
    rightLightBrightness = pinValue;
    ledcWrite(leftLightPwmChannel, percentToValue(pinValue, 1023));
    ledcWrite(rightLightPwmChannel, percentToValue(pinValue, 1023));

    Blynk.virtualWrite(V2, pinValue);
    Blynk.virtualWrite(V4, pinValue);
    Blynk.virtualWrite(V6, pinValue);
    Blynk.virtualWrite(V8, pinValue);
    Blynk.virtualWrite(V9, pinValue);
  }

  BLYNK_WRITE(V8) {  // Left light brightness (stepper)
    int pinValue = param.asInt();
    leftLightState = pinValue;
    ledcWrite(leftLightPwmChannel, percentToValue(pinValue, 1023));
    Blynk.virtualWrite(V4, pinValue);
  }

  BLYNK_WRITE(V9) {  // Right light brightness (stepper)
    int pinValue = param.asInt();
    rightLightState = pinValue;
    ledcWrite(rightLightPwmChannel, percentToValue(pinValue, 1023));
    Blynk.virtualWrite(V6, pinValue);
  }

  // General functions

  void WaitForWifi(int cycleDelayInMilliSeconds) {
    while (WiFi.status() != WL_CONNECTED) {
      delay(cycleDelayInMilliSeconds);
    }
  }

  void WaitForBlynk(int cycleDelayInMilliSeconds) {
    while (!Blynk.connected()) {
      delay(cycleDelayInMilliSeconds);
    }
  }

  void ConnectToWifi(const char* ssid, const char* pass) {
    Serial.printf("Connecting to Wifi: %s\n", ssid);
    try {
      // WiFi.begin(ssid, pass);
      WiFi.disconnect();
      WiFi.begin(ssid, pass);
      WaitForWifi(1000);
    } catch (const std::exception& e) {
      Serial.printf("Error occured: %s\n", e.what());
    }
  }

  void SetupGpio(unsigned short int leftLightEnablePin, unsigned short int rightLightEnablePin, unsigned short int leftLightPwmPin, unsigned short int rightLightPwmPin,
                 unsigned short int leftLightPwmChannel, unsigned short int rightLightPwmChannel, unsigned short int lightsPwmFrequency, unsigned short int lightsPwmResolution) {
    // GPIO Setup
    ledcSetup(leftLightPwmChannel, lightsPwmFrequency, lightsPwmResolution);
    ledcSetup(rightLightPwmChannel, lightsPwmFrequency, lightsPwmResolution);
    ledcAttachPin(leftLightPwmPin, leftLightPwmChannel);
    ledcAttachPin(rightLightPwmPin, rightLightPwmChannel);
    pinMode(leftLightEnablePin, OUTPUT);
    pinMode(rightLightEnablePin, OUTPUT);
  }

  void setInitialStateOfLights() {
    // Turn light on initially with 50% brightness
    ledcWrite(leftLightPwmChannel, percentToValue(50, 1023));
    ledcWrite(rightLightPwmChannel, percentToValue(50, 1023));
    digitalWrite(rightLightEnable, HIGH);
    digitalWrite(leftLightEnable, HIGH);
  }

  int percentToValue(int percent, int maxValue) { return 0 <= percent <= 100 ? round((maxValue / 100) * percent) : 1023; }
