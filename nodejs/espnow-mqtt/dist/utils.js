'use strict';

Object.defineProperty(exports, "__esModule", {
  value: true
});
/** * Created by nat on 7/5/2017 AD.
 */

var slice = exports.slice = function slice(arr, idx, len) {
  return arr.slice(idx, idx + len);
};

var calculateChecksum = exports.calculateChecksum = function calculateChecksum(message) {
  var calculatedSum = 0;
  var lastIdx = message.length;
  for (var i = 0; i < lastIdx; i++) {
    calculatedSum ^= message[i];
  }
  return calculatedSum;
};

var checksum = exports.checksum = function checksum(message) {
  var checkSum = message[message.length - 1];
  var data = slice(message, 0, message.length - 1);
  var calculatedSum = calculateChecksum(data);
  return calculatedSum === checkSum;
};

/*
 * valid is
 *    startwith 0xfc, 0xfd
 *    endwith 0x0d 0x0a
 */
var isValidInComingMessage = exports.isValidInComingMessage = function isValidInComingMessage(message) {
  var msgLength = message.length;

  var payloadType = getPayloadType(Buffer.from(message));
  var isValidHeaderBytes = payloadType !== Constants.PAYLOAD_TYPE_UNKNOWN;
  var isValidEndBytes = slice(message, msgLength - 2, 2).equals(Buffer.from([0x0d, 0x0a]));

  return isValidHeaderBytes && isValidEndBytes;
};

var getPayloadByStrip0D0A = exports.getPayloadByStrip0D0A = function getPayloadByStrip0D0A(message) {
  if (isValidInComingMessage(message)) {
    return message.slice(message, message.length - 2);
  } else {
    return null;
  }
};

var parsePayload = exports.parsePayload = function parsePayload(payload) {
  // START BYTE = 2 BYTES
  // MAC ADDR   = 6 BYTES
  // DATA LEN   = 1 BYTES
  var IDX = {
    START_BYTES: 0,
    MAC_1: 2,
    MAC_2: 2 + 6,
    DATA_PAYLOAD: 2 + 6 + 6 + 1
  };

  var payloadType = getPayloadType(payload);

  if (payloadType === Constants.PAYLOAD_FAFB_TYPE_DEVICE_REGISTRATION) {
    // const type = payload.slice(2, 6)
    var mac1 = slice(payload, IDX.MAC_1 + 4, 6).toString('hex');
    var mac2 = slice(payload, IDX.MAC_2 + 4, 6).toString('hex');
    // mac1 = SOFT_AP_IF
    // mac2 = STA_IF
    return { mac1: mac1, mac2: mac2, payloadType: payloadType };
  } else if (payloadType === Constants.PAYLOAD_FCFD_TYPE_DATA) {
    var _mac = slice(payload, IDX.MAC_1, 6).toString('hex');
    var _mac2 = slice(payload, IDX.MAC_2, 6).toString('hex');
    var len = payload[2 + 6 + 6];
    var dataPayload = slice(payload, IDX.DATA_PAYLOAD, len);
    return { len: len, mac1: _mac, mac2: _mac2, data: dataPayload, payloadType: payloadType };
  } else {
    return {
      payloadType: payloadType
    };
  }
};

var parseDataPayload = exports.parseDataPayload = function parseDataPayload(payload, parserType) {
  // console.log(`parseDataPayload = `, typeof payload, payload, payload.toString('hex'))
  if (parserType === Constants.PAYLOAD_FCFD_TYPE_DATA) {
    var type = payload.slice(2, 6);
    var _ref = [payload.readUInt32LE(6) || 0, payload.readUInt32LE(10) || 0, payload.readUInt32LE(14) || 0, payload.readUInt32LE(18) || 0, payload.readUInt8(22)],
        val1 = _ref[0],
        val2 = _ref[1],
        val3 = _ref[2],
        batt = _ref[3],
        nameLen = _ref[4];


    var name = slice(payload, 23, nameLen);
    return {
      type: type.toString('hex'),
      val1: parseInt(val1.toString(), 10),
      val2: parseInt(val2.toString(), 10),
      val3: parseInt(val3.toString(), 10),
      batt: parseInt(batt.toString(), 10),
      name: name.toString()
    };
  } else if (parserType === Constants.PAYLOAD_FAFB_TYPE_DEVICE_REGISTRATION) {
    // console.log(`parserType = DEVICE REGISTRATION`)
  } else {
      // console.log(`parserType = UNKNOWN`)
    }
};

var getPayloadType = exports.getPayloadType = function getPayloadType(payload) {
  var startBytes = slice(payload, 0, 2);
  if (startBytes.equals(createBufferFromHexString('fafb'))) {
    return Constants.PAYLOAD_FAFB_TYPE_DEVICE_REGISTRATION;
  } else if (startBytes.equals(createBufferFromHexString('fcfd'))) {
    return Constants.PAYLOAD_FCFD_TYPE_DATA;
  } else {
    return Constants.PAYLOAD_TYPE_UNKNOWN;
  }
};

var hexChar = exports.hexChar = function hexChar(b) {
  return b.toString(16);
};
var hexFromChar = exports.hexFromChar = function hexFromChar(c) {
  return c.charCodeAt(0);
};
var UInt32LEByte = exports.UInt32LEByte = function UInt32LEByte(val) {
  var buff = Buffer.allocUnsafe(4);
  buff.writeUInt32LE(val, 0);
  return buff;
};

var createBufferFromHexString = exports.createBufferFromHexString = function createBufferFromHexString(input) {
  return Buffer.from(input, 'hex');
};

var Constants = exports.Constants = {
  PAYLOAD_FAFB_TYPE_DEVICE_REGISTRATION: 'PAYLOAD_FAFB_TYPE_DEVICE_REGISTRATION',
  PAYLOAD_FCFD_TYPE_DATA: 'PAYLOAD_FCFD_TYPE_DATA',
  PAYLOAD_TYPE_UNKNOWN: 'PAYLOAD_TYPE_UNKNOWN'
};
//# sourceMappingURL=utils.js.map
