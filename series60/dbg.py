# $Author: albert $
# $Date: 2005/06/16 18:09:59 $
# $Id: dbg.py,v 1.1 2005/06/16 18:09:59 albert Exp $

"""
dbg is a debugging module used for easier to read debugging output.
The idea is you define a set of debugging modes by modifying 
the dbg_modes.  When you want to print out debugging/tracing output, you
invoke dbg(mode, string) where mode is the category of the output, and string
is the actual text you want to print.  the text will be displayed in the color
specified in dbg_modes.

Now the useful part is selectively choosing what debugging output to display.
This can be defined using the dbg_env_var environment variable.  If you 
decided you only want to see the output for modes 2, 3, and 4, you'd do

bash:

% export DBG="2,3,4"
% run_your_program

tcsh:

% setenv DBG 2,3,4
% run_your_program

"""

import os
import sys
import traceback

import e32

dbg_env_var = "DBG"

# ANSI escape codes for colorized text output
dbg_normal   = "\x1b[0m"
dbg_black    = "\x1b[30;47m"
dbg_black    = "\x1b[31;40m"
dbg_red      = "\x1b[31;40m"
dbg_green    = "\x1b[32;40m"
dbg_yellow   = "\x1b[33;40m"
dbg_blue     = "\x1b[34;40m"
dbg_magenta  = "\x1b[35;40m"
dbg_cyan     = "\x1b[36;40m"
dbg_white    = "\x1b[37;40m"
dbg_bred     = "\x1b[1;31;40m"
dbg_bgreen   = "\x1b[1;32;40m"
dbg_byellow  = "\x1b[1;33;40m"
dbg_bblue    = "\x1b[1;34;40m"
dbg_bmagenta = "\x1b[1;35;40m"
dbg_bcyan    = "\x1b[1;36;40m"
dbg_bwhite   = "\x1b[1;37;40m"

# the debugging modes

# "all" is a special built in mode that enables all modes

dbg_modes = { "comm" : dbg_cyan,
              "main" : dbg_red,
              "ui" : dbg_yellow,
              "sync" : dbg_green,
              "dataman" : dbg_magenta,
              "util" : dbg_bblue,
              } 


dbg_initialized = False
dbg_active = []

def dbg_init():
    global dbg_active
    global dbg_initialized
    global dbg_modes
    global __callgate

    try:
        dbg_active = os.environ[dbg_env_var].split(",")
    except KeyError:
        dbg_active = []
    if "all" in dbg_active:
        dbg_active = dbg_modes.keys()
    for item in dbg_modes.keys():
        if item not in dbg_active: dbg_active.append(item)
    for item in dbg_active:
        if item[0] == "-":
            if item[1:] in dbg_modes.keys():
                dbg_active.remove(item[1:])
            else:
                print "debug mode %s not found" % item[1:]
        elif item not in dbg_modes.keys():
            dbg_active.remove(item)
            print "debug mode %s not found" % item
            dbg_active.remove(item)
    dbg_initialized = True

    __callgate = e32.ao_callgate(__dbg)

def dbg(mode, string):
    global __callgate
    __callgate(mode, string)
    e32.ao_yield()

def dbg_exc(mode):
    global __callgate
    __callgate(mode, "".join(traceback.format_exception(sys.exc_type, \
            sys.exc_value, sys.exc_traceback)))
    e32.ao_yield()

def __dbg(mode, string):
    """If mode is an active debugging mode, then string will be written to
    stdout
    """
    global dbg_active
    global dbg_modes

    sys.stdout.flush()

    if mode not in dbg_modes:
        raise ValueError("invalid debugging mode \"%s\"" % mode)
    if mode in dbg_active:
        sys.stderr.write("%s%s: %s%s\n" % \
        (dbg_modes[mode], mode, string, dbg_normal))

dbg_init()

if __name__ == "__main__":
    print "defined:"
    for mode in dbg_modes.keys():
        print "   %s%s%s" % (dbg_modes[mode], mode, dbg_normal)
    print "active:"
    for mode in dbg_active:
        dbg(mode, "   %s" % mode)
    if len(dbg_active) == 0:
        print "   you have not activated any debugging modes."
        print "   Do this using setenv DBG or export DBG="

# $Log: dbg.py,v $
# Revision 1.1  2005/06/16 18:09:59  albert
# *** empty log message ***
#
# Revision 1.6  2005/06/02 18:39:20  albert
# added dbg_exc, changed dbg to use callgate
#
# Revision 1.5  2005/04/07 19:12:41  albert
# added log tag
#
