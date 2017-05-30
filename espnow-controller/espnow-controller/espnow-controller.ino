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
#define DEBUG_SERIAL 1
#if DEBUG_SERIAL
#define DEBUG_PRINTER Serial
#define DEBUG_PRINT(...) { DEBUG_PRINTER.print(__VA_ARGS__); }
#define DEBUG_PRINTLN(...) { DEBUG_PRINTER.println(__VA_ARGS__); }
#define DEBUG_PRINTF(...) { DEBUG_PRINTER.printf(__VA_ARGS__); }
#else
#define DEBUG_PRINT(...) { }
#define DEBUG_PRINTLN(...) { }
#define DEBUG_PRINTF(...) { }
#endif

bool ledState = LOW;
Ticker ticker;
uint8_t self_ap_slave_macaddr[6];
uint8_t self_sta_master_macaddr[6];

uint8_t buff[50];

// SOFTAP_IF
void printMacAddress(uint8_t* macaddr) {
  DEBUG_PRINT("{");
  for (int i = 0; i < 6; i++) {
    DEBUG_PRINT("0x");
    DEBUG_PRINT(macaddr[i], HEX);
    if (i < 5) DEBUG_PRINT(',');
  }
  DEBUG_PRINTLN("};");
}

void setup() {
  WiFi.disconnect();
  pinMode(LED_BUILTIN, OUTPUT);
#if DEBUG_SERIAL
  Serial.begin(115200);
#endif

  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  swSerial.begin(9600);

  DEBUG_PRINTLN("Initializing... Controller..");
  WiFi.mode(WIFI_STA);
  uint8_t macaddr[6];

  wifi_get_macaddr(STATION_IF, macaddr);
  DEBUG_PRINT("[master] address (STATION_IF): ");
  memcpy(self_sta_master_macaddr, macaddr, 6);
  printMacAddress(macaddr);

  wifi_get_macaddr(SOFTAP_IF, macaddr);
  DEBUG_PRINT("[slave] address (SOFTAP_IF): ");
  printMacAddress(macaddr);
  memcpy(self_ap_slave_macaddr, macaddr, 6);

  if (esp_now_init() == 0) {
    DEBUG_PRINTLN("direct link  init ok");
  } else {
    DEBUG_PRINTLN("dl init failed");
    ESP.restart();
    return;
  }

  swSerial.println("ESPNOW SEND HELLO");
  swSerial.write("HELLO", 5);

  bzero(buff, sizeof(buff));


  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_recv_cb([](uint8_t *macaddr, uint8_t *data, uint8_t len) {
    Serial.println("RECEIVE... ");
    for (size_t i = 0; i < len; i++) {
      Serial.print(data[i], HEX);
    }
    Serial.println();
    printMacAddress(macaddr);
    digitalWrite(LED_BUILTIN, ledState);
    ledState = !ledState;

    uint8_t *client_slave_macaddr = macaddr;

    byte sum = 0;
    for (size_t i = 0; i < len; i++) {
      if (i == 2 || i == 5 || i == 11 || i == 15 || i == 19 || i == 23 || i == 27) {
        Serial.println();
      }

      Serial.printf("%02x ", data[i]);
      if (i == len - 1) {
        Serial.println();
        Serial.printf("calculated sum = %02x \r\n", sum);
        Serial.printf(".....check sum = %02x \r\n", data[i]);
      }
      sum ^= data[i];
    }

    Serial.println();
    uint32_t u32_temp;
    uint32_t u32_humid;
    uint32_t u32_batt;
    u32_temp  = (data[14] << 24) | (data[13] << 16) | (data[12] << 8) | (data[11]);
    u32_humid = (data[18] << 24) | (data[17] << 16) | (data[16] << 8) | (data[15]);
    u32_batt  = (data[22] << 24) | (data[21] << 16) | (data[20] << 8) | (data[19]);

    Serial.printf("temp: %lu \r\n", u32_temp);
    Serial.printf("humid: %lu \r\n", u32_humid);
    Serial.printf("batt: %lu \r\n", u32_batt);

    Serial.printf("recv len = %d, macaddr len = %d \r\n", len, sizeof(macaddr));

    //char mac[32] = {0};
    //snprintf(mac, 32, "%02x:%02x:%02x:%02x:%02x:%02x", MAC2STR(macaddr));
    // Serial.printf("mac: %s\r\n", mac);
    // swSerial.write(mac, 32);

    buff[0] = 0xFC;
    buff[1] = 0xFD;
    printMacAddress(self_sta_master_macaddr);
    printMacAddress(client_slave_macaddr);

    memcpy(buff+2, self_sta_master_macaddr, 6);
    memcpy(buff+6+2, client_slave_macaddr, 6);
    Serial.printf("len = %d==%02x\r\n", len, len);
    buff[6+6+2] = len;
    memcpy(buff+6+2+6+1, data, len);

    byte sum2 = 0;
    for (size_t i = 1; i <= len+2+6+6; i++) {
      sum2 ^= buff[i-1];
    }
    buff[len+2+6+6] = sum2;

    Serial.println("===============");
    Serial.printf("CHECK SUM 2 = %02x \r\n", sum2);
    for (size_t i = 1; i <= len+2+6+6+1; i++) {
      Serial.printf("%02x ", buff[i-1]);
      if (i == 2 || i == 6+2 || i == 6+2+6) {
        Serial.println();
      }
    }

    Serial.println();
    Serial.printf("free heap = %lu \r\n", ESP.getFreeHeap());
    swSerial.write(buff, len+2+6+6+1);
    swSerial.write('\r');
    swSerial.write('\n');
  });

  esp_now_register_send_cb([](uint8_t* macaddr, uint8_t status) {
    // DEBUG_PRINT("send_cb to ");
    printMacAddress(macaddr);
    static uint32_t ok = 0;
    static uint32_t fail = 0;
    if (status == 0) {
      DEBUG_PRINTLN("ESPNOW: SEND_OK");
      ok++;
    }
    else {
      DEBUG_PRINTLN("ESPNOW: SEND_FAILED");
      fail++;
    }
    Serial.printf("[SUCCESS] = %lu/%lu \r\n", ok, ok + fail);
  });
  // DEBUG_PRINTF("ADD PEER: %d\r\n", add_peer_status);
}

void loop() {
  yield();
}
