"use strict";

/** * Created by nat on 7/5/2017 AD.
 */

var checksum = function checksum(message) {
  var calculatedSum = 0;
  var msgLength = message.length;
  var checkSum = message[msgLength - 1];
  for (var i = 0; i < msgLength - 1; i++) {
    calculatedSum ^= message[i];
  }
  console.log("calculated sum = " + calculatedSum.toString(16));
  console.log("check sum = " + checkSum.toString(16));
  return calculatedSum === checkSum;
};

module.exports = { checksum: checksum };
//# sourceMappingURL=utils.js.map
