'use strict';

Object.defineProperty(exports, "__esModule", {
  value: true
});
/**
 * Created by nat on 7/6/2017 AD.
 */

var CONFIG = exports.CONFIG = {
  MQTT: {
    SUB_TOPIC: process.env.MQTT_SUB_TOPIC || 'CMMC/nat/espnow',
    PUB_PREFIX: process.env.MQTT_PUB_PREFIX || 'ESPNOW',
    PUB_TOPIC: process.env.MQTT_PUB_TOPIC,
    HOST: process.env.MQTT_HOST || 'mqtt.cmmc.io'
  }
};
//# sourceMappingURL=conf.js.map
