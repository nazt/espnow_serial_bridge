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

void setup() {
  #if CMMC_DEBUG_SERIAL
    Serial.begin(115200);
    delay(10);
    Serial.flush();
  #endif

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


  Serial.println("MASTER: ");
  printMacAddress(self_sta_master_macaddr);

  swSerial.write(0xFC);
  swSerial.write(0xFA);
  swSerial.write(self_ap_slave_macaddr, 6);
  swSerial.write('\r');
  swSerial.write('\n');

  bzero(buff, sizeof(buff));
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_recv_cb([](uint8_t *client_mac_addr, uint8_t *data, uint8_t len) {
    uint8_t *client_slave_macaddr = client_mac_addr;
    ledState = !ledState;
    Serial.println("recv_cb... incoming mac: ");
    for (size_t i = 0; i < len; i++) {
      Serial.printf("%02x", data[i]);
    }
    Serial.println();
    printMacAddress(client_mac_addr);
    // printMacAddress(self_sta_master_macaddr);
    // printMacAddress(client_slave_macaddr);
    digitalWrite(LED_BUILTIN, ledState);

    // print data payload and calculate checksum
    byte sum = 0;
    for (size_t i = 0; i < len; i++) {
      Serial.printf("%02x ", data[i]);
      if (i == 2 || i == 5 || i == 11 || i == 15 || i == 19 || i == 23 || i == 27) {
        Serial.println();
      }
      sum ^= data[i];
    }

    Serial.println();
    Serial.println("==========================");
    Serial.printf("calculated sum = %02x \r\n", sum);
    Serial.printf(".....check sum = %02x \r\n", data[len-1]);
    if (sum != data[len-1]) {
      Serial.println("Invalid checksum.");
    }
    else {
      Serial.println("checksum ok!");
    }
    Serial.println("==========================");

    Serial.printf("recv payload len = %d, macaddr len = %d \r\n", len, sizeof(macaddr));
    Serial.printf("espnow payload len = %d, hex = %02x\r\n", len, len);

    // being prepared
    buff[0] = 0xfc;
    buff[1] = 0xfd;
    buff[6+6+2] = len;

    memcpy(buff+2, self_sta_master_macaddr, 6);
    memcpy(buff+6+2, client_slave_macaddr, 6);
    memcpy(buff+6+2+6+1, data, len);

    byte sum2 = 0;
    for (size_t i = 1; i <= len+2+6+6; i++) {
      sum2 ^= buff[i-1];
    }
    // add checksum
    buff[len+2+6+6] = sum2;

    for (size_t i = 1; i <= len+2+6+6+1; i++) {
      Serial.printf("%02x ", buff[i-1]);
      if (i == 2 || i == 6+2 || i == 6+2+6 || i == 6+2+6+1) {
        Serial.println();
      }
    }

    Serial.println();
    Serial.println("===============");
    Serial.printf("checksum = %02x \r\n", sum2);
    // len + start + mac1 + mac2 + sum
    swSerial.write(buff, len+2+6+6+1);
    swSerial.write('\r');
    swSerial.write('\n');

    Serial.println();
    Serial.printf("free heap = %lu \r\n", ESP.getFreeHeap());
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
  while(swSerial.available()) {
    // Serial.write(swSerial.read());
  }
}
