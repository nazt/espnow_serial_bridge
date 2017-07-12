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
 * valid packet is [0xfc, 0xfd, ...packetData, 0x0d, 0x0a]
 */
export let isValidInComingMessage = (message) => {
  const msgLength = message.length

  const payloadType = getPayloadType(Buffer.from(message))
  const isValidHeaderBytes = payloadType !== Constants.PAYLOAD_TYPE_UNKNOWN
  const isValidEndBytes = slice(message, msgLength - 2, 2).equals(Buffer.from([0x0d, 0x0a]))

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
  const IDX = {
    START_BYTES: 0,
    MAC_1: 2,
    MAC_2: 2 + 6,
    DATA_PAYLOAD: 2 + 6 + 6 + 1
  }

  const payloadType = getPayloadType(payload)

  if (payloadType === Constants.PAYLOAD_FAFB_TYPE_DEVICE_REGISTRATION) {
    // const type = payload.slice(2, 6)
    const mac1 = slice(payload, IDX.MAC_1 + 4, 6).toString('hex')
    const mac2 = slice(payload, IDX.MAC_2 + 4, 6).toString('hex')
    // mac1 = SOFT_AP_IF
    // mac2 = STA_IF
    return {mac1, mac2, payloadType}
  } else if (payloadType === Constants.PAYLOAD_FCFD_TYPE_DATA) {
    const mac1 = slice(payload, IDX.MAC_1, 6).toString('hex')
    const mac2 = slice(payload, IDX.MAC_2, 6).toString('hex')
    const len = payload[2 + 6 + 6]
    const dataPayload = slice(payload, IDX.DATA_PAYLOAD, len)
    return {len, mac1, mac2, data: dataPayload, payloadType}
  } else {
    return {
      payloadType
    }
  }
}

export let parseDataPayload = (payload, parserType) => {
  // console.log(`parseDataPayload = `, typeof payload, payload, payload.toString('hex'))
  if (parserType === Constants.PAYLOAD_FCFD_TYPE_DATA) {
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
  } else if (parserType === Constants.PAYLOAD_FAFB_TYPE_DEVICE_REGISTRATION) {
    // console.log(`parserType = DEVICE REGISTRATION`)
  } else {
    // console.log(`parserType = UNKNOWN`)
  }
}

export const getPayloadType = (payload) => {
  const startBytes = slice(payload, 0, 2)
  if (startBytes.equals(createBufferFromHexString('fafb'))) {
    return Constants.PAYLOAD_FAFB_TYPE_DEVICE_REGISTRATION
  } else if (startBytes.equals(createBufferFromHexString('fcfd'))) {
    return Constants.PAYLOAD_FCFD_TYPE_DATA
  } else {
    return Constants.PAYLOAD_TYPE_UNKNOWN
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

export const Constants = {
  PAYLOAD_FAFB_TYPE_DEVICE_REGISTRATION: 'PAYLOAD_FAFB_TYPE_DEVICE_REGISTRATION',
  PAYLOAD_FCFD_TYPE_DATA: 'PAYLOAD_FCFD_TYPE_DATA',
  PAYLOAD_TYPE_UNKNOWN: 'PAYLOAD_TYPE_UNKNOWN'
}
