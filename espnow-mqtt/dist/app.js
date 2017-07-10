#!/bin/env node
'use strict';

var _conf = require('./conf');

var _utils = require('./utils');

var chalk = require('chalk');
var mqtt = require('mqtt');
var client = mqtt.connect('mqtt://' + _conf.CONFIG.MQTT.HOST);
var moment = require('moment-timezone');
var _ = require('underscore');

client.on('connect', function () {
  console.log('mqtt connected being subscribed to ' + _conf.CONFIG.MQTT.SUB_TOPIC);
  client.subscribe(_conf.CONFIG.MQTT.SUB_TOPIC);
});

client.on('packetsend', function (packet) {
  // console.log(`packetsend`, packet);
  if (packet.cmd === 'publish') {
    console.log('published to ' + chalk.green(packet.topic));
  } else if (packet.cmd === 'subscribe') {
    console.log('send subscriptions to ' + chalk.green(JSON.stringify(packet.subscriptions)));
  }
});

client.on('message', function (topic, message) {
  console.log('==================================');
  // console.log(`orig message = `, message);

  // rhythm 0xd0xa$
  if (message[message.length - 2] === 0x0d) {
    message = message.slice(0, message.length - 2);
  }
  // console.log(`     message = `, message);

  var statusObject = {};
  if ((0, _utils.checksum)(message)) {
    if (message[0] === 0xfc && message[1] === 0xfd) {
      var len = message[2 + 6 + 6];
      var mac1 = message.slice(2, 2 + 6);
      var mac2 = message.slice(2 + 6, 2 + 6 + 6);
      var payload = message.slice(2 + 6 + 6 + 1, message.length - 1);

      if (payload[0] === 0xff && payload[1] === 0xfa) {
        var _payload = payload.slice(2).toString('hex');
        console.log('len = ' + chalk.cyan(len) + ', payload = ' + chalk.yellow('fffa') + chalk.cyan(_payload));
        var type = payload.slice(2, 5);
        var name = payload.slice(5, 11);
        var mac1String = mac1.toString('hex');
        var mac2String = mac2.toString('hex');
        var val1 = payload.readUInt32LE(11) || 0,
            val2 = payload.readUInt32LE(15) || 0,
            val3 = payload.readUInt32LE(19) || 0,
            batt = payload.readUInt32LE(23) || 0;


        _.extend(statusObject, {
          myName: name.toString(),
          type: type.toString('hex'),
          sensor: type.toString('hex'),
          val1: parseInt(val1.toString()),
          val2: parseInt(val2.toString()),
          val3: parseInt(val3.toString()),
          batt: parseInt(batt.toString()),
          mac1: mac1String,
          mac2: mac2String,
          updated: moment().unix().toString(),
          updatedText: moment().tz('Asia/Bangkok').format('DD/MMMM/YYYY, hh:mm:ss a')
        });

        console.log('==================================');
        console.log('type = ', statusObject.type);
        console.log('name = ', statusObject.name);
        console.log('val1 = ', statusObject.val1);
        console.log('val2 = ', statusObject.val2);
        console.log('val3 = ', statusObject.val3);
        console.log('batt = ', statusObject.batt);
        console.log('[master] mac1 = ', statusObject.mac1);
        console.log('[ slave] mac2 = ', statusObject.mac2);
        console.log('==================================');

        var serializedObjectJsonString = JSON.stringify(statusObject);
        // eslint-disable-next-line no-unused-vars
        var pubTopics = [_conf.CONFIG.MQTT.PUB_PREFIX + '/' + mac1String + '/' + mac2String + '/status', _conf.CONFIG.MQTT.PUB_PREFIX + '/raw/' + mac1String + '/' + mac2String + '/status', _conf.CONFIG.MQTT.PUB_PREFIX + '/' + mac1String + '/' + name.toString() + '/status'].forEach(function (topic, idx) {
          client.publish(topic, serializedObjectJsonString, { retain: false });
        });
      } else {
        console.log('invalid header');
      }
    }
  } else {
    console.log(message);
    console.log('================');
    console.log('================');
    console.log(message.length);
    console.log('invalid checksum');
    console.log('================');
    console.log('================');
  }
});

console.log('Application started ' + moment().tz('Asia/Bangkok'));
//# sourceMappingURL=app.js.map
