extern "C" {
  #include <espnow.h>
  #include <user_interface.h>
}

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

ADC_MODE(ADC_VCC);

AsyncWebServer *server;
AsyncWebSocket *ws;
AsyncEventSource *events;

static uint32_t recv_counter = 0;
static uint32_t send_ok_counter = 0;
static uint32_t send_fail_counter = 0;


char myName[12];
int dhtType = 11;

#define LED_BUILTIN 2
#define BUTTONPIN   0
#define DHTPIN      6

DHT *dht;
#include "_user_tasks.hpp"
bool userReadSensorFlag = false;
#include "doconfig.h"
#include "util.h"

#define MODE_WEBSERVER  1
#define MODE_RUNNING    2

int runMode = MODE_RUNNING;

String ssid = "belkin.636";
String password = "3eb7e66b";
const char* hostName = "cmmc-";
String http_username = "admin";
String http_password = "admin";

CMMC_Blink *blinker;

Ticker ticker;
bool longPressed = false;
bool ledState = HIGH;
#include "webserver.h"
#include "datatypes.h"

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
    Serial.print(" %%\t");
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.print(" *C ");
}

bool isLongPressed(uint8_t gpio) {
    bool longPressed = false;
    Serial.println("Checking.....");
    unsigned long _c = millis();
    while(digitalRead(gpio) == LOW) {
      delay(10);
      if((millis() - _c) >= 1000L) {
        longPressed = true;
        Serial.println("Release the button to enter config mode   .");
        blinker->blink(50, LED_BUILTIN);
        while(digitalRead(gpio) == LOW) {
          Serial.println("RELEASE ME..!");
          delay(1000);
        }
        blinker->detach();
      }
      else {
        longPressed = false;
      }
    }
    return longPressed;
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
  esp_now_register_send_cb([](uint8_t* macaddr, uint8_t status) {
    recv_counter++;
    // Serial.println(millis());
    // Serial.println("send to mac addr: ");
    // printMacAddress(macaddr);
    // Serial.printf("result = %d \r\n", status);
    if (status == 0) {
      send_ok_counter++;
      // Serial.printf("... send_cb OK. [%lu/%lu]\r\n", send_ok_counter,
      //   send_ok_counter + send_fail_counter);
      // digitalWrite(LED_BUILTIN, HIGH);
    }
    else {
      send_fail_counter++;
      // Serial.printf("... send_cb FAILED. [%lu/%lu]\r\n", send_ok_counter,
      //   send_ok_counter + send_fail_counter);
    }
  });
}

void checkBootMode() {
    Serial.println("Wating configuration pin..");
    delay(2000);
    if (isLongPressed(BUTTONPIN)) {
        // Serial.println("...Done");
        // digitalWrite(LED_BUILTIN, LOW);
        runMode = MODE_WEBSERVER;
        startModeConfig();
        setupWebServer();
    } else {
        // printAndStoreEspNowMacInfo();
        initEspNow();
        sendDataOverEspNow();
        delay(100);
        ESP.reset();
    }
}

static uint32_t checksum(uint8_t *data, size_t len) {
    uint32_t sum = 0;
    while(len--) {
        sum ^= *(data++);
    }
    return sum;
}

bool sendDataOverEspNow() {
  SENSOR_T sd;
  bzero(&sd, sizeof(sd));
  // sd.battery = analogRead(A0);
  sd.battery = ESP.getVcc();
  sd.myNameLen = 12;
  strcpy(sd.myName, myName);
  sd.fieldLen = 4;
  sd.field1 = 17;
  sd.field2 = 18;
  sd.field3 = 19;
  sd.field4 = 20;

  PACKET_T packet;
  bzero(&packet, sizeof(packet));
  packet.header[0] = 0xff;
  packet.header[1] = 0xfa;
  packet.dataLen = sizeof(sd);
  packet.data = sd;
  packet.tail[0] = 0x0d;
  packet.tail[1] = 0x0a;
  memcpy(&packet.to, master_mac, sizeof(packet.to));
  memcpy(&packet.from, slave_mac, sizeof(packet.from));

  bool ret;

  while(1) {
    Serial.println();
    // for (size_t i = 0; i < sizeof(pkt); i++) {
    //   Serial.printf("%02x ", pkt[i]);
    //   if ((i+1)%4 == 0 && i != 0) {
    //     Serial.println();
    //   }
    // }
    packet.data.battery = analogRead(A0);
    packet.data.field4 = millis();
    packet.sum = checksum((uint8_t*) &packet, sizeof(packet)-sizeof(packet.sum));

    Serial.println("===========");
    digitalWrite(LED_BUILTIN, ledState);
    esp_now_send(master_mac, (u8*) &packet, sizeof(packet));
    ledState = !ledState;
    delay(20);
  }
  return ret;
}

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("begin...");
    Serial.println("begin...");
    Serial.println("begin...");
    Serial.println("begin...");
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    ticker.attach_ms(1000, []() {
      Serial.printf("%d/s\r\n", recv_counter);
      recv_counter = 0;
    });
    SPIFFS.begin();
    WiFi.disconnect();
    delay(10);
    blinker = new CMMC_Blink();
    blinker->init();
    pinMode(BUTTONPIN, INPUT_PULLUP);
    //
    loadConfig(myName, &dhtType);
    checkBootMode();
    delay(100);
}

bool readDHTSensor(uint32_t* temp, uint32_t* humid) {
  *temp = (uint32_t) dht->readTemperature()*100;
  *humid = (uint32_t) dht->readHumidity()*100;
}

bool isSetupMesh = false;
void setUpMesh() {
    // initUserSensor();

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
          userTaskReadSensor();
          userReadSensorFlag = false;
        }

        if (dirty && ((millis() - markedTime) > 50)) {
          digitalWrite(LED_BUILTIN, LOW);
          dirty = false;
        }
      }
    }
}
