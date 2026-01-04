#include "blinds_433.h"
#include "config.h"
#include "util.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>

static constexpr u8 kDataPin = 4;
Blinds433 blinds(kDataPin, 0xF1331);

void setup() {
  Serial.begin(9600);
  pinMode(kDataPin, OUTPUT);

  WiFi.begin(kWifiSsid, kWifiPassword);
  while (!WiFi.isConnected()) {
    Serial.println("Connecting...");
    delay(500);
  }

  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  Serial.println("sending blinds down");
  blinds.all_down();
  delay(2000);
}