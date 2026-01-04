#include "blinds_433.h"
#include "config.h"
#include "util.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>

Blinds433 blinds(0xF1331);

void setup() {
  Serial.begin(9600);
  init_blinds_pin();

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