/**
 * Created by nat on 7/6/2017 AD.
 */

const ENV = process.env
export const CONFIG = {
  MQTT: {
    SUB_TOPIC: ENV.MQTT_SUB_TOPIC || 'CMMC/nat/espnow',
    PUB_PREFIX: ENV.MQTT_PUB_PREFIX || 'ESPNOW',
    PUB_TOPIC: ENV.MQTT_PUB_TOPIC,
    HOST: ENV.MQTT_HOST || 'mqtt.cmmc.io'
  }
}
