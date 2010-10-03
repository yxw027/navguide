#ifndef __agile_globals_h__
#define __agile_globals_h__

// file:  globals.h
// desc:  prototypes for accessing global/singleton objects -- objects that
//        will typically be created once throughout the lifespan of a program.

#include <glib.h>
#include <lcm/lcm.h>
#include <bot/bot_core.h>
#include "atrans.h"
#if 0
#include "mtrans.h"
#include "rndf.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

lcm_t * globals_get_lcm (void);
void globals_release_lcm (lcm_t *lcm);

BotConf * globals_get_config (void);
void globals_release_config (BotConf *config);

ATrans * globals_get_atrans (void);
void globals_release_atrans (ATrans *atrans);

bot_lcmgl_t * globals_get_lcmgl (const char *name, const int create_if_missing);
#if 0
bot_lcmgl_t * globals_get_lcmgl (const char *name, const int create_if_missing);

// needed to retrieve all lcmgls for switch_buffer
GHashTable *globals_get_lcmgl_hashtable (void);

#endif

#ifdef __cplusplus
}
#endif

#endif
