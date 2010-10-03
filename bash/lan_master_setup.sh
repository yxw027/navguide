#!/bin/bash
#
# this script setup the machine as a gateway for the internet
# assuming that other machines are connected to it through a switch
#
# this script should be run as root
#

route add -net 239.255.0.0/16 dev eth0

iptables -t nat -F
iptables -F

iptables -P INPUT ACCEPT
iptables -P FORWARD ACCEPT
iptables -P OUTPUT ACCEPT

iptables -t nat -A POSTROUTING -o eth1 -j MASQUERADE


