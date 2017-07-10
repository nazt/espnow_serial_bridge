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
      let pubTopics = []
      let mac1 = parsedResult.mac1
      let mac2 = parsedResult.mac2
      if (parsedResult.payloadType === Message.Constants.PAYLOAD_FCFD_TYPE_DATA) {
        const parsedData = Message.parseDataPayload(parsedResult.data, Message.getPayloadType(message))
        delete parsedResult.payloadType
        console.log(` parsedData = `, parsedData)
        console.log(` parsedResult = `, parsedResult)
        // console.log(`${type} ${name} ${val1} ${val2} ${val3} ${batt}`)
        let serializedObjectJsonString = JSON.stringify(parsedData)
        // `${CONFIG.MQTT.PUB_PREFIX}/raw/${mac1String}/${mac2String}/status`,
        // eslint-disable-next-line no-unused-vars
        pubTopics = [
          `${CONFIG.MQTT.PUB_PREFIX}/${mac1}/${mac2}/status`,
          `${CONFIG.MQTT.PUB_PREFIX}/${mac1}/${parsedData.name}/status`
        ]
        pubTopics.forEach((topic, idx) => {
          client.publish(topic, serializedObjectJsonString, {retain: false})
        })
        console.log(pubTopics)
      } else if (parsedResult.payloadType === Message.Constants.PAYLOAD_FAFB_TYPE_DEVICE_REGISTRATION) {
        delete parsedResult.payloadType
        parsedResult.time = new Date().getTime()
        let serializedDevice = JSON.stringify(parsedResult)
        pubTopics = [`${CONFIG.MQTT.PUB_PREFIX}/${mac1}/command`]
        pubTopics.forEach((topic, idx) => {
          client.publish(topic, serializedDevice, {retain: false})
        })
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
  }
})
console.log(`Application started ${moment().tz('Asia/Bangkok')}`)
