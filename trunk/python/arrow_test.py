#!/usr/bin/python

import time

import lcm
from lcmtypes.phone_command_t import phone_command_t

lc = lcm.LCM (transmit_only=True)
msg = phone_command_t ()

for i in range (61):
    msg.theta = i * 6
    lc.publish ("PHONE_COMMAND", msg.encode ())
    time.sleep (0.1)

for i in range (61,-1,-1):
    msg.theta = i * 6
    lc.publish ("PHONE_COMMAND", msg.encode ())
    time.sleep (0.1)
