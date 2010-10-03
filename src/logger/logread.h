#ifndef _LOGREAD_H__
#define _LOGREAD_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>

#include <glib.h>

#include <lcm/lcm.h>
#include <common/dbg.h>
#include <lcm/eventlog.h>


struct lcm_logread_t;

lcm_logread_t * 
logread_create (lcm_t * parent, const char *url);
int
logread_handle (lcm_logread_t * lr);
void
logread_destroy (lcm_logread_t *lr) ;
int
logread_seek_to_relative_time (lcm_logread_t *lr, int32_t offset);
int
logread_resume (lcm_logread_t *lr, int64_t utime);
int64_t
logread_event_time (lcm_logread_t *lr);
int
logread_offset_next_clock_time (lcm_logread_t *lr, int64_t utime);

#endif
