/** * Created by nat on 7/5/2017 AD.
 */

const checksum = (message) => {
  let calculatedSum = 0
  const msgLength = message.length
  const checkSum = message[msgLength - 1]
  for (let i = 0; i < msgLength - 1; i++) {
    calculatedSum ^= message[i]
  }
  console.log(`calculated sum = ${calculatedSum.toString(16)}`)
  console.log(`check sum = ${checkSum.toString(16)}`)
  return calculatedSum === checkSum
}

module.exports = {checksum}
