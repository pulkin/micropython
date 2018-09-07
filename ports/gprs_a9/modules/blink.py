
import machine

def blink(interval):
    print("blink")
    led = machine.Pin(27,machine.Pin.OUT)
    value = 1
    while(1):
        led.value(value)
        # time.sleep(1)
        # value = (value==1)?0:1

