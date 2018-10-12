
import machine
import time

def blink(interval):
    print("blink")
    led = machine.Pin(27,machine.Pin.OUT,0)
    value = 1
    while(1):
        led.value(value)
        time.sleep(interval)
        value = 0 if (value==1) else 1


