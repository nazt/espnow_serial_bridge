#!/bin/env node
'use strict';

var _conf = require('./conf');

var _utils = require('./utils');

var Message = _interopRequireWildcard(_utils);

function _interopRequireWildcard(obj) { if (obj && obj.__esModule) { return obj; } else { var newObj = {}; if (obj != null) { for (var key in obj) { if (Object.prototype.hasOwnProperty.call(obj, key)) newObj[key] = obj[key]; } } newObj.default = obj; return newObj; } }

var chalk = require('chalk');
var mqtt = require('mqtt');
var client = mqtt.connect('mqtt://' + _conf.CONFIG.MQTT.HOST);
var moment = require('moment-timezone');
// const _ = require('underscore')

client.on('connect', function () {
  console.log('mqtt connected being subscribed to ' + _conf.CONFIG.MQTT.SUB_TOPIC);
  client.subscribe(_conf.CONFIG.MQTT.SUB_TOPIC);
});

client.on('packetsend', function (packet) {
  if (packet.cmd === 'publish') {
    console.log('published to ' + chalk.green(packet.topic));
  } else if (packet.cmd === 'subscribe') {
    console.log('send subscriptions to ' + chalk.green(JSON.stringify(packet.subscriptions)));
  }
});

client.on('message', function (topic, message) {
  if (Message.isValidInComingMessage(message)) {
    var payload = Message.getPayloadByStrip0D0A(message);
    if (payload !== null) {
      var parsedResult = Message.parsePayload(payload);
      var pubTopics = [];
      var mac1 = parsedResult.mac1;
      var mac2 = parsedResult.mac2;
      if (parsedResult.payloadType === Message.Constants.PAYLOAD_FCFD_TYPE_DATA) {
        var parsedData = Message.parseDataPayload(parsedResult.data, Message.getPayloadType(message));
        delete parsedResult.payloadType;
        console.log(' parsedData = ', parsedData);
        console.log(' parsedResult = ', parsedResult);
        // console.log(`${type} ${name} ${val1} ${val2} ${val3} ${batt}`)
        var serializedObjectJsonString = JSON.stringify(parsedData);
        // `${CONFIG.MQTT.PUB_PREFIX}/raw/${mac1String}/${mac2String}/status`,
        // eslint-disable-next-line no-unused-vars
        pubTopics = [_conf.CONFIG.MQTT.PUB_PREFIX + '/' + mac1 + '/' + mac2 + '/status', _conf.CONFIG.MQTT.PUB_PREFIX + '/' + mac1 + '/' + parsedData.name + '/status'];
        pubTopics.forEach(function (topic, idx) {
          client.publish(topic, serializedObjectJsonString, { retain: false });
        });
        console.log(pubTopics);
      } else if (parsedResult.payloadType === Message.Constants.PAYLOAD_FAFB_TYPE_DEVICE_REGISTRATION) {
        delete parsedResult.payloadType;
        parsedResult.time = new Date().getTime();
        var serializedDevice = JSON.stringify(parsedResult);
        pubTopics = [_conf.CONFIG.MQTT.PUB_PREFIX + '/' + mac1 + '/command'];
        pubTopics.forEach(function (topic, idx) {
          client.publish(topic, serializedDevice, { retain: false });
        });
      }
    } else {
      console.log(message);
      console.log('================');
      console.log('================');
      console.log(message.length);
      console.log('invalid packet header.');
      console.log('================');
      console.log('================');
    }
  }
});
console.log('Application started ' + moment().tz('Asia/Bangkok'));
//# sourceMappingURL=app.js.map
