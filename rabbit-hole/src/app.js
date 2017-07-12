#!/bin/env node

import { CONFIG } from './conf'

const chalk = require('chalk')
const moment = require('moment-timezone')

console.log(chalk, CONFIG)
console.log(`Application started ${moment().tz('Asia/Bangkok')}`)
