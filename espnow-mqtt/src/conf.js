/**
 * Created by nat on 7/6/2017 AD.
 */

export const CONFIG = {
  MQTT: {
    SUB_TOPIC: process.env.MQTT_SUB_TOPIC || 'CMMC/nat/espnow',
    PUB_PREFIX: process.env.MQTT_PUB_PREFIX || 'ESPNOW',
    PUB_TOPIC: process.env.MQTT_PUB_TOPIC,
    HOST: process.env.MQTT_HOST || 'mqtt.cmmc.io'
  }
}
