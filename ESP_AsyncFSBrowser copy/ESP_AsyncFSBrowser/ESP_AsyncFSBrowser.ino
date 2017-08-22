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

#include "DHT.h"
#include <Ultrasonic.h>

ADC_MODE(ADC_VCC);

extern char deviceName[20];

#define MESSAGE_SIZE 48
uint8_t message[MESSAGE_SIZE] = {0};

#define MODE_WEBSERVER 1
#define MODE_ESPNOW    2

int runMode = MODE_ESPNOW;

#define DHTTYPE       DHT22  // DHT 22  (AM2302), AM2321
#define DHTPIN        12
uint32_t delayMS = 100;
DHT dht(12, DHTTYPE);

const char* ssid = "belkin.636";
const char* password = "3eb7e66b";
const char * hostName = "esp-async";
const char* http_username = "admin";
const char* http_password = "admin";

// ESPNOW
uint8_t master_mac[6];
uint8_t slave_mac[6];

uint32_t dataHasBeenSentAtMillis;

// SKETCH BEGIN
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");
CMMC_Blink *blinker;

uint32_t espNowSentSuccessCounter = 0;
uint32_t espNowSentFailedCounter = 0;
Ticker ticker;
bool longPressed = false;
#include "webserver.h"
bool espNowSentFlagBeChangedInCallback = false;
uint8_t espNowRetries = 1;

#define MAX_ESPNOW_RETRIES 30
uint32_t DEEP_SLEEP_S = 5;

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
        // digitalWrite(LED_BUILTIN, data[0]);
        CMMC_DEBUG_PRINT("data: ");
        for (int i = 0; i < len; i++) {
            CMMC_DEBUG_PRINT(" 0x");
            CMMC_DEBUG_PRINT(data[i], HEX);
        }
        Serial.printf("\r\n");
        goSleep(60*data[0]);
    });

    esp_now_register_send_cb([](uint8_t * macaddr, uint8_t status) {
        CMMC_DEBUG_PRINT(millis());
        CMMC_DEBUG_PRINT("send to mac addr: ");
        printMacAddress(macaddr);
        if (status == 0) {
            espNowSentFlagBeChangedInCallback = false;
            espNowSentSuccessCounter++;
            CMMC_DEBUG_PRINTF("... send_cb OK. [%lu/%lu]\r\n",
            espNowSentSuccessCounter,
            espNowSentSuccessCounter + espNowSentFailedCounter);
        } else {
            espNowSentFlagBeChangedInCallback = true;
            espNowSentFailedCounter++;

            CMMC_DEBUG_PRINTF("... send_cb FAILED. [%lu/%lu]\r\n",
              espNowSentSuccessCounter,
              espNowSentSuccessCounter + espNowSentFailedCounter);
        }
    });
};

void initUserSensor() {
    // Initialize device.
    Serial.println("Initializing dht.");
    dht.begin();
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.print(" *C ");
}

void initGpio() {
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(13, INPUT_PULLUP);
    digitalWrite(LED_BUILTIN, HIGH);
    blinker = new CMMC_Blink;
    blinker->init();
}

void printAndStoreEspNowMacInfo() {
    uint8_t macaddr[6];
    Serial.println("====================");
    Serial.println("    MODE = ESPNOW   ");
    Serial.println("====================");
    WiFi.disconnect();
    Serial.println("Initializing ESPNOW...");
    CMMC_DEBUG_PRINTLN("Initializing... SLAVE");
    WiFi.mode(WIFI_AP_STA);
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
    _wait_config_signal(13,&longPressed);
    Serial.println("...Done");
    if (longPressed) {
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

void goSleep(uint32_t deepSleepS) {
    Serial.printf("Go sleep for .. %lu seconds. \r\n", deepSleepS);
    ESP.deepSleep(1000000 * deepSleepS);
}

void sendDataToMaster(uint8_t * message_ptr, size_t msg_size) {
    // transmit
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    esp_now_send(master_mac, message_ptr, msg_size);
    delay(10);

    // retransmitt when failed
    while (espNowSentFlagBeChangedInCallback) {
        delay(ESPNOW_RETRY_DELAY);
        // digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        esp_now_send(master_mac, message_ptr, msg_size);
        espNowRetries = espNowRetries + 1;
        // sleep after reach max retries.
        if (espNowRetries > MAX_ESPNOW_RETRIES) {
            goSleep(DEEP_SLEEP_S);
        }
    };
}

bool readDHTSensor(uint32_t* temp, uint32_t* humid) {
  *temp = (uint32_t) dht.readTemperature()*100;
  *humid = (uint32_t) dht.readHumidity()*100;
}

void addDataField(uint8_t *message, uint32_t field1, uint32_t field2, uint32_t field3) {
    uint32_t battery = ESP.getVcc();
    message[0] = 0xff;
    message[1] = 0xfa;

    message[2] = 0x01;
    message[3] = 0x02;
    message[4] = 0x03;
    message[5] = 0x01;

    memcpy(&message[6], (const void *)&field1, 4);
    memcpy(&message[10], (const void *)&field2, 4);
    memcpy(&message[14], (const void *)&field3, 4);
    memcpy(&message[18], (const void *)&battery, 4);
    message[22] = strlen(deviceName);
    memcpy(&message[23], deviceName, strlen(deviceName));

    Serial.println("==========================");
    Serial.println("       print data         ");
    Serial.println("==========================");
    byte sum = 0;
    for (size_t i = 0; i < MESSAGE_SIZE-1; i++) {
      Serial.printf("%02x", message[i]);
      sum ^= message[i];
      if ((i+1) % 4 == 0) {
        Serial.println();
      }
    }

    message[MESSAGE_SIZE-1] = sum;

    Serial.println();
    Serial.println("==========================");
    Serial.printf("calculated sum = %02x \r\n", sum);
    Serial.printf(".....check sum = %02x \r\n", message[MESSAGE_SIZE-1]);

    Serial.printf("field1: %02x - %lu\r\n", field1, field1);
    Serial.printf("field2: %02x - %lu\r\n", field2, field2);
    Serial.printf("field3:  %d \r\n", field3);
    Serial.printf("batt: %d \r\n", battery);
}


void loop() {
    bzero(message, sizeof(message));
    ArduinoOTA.handle();
    if (runMode == MODE_WEBSERVER) {
      return;
    } else {
      uint32_t temperature_uint32;
      uint32_t humidity_uint32;
      uint32_t cmdistance = 999; // in cm
      // write reference
      readDHTSensor(&temperature_uint32, &humidity_uint32);
      addDataField(message, temperature_uint32, humidity_uint32, cmdistance);

      sendDataToMaster(message, sizeof(message));

      // wait for a command message.
      dataHasBeenSentAtMillis = millis();
      while(true) {
        delay(1000);
        Serial.println("Waiting a command message...");
        if (millis() > (dataHasBeenSentAtMillis + 500)) {
          Serial.println("TIMEOUT!!!!");
          Serial.println("go to bed!");
          Serial.println("....BYE");
          goSleep(DEEP_SLEEP_S);
        }
      }
    }
}
