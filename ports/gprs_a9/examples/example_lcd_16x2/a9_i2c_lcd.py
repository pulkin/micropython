"""Implements a HD44780 character LCD connected via PCF8574 on I2C.
   this file is ported from micropython esp8622 lcd: https://github.com/dhylands/python_lcd"""
from lcd_api import LcdApi
import i2c
from time import sleep_ms

# Timings (should work)
CMD_TIMEOUT = 20

# Defines shifts or masks for the various LCD line attached to the PCF8574
MASK_RS = 0x01
MASK_RW = 0x02
MASK_E = 0x04
SHIFT_BACKLIGHT = 3
SHIFT_DATA = 4


class I2cLcd(LcdApi):
    """Implements a HD44780 character LCD connected via PCF8574 on I2C."""
    def __init__(self, i2c, i2c_addr,i2c_id, num_lines, num_columns):
        self.i2c_id=i2c_id
        self.i2c = i2c
        self.i2c_addr = i2c_addr
        self.i2c.transmit(self.i2c_id,self.i2c_addr, bytearray([0]),CMD_TIMEOUT)
        sleep_ms(20)   # Allow LCD time to powerup
        # Send reset 3 times
        self.hal_write_init_nibble(self.LCD_FUNCTION_RESET)
        sleep_ms(5)    # need to delay at least 4.1 msec
        self.hal_write_init_nibble(self.LCD_FUNCTION_RESET)
        sleep_ms(1)
        self.hal_write_init_nibble(self.LCD_FUNCTION_RESET)
        sleep_ms(1)
        # Put LCD into 4 bit mode
        self.hal_write_init_nibble(self.LCD_FUNCTION)
        sleep_ms(1)
        LcdApi.__init__(self, num_lines, num_columns)
        cmd = self.LCD_FUNCTION
        if num_lines > 1:
            cmd |= self.LCD_FUNCTION_2LINES
        self.hal_write_command(cmd)

    def hal_write_init_nibble(self, nibble):
        """Writes an initialization nibble to the LCD.

        This particular function is only used during initialization.
        """
        byte = ((nibble >> 4) & 0x0f) << SHIFT_DATA
        self.i2c.transmit(self.i2c_id,self.i2c_addr, bytearray([byte | MASK_E]),CMD_TIMEOUT)
        self.i2c.transmit(self.i2c_id,self.i2c_addr, bytearray([byte]),CMD_TIMEOUT)

    def hal_backlight_on(self):
        """Allows the hal layer to turn the backlight on."""
        self.i2c.transmit(self.i2c_id,self.i2c_addr, bytearray([1 << SHIFT_BACKLIGHT]),CMD_TIMEOUT)

    def hal_backlight_off(self):
        """Allows the hal layer to turn the backlight off."""
        self.i2c.transmit(self.i2c_id,self.i2c_addr, bytearray([0]),CMD_TIMEOUT)

    def hal_write_command(self, cmd):
        """Writes a command to the LCD.

        Data is latched on the falling edge of E.
        """
        byte = ((self.backlight << SHIFT_BACKLIGHT) | (((cmd >> 4) & 0x0f) << SHIFT_DATA))
        self.i2c.transmit(self.i2c_id,self.i2c_addr, bytearray([byte | MASK_E]),CMD_TIMEOUT)
        self.i2c.transmit(self.i2c_id,self.i2c_addr, bytearray([byte]),CMD_TIMEOUT)
        byte = ((self.backlight << SHIFT_BACKLIGHT) | ((cmd & 0x0f) << SHIFT_DATA))
        self.i2c.transmit(self.i2c_id,self.i2c_addr, bytearray([byte | MASK_E]),CMD_TIMEOUT)
        self.i2c.transmit(self.i2c_id,self.i2c_addr, bytearray([byte]),CMD_TIMEOUT)
        if cmd <= 3:
            # The home and clear commands require a worst case delay of 4.1 msec
            sleep_ms(5)

    def hal_write_data(self, data):
        """Write data to the LCD."""
        byte = (MASK_RS | (self.backlight << SHIFT_BACKLIGHT) | (((data >> 4) & 0x0f) << SHIFT_DATA))
        self.i2c.transmit(self.i2c_id,self.i2c_addr, bytearray([byte | MASK_E]),CMD_TIMEOUT)
        self.i2c.transmit(self.i2c_id,self.i2c_addr, bytearray([byte]),CMD_TIMEOUT)
        byte = (MASK_RS | (self.backlight << SHIFT_BACKLIGHT) | ((data & 0x0f) << SHIFT_DATA))
        self.i2c.transmit(self.i2c_id,self.i2c_addr, bytearray([byte | MASK_E]),CMD_TIMEOUT)
        self.i2c.transmit(self.i2c_id,self.i2c_addr, bytearray([byte]),CMD_TIMEOUT)
