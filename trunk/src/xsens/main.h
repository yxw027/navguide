#ifndef _XSENS_DRIVER_H__
#define _XSENS_DRIVER_H__

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <inttypes.h>
#include <glib.h>

#include <common/timestamp.h>
#include "common/dbg.h"
#include "xsens.h"

#include <lcm/lcm.h>

typedef struct dead_reckon_t { double angle; double trans; };

int xsens_init (float gain, float mag_gain, int set_amd, lcm_t *lcm);
void xsens_write_data (int64_t utime);
void xsens_stop ();
void xsens_start (const char *filename);
void xsens_inc ();
xsens_data_t xsens_get_data ();
int xsens_read_data (const char *filename, int64_t timestamp);
int xsens_running ();
int dead_reckon (const char *filename, int64_t timestamp, dead_reckon_t *dr);
int dead_reckon_diff (const char *filename, int64_t t1, int64_t t2, dead_reckon_t *dr);
int dead_reckon_diff (int64_t t1, int64_t t2, dead_reckon_t *dr);
int xsens_terminate ();

#endif
