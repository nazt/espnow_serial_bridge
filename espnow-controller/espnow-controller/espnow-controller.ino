#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include "datatypes.h"
extern "C" {
  #include <espnow.h>
  #include <user_interface.h>
}
bool ledState = LOW;
Ticker ticker;

#define BUTTON_PIN 0

uint32_t freqCounter = 0;;

void setup() {
  Serial.begin(230400);
  delay(10);
  Serial.flush();
  WiFi.disconnect();
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.print("Initializing... Controller..");
  WiFi.mode(WIFI_STA);
  uint8_t macaddr[6];

  wifi_get_macaddr(STATION_IF, macaddr);
  Serial.print("[master] address (STATION_IF): ");
  printMacAddress(macaddr);
  Serial.println();

  wifi_get_macaddr(SOFTAP_IF, macaddr);
  Serial.print("[slave] address (SOFTAP_IF): ");
  printMacAddress(macaddr);
  Serial.println();

  ticker.attach_ms(1000, []() {
    Serial.printf("%d/s\r\n", freqCounter);
    freqCounter = 0;
  });

  if (esp_now_init() == 0) {
    Serial.print("direct link  init ok");
  } else {
    Serial.print("dl init failed");
    ESP.restart();
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_recv_cb([](uint8_t *macaddr, uint8_t *data, uint8_t len) {
    freqCounter++;
    if (digitalRead(BUTTON_PIN) == LOW) {
      // PACKET_T pkt;
      // memcpy(&pkt, data, sizeof(pkt));
      // SENSOR_T sensor_data = pkt.data;
      Serial.write(data, len);
    }
    digitalWrite(LED_BUILTIN, ledState);
    ledState = !ledState;
  });
}

void loop() {
  yield();
}
