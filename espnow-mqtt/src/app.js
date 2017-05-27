export default (a, b) => a + b
const mqtt = require('mqtt')
const client = mqtt.connect('mqtt://mqtt.cmmc.io')

let checksum = (message) => {
  let calculatedSum
  let checkSum = message[message.length - 1]
  for (let i = 0; i < message.length - 1; i++) {
    let v = message[i]
    calculatedSum ^= v
  }
  console.log(`calculated sum = ${calculatedSum.toString(16)}`)
  console.log(`check sum = ${checkSum.toString(16)}`)
  return calculatedSum === checkSum
}

client.on('connect', function () {
  client.subscribe('/espnow/#')
})

client.on('message', function (topic, message) {
  console.log('================')
  if (message[message.length - 1] === 0x0d) {
    message = message.slice(0, message.length - 1)
  }

  console.log(message)
  console.log('================')
  console.log(message.length)

  if (checksum(message)) {
    if (message[0] === 0xfc && message[1] === 0xfd) {
      console.log(message)
      let mac1 = message.slice(2, 2 + 6)
      let mac2 = message.slice(2 + 6, 2 + 6 + 6)
      console.log(mac1, mac2)
      let len = message[2 + 6 + 6]
      let payload = message.slice(2 + 6 + 6 + 1, message.length - 1)
      console.log(`len = ${len}, payload = ${payload.toString('hex')}`)
      // if (checksum(payload)) {
      //   console.log('YAY!')
      // }
    }
    //   if (message[0] === 0xff && message[1] === 0xfa) {
    //     let type = message.slice(2, 5)
    //     let name = message.slice(5, 11)
    //     let mac = message.slice(27, 27 + 6)
    //
    //     console.log(`type = `, type)
    //     console.log(`name = `, name.toString())
    //     console.log(`val1 = `, message.readUInt32LE(11))
    //     console.log(`val2 = `, message.readUInt32LE(15))
    //     console.log(`val3 = `, message.readUInt32LE(19))
    //     console.log(`batt = `, message.readUInt32LE(23))
    //     console.log(`mac = `, mac.toString('hex'))
    //   } else {
    //     console.log('invalid header')
    //   }
  } else {
    console.log('invalid checksum')
  }
})
