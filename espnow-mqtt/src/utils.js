/** * Created by nat on 7/5/2017 AD.
 */

export let checksum = (message) => {
  let calculatedSum = 0
  let checkSum = message[message.length - 1]
  for (let i = 0; i < message.length - 1; i++) {
    calculatedSum ^= message[i]
  }
  // console.log(`calculated sum = ${chalk.yellow(hexChar(calculatedSum))}`)
  // console.log(`    >check sum = ${chalk.green(hexChar(checkSum))}`)
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

export const hexChar = (b) => b.toString(16)
