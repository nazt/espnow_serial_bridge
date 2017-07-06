/** * Created by nat on 7/5/2017 AD.
 */

var chalk = require('chalk')
export const slice = (arr, idx, len) => {
  return arr.slice(idx, idx + len)
}

export let calculateChecksum = (message) => {
  let calculatedSum = 0
  let lastIdx = message.length
  for (let i = 0; i < lastIdx; i++) {
    calculatedSum ^= message[i]
  }
  return calculatedSum
}

export let checksum = (message) => {
  let checkSum = message[message.length - 1]
  let data = slice(message, 0, message.length - 1)
  let calculatedSum = calculateChecksum(data)
  console.log(`          >check sum = ${chalk.green(hexChar(checkSum))}`)
  console.log(`calculated check sum = ${chalk.green(hexChar(calculatedSum))}`)
  console.log(`checksum result = ${calculatedSum === checkSum}`)
  return calculatedSum === checkSum
}

export let isValidInComingMessage = (message) => {
  const msgLength = message.length
  const lastIdx = msgLength - 1
  const isValidHeaderBytes = (message[0] === 0xfc && message[1] === 0xfd)
  const isValidEndBytes = (message[lastIdx - 1] === 0x0d && message[lastIdx] === 0x0a)
  return isValidHeaderBytes && isValidEndBytes
}

export let getPayload = (message) => {
  if (isValidInComingMessage(message)) {
    return message.slice(message, message.length - 2)
  } else {
    return null
  }
}

export let parsePayload = (message) => {
  // START BYTE = 2 BYTES
  // MAC ADDR   = 6 BYTES
  // DATA LEN   = 1 BYTES
  const IDX = {START_BYTES: 0, MAC_1: 2, MAC_2: 2 + 6, DATA_PAYLOAD: 2 + 6 + 6 + 1}

  const len = message[2 + 6 + 6]
  const mac1 = slice(message, IDX.MAC_1, 6)
  const mac2 = slice(message, IDX.MAC_2, 6)
  const dataPayload = slice(message, IDX.DATA_PAYLOAD, len)
  // console.log(`message = `, message)
  console.log(`dataPayload = `, dataPayload)
  return {len, mac1, mac2, data: dataPayload}
}

export const hexChar = (b) => b.toString(16)
