# Micropython a9g example
# Source: https://github.com/pulkin/micropython
# Author: pulkin
# Demonstrates how to retrieve GPS coordinates from the built-in GPS module
import gps
gps.on()
print("Location", gps.get_location())
print("Satellites (tracked, visible)", gps.get_satellites())
gps.off()

