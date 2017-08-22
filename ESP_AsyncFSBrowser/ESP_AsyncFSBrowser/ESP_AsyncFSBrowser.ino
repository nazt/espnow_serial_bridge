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

String ssid = "belkin.636";
String password = "3eb7e66b";
const char* hostName = "cmmc-";
String http_username = "admin";
String http_password = "admin";

// ESPNOW
// uint8_t toMasterMac[6];


// SKETCH BEGIN
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");
CMMC_Blink *blinker;

Ticker ticker;
bool longPressed = false;
#include "webserver.h"

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
    digitalWrite(LED_BUILTIN, HIGH);
    pinMode(13, INPUT_PULLUP);
    blinker = new CMMC_Blink();
    blinker->init();
}

void waitConfigSignal(uint8_t gpio, bool* longpressed) {
    Serial.println("HELLO...");
    unsigned long _c = millis();
    while(digitalRead(gpio) == LOW) {
      delay(10);
      // Serial.println("waiting....");
      // Serial.println(millis() - _c);
      if((millis() - _c) >= 1000L) {
        *longpressed = true;
        Serial.println("Release to take an effect.");
        blinker->blink(50, LED_BUILTIN);
        while(digitalRead(gpio) == LOW) {
          Serial.println("RELEASE ME..!");
          delay(1000);
        }
        blinker->detach();
        wifi_status_led_install(2,  PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
        // blinker->detach();
        // blinker->blink(500, LED_BUILTIN);
      }
      else {
        *longpressed = false;
      }
    }
    // Serial.println("/NORMAL");
}

void startModeConfig() {
    Serial.println("====================");
    Serial.println("   MODE = CONFIG    ");
    Serial.println("====================");
    String apName = String(hostName) + String(ESP.getChipId());
    Serial.printf("ApName = %s \r\n", apName.c_str());
    WiFi.hostname(hostName);
    WiFi.disconnect();
    delay(20);
    WiFi.mode(WIFI_AP_STA);
    delay(20);
    Serial.println("Starting softAP..");
    // const char *apPass_cstr = String(ESP.getChipId()).c_str();
    // Serial.println(String(apPass_cstr));
    WiFi.softAP(apName.c_str(), apName.c_str());
    // WiFi.begin(ssid.c_str(), password.c_str());
    // if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    //     Serial.printf("STA: Failed!\n");
    //     delay(1000);
    //     WiFi.disconnect(false);
    //     WiFi.begin(ssid.c_str(), password.c_str());
    // }
}

void checkBootMode() {
    Serial.println("Wating configuration pin..");
    waitConfigSignal(13, &longPressed);
    Serial.println("...Done");
    if (longPressed) {
        runMode = MODE_WEBSERVER;
        startModeConfig();
        setupWebServer();
    } else {
        // printAndStoreEspNowMacInfo();
    }
}

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    SPIFFS.begin();
    WiFi.disconnect();
    delay(100);
    initGpio();
    checkBootMode();
    initUserSensor();
    setupOTA();
    char myName[30];
    bzero(myName, 0);
    loadConfig(myName);
    Serial.printf("myName = %s\r\n", myName);
}

void goSleep(uint32_t deepSleepS) {
    Serial.printf("Go sleep for .. %lu seconds. \r\n", deepSleepS);
    ESP.deepSleep(1000000 * deepSleepS);
}

bool readDHTSensor(uint32_t* temp, uint32_t* humid) {
  *temp = (uint32_t) dht.readTemperature()*100;
  *humid = (uint32_t) dht.readHumidity()*100;
}

void loop() {
    bzero(message, sizeof(message));
    ArduinoOTA.handle();
    if (runMode == MODE_WEBSERVER) {
      return;
    } else {
      // write reference
      // readDHTSensor(&temperature_uint32, &humidity_uint32);
    }
}
