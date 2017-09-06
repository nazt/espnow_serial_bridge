#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <Ticker.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
#include <ArduinoJson.h>
#include <CMMC_Blink.hpp>
#include "DHT.h"

AsyncWebServer *server;
AsyncWebSocket *ws;
AsyncEventSource *events;

char myName[15];
int dhtType = 11;

#define LED_BUILTIN 14
#define BUTTONPIN   0
#define DHTPIN      12

DHT *dht;
#include "_user_tasks.hpp"
// Send message to the logServer every 10 seconds
bool userReadSensorFlag = false;

#include "doconfig.h"
#include "util.h"
extern "C" {
  #include <espnow.h>
  #include <user_interface.h>
}

#define MODE_WEBSERVER 1
#define MODE_MESH    2

int runMode = MODE_MESH;

String ssid = "belkin.636";
String password = "3eb7e66b";
const char* hostName = "cmmc-";
String http_username = "admin";
String http_password = "admin";

CMMC_Blink *blinker;

Ticker ticker;
bool longPressed = false;
#include "webserver.h"

void initUserSensor() {
    Serial.println("Initializing dht.");
    dht = new DHT(DHTPIN, dhtType);
    // Initialize device.
    dht->begin();
    float h = dht->readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht->readTemperature();
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

void waitConfigSignal(uint8_t gpio, bool* longpressed) {
    Serial.println("Checking.....");
    unsigned long _c = millis();
    while(digitalRead(gpio) == LOW) {
      delay(10);
      // Serial.println("waiting....");
      // Serial.println(millis() - _c);
      if((millis() - _c) >= 1000L) {
        *longpressed = true;
        Serial.println("Release the button to enter config mode   .");
        blinker->blink(50, LED_BUILTIN);
        while(digitalRead(gpio) == LOW) {
          Serial.println("RELEASE ME..!");
          delay(1000);
        }
        blinker->detach();
        // wifi_status_led_install(2,  PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
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
  // SKETCH BEGIN
    server = new AsyncWebServer(80);
    ws = new AsyncWebSocket("/ws");
    events = new AsyncEventSource("/events");

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

uint8_t master_mac[6];
uint8_t slave_mac[6];
void initEspNow() {
  Serial.println("====================");
  Serial.println("   MODE = ESPNOW    ");
  Serial.println("====================");
  WiFi.disconnect();
  Serial.println("Initializing ESPNOW...");
  Serial.println("Initializing... SLAVE");
  WiFi.mode(WIFI_AP_STA);

  uint8_t macaddr[6];
  wifi_get_macaddr(STATION_IF, macaddr);
  Serial.print("[master] mac address (STATION_IF): ");
  printMacAddress(macaddr);

  wifi_get_macaddr(SOFTAP_IF, macaddr);
  Serial.println("[slave] mac address (SOFTAP_IF): ");
  printMacAddress(macaddr);
  memcpy(slave_mac, macaddr, 6);

  if (esp_now_init() == 0) {
    Serial.println("init");
  } else {
    Serial.println("init failed");
    ESP.restart();
    return;
  }
  Serial.println("SET ROLE SLAVE");
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);

}

void checkBootMode() {
    Serial.println("Wating configuration pin..");
    delay(2000);
    waitConfigSignal(BUTTONPIN, &longPressed);
    Serial.println("...Done");
    digitalWrite(LED_BUILTIN, LOW);
    if (longPressed) {
        runMode = MODE_WEBSERVER;
        startModeConfig();
        setupWebServer();
    } else {
        // printAndStoreEspNowMacInfo();
        initEspNow();
        esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
        uint8_t recv_counter = 0;
    }
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    SPIFFS.begin();
    WiFi.disconnect();
    loadConfig(myName, &dhtType);
    delay(10);
    pinMode(BUTTONPIN, INPUT_PULLUP);
    blinker = new CMMC_Blink();
    blinker->init();
    checkBootMode();
    bzero(myName, 0);
    // Serial.printf("myName = %s\r\n", myName);
    delay(100);
}

bool readDHTSensor(uint32_t* temp, uint32_t* humid) {
  *temp = (uint32_t) dht->readTemperature()*100;
  *humid = (uint32_t) dht->readHumidity()*100;
}

bool isSetupMesh = false;
void setUpMesh() {
    initUserSensor();
}

uint32_t markedTime;
bool dirty = false;

void loop() {
    if (runMode == MODE_WEBSERVER) {
      return;
    } else {
      if (!isSetupMesh) {
        isSetupMesh = true;
        setUpMesh();
        blinker->blink(5000, LED_BUILTIN);
        attachInterrupt(LED_BUILTIN, []() {
          dirty = true;
          markedTime = millis();
        } , RISING);
      } else {
        if (userReadSensorFlag) {
          Serial.println("READ USER SENSOR...");
          Serial.println("READ USER SENSOR...");
          Serial.println("READ USER SENSOR...");
          Serial.println("READ USER SENSOR...");
          userTaskReadSensor();
          userReadSensorFlag = false;
        }

        if (dirty && (millis() - markedTime > 50)) {
          digitalWrite(LED_BUILTIN, LOW);
          dirty = false;
        }
      }
    }
}
