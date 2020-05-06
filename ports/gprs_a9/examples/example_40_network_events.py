# Micropython a9g example
# Source: https://github.com/pulkin/micropython
# Author: pulkin
# Demonstrates how to track GSM status
import cellular
import time

cellular.flight_mode(True)

global flag
flag = 0

def evt_handler(status):
    global flag
    print("Network status changed to", status)
    flag = status

cellular.on_status_event(evt_handler)

print("Doing something important ...")
cellular.flight_mode(False)
while flag == 0:
    time.sleep(1)

print("Online!")
cellular.on_status_event(None)

