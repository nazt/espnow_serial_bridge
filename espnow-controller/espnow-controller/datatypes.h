#include <Arduino.h>

typedef struct __attribute((__packed__)) {
    uint16_t battery;
    uint8_t myNameLen;
    char myName[12];
    uint8_t fieldLen;
    uint32_t field1;
    uint32_t field2;
    uint32_t field3;
    uint32_t field4;
} SENSOR_T  ;

typedef struct __attribute((__packed__)) {
  uint8_t header[2];
  uint8_t reserved;
  uint8_t from[6];
  uint8_t to[6];
  uint8_t type;
  uint8_t dataLen;
  SENSOR_T data;
  uint8_t sum;
  uint8_t tail[2];
} PACKET_T;
