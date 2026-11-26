#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "secrets.h" // Remove this part and just insert your own SSID and corresponding password

// WiFi Settings
const char* ssid = "your-network"; 
const char* password = "your-password";


// Rotary Encoder KY-040 pins:
int pinCLK = 14;
int pinDT = 27;
int pinSW = 26;

int currentClockState = 0;
int lastClockState;

// Debounce Code:
bool lastRawSWState = HIGH;
bool stableSWState = HIGH;
unsigned long lastSWDebounce = 0;
const unsigned long debounceDelay = 50; //ms

// MOSFET pins (Directly attached to LEDs):
int pinWW = 2;
int pinCW = 4;

// Manual Switch pins:
int pinSwitch = 25;

// PIR settings:
int pinPIR = 15;
unsigned long lastMotionTime = 0;
const unsigned long motionHoldDuration = 30000; // 30s, tune to taste

// Dimming Capabilities:
const int pwmFreq = 5000; // Hz - high enought that switching itself is invinsible
const int pwmResolution = 8; // 8-bit -> Duty values range 0-255

// LED default settings:
int brightness = 128; // 0-255
int colorTemp = 128; // 0 = warmest (full WW), 255 = coolest (full CW)
bool adjustingColorTemp = false; // false = encoder controls brightness, true = colorTemp
const int adjustStepSize = 12; // duty units per detent - bigger than the old hold-to-repeat buttons, since each click is on discrete event

void setup() {
  pinMode(pinCLK, INPUT_PULLUP);
  pinMode(pinDT, INPUT_PULLUP);
  pinMode(pinSW, INPUT_PULLUP);
  pinMode(pinSwitch, INPUT_PULLUP);
  ledcAttach(pinWW, pwmFreq, pwmResolution); // ledcAttach configures the pin to be driven by ESP32's dedicated PWM hardware peripheral (LEDC)
  ledcAttach(pinCW, pwmFreq, pwmResolution);
  pinMode(pinPIR, INPUT_PULLDOWN);
  Serial.begin(9600);
  lastClockState = digitalRead(pinCLK); // Captures CLK's resting state — HIGH by design, since KY-040 detents rest with both contacts open

  WiFi.begin(ssid, password);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000)
    delay(250);


  if (WiFi.status() == WL_CONNECTED) {
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setHostname("desk-light");
  ArduinoOTA.begin(); // sets up the listener for incoming OTA flashes
  }
}

void handleSWButton() { //debounce code
  bool reading = digitalRead(pinSW);

  // Any raw change (even bounce) retsrats the debounce timer
  if (reading != lastRawSWState) {
    lastSWDebounce = millis(); // returns the amount of milliseconds that has elapsed since ESP32 began running
  }

  // Only accept the reading as "real" once it's been stable for longer than debounce delay (The check for stability of the switch)
  if ((millis() - lastSWDebounce) > debounceDelay) {
    if (reading != stableSWState) {
      stableSWState = reading;
      if (stableSWState == LOW) {
        adjustingColorTemp = !adjustingColorTemp; // Flips the bool value
      }
    }
  }

  lastRawSWState = reading;
}

// The best way to understand it is by visualising how the KY-040 mechanism  works. Have the states of CLK and DT in mind, take a picture, and run the command in your head multiple times
void loop() {
  ArduinoOTA.handle(); // Checks for an incoming OTA update, non-blocking - same pattern as your other loop() checks

  handleSWButton();

  bool switchOn = (digitalRead(pinSwitch) == HIGH); // Logic: digitalRead(pinSwitch) is either True or False. '== HIGH' Checks whether it is HIGH. If HIGH, then True
  bool motionDetected = (millis() - lastMotionTime) < motionHoldDuration;
  bool lightsOn = switchOn || motionDetected;
  if (digitalRead(pinPIR) == HIGH) {
    lastMotionTime = millis();
  }

  if (lightsOn) {
    int cwDuty = (brightness * colorTemp) / 255;         // The math is essentially changing the ratios of CW to WW for changing temperature and the total duty value of
    int wwDuty = (brightness * (255 - colorTemp)) / 255; // value of an LED (Either CW or WW) for changing brightness
    ledcWrite(pinCW, cwDuty);
    ledcWrite(pinWW, wwDuty);
  } else {
    ledcWrite(pinCW, 0);
    ledcWrite(pinWW, 0);
  }
 
  int exactClockState = digitalRead(pinCLK);
  if (exactClockState != lastClockState && exactClockState == LOW) {
    if (digitalRead(pinDT) != exactClockState) { // If DT pin is still HIGH when CLK is low, Clockwise
      if (adjustingColorTemp) colorTemp = min(255, colorTemp + adjustStepSize); // recall adjustStepSize = 12, adjustingColorTemp = False,
      else                    brightness = min(255, brightness + adjustStepSize); // recall int brightness = 128, int colorTemp = 128
    } else { // The Outer else for the if-else statement where it is still deciding whether it is going Clockwise (Increasing) or Anti-Clockwise (Decreasing)
      if (adjustingColorTemp) colorTemp = max(0, colorTemp - adjustStepSize); // min() / max() are just clamps -> min(255, x) stops it from increasing past 255
      else                    brightness = max(0, brightness - adjustStepSize); // -> max(0, x) stops it from decreasing below 0
    } // Note, for "brightness = max(0, brightness - adjustStepSize)", the brightness value is minimally 0, and hence in the math, no matter how much you turn, it would be minimally 0
    Serial.printf("brightness:%d colorTemp:%d mode:%s switch:%s pir:%d\n", brightness, colorTemp, adjustingColorTemp ? "colorTemp" : "brightness", switchOn ? "on" : "off", digitalRead(pinPIR));
  }
  lastClockState = exactClockState; 
}