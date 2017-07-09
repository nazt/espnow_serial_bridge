#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <Task.h>
extern "C" {
#include <espnow.h>
#include <user_interface.h>
}
#define rxPin 14
#define txPin 12

TaskManager taskManager;
void OnWriteSerialTask(uint32_t deltaTime);
FunctionTask taskSerialWrite(OnWriteSerialTask, MsToTaskTime(10*1000)); // turn off the led in 600ms

SoftwareSerial swSerial(rxPin, txPin);
bool swSerialLock = false;

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
uint8_t buff[64];

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

u8 checksum(u8* arr, uint8_t len) {
  byte sum = 0;
  for (size_t i = 0; i < len; i++) {
    Serial.printf("%02x ", arr[i]);
    if (i == 2 || i == 5 || i == 11 || i == 15 || i == 19 || i == 23 || i == 27) {
      Serial.println();
    }
    sum ^= arr[i];
  }
  Serial.printf("last byte = %lu, checksum = %lu\r\n", arr[len-1], sum);
  return sum;
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

  wifi_get_macaddr(STATION_IF, self_sta_master_macaddr);
  CMMC_DEBUG_PRINT("[master] address (STATION_IF): ");
  printMacAddress(self_sta_master_macaddr);

  wifi_get_macaddr(SOFTAP_IF, self_ap_slave_macaddr);
  CMMC_DEBUG_PRINT("[slave] address (SOFTAP_IF): ");
  printMacAddress(self_ap_slave_macaddr);

  if (esp_now_init() == 0) {
    CMMC_DEBUG_PRINTLN("direct link  init ok");
  } else {
    CMMC_DEBUG_PRINTLN("dl init failed");
    ESP.restart();
    return;
  }

  bzero(buff, sizeof(buff));
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_recv_cb([](uint8_t *client_mac_addr, uint8_t *data, uint8_t len) {
    uint8_t *client_slave_macaddr = client_mac_addr;
    Serial.println("MASTER: ");
    printMacAddress(self_sta_master_macaddr);
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
    Serial.println("recv_cb... with payload: ");
    for (size_t i = 0; i < len; i++) {
      Serial.printf("%02x", data[i]);
    }


    Serial.println();
    printMacAddress(client_mac_addr);

    // print data payload and calculate checksum
    u8 sum = checksum(data, len);

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

    Serial.printf("recv payload len = %d, macaddr len = %d \r\n", len, sizeof(self_sta_master_macaddr));
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

    // const uint8_t CHECKSUM_IDX = 2+
    // add checksum
    buff[len+2+6+6] = sum2;

    Serial.println("payload....");
    for (size_t i = 1; i <= len+2+6+6+1; i++) {
      Serial.printf("%02x ", buff[i-1]);
      if (i == 2 || i == 6+2 || i == 6+2+6 || i == 6+2+6+1) {
        Serial.println();
      }
    }

    Serial.printf("\r\n===============\r\n");
    Serial.printf("checksum = %02x \r\n", sum2);
    swSerialLock = true;
    // len + start + mac1 + mac2 + sum
    swSerial.write(buff, len+2+6+6+1);
    swSerial.write('\r');
    swSerial.write('\n');
    swSerial.flush();
    swSerialLock = false;

    Serial.println();
    Serial.printf("free heap = %lu \r\n", ESP.getFreeHeap());
    Serial.printf("peer is exist status = %d \r\n",
                        esp_now_is_peer_exist(client_mac_addr));
    if (esp_now_is_peer_exist(client_mac_addr) == 0) {
      Serial.printf("peer is not exists... \r\n");
      int add_peer_status = esp_now_add_peer(client_mac_addr,
                            ESP_NOW_ROLE_SLAVE, WIFI_DEFAULT_CHANNEL, NULL, 0);
      if (add_peer_status  == 0) {
        Serial.println("add_peer_status is ok!");
        // fetch peer
        u8* peer = esp_now_fetch_peer(true);
        uint8_t d[1] = {0x0a};
        esp_now_send(peer, d, 1);
        esp_now_del_peer(peer);
      }
      else {
        Serial.println("add_peer_status is failed!");
      }
    }
    else {
      Serial.printf("peer is exists in db... \r\n");
    }
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

  taskManager.StartTask(&taskSerialWrite); // start with turning it on
}


void loop() {
  taskManager.Loop();
  while(swSerial.available()) {
    // Serial.write(swSerial.read());
  }
}

void OnWriteSerialTask(uint32_t deltaTime)
{
  static u8 registerMessageBuffer[50] = {
    0xff, 0xfa, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff,
    0x0d, 0x0a
  };


  // being prepared
  memcpy(registerMessageBuffer+2+4, self_ap_slave_macaddr, 6);
  memcpy(registerMessageBuffer+2+4+6, self_sta_master_macaddr, 6);
  u8 sum = checksum(registerMessageBuffer, 18);
  registerMessageBuffer[18] = sum;
  while (swSerialLock) {
    Serial.printf("swSerial is locked waiting... \r\n");;;;
    delay(10);
  }
  uint8_t wroteBytes = swSerial.write(registerMessageBuffer, 21);
  Serial.printf("wrote %d \r\n", wroteBytes);
  swSerial.flush();
}
