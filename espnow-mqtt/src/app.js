#!/bin/env node

import { CONFIG } from './conf'
import * as Message from './utils'

const chalk = require('chalk')
const mqtt = require('mqtt')
const client = mqtt.connect(`mqtt://${CONFIG.MQTT.HOST}`)
const moment = require('moment-timezone')
// const _ = require('underscore')

client.on('connect', function () {
  console.log(`mqtt connected being subscribed to ${CONFIG.MQTT.SUB_TOPIC}`)
  client.subscribe(CONFIG.MQTT.SUB_TOPIC)
})

client.on('packetsend', function (packet) {
  if (packet.cmd === 'publish') {
    console.log(`published to ${chalk.green(packet.topic)}`)
  } else if (packet.cmd === 'subscribe') {
    console.log(`send subscriptions to ${chalk.green(JSON.stringify(packet.subscriptions))}`)
  }
})

client.on('message', function (topic, message) {
  if (Message.isValidInComingMessage(message)) {
    const payload = Message.getPayloadByStrip0D0A(message)
    if (payload !== null) {
      const parsedResult = Message.parsePayload(payload)
      console.log(parsedResult)

      if (parsedResult.payloadType === Message.Constants.PAYLOAD_FCFD_TYPE_DATA) {
        const parsedData = Message.parseDataPayload(parsedResult.data, Message.getPayloadType(message))
        console.log(` parsedData = `, parsedData)
        /*
        const {type, val1, val2, val3, batt, name} =
          console.log(`${type} ${name} ${val1} ${val2} ${val3} ${batt}`)
        let serializedObjectJsonString = JSON.stringify(statusObject)
        // eslint-disable-next-line no-unused-vars
        let pubTopics = [
          `${CONFIG.MQTT.PUB_PREFIX}/${mac1String}/${mac2String}/status`,
          `${CONFIG.MQTT.PUB_PREFIX}/raw/${mac1String}/${mac2String}/status`,
          `${CONFIG.MQTT.PUB_PREFIX}/${mac1String}/${name.toString()}/status`
        ].forEach((topic, idx) => {
          client.publish(topic, serializedObjectJsonString, {retain: false})
        })
        */
      }
    }
  } else {
    console.log(message)
    console.log('================')
    console.log('================')
    console.log(message.length)
    console.log('invalid packet header.')
    console.log('================')
    console.log('================')
  }
})

console.log(`Application started ${moment().tz('Asia/Bangkok')}`)
