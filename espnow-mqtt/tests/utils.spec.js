import * as Utils from '../src/utils'

const UInt32LEByte = Utils.UInt32LEByte
const hexFromChar = Utils.hexFromChar

describe('src/utils.js', () => {
  let validBuffer
  let invalidStartBytesBuffer
  let invalidEndBytesBuffer
  let bufferSize
  let bufferLastIdx

  let dataBytes
  let type, field1, field2, field3, battery, nameLen, name
  const [startBytes, mac1, mac2, endBytes] = [
    [0xfc, 0xfd], /* start byte */
    [0x18, 0xfe, 0x34, 0xdb, 0x43, 0x10], /* mac1 */
    [0x18, 0xfe, 0x34, 0xda, 0xf4, 0x98], /* mac2 */
    [0x0d, 0x0a] /* end byte */
  ]

  console.log(`startBytes: `, startBytes)
  console.log(`endBytes: `, endBytes)

  beforeEach(() => {
    [type, field1, field2, field3, battery, nameLen, name] = [
      [0x01, 0x02, 0x03, 0x01], /* type */
      UInt32LEByte(3200), /* field1 */
      UInt32LEByte(7200), /* field2 */
      UInt32LEByte(1111), /* field3 */
      UInt32LEByte(8000), /* battery */
      [6], /* name len: 6 for 'nat001' */
      [hexFromChar('n'), hexFromChar('a'), hexFromChar('t'), 48, 48, 49] /* name */
    ]

    dataBytes = [...[0xff, 0xfa], ...type, ...field1, ...field2, ...field3, ...battery, ...nameLen, ...name]
    dataBytes.push(Utils.calculateChecksum(Buffer.from(dataBytes)))

    let tmp = [...mac1, ...mac2, dataBytes.length, ...dataBytes]
    let packetSum = Utils.calculateChecksum(Buffer.from(tmp))
    let packetPayload = [...[0xfc, 0xfd], ...tmp, packetSum, ...[0x0d, 0x0a]]

    validBuffer = Buffer.from(packetPayload)

    bufferSize = validBuffer.length
    bufferLastIdx = bufferSize - 1

    invalidStartBytesBuffer = Buffer.from(validBuffer)
    invalidEndBytesBuffer = Buffer.from(validBuffer)

    invalidStartBytesBuffer[0] = 0x0f
    invalidStartBytesBuffer[1] = 0x0f

    invalidEndBytesBuffer[bufferLastIdx] = 0xff
    invalidEndBytesBuffer[bufferLastIdx - 1] = 0xff
  })

  describe('isValidInComingMessage', () => {
    it('should be a valid incoming message', () => {
      // default valid message
      expect(Utils.isValidInComingMessage(validBuffer)).toBeTruthy()
    })

    it('should be an invalid incoming message when start with an invalid header bytes', () => {
      expect(Utils.isValidInComingMessage(invalidStartBytesBuffer)).toBeFalsy()
      expect(Utils.isValidInComingMessage(validBuffer)).toBeTruthy()
      expect(Utils.isValidInComingMessage(invalidStartBytesBuffer)).toBeFalsy()
    })

    it('should be an invalid incoming message when end with an invalid header bytes', () => {
      expect(Utils.isValidInComingMessage(invalidEndBytesBuffer)).toBeFalsy()
    })
  })

  describe('getPayloadByStrip0D0A', () => {
    it('should return sliced payload when call with valid message', () => {
      const matchedBuffer = validBuffer.slice(0, bufferSize - 2)
      expect(Utils.getPayloadByStrip0D0A(validBuffer)).toMatchObject(matchedBuffer)
    })
    it('should return null when call getPayloadByStrip0D0A with invalidBuffer', () => {
      expect(Utils.getPayloadByStrip0D0A(invalidEndBytesBuffer)).toBeNull()
    })
  })

  describe('payload parser', () => {
    it('should parse payload wrapper correctly', () => {
      const result = Utils.parsePayload(validBuffer)
      expect(result).toMatchObject({
        mac1: Buffer.from(mac1),
        mac2: Buffer.from(mac2),
        data: Buffer.from(dataBytes),
        len: dataBytes.length
      })
      console.log(`validBuffer = `, validBuffer)
      console.log(`result = `, result)
    })

    it('should parse data payload correctly', () => {
      const payload = Utils.parsePayload(validBuffer)
      const dataPayload = payload.data
      const result = Utils.parseDataPayload(dataPayload)
      console.log(result)

      expect(result).toMatchObject({
        val1: 3200,
        val2: 7200,
        val3: 1111,
        batt: 8000,
        name: 'nat001'
      })
    })
  })

  describe('checksum function', () => {
    it('should checksum correct', () => {
      const data = Utils.slice(validBuffer, 0, validBuffer.length - 2)
      // const result = Utils.checksum(data)
      Utils.checksum(data)
    })
  })
})
