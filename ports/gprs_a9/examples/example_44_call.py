# Micropython a9g example
# Source: https://github.com/pulkin/micropython
# Author: esn4dz
# Demonstrates how to control Relay by phone calls 

import cellular
import machine
import time
global flag
global first_call
flag = 1
first_call = 1 
relay=2 #relay pin

# to minimize power consumption (in my pudding a9 power reduced from 50mA to 30mA -measured with cheap usb meter)
machine.set_idle(1)
# uncomment if you are living in place with Poor Coverage Area :
#but it cost you to add a battery and a power manager module see this:
#https://github.com/Ai-Thinker-Open/GPRS_C_SDK/issues/421
#cellular.set_bands(cellular.NETWORK_FREQ_BAND_GSM_900E)
time.sleep(2)
#turn leds on
led1 = machine.Pin(27, machine.Pin.OUT, 1)
led2 = machine.Pin(28, machine.Pin.OUT, 1)

value = 1
while (not cellular.is_network_registered() )   :
    led1.value(value)
    led2.value(not value)
    value = 0 if (value==1) else 1    
    print("waiting network to register..")
    time.sleep(1)


def call_handler(evt):
    global flag
    global first_call
    print(evt)
    if type(evt) is not bool:   
        if evt == "0697635555":
            print("call from me !")
            if first_call:
                print("relay will turned_on")
                machine.Pin(relay, machine.Pin.OUT, first_call)
                first_call=0
            else:
                print("relay will turned_off")
                machine.Pin(relay, machine.Pin.OUT, first_call)
                time.sleep(4)
                cellular.dial(0)
                first_call=1
        else :
            print("unknown number, reject ...")
            cellular.dial(0)
            #flag = 0

cellular.on_call(call_handler)


print("Doing something important ...")
while flag:
    ok=1
    print(cellular.get_signal_quality()[0])
    print(machine.get_input_voltage() )
    if cellular.get_network_status()!=1:
        led1.value(0)
        led2.value(0)
        time.sleep(5) 
        ok=0
    if cellular.get_signal_quality()[0]<15 :
        led1.value(1)
        time.sleep(0.1)
        led1.value(0)
        time.sleep(0.1)
        
        led2.value(1)
        time.sleep(0.1)
        led2.value(0)
        time.sleep(0.1)
        ok=0
    if ok:
        led2.value(0)
        led1.value(1)
        time.sleep(0.1)
        led1.value(0)
    time.sleep(1)    

print("Done!")