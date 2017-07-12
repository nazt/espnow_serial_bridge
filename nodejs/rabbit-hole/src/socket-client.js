const net = require('net')

const client = new net.Socket()
const args = process.argv.slice(2)
const versionToTest = args[0] || 1

console.log(`to be tested version = ${versionToTest}`)

//  'data0': [d.version, d.version, d.version, d.version, d.version],
//  'data1': [d.distance_cm, d.lid_count, d.temperature_c, d.humidity_percent_rh, d.flame_bool],
//  'data2': [d.gyro_pitch_degree, d.gyro_roll_degree, d.pressure_pa, d.battery_percent],
//  'data3': [d.sound_db_avg, d.gas_co_raw, d.gas_ch4_raw, d.light_lux, d.period_sec, d.running_s],
//  'gps':   [d.latitude, d.longitude, d.altitude],
//  'data5': [d.rssi, d.battery_raw]

let packetPayloadFCFD = Buffer.from([...[0xfc, 0xfd], ...[0xff, 0xff, 0xff, 0xff], ...[0x0d, 0x0a]])
let packetPayloadFAFB = Buffer.from([...[0xfa, 0xfb], ...[0xff, 0xff, 0xff, 0xff], ...[0x0d, 0x0a]])

const versionInt = parseInt(versionToTest, 10)
// client.connect(10777, 'api.traffy.xyz', function () {
client.connect(10778, 'localhost', () => {
  console.log(`this is for version ${versionInt}`)
  client.write(Buffer.from([0xfa, 0xfb, 0xff, 0xff, 0xff, 0xff, 0x5c, 0xcf, 0x7f, 0x9, 0x50, 0xa7, 0x5e, 0xcf, 0x7f,
    0x9, 0x50, 0xa7, 0x3, 0xd, 0xa]))
  client.flush()
  client.write(packetPayloadFAFB)
  client.flush()
  client.write(packetPayloadFCFD)
  client.flush()
  client.end()
})
