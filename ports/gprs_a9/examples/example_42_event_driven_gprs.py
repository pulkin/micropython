# Micropython a9g example
# Source: https://github.com/pulkin/micropython
# Author: pulkin
# Demonstrates how to connect to GPRS automatically in event-driven non-blocking manner
import cellular
import socket

host = "httpstat.us"
port = 80
message = "GET /200 HTTP/1.1\r\nHost: {}\r\nConnection: close\r\n\r\n"

def handler(status):
    if cellular.gprs():
        s = socket.socket()
        s.connect((host, port))
        s.write(message.format(host))
        print(s.read(256))
        s.close()
        cellular.on_status_event(None)
        cellular.gprs(False)
        print("Done")
    else:
        print("Skipping status " + str(status))

cellular.on_status_event(handler)
cellular.gprs("internet", "", "", False)
print("Submitted")

