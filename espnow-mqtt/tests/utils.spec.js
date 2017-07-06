import * as Utils from '../src/utils'

describe('src/utils.js', () => {
  let validBuffer
  let inValidBuffer
  let bufferSize
  let bufferLastIdx

  beforeEach(() => {
    validBuffer = Buffer.from([0xfc, 0xfd, 0x18, 0xfe, 0x34, 0xdb, 0x43, 0x10, 0x18, 0xfe, 0x34, 0xda, 0xf4, 0x98,
      0x1e, 0xff, 0xfa, 0x1, 0x1, 0x3, 0x64, 0x68, 0x31, 0x78, 0x30, 0x32, 0xc4, 0x9, 0x0, 0x0, 0xdc, 0x5, 0x0, 0x0,
      0x0, 0x0, 0x0, 0x0, 0x1d, 0x5, 0x0, 0x0, 0x0, 0x0, 0x6c, 0xd, 0xa])
    inValidBuffer = Buffer.from([0x00, 0x00, 0x18, 0xfe, 0x34, 0xdb, 0x43, 0x10, 0x18, 0xfe, 0x34, 0xda, 0xf4, 0x98,
      0x1e, 0xff, 0xfa, 0x1, 0x1, 0x3, 0x64, 0x68, 0x31, 0x78, 0x30, 0x32, 0xc4, 0x9, 0x0, 0x0, 0xdc, 0x5, 0x0, 0x0,
      0x0, 0x0, 0x0, 0x0, 0x1d, 0x5, 0x0, 0x0, 0x0, 0x0, 0x6c, 0xd, 0xa])
    bufferSize = validBuffer.length
    bufferLastIdx = bufferSize - 1
  })

  describe('isValidInComingMessage', () => {
    it('should be a valid incoming message', () => {
      // default valid message
      expect(Utils.isValidInComingMessage(validBuffer)).toBeTruthy()
    })

    it('should be an invalid incoming message when start with an invalid header bytes', () => {
      expect(Utils.isValidInComingMessage(inValidBuffer)).toBeFalsy()
      expect(Utils.isValidInComingMessage(validBuffer)).toBeTruthy()

      validBuffer[0] = 0xff
      expect(Utils.isValidInComingMessage(validBuffer)).toBeFalsy()

      validBuffer[0] = 0xfc
      validBuffer[1] = 0xfc
      expect(Utils.isValidInComingMessage(validBuffer)).toBeFalsy()

      validBuffer[bufferLastIdx] = 0x00
      validBuffer[bufferLastIdx - 1] = 0x00
      expect(Utils.isValidInComingMessage(validBuffer)).toBeFalsy()
    })

    it('should be an invalid incoming message when end with an invalid header bytes', () => {
      validBuffer[bufferLastIdx - 1] = 0x00
      validBuffer[bufferLastIdx] = 0xff
      expect(Utils.isValidInComingMessage(validBuffer)).toBeFalsy()
    })
  })

  describe('getPayload', () => {
    it('should return sliced payload when call with valid message', () => {
      Utils.getPayload(validBuffer)
      const matchedBuffer = validBuffer.slice(0, bufferSize - 2)
      expect(Utils.getPayload(validBuffer)).toMatchObject(matchedBuffer)
    })
  })
  it('should checksum correct', () => {
    const data = validBuffer.slice(0, validBuffer.length - 2)
    const result = Utils.checksum(data)
    expect(result).toBeTruthy()
  })
})
