/** * Created by nat on 7/5/2017 AD.
 */

const chalk = require('chalk')
export const hexChar = (b) => b.toString(16)
export const checksum = (message) => {
  let calculatedSum = 0
  let checkSum = message[message.length - 1]
  for (let i = 0; i < message.length - 1; i++) {
    calculatedSum ^= message[i]
  }
  console.log(`calculated sum = ${chalk.yellow(hexChar(calculatedSum))}`)
  console.log(`     check sum = ${chalk.green(hexChar(calculatedSum))}`)
  return calculatedSum === checkSum
}
