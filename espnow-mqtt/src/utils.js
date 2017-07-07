/** * Created by nat on 7/5/2017 AD.
 */

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
  const IDX = {
    START_BYTES: 0,
    MAC_1: 2,
    MAC_2: 2 + 6,
    DATA_PAYLOAD: 2 + 6 + 6 + 1
  }

  const len = message[2 + 6 + 6]
  const mac1 = slice(message, IDX.MAC_1, 6)
  const mac2 = slice(message, IDX.MAC_2, 6)
  const dataPayload = slice(message, IDX.DATA_PAYLOAD, len)

  return {len, mac1, mac2, data: dataPayload}
}

export let parseDataPayload = (payload) => {
  const type = payload.slice(2, 5)
  const [val1, val2, val3, batt, nameLen] = [
    payload.readUInt32LE(5) || 0,
    payload.readUInt32LE(9) || 0,
    payload.readUInt32LE(13) || 0,
    payload.readUInt32LE(17) || 0,
    payload.readUInt8(21)
  ]

  const name = slice(payload, 22, nameLen)
  console.log(`
    type = ${type.toString('hex')}
    val1 = ${val1}
    val2 = ${val2}
    val3 = ${val3}
    batt = ${batt}
    nameLen= ${nameLen}, hex = ${hexChar(nameLen)}
    name = ${name.toString()}
   `)
}

export const hexChar = (b) => b.toString(16)
