import machine
import time

machine.watchdog_on(1)
# Once on, watchdog expects resetting every 1 second in this case
time.sleep(.5)
machine.watchdog_reset()
time.sleep(.5)
print("Still online")
# Otherwise, it hard-resets
time.sleep(2)
print("This never prints")

