#!/usr/bin/python
import sys
import time
import struct
from cStringIO import StringIO
import Image

import bluetooth
import lcm
from botlcm.image_t import image_t
from navlcm.generic_cmd_t import generic_cmd_t

import gobject

def make_fourcc (word):
    return ord(word[0]) | (ord(word[1]) << 8) | (ord(word[2]) << 16) | \
            (ord(word[3]) << 24)

def dbg (text):
    sys.stderr.write (text)
    sys.stderr.write ("\n")

def count_bits_set (n):
    c = 0
    while n:
        c += 1 & n
        n = n >> 1
    return c

def is_power_of_two (n):
    return count_bits_set (n) == 1

class Application:
    def __init__ (self):
        self.lcm = lcm.LCM ("udpm://?ttl=1")
        self.lcm.subscribe ("PHONE_THUMB0", self.on_phone_thumb0)
        self.lcm.subscribe ("PHONE_THUMB1", self.on_phone_thumb1)
        self.lcm.subscribe ("PHONE_THUMB2", self.on_phone_thumb2)
        self.lcm.subscribe ("PHONE_THUMB3", self.on_phone_thumb3)
        self.lcm.subscribe ("PHONE_PRINT", self.on_phone_print)
        self.lcm.subscribe ("TABLET_SCRSHOT_REQUEST", self.on_scrshot_request)
        self.lcm.subscribe ("CLASS_STATE", self.on_class_state)
        self.lcm.subscribe ("SYSTEM_INFO", self.on_system_info)
        self.lcm.subscribe ("UI_MAP", self.on_ui_map)
        self.lcm.subscribe ("LOGGER_INFO", self.on_logger_info)
        self.lcm.subscribe ("IMAGE_ANNOUNCE", self.on_announce)
        self.lcm.subscribe ("FEATURES_ANNOUNCE", self.on_announce)
        self.lcm.subscribe ("MAP_LIST", self.on_map_list)
        self.lcm.subscribe ("FUTURE_DIRECTION", self.on_map_list)

        print "starting phoned..."

        self.server = bluetooth.BluetoothSocket ()
        self.server.bind (("", 17))
        self.server.listen (1)
        self.client = None
        self.client_ios = 0
        self.verbose = True

        self.thumb_to_send = [None, None, None, None]
        self.thumb_dirty = [False, False, False, False]

        # bluetooth message handling variables
        self._data_buf = None
        self._data_channel = ""
        self._recv_mode = "channel"
        self._datalen_buf = ""
        self._datalen = 0
        self._data_received = 0

    def disconnect (self):
        if not self.client: return

        self.client.close ()
        gobject.source_remove (self.client_ios)
        self.client = None
        dbg ("disconnected")

    def send_message_to_phone (self, channel, data):
        if not self.client: return

        tosend = [channel, "\0", struct.pack (">I", len(data)), data]
        print "sending [%s] (%d)" % (channel, len (data))
        try:
            self.client.sendall ("".join (tosend))
        except IOError:
            self.disconnect ()

    def send_heartbeat_cb (self):
        cmd = generic_cmd_t ();
        cmd.code = 0;
        cmd.text = "";
        self.send_message_to_phone ("HEARTBEAT", cmd.encode())
        return True

    def send_thumb_cb (self):
        self.send_thumb (0)
        self.send_thumb (1)
        self.send_thumb (2)
        self.send_thumb (3)
        return True
    
    def send_thumb (self, id):
        if not self.thumb_dirty[id]: return True

        assert self.thumb_to_send[id] is not None

        # convert image to JPEG
        jpegdata = StringIO ()
        imsz = (self.thumb_to_send[id].width, self.thumb_to_send[id].height)
        if self.thumb_to_send[id].pixelformat == make_fourcc ("RGB3"):
            im = Image.fromstring ("RGB", imsz, self.thumb_to_send[id].data)
        elif self.thumb_to_send[id].pixelformat == make_fourcc ("RGB4"):
            im = Image.fromstring ("RGBA", imsz, self.thumb_to_send[id].data)
        elif self.thumb_to_send[id].pixelformat == make_fourcc ("GREY"):
            im = Image.fromstring ("L", imsz, self.thumb_to_send[id].data)
        else:
            print "unrecognized image format"
            print "%X %X" % (self.thumb_to_send[id].pixelformat, make_fourcc ("RGB3"))
            return True
        im.save (jpegdata, "JPEG")

        self.thumb_to_send[id].data = jpegdata.getvalue ()
        self.thumb_to_send[id].size = len (self.thumb_to_send[id].data)
        self.thumb_to_send[id].pixelformat = make_fourcc ("MJPG")

        data = self.thumb_to_send[id].encode ()
        m = image_t.decode (data)
        self.send_message_to_phone ("BLIT%d" % id, data)
        self.thumb_dirty[id] = False
        self.thumb_to_send[id] = None
        return True

    def on_phone_print (self, channel, data):
        self.send_message_to_phone ("PRINT", data)

    def on_scrshot_request (self, channel, data):
        self.send_message_to_phone ("SCRSHOT_REQUEST", data)

    def on_class_state (self, channel, data):
        self.send_message_to_phone ("CLASS_STATE", data)

    def on_system_info (self, channel, data):
        self.send_message_to_phone ("SYSTEM_INFO", data)

    def on_ui_map (self, channel, data):
        print "Sending UI Map"
        self.send_message_to_phone ("UI_MAP", data)

    def on_logger_info (self, channel, data):
        self.send_message_to_phone ("LOGGER_INFO", data)

    def on_announce (self, channel, data):
        self.send_message_to_phone (channel, data)

    def on_map_list (self, channel, data):
        self.send_message_to_phone (channel, data)

    def on_phone_thumb0 (self, channel, data):
        msg = image_t.decode (data)
        self.thumb_to_send[0] = msg
        self.thumb_dirty[0] = True

    def on_phone_thumb1 (self, channel, data):
        msg = image_t.decode (data)
        self.thumb_to_send[1] = msg
        self.thumb_dirty[1] = True

    def on_phone_thumb2 (self, channel, data):
        msg = image_t.decode (data)
        self.thumb_to_send[2] = msg
        self.thumb_dirty[2] = True

    def on_phone_thumb3 (self, channel, data):
        msg = image_t.decode (data)
        self.thumb_to_send[3] = msg
        self.thumb_dirty[3] = True

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
                print "publishing %s" % (self._data_channel)
                # complete message received.  re-transmit it over LC
                self.lcm.publish (self._data_channel, self._data_buf.getvalue())

                self._data_buf.close ()
                self._data_buf = None
                self._datalen = 0
                self._data_channel = ""
                self._data_received = 0
                self._recv_mode = "channel"
                if msglen > toadd:
                    self._handle_chunk (msg[toadd:])

    def on_bluetooth_data (self, source, cond):
        try:
            self._handle_chunk (self.client.recv (672))
            return True
        except IOError:
            self.disconnect ()
            return False

    def on_incoming_connection (self, source, condition):
        self.disconnect ()
        if condition == gobject.IO_IN:
            self.client, addr = self.server.accept ()
            self.client_ios = gobject.io_add_watch (self.client, 
                    gobject.IO_IN | gobject.IO_HUP, 
                    self.on_bluetooth_data)
            print "connected"
        return True

    def run (self):
        self.mainloop = gobject.MainLoop ()
        lcm_ios = gobject.io_add_watch (self.lcm, gobject.IO_IN,
                lambda *s: self.lcm.handle () or True)
        server_ios = gobject.io_add_watch (self.server, gobject.IO_IN, 
                self.on_incoming_connection)
        gobject.timeout_add (500, self.send_thumb_cb)
        gobject.timeout_add (1000, self.send_heartbeat_cb)
        self.mainloop.run ()
        gobject.source_remove (server_ios)
        gobject.source_remove (lcm_ios)

Application ().run ()
