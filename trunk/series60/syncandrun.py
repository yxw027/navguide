# synncandrun.py
# Albert Huang <albert@csail.mit.edu>
#
# large portions of code taken from btconsole.py (c) Nokia
SYMBIAN_UID=0x02111804

import sys
import os
import os.path
import socket
import e32
import thread
import traceback

CONFIG_DIR='c:/system/apps/navguide'
MTIMES_FILE='c:/system/apps/navguide/mtimes.txt'


class socket_stdio:
    def __init__(self,sock):
        self.socket=sock
        self.history=['pr()']
        self.histidx=0
    def read(self,n=1):
        return self.socket.recv(n)
    def write(self,str):
        return self.socket.send(str.replace('\n','\r\n'))
    def readline(self,n=None):
        buffer=[]
        while 1:
            ch=self.read(1)
            if ch == '\n' or ch == '\r':   # return
                buffer.append('\n')
                self.write('\n')
                line=''.join(buffer)
                histline=line.rstrip()
                if len(histline)>0:
                    self.history.append(histline)
                    self.histidx=0
                break
            if ch == '\177' or ch == '\010': # backspace
                self.write('\010 \010') # erase character from the screen
                del buffer[-1:] # and from the buffer
            elif ch == '\004': 
                raise EOFError
            elif ch == '\020' or ch == '\016': # ctrl-p, ctrl-n
                self.histidx=(self.histidx+{'\020':-1,'\016':1}[ch]) \
                         % len(self.history)
                #erase current line from the screen
                self.write(('\010 \010'*len(buffer)))
                buffer=list(self.history[self.histidx])
                self.write(''.join(buffer))
                self.flush()
            elif ch == '\025':
                self.write(('\010 \010'*len(buffer)))
                buffer=[]
            else:
                self.write(ch)
                buffer.append(ch)
            if n and len(buffer)>=n:
                break
        return ''.join(buffer)
    def raw_input(self,prompt=""):
        self.write(prompt)
        return self.readline()
    def flush(self):
        pass


def connect(address=None):
    """Form an RFCOMM socket connection to the given address. If
    address is not given or None, query the user where to connect. The
    user is given an option to save the discovered host address and
    port to a configuration file so that connection can be done
    without discovery in the future.

    Return value: opened Bluetooth socket or None if the user cancels
    the connection.
    """
    
    # Bluetooth connection
    sock=socket.socket(socket.AF_BT,socket.SOCK_STREAM)

    if not address:
        import appuifw
        CONFIG_FILE=os.path.join(CONFIG_DIR,'syncandrun.txt')
        try:
            f=open(CONFIG_FILE,'rt')
            config=eval(f.read())
            f.close()
        except:
            config={}
    
        address=config.get('default_target','')
    
        if address:
            choice=appuifw.popup_menu([u'Default host',
                                       u'Other...'],u'Connect to:')
            if choice==1:
                address=None
            if choice==None:
                return None # popup menu was cancelled.    
        if not address:
            print "Discovering..."
            addr,services=socket.bt_discover()
            print "Discovered: %s, %s"%(addr,services)
            port = 5
            address=(addr,port)
            choice=appuifw.query(u'Set as default?','query')
            if choice:
                config['default_target']=address
                # make sure the configuration file exists.
                if not os.path.isdir(CONFIG_DIR):
                    os.makedirs(CONFIG_DIR)
                f=open(CONFIG_FILE,'wt')
                f.write(repr(config))
                f.close()
                
    print "Connecting to "+str(address)+"...",
    sock.connect(address)
    print "OK."
    return sock

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

def run(banner=None,names=None):
    """Connect to a remote host via Bluetooth and run an interactive
    console over that connection. If sys.stdout is already connected
    to a Bluetooth console instance, use that connection instead of
    starting a new one. If names is given, use that as the local
    namespace, otherwise use namespace of module __main__."""
    if names is None:
        import __main__
        names=__main__.__dict__

    try:
        f = open(MTIMES_FILE, 'rt')
        mtimes = eval(f.read())
        f.close()
    except:
        mtimes = {}

    sock=None
    try:
        sock=connect()
        if sock is None:
            print "Connection cancelled."
            return

        sockio = socket_stdio(sock)

        # receive file offerings
        while True:
            s = readline(sock).split()
            if s[0] == "OFFER":
                dest = s[1]
                mtime = int(s[2])

                if dest not in mtimes or mtimes[dest] < mtime:
                    sock.send("%s\n" % dest)
                    mtimes[dest] = mtime
            elif s[0] == "OFFERDONE":
                sock.send("REQUESTDONE\n")
                break
    
        # download updated files
        while True:
            s = readline(sock).split()
            if s[0] == "FILE":
                filename = s[1]
                size = int(s[2])

                dirname = os.path.dirname(filename)
                if not os.path.exists(dirname):
                    os.makedirs(dirname)

                try:
                    f = file(filename, "w+")
                except Exception, e:
                    sock.send("ERROR %s\n" % str(e))
                    return

                sock.send("OK\n")

                received = 0
                while received < size:
                    data = sock.recv(size - received)
                    received += len(data)
                    f.write(data)
                f.close()
                print "received %s" % filename

            elif s[0] == "RUN":
                # get the name of the python program to run
                target = s[1]
                break
            else:
                print s
                return

        # make sure the configuration file exists.
        if not os.path.isdir(CONFIG_DIR):
            os.makedirs(CONFIG_DIR)
        f=open(MTIMES_FILE,'wt')
        f.write(repr(mtimes))
        f.close()

        print "running %s" % target

        # redirect stdout and stderr (but not stdin) and run it.
        old_stdin, old_stdout, old_stderr = (sys.stdin, sys.stdout, sys.stderr)
        sys.stdin, sys.stdout, sys.stderr = (sockio, sockio, sockio)
        try:
            g = { "__builtins__" : __builtins__ , 
                  "__name__" : "__main__" , 
                  "__doc__" : None }
            execfile(target, g)
        except:
            traceback.print_exc(file=sys.stdout)
            sys.stdout.flush()
        sys.stdin, sys.stdout, sys.stderr = (old_stdin, old_stdout, old_stderr)

    finally:
        if sock: sock.close()

def main():
    """The same as execfile()ing the btconsole.py script. Set the
    application title and run run() with the default banner."""
    if e32.is_ui_thread():
        import appuifw
        old_app_title=appuifw.app.title
        appuifw.app.title=u'syncandrun'
    try:
        run()
    finally:
        if e32.is_ui_thread():
            appuifw.app.title=old_app_title

__all__=['connect','run','interact','main','run_with_redirected_io',
         'inside_btconsole']

if __name__ == "__main__":
    main()
