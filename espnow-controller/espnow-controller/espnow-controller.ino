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
uint8_t self_ap_slave_macaddr[6];
uint8_t self_sta_master_macaddr[6];

#define BUTTON_PIN 0

// uint8_t buff[50];

// SOFTAP_IF
void printMacAddress(uint8_t* macaddr) {
  Serial.print("{");
  for (int i = 0; i < 6; i++) {
    Serial.print("0x");
    Serial.print(macaddr[i], HEX);
    if (i < 5) Serial.print(',');
  }
  Serial.print("};");
}

uint32_t freqCounter = 0;;

void setup() {
  Serial.begin(230400);
  delay(10);
  Serial.flush();
  Serial.println("after ticker");
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
  // memcpy(self_sta_master_macaddr, macaddr, 6);
  // memcpy(self_ap_slave_macaddr, macaddr, 6);

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

  // // swSerial.println("ESPNOW SEND HELLO");
  // // swSerial.write("HELLO", 5);
  //
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_recv_cb([](uint8_t *macaddr, uint8_t *data, uint8_t len) {
    freqCounter++;
    // Serial.println("RECEIVE... ");

    // for (size_t i = 0; i < len; i++) {
    //   // Serial.print(data[i], HEX);
    //   Serial.printf("%02x ", data[i]);
    //   if ((i+1)%4==0) {
    //     Serial.println();
    //   }
    // }
    // Serial.println();
    // Serial.printf("type = %lu\r\n", pkt.type);
    // Serial.printf("battery = %lu\r\n", sensor_data.battery);
    // Serial.printf("field1 = %lu\r\n", sensor_data.field1);
    // Serial.printf("field2 = %lu\r\n", sensor_data.field2);
    // Serial.printf("field3 = %lu\r\n", sensor_data.field3);
    // Serial.printf("field4 = %lu\r\n", sensor_data.field4);
    // Serial.printf("myName= %s\r\n", sensor_data.myName);

    // printMacAddress(macaddr);
    if (digitalRead(BUTTON_PIN) == LOW) {
      // PACKET_T pkt;
      // memcpy(&pkt, data, sizeof(pkt));
      // SENSOR_T sensor_data = pkt.data;
      Serial.write(data, len);
    }

    digitalWrite(LED_BUILTIN, ledState);
    ledState = !ledState;
  });
  //
  esp_now_register_send_cb([](uint8_t* macaddr, uint8_t status) {
    // Serial.print("send_cb to ");
    printMacAddress(macaddr);
    static uint32_t ok = 0;
    static uint32_t fail = 0;
    if (status == 0) {
      Serial.print("ESPNOW: SEND_OK");
      ok++;
    }
    else {
      Serial.print("ESPNOW: SEND_FAILED");
      fail++;
    }
    // Serial.printf("[SUCCESS] = %lu/%lu \r\n", ok, ok + fail);
  });
  // DEBUG_PRINTF("ADD PEER: %d\r\n", add_peer_status);
}

void loop() {
  yield();
}
