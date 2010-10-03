#!/usr/bin/python

import os
import os.path
import sys
import tty, termios
import select

import bluetooth

MAXCHUNKSIZE = 4096

def readline(sock):
    buffer=[]
    while 1:
        ch=sock.recv(1)
        if ch == '\n' or ch == '\r':   # return
            buffer.append('\n')
            break
        else:
            buffer.append(ch)
    return ''.join(buffer)

def sendall(sock, data):
    while len(data) > 0:
        if len(data) > MAXCHUNKSIZE: chunk = data[:MAXCHUNKSIZE]
        else: chunk = data
        s = sock.send(chunk)
        data = data[s:]

def sendline(sock, data):
    sendall(sock, "%s\n" % data)
    

if __name__ == "__main__":

    if len(sys.argv) < 2:
        print "usage: syncandrund.py <configfile>"
        print """

    convenience tool to bypass the upload-install-run cycle.
    configfile specifies which files you want uploaded to the phone, along
    with the program you want to run.  Run the syncandrun.py on the phone,
    connect to the PC running syncandrund, and the phone will automatically
    download the files and run the specified program.
"""
        sys.exit(2)

    execfile(sys.argv[1])

    server_sock = bluetooth.BluetoothSocket(bluetooth.RFCOMM)

    server_sock.bind( ("00:00:00:00:00", 5) )
    server_sock.listen(1)
    bluetooth.advertise_service (server_sock, "SyncAndRun", 
            #service_classes = [ bluetooth.SERIAL_PORT_CLASS ],
            profiles = [ bluetooth.SERIAL_PORT_PROFILE ])

    while True:
        print "waiting for new connection"
        c, address = server_sock.accept()
        print "accepted connection from %s" % str(address)

        runit = True
        try:
            for source, dest in SYNCFILES:
                s = os.stat(source)
                print "offering %s" % dest
                sendline(c, "OFFER %s %d" % (dest, s.st_mtime))

            print "sending offerdone"
            sendline(c, "OFFERDONE")
            tosend = []
            
            filename = readline(c).strip()
            while filename != "REQUESTDONE":
                print "queueing %s" % filename
                tosend.append(filename)
                filename = readline(c).strip()
        except bluetooth.BluetoothError, e:
            print e
            runit = False

        for dest in tosend:
            source = None
            for s, d in SYNCFILES:
                if d == dest: 
                    source = s
                    break
            assert source is not None

            try:
                f = file(source)
                data = f.read()
                f.close()
            except IOError, e:
                print e
                runit = False

            try:
                print "sending %s" % source
                sendline(c, "FILE %s %d" % (dest, len(data)))

                # first make sure it's okay to send the file
                prelim = readline(c)
                if prelim.strip() != "OK":
                    print prelim
                    break

                sendall(c, data)
            except bluetooth.BluetoothError, e:
                print e
                runit = False

        if runit:
            print "instructing phone to run %s" % MAINFILE
            # send the name of the file to run
            sendline(c, "RUN %s" % MAINFILE)
            print "acting as debug console..."

            # switch stdin to raw input mode so we can read it char by char
            fd = sys.stdin.fileno()
            old_settings = termios.tcgetattr(fd)
            tty.setraw(fd)

            try:
                while True:
                    readable = select.select( [ sys.stdin, c], [], [] )[0]
                    if c in readable:
                        d = c.recv(4096)
                        if len(d) == 0: break
                        sys.stdout.write( d )
                        sys.stdout.flush()
                    if sys.stdin in readable:
                        ch = sys.stdin.read(1)
                        c.send(ch)
            except bluetooth.BluetoothError: 
                pass

            # switch stdin back to line-buffered mode
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)

        c.close()
        print "---- CONNECTION CLOSED ----"
