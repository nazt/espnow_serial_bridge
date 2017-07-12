'use strict';

Object.defineProperty(exports, "__esModule", {
  value: true
});
/**
 * Created by nat on 7/6/2017 AD.
 */

var ENV = process.env;
var CONFIG = exports.CONFIG = {
  MQTT: {
    SUB_TOPIC: ENV.MQTT_SUB_TOPIC || 'CMMC/nat/espnow',
    PUB_PREFIX: ENV.MQTT_PUB_PREFIX || 'ESPNOW',
    PUB_TOPIC: ENV.MQTT_PUB_TOPIC,
    HOST: ENV.MQTT_HOST || 'mqtt.cmmc.io'
  }
};
//# sourceMappingURL=conf.js.map
