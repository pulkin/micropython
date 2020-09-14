# Micropython a9g example
# Source: https://github.com/pulkin/micropython
# Demonstrates how to control device from DTMF, 
# in call, press key "1" from your phone to turn ON builtin led, "0" to turn it OFF

import cellular
import machine
import time

led1 = machine.Pin(27, machine.Pin.OUT, 1)
led2 = machine.Pin(28, machine.Pin.OUT, 1)

led_stat = 1
while (not cellular.is_network_registered() )   :
    print("waiting network to register..")
    led1.value(led_stat)
    led2.value(not led_stat)
    led_stat = not led_stat    
    time.sleep(1)

led1.value(0)
led2.value(0)

cellular.dial('0xxxxxxxxx')

def dtmf_handler(evt):
    print(evt)
    if evt == "1":
        led1.value(1)
    elif evt == "0":
        led1.value(0)

cellular.on_dtmf(dtmf_handler)          
