#!/usr/bin/python

import time

import lcm
from lcmtypes.phone_print_t import phone_print_t

lc = lcm.LCM (transmit_only=True)
msg = phone_print_t ()

for i in range (61):
    msg.text = "print %d" % i
    lc.publish ("PHONE_PRINT", msg.encode ())
    time.sleep (0.1)
