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

/*
 * valid is
 *    startwith 0xfc, 0xfd
 *    endwith 0x0d 0x0a
 */
export let isValidInComingMessage = (message) => {
  const msgLength = message.length
  const lastIdx = msgLength - 1
  const isValidHeaderBytes = ((message[0] === 0xfc && message[1] === 0xfd) ||
    (message[0] === 0xfa && message[1] === 0xfb))
  const isValidEndBytes = (message[lastIdx - 1] === 0x0d && message[lastIdx] === 0x0a)
  return isValidHeaderBytes && isValidEndBytes
}

export let getPayloadByStrip0D0A = (message) => {
  if (isValidInComingMessage(message)) {
    return message.slice(message, message.length - 2)
  } else {
    return null
  }
}

export let parsePayload = (payload) => {
  // START BYTE = 2 BYTES
  // MAC ADDR   = 6 BYTES
  // DATA LEN   = 1 BYTES
  const startBytes = slice(payload, 0, 2)

  const IDX = {
    START_BYTES: 0,
    MAC_1: 2,
    MAC_2: 2 + 6,
    DATA_PAYLOAD: 2 + 6 + 6 + 1
  }

  if (startBytes.equals(createBufferFromHexString('fafb'))) {
    const mac1 = slice(payload, IDX.MAC_1, 6)
    const mac2 = slice(payload, IDX.MAC_2, 6)
    // const type = payload.slice(2, 6)
    // const [val1, val2, val3, batt, nameLen] = [
    return {mac1, mac2, data: ''}
  } else if (startBytes.equals(createBufferFromHexString('fcfd'))) {
    const mac1 = slice(payload, IDX.MAC_1, 6)
    const mac2 = slice(payload, IDX.MAC_2, 6)
    const len = payload[2 + 6 + 6]
    const dataPayload = slice(payload, IDX.DATA_PAYLOAD, len)
    return {len, mac1, mac2, data: dataPayload}
  }
}

export let parseDataPayload = (payload) => {
  console.log(`parseDataPayload = `, typeof payload, payload, payload.toString('hex'))
  const type = payload.slice(2, 6)
  const [val1, val2, val3, batt, nameLen] = [
    payload.readUInt32LE(6) || 0,
    payload.readUInt32LE(10) || 0,
    payload.readUInt32LE(14) || 0,
    payload.readUInt32LE(18) || 0,
    payload.readUInt8(22)
  ]

  const name = slice(payload, 23, nameLen)
  return {
    type: type.toString('hex'),
    val1: parseInt(val1.toString(), 10),
    val2: parseInt(val2.toString(), 10),
    val3: parseInt(val3.toString(), 10),
    batt: parseInt(batt.toString(), 10),
    name: name.toString()
  }
}

export const getPayloadType = (payload) => {
  const startBytes = slice(payload, 0, 2)
  if (startBytes.equals(createBufferFromHexString('fafb'))) {
    return CONST.PAYLOAD_FAFB_TYPE_DEVICE_REGISTRATION
  } else if (startBytes.equals(createBufferFromHexString('fcfd'))) {
    return CONST.PAYLOAD_FCFD_TYPE_DATA
  }
  else {
    return CONST.PAYLOAD_TYPE_UNKNOWN
  }
}

export const hexChar = (b) => b.toString(16)
export const hexFromChar = (c) => c.charCodeAt(0)
export const UInt32LEByte = (val) => {
  const buff = Buffer.allocUnsafe(4)
  buff.writeUInt32LE(val, 0)
  return buff
}

export const createBufferFromHexString = (input) => {
  return Buffer.from(input, 'hex')
}

export const CONST = {
  PAYLOAD_FAFB_TYPE_DEVICE_REGISTRATION: 'PAYLOAD_FAFB_TYPE_DEVICE_REGISTRATION',
  PAYLOAD_FCFD_TYPE_DATA: 'PAYLOAD_FCFD_TYPE_DATA',
  PAYLOAD_TYPE_UNKNOWN: 'PAYLOAD_TYPE_UNKNOWN'
}
