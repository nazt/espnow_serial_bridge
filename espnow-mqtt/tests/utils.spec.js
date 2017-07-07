import * as Utils from '../src/utils'

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

  let hexFromChar = (c) => c.charCodeAt(0)
  beforeEach(() => {
    [type, field1, field2, field3, battery, nameLen, name] = [
      [0x00, 0x00, 0x01], /* type */
      [0x64, 0x68, 0x31, 0x78], /* field1 */
      [0x30, 0x32, 0xc4, 0x9], /* field2 */
      [0x00, 0x00, 0xdc, 0x5], /* field3 */
      [0x00, 0x00, 0x00, 0x10], /* battery */
      [6], /* name len: 6 for 'nat001' */
      [hexFromChar('a'), hexFromChar('a'), hexFromChar('t'), 0x07, 0x08, 0x09] /* name */
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

  describe('getPayload', () => {
    it('should return sliced payload when call with valid message', () => {
      const matchedBuffer = validBuffer.slice(0, bufferSize - 2)
      expect(Utils.getPayload(validBuffer)).toMatchObject(matchedBuffer)
    })
    it('should return null when call getPayload with invalidBuffer', () => {
      expect(Utils.getPayload(invalidEndBytesBuffer)).toBeNull()
    })
  })

  describe('parsePayload', () => {
    it('should parse payload wrapper correctly', () => {
      console.log(`validBuffer = `, validBuffer)
      const result = Utils.parsePayload(validBuffer)
      console.log(`result = `, result)
      expect(result).toMatchObject({
        mac1: Buffer.from([0x18, 0xfe, 0x34, 0xdb, 0x43, 0x10]),
        mac2: Buffer.from([0x18, 0xfe, 0x34, 0xda, 0xf4, 0x98]),
        data: Buffer.from(dataBytes),
        // data: Buffer.from([0xff, 0xfa, 0x00, 0x00, 0x01, 0x64, 0x68, 0x31, 0x78, 0x30, 0x32, 0xc4, 0x09, 0x00, 0x00,
        //   0xdc, 0x05, 0x00, 0x00, 0x00, 0x10, 0x06, 0x61, 0x61, 0x74, 0x07, 0x08, 0x09, 0x33]),
        len: 0x1d
        // })
        // console.log(`result = `, result)
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
})

