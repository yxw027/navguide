#ifndef _DATE_H__
#define _DATE_H__

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

int get_current_day ();
int get_current_month ();
int get_current_year ();
int get_current_hour ();
int get_current_min ();
int get_current_sec ();
char *diff_time_to_str (int64_t time);

#endif
