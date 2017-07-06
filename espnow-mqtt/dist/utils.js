'use strict';

Object.defineProperty(exports, "__esModule", {
  value: true
});
/** * Created by nat on 7/5/2017 AD.
 */

var chalk = require('chalk');
var hexChar = exports.hexChar = function hexChar(b) {
  return b.toString(16);
};
var checksum = exports.checksum = function checksum(message) {
  var calculatedSum = 0;
  var checkSum = message[message.length - 1];
  for (var i = 0; i < message.length - 1; i++) {
    calculatedSum ^= message[i];
  }
  console.log('calculated sum = ' + chalk.yellow(hexChar(calculatedSum)));
  console.log('     check sum = ' + chalk.green(hexChar(calculatedSum)));
  return calculatedSum === checkSum;
};
//# sourceMappingURL=utils.js.map
