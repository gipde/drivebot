#include <Arduino.h>

#include "ThingSpeak.h"
#include <ArduinoOTA.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <OneWire.h>
#include <WiFiUdp.h>

// Stromverbrauch im DeepSleep ESP-01   339uA
// Stromverbrauch im DeepSleep ESP-12F   17uA

//#define DEBUG

// shift register
#define dataPin 4
#define clockPin 5
#define latchPin 2

// to get VCC
ADC_MODE(ADC_VCC);

// WLAN Data
const char ssid[] = "nkmhh";
const char pass[] = "Dremat-BlurEgef7";
const char *host = "OTA-THING";
WiFiClient client;

// Pass our oneWire reference to Dallas Temperature.
OneWire oneWire(14);
DallasTemperature sensors(&oneWire);

// Thingspeak Data
unsigned long myChannelNumber = 152224;
const char *myWriteAPIKey = "H6W63PGLYEOFNY4A";

void shiftWrite(int whichPin, int whichState) {
  byte bitsToSend = 0;
  digitalWrite(latchPin, LOW);
  bitWrite(bitsToSend, whichPin, whichState);
  shiftOut(dataPin, clockPin, MSBFIRST, bitsToSend);
  digitalWrite(latchPin, HIGH);
}

void blinkLed(int ms) {
  shiftWrite(0, HIGH);
  delay(ms / 2);
  shiftWrite(0, LOW);
  delay(ms / 2);
}

// Time to sleep (in seconds):
const int sleepTimeS = 15 * 60;

void setup() {
#ifdef DEBUG
  Serial.begin(74880);
  Serial.println("Booting...");
#endif

  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  pinMode(12, OUTPUT);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

#ifdef DEBUG
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    WiFi.begin(ssid, pass);
    blinkLed(80);
    Serial.println("Retrying connection...");
  }
#else
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    ESP.deepSleep(sleepTimeS * 1000000);
  }
#endif

  ThingSpeak.begin(client);

  ArduinoOTA.setHostname(host);
  ArduinoOTA.onError([](ota_error_t error) { ESP.restart(); });
  ArduinoOTA.begin();
}

float readTemp() {
  sensors.requestTemperatures();
  float r = sensors.getTempCByIndex(0);
  if (r > 60) {
#ifdef DEBUG
    Serial.println("Invalid Data on last read");
#endif
    delay(1000);
    r = sensors.getTempCByIndex(0);
  }
#ifdef DEBUG
  Serial.print("Temperature for the device 1 (index 0) is: ");
  Serial.println(r);
#endif
  return r;
}

void loop() {

  Serial.println("Starting...");

#ifdef DEBUG
  ArduinoOTA.handle();
#endif

  blinkLed(500);
  blinkLed(200);
  ThingSpeak.writeField(myChannelNumber, 1, readTemp(), myWriteAPIKey);
  ThingSpeak.writeField(myChannelNumber, 1, ESP.getVcc(), "11DEC49KEN7WNCG0");
#ifdef DEBUG
  Serial.println("We sleep for 10 s");
  ESP.deepSleep(10 * 1000000);
#else
  ESP.deepSleep(sleepTimeS * 1000000);
#endif
}
