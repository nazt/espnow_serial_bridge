import serial, sys, time, commands, re 
import logging
import paho.mqtt.client as paho
import time 
#reload(sys)  
#sys.setdefaultencoding('utf8')

DEBUG = True
USE_SERIAL = False

def dbg(msg):
    global DEBUG
    if DEBUG:
        print msg
        sys.stdout.flush()

def on_publish(client, userdata, mid):
    #print("on_publish")
    #print("mid: "+str(mid))
    pass
 
def on_message(client1, userdata, message):
    print("message received  "  , str(message.payload.decode("utf-8")))


client = paho.Client()
client.on_publish = on_publish
client.on_message = on_message 
client.connect("mqtt.cmmc.io", 1883)
client.loop_start() 
client1.subscribe("house/bulbs/bulb1")

if USE_SERIAL:
    device = None
    baud = 9600

    if not device:
        devicelist = commands.getoutput("ls /dev/ttyAMA*")
        if devicelist[0] == '/':
            device = devicelist
        if not device: 
            print "Fatal: Can't find usb serial device."
            sys.exit(0);
        else:
            print "Success: device = %s"% device
    ser = serial.Serial(
        port=device,
        baudrate=baud,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        bytesize=serial.EIGHTBITS
    )

#https://stackoverflow.com/a/27628622
def readline(a_serial, eol=b'\r\n'):
    leneol = len(eol)
    line = bytearray()
    while True:
        c = a_serial.read(1)
        if c:
            line += c
            if line[-leneol:] == eol:
                break
        else:
            break
    return (line)

def str2hexstr(line):
  return " ".join(hex(ord(n)) for n in line)


dbg("Entering main loop\n")
while True:
    try:
        # Allow time for a ctrl-c to stop the process
        p.poll(100)
        time.sleep(0.1)
    except Exception, e:
        dbg(e)
        break


