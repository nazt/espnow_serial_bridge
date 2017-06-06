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
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Ultrasonic.h>

ADC_MODE(ADC_VCC);

int trigpin = 4;//appoint trigger pin
int echopin = 5;//appoint echo pin

#define MESSAGE_SIZE 30
uint8_t message[MESSAGE_SIZE] = {0};

#define MODE_WEBSERVER 1
#define MODE_ESPNOW 2

int runMode = MODE_ESPNOW;

Ultrasonic ultrasonic(trigpin,echopin);

#define DHTPIN            12
uint32_t delayMS;
// Uncomment the type of sensor in use:
#define DHTTYPE           DHT22     // DHT 22 (AM2302)

// See guide for details on sensor wiring and usage:
//   https://learn.adafruit.com/dht/overview

DHT_Unified dht(DHTPIN, DHTTYPE);

const char* ssid = "belkin.636";
const char* password = "3eb7e66b";
const char * hostName = "esp-async";
const char* http_username = "admin";
const char* http_password = "admin";

// ESPNOW
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
bool espnowRetryFlag = false;
uint8_t espnowRetries = 1;

#define MAX_ESPNOW_RETRIES 30
#define DEEP_SLEEP_S 5
#define ESPNOW_RETRY_DELAY 30
void initUserEspNow() {
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
    esp_now_register_recv_cb([](uint8_t * macaddr, uint8_t * data, uint8_t len) {
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

    esp_now_register_send_cb([](uint8_t * macaddr, uint8_t status) {
        CMMC_DEBUG_PRINT(millis());
        CMMC_DEBUG_PRINT("send to mac addr: ");
        printMacAddress(macaddr);
        if (status == 0) {
            espnowRetryFlag = false;
            send_ok_counter++;
            counter++;
            CMMC_DEBUG_PRINTF("... send_cb OK. [%lu/%lu]\r\n", send_ok_counter, send_ok_counter + send_fail_counter);
            digitalWrite(LED_BUILTIN, HIGH);
        } else {
            espnowRetryFlag = true;
            send_fail_counter++;
            CMMC_DEBUG_PRINTF("... send_cb FAILED. [%lu/%lu]\r\n", send_ok_counter, send_ok_counter + send_fail_counter);
        }
    });
};

void initUserSensor() {
    // Initialize device.
    dht.begin();
    // Print temperature sensor details.
    sensor_t sensor;
    dht.temperature().getSensor(&sensor);
    dht.humidity().getSensor(&sensor);
    delayMS = sensor.min_delay / 1000;
}

void initGpio() {
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(13, INPUT_PULLUP);
    digitalWrite(LED_BUILTIN, HIGH);
    blinker = new CMMC_Blink;
    blinker->init();
}

void printAndStoreEspNowMacInfo() {
    Serial.println("====================");
    Serial.println("    MODE = ESPNOW   ");
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
}
void startModeConfig() {
    Serial.println("====================");
    Serial.println("   MODE = CONFIG    ");
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
}

void checkBootMode() {
    Serial.println("Wating configuration pin..");
    _wait_config_signal(13,&longpressed);
    Serial.println("...Done");
    if (longpressed) {
        runMode = MODE_WEBSERVER;
        startModeConfig();
        setupWebServer();
    } else {
        printAndStoreEspNowMacInfo();
    }
}

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    SPIFFS.begin();
    WiFi.disconnect();
    delay(100);

    initGpio();
    initUserSensor();
    initUserEspNow();
    setupOTA();
    loadConfig(master_mac);
    // initBattery();
}

void goSleep() {
    Serial.printf("Go sleep for .. %lu seconds. \r\n", DEEP_SLEEP_S);
    ESP.deepSleep(1000000 * DEEP_SLEEP_S);
}

void sendDataToMaster(uint8_t * message_ptr, size_t msg_size) {
    // transmit
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    esp_now_send(master_mac, message_ptr, msg_size);
    delay(ESPNOW_RETRY_DELAY);

    // retransmitt when failed
    while (espnowRetryFlag) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        esp_now_send(master_mac, message_ptr, msg_size);
        espnowRetries = espnowRetries + 1;
        delay(ESPNOW_RETRY_DELAY);
        // sleep after reach max retries.
        if (espnowRetries > MAX_ESPNOW_RETRIES) {
            goSleep();
        }
    };
}

bool readDHTSensor(uint32_t* temp, uint32_t* humid) {
    digitalWrite(LED_BUILTIN, LOW);
    // Delay between measurements.
    delay(delayMS);
    // Get temperature event and print its value.
    sensors_event_t event;
    dht.temperature().getEvent(&event);

    if (isnan(event.temperature)) {
        Serial.println("Error reading temperature!");
    } else {
        Serial.print("Temperature: ");
        Serial.print(event.temperature);
        Serial.println(" *C");
    }
    *temp = (uint32_t)(event.temperature * 100);

    // Get humidity event and print its value.
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
        Serial.println("Error reading humidity!");
    } else {
        Serial.print("Humidity: ");
        Serial.print(event.relative_humidity);
        Serial.println("%");
    }

    *humid = (uint32_t)(event.relative_humidity * 100);
}

void addDataField(uint8_t *message, uint32_t field1, uint32_t field2, uint32_t field3) {
    uint32_t battery = ESP.getVcc();
    message[0] = 0xff;
    message[1] = 0xfa;

    message[2] = 0x01;
    message[3] = 0x01;
    message[4] = 0x03;

    memcpy(message + 11, (const void * )&field1, 4);
    memcpy(message + 15, (const void * )&field2, 4);
    memcpy(message + 19, (const void * )&field3, 4);
    memcpy(message + 23, (const void * )&battery, 4);

    byte sum = 0;
    for (size_t i = 0; i < sizeof(message) - 1; i++) {
        sum ^= message[i];
    }
    message[MESSAGE_SIZE - 1] = sum;
    Serial.printf("temp: %02x - %lu\r\n", field1, field1);
    Serial.printf("humid: %02x - %lu\r\n", field2, field2);
    Serial.printf("distance:  %d \r\n", field3);
    Serial.printf("batt: %d \r\n", battery);
}

void loop() {
    bzero(message, sizeof(message));
    ArduinoOTA.handle();
    if (runMode == MODE_WEBSERVER) {
      return;
    } else {
      uint32_t temperature_uint32 = 0;
      uint32_t humidity_uint32 = 0;
       //this result unit is centimeter
      uint32_t cmdistance = ultrasonic.distanceRead();
      readDHTSensor(&temperature_uint32, &humidity_uint32);
      // UUID
      message[5] = 'n';
      message[6] = 'a';
      message[7] = 't';
      message[8] = '0';
      message[9] = '0';
      message[10] = '2';
      addDataField(message, temperature_uint32, humidity_uint32, cmdistance);
      sendDataToMaster(message, sizeof(message));
      goSleep();
    }
}
