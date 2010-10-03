#!/usr/bin/python
import time
import bluetooth

server = bluetooth.BluetoothSocket ()
server.bind (("", 17))
server.listen (1)

print "waiting for connection..."
client, addr = server.accept ()

print "sending..."
for i in range (61):
    t = "ARROW %d\n" % (i * 6)
    client.send (t)
    print t
for i in range (61,-1,-1):
    t = "ARROW %d\n" % (i * 6)
    client.send (t)
    print t

print "okay."
time.sleep (60)
client.close ()
server.close ()
