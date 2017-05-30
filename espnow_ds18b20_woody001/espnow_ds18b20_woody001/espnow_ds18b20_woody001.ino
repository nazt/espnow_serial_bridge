#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <Ticker.h>
#include <Hash.h>
#include <ArduinoOTA.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
#include <ArduinoJson.h>
#include <CMMC_Blink.hpp>

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

#include "ota.h"
#include "doconfig.h"
#include "util.h"
extern "C" {
  #include <espnow.h>
  #include <user_interface.h>
}
#include <Ultrasonic.h>

int trigpin = 4;//appoint trigger pin
int echopin = 5;//appoint echo pin


#define MODE_WEBSERVER 1
#define MODE_ESPNOW 2

int runMode = MODE_ESPNOW;

Ultrasonic ultrasonic(trigpin,echopin);

// Uncomment the type of sensor in use:
// See guide for details on sensor wiring and usage:
//   https://learn.adafruit.com/dht/overview

const char* ssid = "belkin.636";
const char* password = "3eb7e66b";
const char * hostName = "esp-async";
const char* http_username = "admin";
const char* http_password = "admin";

uint8_t master_mac[6];
uint8_t slave_mac[6];

// SKETCH BEGIN
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");
CMMC_Blink *blinker;

uint32_t counter = 0;
uint32_t send_ok_counter = 0;
uint32_t send_fail_counter = 0;
bool must_send_data = 0;
Ticker ticker;
bool longpressed = false;
#include "webserver.h"

void setup(){
  pinMode(0, OUTPUT);
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  initBattery();
  SPIFFS.begin();
  WiFi.disconnect();
  delay(100);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(13, INPUT_PULLUP);
  digitalWrite(LED_BUILTIN, HIGH);
  blinker = new CMMC_Blink;
  blinker->init();

  Serial.println("Wating configuration pin..");
  _wait_config_signal(13, &longpressed);
  Serial.println("...Done");
  if (longpressed) {
    runMode = MODE_WEBSERVER;
    Serial.println("====================");
    Serial.println("    MODE = CONFIG   ");
    Serial.println("====================");
    WiFi.hostname(hostName);
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(hostName);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.printf("STA: Failed!\n");
      WiFi.disconnect(false);
      delay(1000);
      WiFi.begin(ssid, password);
    }
    setupWebServer();
  }
  else {
    Serial.println("====================");
    Serial.println("   MODE = ESPNOW    ");
    Serial.println("====================");
    WiFi.disconnect();
    Serial.println("Initializing ESPNOW...");
    CMMC_DEBUG_PRINTLN("Initializing... SLAVE");
    WiFi.mode(WIFI_AP_STA);

    uint8_t macaddr[6];
    wifi_get_macaddr(STATION_IF, macaddr);
    CMMC_DEBUG_PRINT("[master] mac address (STATION_IF): ");
    printMacAddress(macaddr);

    wifi_get_macaddr(SOFTAP_IF, macaddr);
    CMMC_DEBUG_PRINTLN("[slave] mac address (SOFTAP_IF): ");
    printMacAddress(macaddr);
    memcpy(slave_mac, macaddr, 6);

    if (esp_now_init() == 0) {
      CMMC_DEBUG_PRINTLN("init");
    } else {
      CMMC_DEBUG_PRINTLN("init failed");
      ESP.restart();
      return;
    }
    CMMC_DEBUG_PRINTLN("SET ROLE SLAVE");
    esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
    static uint8_t recv_counter = 0;
    esp_now_register_recv_cb([](uint8_t *macaddr, uint8_t *data, uint8_t len) {
      recv_counter++;
      // CMMC_DEBUG_PRINTLN("recv_cb");
      // CMMC_DEBUG_PRINT("mac address: ");
      // printMacAddress(macaddr);
      CMMC_DEBUG_PRINT("data: ");
      for (int i = 0; i < len; i++) {
        CMMC_DEBUG_PRINT(" 0x");
        CMMC_DEBUG_PRINT(data[i], HEX);
      }

      CMMC_DEBUG_PRINT(data[0], DEC);
      CMMC_DEBUG_PRINTLN("");
      digitalWrite(LED_BUILTIN, data[0]);
    });

    esp_now_register_send_cb([](uint8_t* macaddr, uint8_t status) {
      CMMC_DEBUG_PRINT(millis());
      CMMC_DEBUG_PRINT("send to mac addr: ");
      printMacAddress(macaddr);
      if (status == 0) {
        send_ok_counter++;
        counter++;
        CMMC_DEBUG_PRINTF("... send_cb OK. [%lu/%lu]\r\n", send_ok_counter, send_ok_counter + send_fail_counter);
        digitalWrite(LED_BUILTIN, HIGH);
      }
      else {
        send_fail_counter++;
        CMMC_DEBUG_PRINTF("... send_cb FAILED. [%lu/%lu]\r\n", send_ok_counter, send_ok_counter + send_fail_counter);
      }
    });
  }
  setupOTA();
  loadConfig(master_mac);
  // ticker.attach_ms(500, [&]() {
  //   must_send_data = 1;
  // });
}

#define MESSAGE_SIZE 30
uint8_t message[MESSAGE_SIZE] = {0};

void loop(){
  bzero(message, sizeof(message));
  ArduinoOTA.handle();
  if (runMode == MODE_WEBSERVER) {
    return;
  }
  else {
    uint32_t temperature_uint32  = 0;
    uint32_t humidity_uint32 = 0;
  	uint32_t cmdistance = ultrasonic.distanceRead();
    pinMode(A0, INPUT);
    uint32_t battery = (analogRead(A0) * 5000) / 880;

    digitalWrite(LED_BUILTIN, LOW);
    message[0] = 0xff;
    message[1] = 0xfa;

    message[2] = 0x01;
    message[3] = 0x01;
    message[4] = 0x03;

    // NICK NAME
    message[5]  = 'd';
    message[6]  = 'h';
    message[7]  = 't';
    message[8]  = '0';
    message[9]  = '0';
    message[10] = '1';

    memcpy(message+11, (const void*)&temperature_uint32, 4);
    memcpy(message+15, (const void*)&humidity_uint32, 4);
    memcpy(message+19, (const void*)&cmdistance, 4);
    memcpy(message+23, (const void*)&battery, 4);

    byte sum = 0;
    for (size_t i = 0; i < sizeof(message)-1; i++) {
      sum ^= message[i];
    }
    message[MESSAGE_SIZE-1] = sum;

    Serial.printf("temp: %02x - %lu\r\n", temperature_uint32, temperature_uint32);
    Serial.printf("humid: %02x - %lu\r\n", humidity_uint32, humidity_uint32);
    Serial.printf("distance:  %d \r\n", cmdistance);
    Serial.printf("batt: %d \r\n", battery);

    // uint8_t master_mac2[] = {0x18,0xFE,0x34,0xEE,0xA0,0xF9};
    esp_now_send(master_mac, message, sizeof(message));
    digitalWrite(LED_BUILTIN, HIGH);

    ESP.deepSleep(1000000*120); // 10 sec
  }
}
