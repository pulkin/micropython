# Micropython a9g example
# Source: https://github.com/pulkin/micropython
# Demonstrates how to use 16x2 LCD with I2C
import machine
import i2c
#for lcd
from lcd_api import LcdApi
from a9_i2c_lcd import I2cLcd
#to get uptime
import time
#to get free memory
import gc
#to get GSM signal db
import cellular

# The PCF8574 has a jumper selectable address: 0x20 - 0x27
DEFAULT_I2C_ADDR = 0x27
# i2c pins : https://raw.githubusercontent.com/Ai-Thinker-Open/GPRS_C_SDK/master/doc/assets/pudding_pin.png 
i2c_id = 2

#need to improve this newbie code:
try:
    #I2C_FREQ:100K,400K https://ai-thinker-open.github.io/GPRS_C_SDK_DOC/en/c-sdk/function-api/iic.html
    i2c.init(i2c_id, 400)
    #16x2 lcd display
    lcd = I2cLcd(i2c,DEFAULT_I2C_ADDR,i2c_id, 2, 16)
    lcd.putstr("Hello !")
    #move to second line
    lcd.move_to(0, 1)
    lcd.putstr("free Mem:"+str(gc.mem_free()))
    # battery icons
    battery0 = bytearray([0x0E,0x11,0x11,0x11,0x11,0x11,0x11,0x1F])  # 0% Empty
    battery1 = bytearray([0x0E,0x11,0x11,0x11,0x11,0x11,0x1F,0x1F])  # 16%
    battery2 = bytearray([0x0E,0x11,0x11,0x11,0x11,0x1F,0x1F,0x1F])  # 33%
    battery3 = bytearray([0x0E,0x11,0x11,0x11,0x1F,0x1F,0x1F,0x1F])  # 50%
    battery4 = bytearray([0x0E,0x11,0x11,0x1F,0x1F,0x1F,0x1F,0x1F]) # 66%
    battery5 = bytearray([0x0E,0x11,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F])  # 83%
    battery6 = bytearray([0x0E,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F])  # 100% Full
    battery7 = bytearray([0x0E,0x1F,0x1B,0x1B,0x1B,0x1F,0x1B,0x1F])  # ! Error
    # store icons to lcd RAM
    lcd.custom_char(0,battery0)
    lcd.custom_char(1,battery1)
    lcd.custom_char(2,battery2)
    lcd.custom_char(3,battery3)
    lcd.custom_char(4,battery4)
    lcd.custom_char(5,battery5)
    lcd.custom_char(6,battery6)
    lcd.custom_char(7,battery7)
except:
        pass  # very bad idea

time.sleep(2)

def uptime():
    #modified from:https://thesmithfam.org/blog/2005/11/19/python-uptime-script/
    #Gives a human-readable uptime string
    total_seconds=time.ticks_ms() // 1000
     # Helper vars:
    MINUTE  = 60
    HOUR    = MINUTE * 60
    DAY     = HOUR * 24
 
    # Get the days, hours, etc:
    days    = int( total_seconds / DAY )
    hours   = int( ( total_seconds % DAY ) / HOUR )
    minutes = int( ( total_seconds % HOUR ) / MINUTE )
    seconds = int( total_seconds % MINUTE )
     # Build up the pretty string (like this: "N days, N hours, N minutes, N seconds")
    string = "up:"
    if days > 0:
        string += str(days) + "d-"
        
    string += "{:02}:{:02}:{:02}".format(hours, minutes, seconds)
    return string;

for i in range(10):
    lcd.clear()
    lcd.putstr(uptime())
    lcd.move_to(0, 1)     
    #get battery percent
    battery=machine.get_input_voltage()[1]   
    #print signal_quality
    s=str(cellular.get_signal_quality()[0])
    lcd.putstr(s)
    #signal symbol ,need to be changed
    lcd.putchar(chr(250))
    s=str(machine.get_input_voltage()[0])+"mV "+str(battery)
    lcd.putstr(s)
    #97//16= icon 6 -- 100% icon
    #20//16= icon 1 -- 16% icon
    lcd.putchar(chr(battery//16)) 
    time.sleep(1)