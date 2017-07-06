/** * Created by nat on 7/5/2017 AD.
 */

var chalk = require('chalk')

export const slice = (arr, idx, len) => {
  arr.slice(idx, idx + len)
}

export let checksum = (message) => {
  let calculatedSum = 0
  let checkSum = message[message.length - 1]
  for (let i = 0; i < message.length - 1; i++) {
    calculatedSum ^= message[i]
  }
  console.log(`content = `, message)
  console.log(`calculated sum = ${chalk.yellow(hexChar(calculatedSum))}`)
  console.log(`    >check sum = ${chalk.green(hexChar(checkSum))}`)
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
  // console.log(`isValidInComingMessage = `, isValidInComingMessage)
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
  const IDX_END = {START_BYTES: 0, MAC_1: 2 + 6, MAC_2: 2 + 6 + 6}

  const len = message[2 + 6 + 6]
  const mac1 = message.slice(IDX.MAC_1, IDX_END.MAC_1)
  const mac2 = message.slice(IDX.MAC_2, IDX_END.MAC_2)
  const dataPayload = message.slice(IDX.DATA_PAYLOAD, IDX.DATA_PAYLOAD + len)
  console.log(`message = `, message)
  console.log(`dataPayload = `, dataPayload)
  return {len, mac1, mac2, data: dataPayload}
}

export const hexChar = (b) => b.toString(16)
