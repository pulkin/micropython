# Micropython a9g example
# Source: https://github.com/pulkin/micropython
# Author: pulkin
# Demonstrates how to send and receive SMS
import cellular
import time

global flag
flag = 1

def sms_handler(evt):
    global flag
    if evt == cellular.SMS_SENT:
        print("SMS sent")

    elif evt == cellular.SMS_RECEIVED:
        print("SMS received, attempting to read ...")
        ls = cellular.SMS.list()
        print(ls[-1])
        flag = 0

cellular.on_sms(sms_handler)
cellular.SMS("8800", "asd").send()

print("Doing something important ...")
while flag:
    time.sleep(1)

print("Done!")

