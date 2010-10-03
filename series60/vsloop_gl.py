import sys
PYTHON_EXTRA_PATH_DIR='e:/system/apps/vsloop'

if PYTHON_EXTRA_PATH_DIR not in sys.path:
    sys.path.append (PYTHON_EXTRA_PATH_DIR)

print "path: %s" % sys.path

import appuifw, e32
import os
import struct
import socket
import select
from cStringIO import StringIO
from glcanvas import *
from gles import *
from key_codes import *

from camlcm.image_t import image_t
from lcmtypes.phone_command_t import phone_command_t
from lcmtypes.s60_key_event_t import s60_key_event_t

CONFIG_DIR='e:/system/apps/vsloop'

class SimpleCube:
    def __init__(self):
        """Initializes OpenGL ES, sets the vertex and color arrays and
        pointers, and selects the shading mode."""
    
        # It's best to set these before creating the GL Canvas
        self.exitflag = False
        self.render=0
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

        # Texturing
        self._toblit_dirty = [True, True, True, True]
        for i in range(4):
            self._toblit[i] = image_t ()
            self._toblit_dirty[i] = True
            self._toblit[i].width = 100
            self._toblit[i].height = 64

#        self._toblit.data = file (CONFIG_DIR + "/tmp.ppm").read ().split("\n",1)[1]
            self._toblit[i].size = len (self._toblit.data)
            self._toblit[i].data = []
            for i in range (64):
                for j in range (100):
                    r = i * 255 / 64
                    g = j * 255 / 100
                    b = 0
                    self._toblit[i].data.append (struct.pack ("BBB", r, g, b))
            self._toblit.data[i] = "".join (self._toblit[i].data)


        for i in range(4):
            self._texname[i] = None
            self._texcoords = None

        try:
            self.canvas=GLCanvas(redraw_callback=self.redraw, 
                    event_callback=self.event, resize_callback=self.resize)
            appuifw.app.body=self.canvas
        except Exception,e:
            appuifw.note(u"Exception: %s" % (e))
            self.set_exit()
            return
        self.arrow_theta = 0
        
        # binds are exactly same as with normal Canvas objects
        appuifw.app.screen = 'full'
        
        self.recv_buf = ""
        try:
            self.initgl()
        except Exception,e:
            appuifw.note(u"Exception: %s" % (e))
            self.set_exit()

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
  
    def resize(self):
        """Resize handler"""
        # This may get called before the canvas is created, so check that the
        # canvas exists
        if not self.canvas: return

        w, h = self.canvas.size
        glViewport(0, 0, w, h)
        glMatrixMode (GL_PROJECTION)
        glLoadIdentity()
        glOrthof (0, w, h, 0, -1, 1)
    
    def initgl(self):
        """Initializes OpenGL and sets up the rendering environment"""
        glClearColor (0.0, 0.0, 0.0, 0.0)
        self.resize()
        glMatrixMode (GL_MODELVIEW)
        glShadeModel (GL_FLAT) 
        glEnableClientState (GL_VERTEX_ARRAY)
        glEnableClientState(GL_TEXTURE_COORD_ARRAY)
        glEnable (GL_TEXTURE_2D)
        glTexEnvx(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE)
        # Disable mip mapping
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR )
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR )

        for i in range(4):
            self._texname[i] = glGenTextures(1)
        self.render=1
    
    arrow_vertices = array(GL_FLOAT, 3, (
        [-.5, -.4, 0],
        [.2, -.4, 0],
        [.2, .4, 0],
        [-.5, .4, 0],

        [.2, -.7, 0],
        [.6, 0, 0],
        [.2, .7, 0],
       )
       )
  
    arrow_triangles = [ \
            [0,1,2],
            [2,3,0],
            [4,5,6],
            ]

#              [1, 1], [0, 1], [0, 0]])
#    texcoords = array(GL_BYTE, 2, [
#        0,0, 0,1, 1,0, 1,1, 0,0, 0,1, 1,0, 1,1
#        ] )

    cross_vertices = array (GL_FLOAT, 2, [ [0, 0], [1, 1], [1, 0], [0, 1] ])

    tex_verts1[0] = array (GL_BYTE, 2, [ [0, 1], [0, 2], [1, 2] ])
    tex_verts2[0] = array (GL_BYTE, 2, [ [0, 1], [1, 1], [1, 2] ])

    tex_verts1[1] = array (GL_BYTE, 2, [ [1, 1], [1, 2], [2, 2] ])
    tex_verts2[1] = array (GL_BYTE, 2, [ [1, 1], [2, 1], [2, 2] ])

    tex_verts1[2] = array (GL_BYTE, 2, [ [0, 3], [0, 4], [1, 4] ])
    tex_verts2[2] = array (GL_BYTE, 2, [ [0, 3], [1, 3], [1, 4] ])

    tex_verts1[3] = array (GL_BYTE, 2, [ [1, 3], [1, 4], [2, 4] ])
    tex_verts2[3] = array (GL_BYTE, 2, [ [1, 3], [2, 3], [2, 4] ])

    tex_triangles1 = array (GL_UNSIGNED_BYTE, 3, [ [0,1,2] ])

    def draw_texture (self, id):
        cw, ch = self.canvas.size
        if self._toblit[id]:
            tw, th = self._toblit[id].width, self._toblit[id].height
            glBindTexture (GL_TEXTURE_2D, self._texname[id])

            if self._toblit_dirty[id]:
                glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, tw, th,
                        0, GL_RGB, GL_UNSIGNED_BYTE, self._toblit[id].data)

                self._toblit_dirty[id] = False

            glPushMatrix ()
            if tw * ch > th * cw:
                glScalef (cw/2, cw/4, 1)
            else:
                glScalef (ch/2, ch/4, 1)
            glColor4f (1, 1, 1, 1)
            glVertexPointerb (self.tex_verts1[id])
            glTexCoordPointerb (self.tex_verts1[id])
            glDrawElementsub (GL_TRIANGLES, self.tex_triangles1)

            glVertexPointerb (self.tex_verts2[id])
            glTexCoordPointerb (self.tex_verts2[id])
            glDrawElementsub (GL_TRIANGLES, self.tex_triangles1)

            glBindTexture (GL_TEXTURE_2D, 0)
            glPopMatrix ()
         
    def redraw(self,frame,id=0):
        if self.render == 0: return
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        glMatrixMode (GL_MODELVIEW)
        glLoadIdentity()
        
        cw, ch = self.canvas.size

#        glPushMatrix ()
#        glColor4f (0, 1, 0, 1)
#        glScalef (w, h, 1)
#        glVertexPointerf (self.cross_vertices)
#        glDrawArrays (GL_LINES, 0, 4)
#        glPopMatrix ()

        # draw text??

        # draw texture?
        draw_texture (self, id)
           
        # draw the arrow
        if self.client:
            glColor4x (0, 1 << 16, 0, 1 << 16)
        else:
            glColor4f (0.5, 0.5, 0.5, 1)
        glPushMatrix ()
        glTranslatef (3 * cw/4, ch/4, 0)
        glScalef (cw/4, ch/8, 1.0)
        glRotatef (self.arrow_theta, 0, 0, 1)
        glVertexPointerf(self.arrow_vertices)
        glDrawElementsub (GL_TRIANGLES, self.arrow_triangles)
        glPopMatrix ()

    def set_exit(self):
        self.exitflag = True
        for i in range(4):
            if self._texname[id]:
                glDeleteTextures ([self._texname[id]])
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
            print "Discovering..."
            addr,services=socket.bt_discover()
            print "Discovered: %s, %s"%(addr,services)
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
            self.client.close ()
            self.client = None
            self._connected = False

    def _handle_CMD (self, data):
        c = phone_command_t.decode (data)
        self.arrow_theta = c.theta

    def _handle_BLIT0 (self, data):
        self._toblit[0] = image_t.decode (data)
        self._toblit_dirty[0] = True

    def _handle_BLIT1 (self, data):
        self._toblit[1] = image_t.decode (data)
        self._toblit_dirty[1] = True

    def _handle_BLIT2 (self, data):
        self._toblit[2] = image_t.decode (data)
        self._toblit_dirty[2] = True

    def _handle_BLIT3 (self, data):
        self._toblit[3] = image_t.decode (data)
        self._toblit_dirty[3] = True

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
                    print "unrecognized channel %s" % self._data_channel
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
            self.canvas.drawNow()
            self.try_receive_message ()

            while len (self._send_q):
                channel, data = self._send_q.pop ()
                if self.client:
                    print "sending [%s] (%d)" % (channel, len (data))
                    tosend = "".join ([ channel, "\0", 
                        struct.pack (">I", len(data)), data])
                    self.client.sendall (tosend)

        appuifw.app.body=self.old_body
        self.canvas=None
        self.draw=None
        appuifw.app.exit_key_handler=None
        self._disconnect ()
    
appuifw.app.screen='full'
try:
    app=SimpleCube()
except Exception,e:
    appuifw.note(u'Exception: %s' % (e))
else:
    app.run()
    del app
    print "all done..."
