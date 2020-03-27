# Micropython a9g example
# Source: https://github.com/pulkin/micropython
# Author: sebi5361
# Demonstrates how to interface with traccar

# Get online
import cellular
import gps
import socket

ID = 76548 # Same arbitrary number as in Traccar
url = "demo.traccar.org"
port = 5055

cellular.gprs("internet", "", "")

gps.on()
loc = gps.get_location()

s = socket.socket()
s.connect((url, port))
s.send(bytes('POST /?id={}&lat={}&lon={} HTTP/1.1\r\nHost: {}:{}\r\n\r\n'.format(ID, loc[0], loc[1], url, port), 'utf8'))
rsp = s.recv(50)
s.close()
gps.off()
cellular.gprs(False)

assert rsp == b'HTTP/1.1 200 OK\r\ncontent-length: 0\r\n\r\n'

