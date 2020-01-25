import gps
gps.on()
print("Location", gps.get_location())
print("Satellites (tracked, visible)", gps.get_satellites())
gps.off()

