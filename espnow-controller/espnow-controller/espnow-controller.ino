#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
extern "C" {
#include <espnow.h>
#include <user_interface.h>
}
#define rxPin 14
#define txPin 12



SoftwareSerial swSerial(rxPin, txPin);

#define WIFI_DEFAULT_CHANNEL 1
#define CMMC_DEBUG_SERIAL 1
#if CMMC_DEBUG_SERIAL
    #define CMMC_DEBUG_PRINTER Serial
    #define CMMC_DEBUG_PRINT(...) { CMMC_DEBUG_PRINTER.print(__VA_ARGS__); }
    #define CMMC_DEBUG_PRINTLN(...) { CMMC_DEBUG_PRINTER.println(__VA_ARGS__); }
    #define CMMC_DEBUG_PRINTF(...) { CMMC_DEBUG_PRINTER.printf(__VA_ARGS__); }
#else
    #define CMMC_DEBUG_PRINT(...) { }
    #define CMMC_DEBUG_PRINTLN(...) { }
    #define CMMC_DEBUG_PRINTF(...) { }
#endif

bool ledState = LOW;
Ticker ticker;
uint8_t self_ap_slave_macaddr[6];
uint8_t self_sta_master_macaddr[6];

uint8_t buff[50];

// SOFTAP_IF
void printMacAddress(uint8_t* macaddr) {
  CMMC_DEBUG_PRINT("{");
  for (int i = 0; i < 6; i++) {
    CMMC_DEBUG_PRINT("0x");
    CMMC_DEBUG_PRINT(macaddr[i], HEX);
    if (i < 5) CMMC_DEBUG_PRINT(',');
  }
  CMMC_DEBUG_PRINTLN("};");
}

uint32_t freqCounter = 0;;

void setup() {
  #if CMMC_DEBUG_SERIAL
    Serial.begin(115200);
    delay(10);
    Serial.flush();
  #endif

  ticker.attach_ms(1000, []() {
    Serial.printf("%d/s\r\n", freqCounter);
    freqCounter = 0;
  });

  WiFi.disconnect();
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  swSerial.begin(9600);

  CMMC_DEBUG_PRINTLN("Initializing... Controller..");
  WiFi.mode(WIFI_STA);
  uint8_t macaddr[6];

  wifi_get_macaddr(STATION_IF, macaddr);
  CMMC_DEBUG_PRINT("[master] address (STATION_IF): ");
  memcpy(self_sta_master_macaddr, macaddr, 6);
  printMacAddress(macaddr);

  wifi_get_macaddr(SOFTAP_IF, macaddr);
  CMMC_DEBUG_PRINT("[slave] address (SOFTAP_IF): ");
  printMacAddress(macaddr);
  memcpy(self_ap_slave_macaddr, macaddr, 6);

  if (esp_now_init() == 0) {
    CMMC_DEBUG_PRINTLN("direct link  init ok");
  } else {
    CMMC_DEBUG_PRINTLN("dl init failed");
    ESP.restart();
    return;
  }

  swSerial.println("ESPNOW SEND HELLO");
  swSerial.write("HELLO", 5);

  bzero(buff, sizeof(buff));


  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_recv_cb([](uint8_t *macaddr, uint8_t *data, uint8_t len) {
    freqCounter++;
    Serial.println("RECEIVE... ");
    for (size_t i = 0; i < len; i++) {
      // Serial.print(data[i], HEX);
      Serial.printf("%02x ", data[i]);
      if ((i+1)%4==0) {
        Serial.println();
      }
    }
    Serial.println();
    printMacAddress(macaddr);
    digitalWrite(LED_BUILTIN, ledState);
    ledState = !ledState;

    uint8_t *client_slave_macaddr = macaddr;
    Serial.println();
    swSerial.write('\r');
    swSerial.write('\n');
  });

  esp_now_register_send_cb([](uint8_t* macaddr, uint8_t status) {
    // CMMC_DEBUG_PRINT("send_cb to ");
    printMacAddress(macaddr);
    static uint32_t ok = 0;
    static uint32_t fail = 0;
    if (status == 0) {
      CMMC_DEBUG_PRINTLN("ESPNOW: SEND_OK");
      ok++;
    }
    else {
      CMMC_DEBUG_PRINTLN("ESPNOW: SEND_FAILED");
      fail++;
    }
    Serial.printf("[SUCCESS] = %lu/%lu \r\n", ok, ok + fail);
  });
  // DEBUG_PRINTF("ADD PEER: %d\r\n", add_peer_status);
}

void loop() {
  yield();
}
