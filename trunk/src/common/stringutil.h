#ifndef __stringutil_h__
#define __stringutil_h__

#include <glib.h>
#include <string.h>

/** remove leading and trailing whitespace from a string **/
void strstrip( char *buf );

/** splits a string on whitespace, and writes the address of the start of 
 * each word to the words array, up to maxwords.  The last entry of words is
 * set to NULL as a sentinel.  Whitespace characters are replaced with the zero
 * byte.
 *
 * buf is modified during this operation.
 */
void strsplit( char *buf, char **words, int maxwords );
gint g_strcmp (gconstpointer a, gconstpointer b, gpointer user_data);
    
#endif
