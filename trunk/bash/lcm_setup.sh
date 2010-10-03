#!/bin/bash
#
sudo ifconfig eth0 192.168.9.14
sudo route add -net 239.255.0.0/16 dev eth0

