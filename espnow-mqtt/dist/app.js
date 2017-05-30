'use strict';

Object.defineProperty(exports, "__esModule", {
  value: true
});

exports.default = function (a, b) {
  return a + b;
};

var mqtt = require('mqtt');
var client = mqtt.connect('mqtt://mqtt.cmmc.io');
var moment = require('moment');

var checksum = function checksum(message) {
  var calculatedSum = 0;
  var checkSum = message[message.length - 1];
  for (var i = 0; i < message.length - 1; i++) {
    var v = message[i];
    calculatedSum ^= v;
  }
  console.log('calculated sum = ' + calculatedSum.toString(16));
  console.log('check sum = ' + checkSum.toString(16));
  return calculatedSum === checkSum;
};

client.on('connect', function () {
  client.subscribe('CMMC/espnow/#');
});

client.on('message', function (topic, message) {
  console.log('================');
  if (message[message.length - 1] === 0x0d) {
    message = message.slice(0, message.length - 1);
  }

  console.log(message);
  console.log('================');
  console.log(message.length);

  var statusObject = {};
  if (checksum(message)) {
    var mac1 = void 0,
        mac2 = void 0;
    if (message[0] === 0xfc && message[1] === 0xfd) {
      console.log(message);
      mac1 = message.slice(2, 2 + 6);
      mac2 = message.slice(2 + 6, 2 + 6 + 6);
      var len = message[2 + 6 + 6];
      var payload = message.slice(2 + 6 + 6 + 1, message.length - 1);
      console.log('len = ' + len + ', payload = ' + payload.toString('hex'));
      // if (checksum(payload)) {
      //   console.log('YAY!')
      // }
      if (payload[0] === 0xff && payload[1] === 0xfa) {
        var type = payload.slice(2, 5);
        var name = payload.slice(5, 11);
        var mac1String = mac1.toString('hex');
        var mac2String = mac2.toString('hex');
        var val1 = payload.readUInt32LE(11) || 0,
            val2 = payload.readUInt32LE(15) || 0,
            val3 = payload.readUInt32LE(19) || 0,
            batt = payload.readUInt32LE(23) || 0;


        console.log('type = ', type);
        console.log('name = ', name.toString());
        console.log('val1 = ', val1);
        console.log('val2 = ', val2);
        console.log('val3 = ', val3);
        console.log('batt = ', batt);
        console.log('[master] mac1 = ', mac1String);
        console.log('[ slave] mac2 = ', mac2String);

        statusObject.myName = name.toString();
        statusObject.type = type.toString('hex');
        statusObject.sensor = type.toString('hex');
        statusObject.val1 = parseInt(val1.toString());
        statusObject.val2 = parseInt(val2.toString());
        statusObject.val3 = parseInt(val3.toString());
        statusObject.batt = parseInt(batt.toString());
        statusObject.mac1 = mac1String;
        statusObject.mac2 = mac2String;
        statusObject.updated = moment().unix().toString();
        statusObject.updatedText = moment().format('MMMM Do YYYY, h:mm:ss a');

        client.publish('CMMC/espnow/' + mac1String + '/' + mac2String + '/batt', batt.toString(), { retain: true });
        client.publish('CMMC/espnow/' + mac1String + '/' + mac2String + '/status', JSON.stringify(statusObject), { retain: true });
        client.publish('CMMC/espnow/' + mac1String + '/' + name.toString() + '/status', JSON.stringify(statusObject), { retain: true });
      } else {
        console.log('invalid header');
      }
    }
  } else {
    console.log('invalid checksum');
  }
});

console.log('started');
//# sourceMappingURL=app.js.map
