#!/bin/bash
#
# this script sets the maximum cstate of the laptop to 2 in order
# to avoid overflows on the 1394 buffer
#
# this script should be run as root
#
echo 2 > /sys/module/processor/parameters/max_cstate
cat /proc/acpi/processor/CPU0/power

