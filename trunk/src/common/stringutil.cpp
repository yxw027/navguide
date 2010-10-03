#include <stdio.h>
#include <ctype.h>

#include "stringutil.h"

// a string comparison function usable by g_queue_sort (i.e. GCompareDataFunc style)
gint g_strcmp (gconstpointer a, gconstpointer b, gpointer user_data)
{
    char *sa = (char*)a;
    char *sb = (char*)b;

    return strcmp (sa, sb);
}

// remove leading and trailing whitespace from a string
void strstrip( char *buf )
{
    int i = 0;
    int j = 0;
    for( i = 0; buf[i] != 0 && isspace(buf[i]); i++ );
    int last_nonspace = i;

    for( j=0; buf[i+j] != 0; j++ ) {
        if( i != 0 ) {
            buf[j] = buf[i+j];
        }
        if( ! isspace( buf[i+j] ) ) {
            last_nonspace = i+j;
        }
    }
    buf[j] = 0;
    buf[last_nonspace-i] = 0;
}

void strsplit( char *buf, char **words, int maxwords )
{
    int inword = 0;
    int i;
    int wordind = 0;
    for( i=0; buf[i] != 0; i++ ) {
        if( isspace(buf[i]) ) {
            inword = 0;
            buf[i] = 0;
        } else {
            if( ! inword ) {
                words[wordind] = buf + i;
                wordind++;
                if( wordind >= maxwords ) break;
                inword = 1;
            }
        }
    }
    words[wordind] = NULL;
}

