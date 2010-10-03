import sys
PYTHON_EXTRA_PATH_DIR='e:/system/apps/vsloop'

if PYTHON_EXTRA_PATH_DIR not in sys.path:
    sys.path.append (PYTHON_EXTRA_PATH_DIR)

import appuifw, e32
import os
import struct
import socket
import select
import traceback
import math
from cStringIO import StringIO

import graphics
from key_codes import *

from camlcm.image_t import image_t
from lcmtypes.phone_command_t import phone_command_t
from lcmtypes.phone_print_t import phone_print_t
from lcmtypes.s60_key_event_t import s60_key_event_t

CONFIG_DIR='e:/system/apps/vsloop'

def rotate (v, theta):
    sintheta, costheta = math.sin (theta), math.cos (theta)
    return (v[0]*costheta - v[1]*sintheta, v[0]*sintheta + v[1]*costheta)

def scale (v, sx, sy):
    return (sx * v[0], sy * v[1])

class VSloop:
    def __init__(self):
        self.exitflag = False
        self.canvas = None
    
        self.old_body=appuifw.app.body
        self.client = None
        self._connected = False

        # message handling variables
        self._data_buf = None
        self._data_channel = ""
        self._recv_mode = "channel"
        self._datalen_buf = ""
        self._datalen = 0
        self._data_received = 0

        self._send_q = []

        # image to show
        self._toblit = [None, None, None, None]
        self._toblit_dirty = [False, False, False, False]
        self._toblit_resized = [None, None, None, None]

        # arrow angle
        self._arrow_theta = 0

        # progress
        self._progress = 0
        
        # offscreen render buffer
        self.canvas_buf = None

        # text to display
        self._textlines = [ ]

        try:
            self.canvas=appuifw.Canvas(redraw_callback=self.redraw, 
                    event_callback=self.event, resize_callback=self.resize)
            appuifw.app.body=self.canvas
        except Exception,e:
            appuifw.note(u"Exception: %s" % (e))
            self.set_exit()
            return
        
#        appuifw.app.screen = 'full'
        
        self.update_menu ()
        
    def event(self, ev):
        """Event handler"""
        if self.client:
            msg = s60_key_event_t ()
            msg.scancode = ev["scancode"]
            msg.modifiers = ev["modifiers"]
            msg.type = ev["type"]
            msg.keycode = ev["keycode"]
            self._send_q.append (("KEY", msg.encode ()))
  
    def resize(self, newsize):
        """Resize handler"""
        if not self.canvas: return
        self.canvas_buf = graphics.Image.new (self.canvas.size, "RGB")

    def clear (self):
        self.canvas_buf.clear()
        
    def redraw(self, dirty_rect=None, id=0):
        if not self.canvas: return
        if not self.canvas_buf:
            self.canvas_buf = graphics.Image.new (self.canvas.size, "RGB")
        #self.canvas_buf.clear ()

        cw, ch = self.canvas.size

        img_vert = [(0,-2*ch/3), (0,-ch/4), (-cw/2,-ch/4), (-cw/2, -2*ch/3)]

        print "redraw id = %d (%d x %d)" % (id,cw,ch)

        # blit image
        if self._toblit_dirty[id]:
            self._toblit_resized[id] = None
            tmp_fname = "d:\\vsloop.jpg"
            file (tmp_fname, "w").write (self._toblit[id].data)
            tmpimg = graphics.Image.open (tmp_fname)

            tw, th = self._toblit[id].width, self._toblit[id].height
            if tw * ch > th * cw:
                rw = cw / 2
                rh = rw * th / tw
            else:
                rh = ch / 2
                rw = rh * tw / th
            self._toblit_resized[id] = tmpimg.resize ((rw, rh))
            del tmpimg
            self._toblit_dirty[id] = False

        if self._toblit_resized[id]:
            self.canvas_buf.blit (self._toblit_resized[id], img_vert[id])

        # draw arrow?
        xoff = cw/2
        yoff = cw/8
        scal = 30
        
        verts = [[-0.3, 0.8],
                 [-0.3, -0.2],
                 [-0.6, -0.2],
                 [0, -0.9],
                 [0.6, -0.2],
                 [0.3, -0.2],
                 [0.3, 0.8]]
        r = [ rotate (v, self._arrow_theta * math.pi/180) for v in verts ]
        s = [ scale (v, scal, scal) for v in r ]
        t = [ (v[0] + xoff, v[1] + yoff) for v in s ]

        self.canvas_buf.polygon (t, fill = 0x00ff00)

        # divider
        self.canvas_buf.line ((0,ch-30,cw,ch-30), fill=0x0, outline=0x0, width=1)

        # draw text
        t = [ (0,ch-30), (cw, ch-30), (cw,ch), (0,ch)]
        self.canvas_buf.polygon (t, fill = 0xffffff)
        y = ch - 10
        xoff = 10
        vspace = 2
        lastline = 0
        font = u"Series 60 Sans TitleSmBd"
        for t in self._textlines:
            t = unicode (t)
            bb = self.canvas_buf.measure_text (t, font=font)[0]
            h = max ((bb[3] - bb[1]) + vspace, 20)
            if y - h  < ch/3: break
            self.canvas_buf.text ((xoff, y), t, font=font)
            y -= h
            lastline += 1
            break
        
        while len (self._textlines) > lastline:
            self._textlines.pop(-1)

        # draw progress
        yoff = ch/2+30
        xoff = cw/6
        boff = 8
        self.canvas_buf.line ((xoff,yoff,cw-xoff,yoff), fill=0x0, outline=0x0, width=2)
        t = [ (xoff-boff,yoff-boff) , (xoff+boff,yoff-boff),
              (xoff+boff,yoff+boff) , (xoff-boff,yoff+boff)]
        self.canvas_buf.polygon (t, fill = 0x000000)
        xoff = 5*cw/6
        t = [ (xoff-boff,yoff-boff) , (xoff+boff,yoff-boff),
              (xoff+boff,yoff+boff) , (xoff-boff,yoff+boff)]
        self.canvas_buf.polygon (t, fill = 0x000000)
        xoff = cw/6+self._progress/100.0*4*cw/6
        t = [ (xoff-boff,yoff-boff) , (xoff+boff,yoff-boff),
              (xoff+boff,yoff+boff) , (xoff-boff,yoff+boff)]
        self.canvas_buf.polygon (t, fill = 0xff0000)
        yoff -= 15
        xoff -= 10
        self.canvas_buf.text ((xoff,yoff), unicode("%d%%" % self._progress), font=font)
        
        self.canvas.blit (self.canvas_buf)


    def set_exit(self):
        print "exiting.."
        self.exitflag = True
        self._disconnect ()

    def update_menu (self):
        if self.client:
            appuifw.app.menu = [ 
                    ( u"disconnect", self.manual_disconnect),
                    ( u"exit", self.set_exit), 
                    ( u"abort", sys.exit) ]
        else:
            appuifw.app.menu = [ 
                    ( u"connect", self.connect),
                    ( u"exit", self.set_exit), 
                    ( u"abort", sys.exit) ]

    def connect (self):
        if self.client: return
        self.client = socket.socket(socket.AF_BT,socket.SOCK_STREAM)
        CONFIG_FILE=os.path.join(CONFIG_DIR,'savedaddr.txt')
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
            addr,services=socket.bt_discover()
            port = 17
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
        try:
            self.client.connect (address)
            self._connected = True
        except socket.error:
            self.client.close ()
            self.client = None
            self._connected = False

    def _disconnect (self):
        if self.client:
            print "closing connection.."
            self.client.close ()
            self.client = None
            self._connected = False

    def _handle_PRINT (self, data):
        m = phone_print_t.decode (data)
        self._textlines.insert (0, m.text)
        self.redraw ()

    def _handle_PROGRESS (self, data):
        c = phone_progress_t.decode (data)
        self._progress = c.progress
        self.redraw ()
        
    def _handle_CMD (self, data):
        c = phone_command_t.decode (data)
        self._arrow_theta = c.theta
        self.redraw ()

    def _handle_BLIT0 (self, data):
        print "handle BLIT 0"
        self._toblit[0] = image_t.decode (data)
        self._toblit_dirty[0] = True
        self.redraw (None, 0)

    def _handle_BLIT1 (self, data):
        print "handle BLIT 1"
        self._toblit[1] = image_t.decode (data)
        self._toblit_dirty[1] = True
        self.redraw (None, 1)

    def _handle_BLIT2 (self, data):
        self._toblit[2] = image_t.decode (data)
        self._toblit_dirty[2] = True
        self.redraw (None, 2)

    def _handle_BLIT3 (self, data):
        self._toblit[3] = image_t.decode (data)
        self._toblit_dirty[3] = True
        self.redraw (None, 3)

    def _handle_CLEAR (self, data):
        self.clear ()
        
    def _handle_chunk (self, msg):
        msglen = len (msg)
        if msglen == 0: return

        if self._recv_mode == "channel":
            s = msg.split ("\0", 1)
            self._data_channel = self._data_channel + s[0]
            if (len (s) > 1):
                self._recv_mode = "datalen"
                self._datalen_buf = ""
                self._handle_chunk (s[1])
        elif self._recv_mode == "datalen":
            toadd = min (msglen, 4 - len (self._datalen_buf))
            self._datalen_buf = self._datalen_buf + msg[:toadd]
            if len (self._datalen_buf) == 4:
                self._datalen = struct.unpack (">I", self._datalen_buf)[0]
                self._recv_mode = "data"
                self._data_buf = StringIO ()
                self._data_received = 0
                self._datalen_buf = ""
                if msglen > toadd:
                    self._handle_chunk (msg[toadd:])
        elif self._recv_mode == "data":
            toadd = min (msglen, self._datalen - self._data_received)
            if toadd != msglen:
                self._data_buf.write (msg[:toadd])
            else:
                self._data_buf.write (msg)

            self._data_received += toadd
            if self._data_received == self._datalen:
                # complete message received.  handle it?
                try:
                    handler = getattr (self, "_handle_%s" % self._data_channel)
                except AttributeError:
#                    print "unrecognized channel %s" % self._data_channel
                    handler = None
                if handler:
                    handler (self._data_buf.getvalue ())
                self._data_buf.close ()
                self._data_buf = None
                self._datalen = 0
                self._data_channel = ""
                self._data_received = 0
                self._recv_mode = "channel"
                if msglen > toadd:
                    self._handle_chunk (msg[toadd:])

    def try_receive_message (self):
        if self._connected:
            try:
                rfds, wfds, efds = select.select ([self.client], [], [], 0.033)
                if not rfds or self.exitflag: return
                msg = self.client.recv (672)

                self._handle_chunk (msg)
            except socket.error, e:
                print "error! ", e
                self._disconnect ()
                self.update_menu ()
        else:
            e32.ao_sleep(0.03)

    def run (self):
        appuifw.app.exit_key_handler=self.set_exit
        e32.ao_sleep (1)
        while not self.exitflag:
            self.try_receive_message ()

            while len (self._send_q):
                channel, data = self._send_q.pop ()
                if self.client:
                    tosend = "".join ([ channel, "\0", 
                        struct.pack (">I", len(data)), data])
                    self.client.sendall (tosend)

        del self.canvas
        appuifw.app.body = self.old_body
        appuifw.app.exit_key_handler = None
        self._disconnect ()
    
appuifw.app.screen='full'
#print appuifw.available_fonts ()
try:
    app=VSloop()
except Exception,e:
    appuifw.note(u'Exception: %s' % (e))
else:
    app.run()
    del app
    print "all done.."
