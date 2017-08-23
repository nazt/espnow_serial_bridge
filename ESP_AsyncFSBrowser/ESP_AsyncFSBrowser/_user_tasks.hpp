#include <ArduinoJson.h>
#include "painlessMesh.h"
#include <DHT.h>

extern DHT *dht;
extern painlessMesh mesh;
extern size_t logServerId;
extern char myName[];
extern int dhtType;

uint32_t blinkAtMs;


void userTaskReadSensor() {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    JsonObject& data = jsonBuffer.createObject();
    JsonObject& info = jsonBuffer.createObject();

    float h = dht->readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht->readTemperature();

    if (isnan(t) ||  isnan(h)) {
        h = 0;
        t = 0;
    }
    else {
      data["temp"] = t;
      data["temperature_celcius"] = t;
      data["humid"] = h;
      data["humidity_percent_rh"] = h;
    }

    data["type"] = "sensor";
    data["myName"] = String(myName) + String("-")+ mesh.getNodeId();
    data["sensor"] = "dht";
    data["sensorType"] = dhtType;
    data["heap"] = ESP.getFreeHeap();
    data["millis"] = millis();

    info["clientId"] = ESP.getChipId();
    info["client_id"] = ESP.getChipId();
    info["nodeId"] = mesh.getNodeId();
    info["ip"] = "mesh";

    root["d"] = data;
    root["info"] = info;

    String str;
    root.printTo(str);
    if (logServerId == 0) // If we don't know the logServer yet
        mesh.sendBroadcast(str);
    else
        mesh.sendSingle(logServerId, str);

    // log to serial
    data.printTo(Serial);
    Serial.printf("\n");
}
