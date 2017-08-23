# 1 "/var/folders/ff/__xk2dnn5wx5m8lx4g5m5yvm0000gn/T/tmpQMMbtd"
#include <Arduino.h>
# 1 "/Users/nat/espnow_serial_bridge/ESP_AsyncFSBrowser/ESP_AsyncFSBrowser/ESP_AsyncFSBrowser.ino"
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
#include "painlessMesh.h"
#include "DHT.h"

#define MESH_PREFIX "natnatnat"
#define MESH_PASSWORD "tantantan"
#define MESH_PORT 5555

char myName[72];
int dhtType = 11;
#define LED_BUILTIN 14
#define BUTTONPIN 0
#define DHTPIN 12
#define INTERRUPT_TYPE RISING

DHT *dht;
painlessMesh mesh;
size_t logServerId = 0;
#include "_user_tasks.hpp"

bool userReadSensorFlag = false;
Task myLoggingTask(10000, TASK_FOREVER, []() {
  userReadSensorFlag = true;
});
void receivedCallback( uint32_t from, String &msg );
void initUserSensor();
void initGpio();
void waitConfigSignal(uint8_t gpio, bool* longpressed);
void startModeConfig();
void checkBootMode();
void setup();
void goSleep(uint32_t deepSleepS);
bool readDHTSensor(uint32_t* temp, uint32_t* humid);
void setUpMesh();
void loop();
#line 37 "/Users/nat/espnow_serial_bridge/ESP_AsyncFSBrowser/ESP_AsyncFSBrowser/ESP_AsyncFSBrowser.ino"
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("logClient: Received from %u msg=%s\n", from, msg.c_str());
# 50 "/Users/nat/espnow_serial_bridge/ESP_AsyncFSBrowser/ESP_AsyncFSBrowser/ESP_AsyncFSBrowser.ino"
}

#include "ota.h"
#include "doconfig.h"
#include "util.h"
extern "C" {
  #include <espnow.h>
  #include <user_interface.h>
}

#define MODE_WEBSERVER 1
#define MODE_MESH 2

int runMode = MODE_MESH;

String ssid = "belkin.636";
String password = "3eb7e66b";
const char* hostName = "cmmc-";
String http_username = "admin";
String http_password = "admin";


AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");
CMMC_Blink *blinker;

Ticker ticker;
bool longPressed = false;
#include "webserver.h"

void initUserSensor() {
    Serial.println("Initializing dht.");
    dht = new DHT(DHTPIN, dhtType);

    dht->begin();
    float h = dht->readHumidity();

    float t = dht->readTemperature();

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
}

void waitConfigSignal(uint8_t gpio, bool* longpressed) {
    Serial.println("Checking.....");
    unsigned long _c = millis();
    while(digitalRead(gpio) == LOW) {
      delay(10);


      if((millis() - _c) >= 1000L) {
        *longpressed = true;
        Serial.println("Release the button to enter config mode   .");
        blinker->blink(50, LED_BUILTIN);
        while(digitalRead(gpio) == LOW) {
          Serial.println("RELEASE ME..!");
          delay(1000);
        }
        blinker->detach();



      }
      else {
        *longpressed = false;
      }
    }

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


    WiFi.softAP(apName.c_str(), apName.c_str());







}

void checkBootMode() {
    Serial.println("Wating configuration pin..");
    delay(2000);
    waitConfigSignal(BUTTONPIN, &longPressed);
    Serial.println("...Done");
    if (longPressed) {
        runMode = MODE_WEBSERVER;
        startModeConfig();
        setupWebServer();
    } else {

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
    initUserSensor();
    setupOTA();
    bzero(myName, 0);

    delay(100);
}

void goSleep(uint32_t deepSleepS) {
    Serial.printf("Go sleep for .. %lu seconds. \r\n", deepSleepS);
    ESP.deepSleep(1000000 * deepSleepS);
}

bool readDHTSensor(uint32_t* temp, uint32_t* humid) {
  *temp = (uint32_t) dht->readTemperature()*100;
  *humid = (uint32_t) dht->readHumidity()*100;
}

bool isSetupMesh = false;
void setUpMesh() {

    mesh.init( MESH_PREFIX, MESH_PASSWORD, MESH_PORT, STA_AP, AUTH_WPA2_PSK, 9,
      PHY_MODE_11G, 82, 1, 4);
    mesh.onReceive(&receivedCallback);

    mesh.scheduler.addTask(myLoggingTask);
    myLoggingTask.enable();


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
        } , INTERRUPT_TYPE);
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
        mesh.update();
      }
    }
}