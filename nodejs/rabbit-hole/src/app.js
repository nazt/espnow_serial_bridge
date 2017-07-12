#!/bin/env node

import net from 'net'
import pkg from '../package.json'
import { CONFIG } from './conf'

const chalk = require('chalk')
const moment = require('moment-timezone')

console.log(chalk, CONFIG)
console.log(`Application started ${moment().tz('Asia/Bangkok')}`)

const PORT = process.env.PORT || 10778
console.log(`starting application v.${pkg.version} on port ${PORT} `)

const server = net.createServer((socket) => {
  socket.name = `${socket.remoteAddress}:${socket.remotePort}`
  // incoming data
  socket.on('data', (data) => {
    console.log(`data = `, data)
  })
})
server.listen(PORT, '0.0.0.0')
