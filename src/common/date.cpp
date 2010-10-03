#include "date.h"

int get_current_day ()
{
    timeval tim;
    gettimeofday(&tim, NULL);
    tm *lc = localtime (&tim.tv_sec);
    return lc->tm_mday;
}

int get_current_month ()
{
    timeval tim;
    gettimeofday(&tim, NULL);
    tm *lc = localtime (&tim.tv_sec);
    return lc->tm_mon+1;
}

int get_current_year ()
{
    timeval tim;
    gettimeofday(&tim, NULL);
    tm *lc = localtime (&tim.tv_sec);
    return lc->tm_year + 1900;
}

int get_current_hour ()
{
    timeval tim;
    gettimeofday(&tim, NULL);
    tm *lc = localtime (&tim.tv_sec);
    return lc->tm_hour;
}

int get_current_min ()
{
    timeval tim;
    gettimeofday(&tim, NULL);
    tm *lc = localtime (&tim.tv_sec);
    return lc->tm_min;
}

int get_current_sec ()
{
    timeval tim;
    gettimeofday(&tim, NULL);
    tm *lc = localtime (&tim.tv_sec);
    return lc->tm_sec;
}

char *diff_time_to_str (int64_t time)
{
    time = time / 1000000;
    int hours = (int)(time / 3600);
    time -= hours * 3600;
    int mins = (int)(time / 60);
    time -= mins * 60;
    int secs = (int)(time);

    char *str = (char*)malloc(20*sizeof(char));
    sprintf (str, "%02d:%02d:%02d", hours, mins, secs);

    return str;
}

