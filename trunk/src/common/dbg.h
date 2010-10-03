// to use:
// % export SIMCAM_DBG=all

#ifndef DBG_H
#define DBG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>

#define _NORMAL_    "\x1b[0m"
#define _BLACK_     "\x1b[30;47m"
#define _RED_       "\x1b[31;40m"
#define _GREEN_     "\x1b[32;40m"
#define _YELLOW_    "\x1b[33;40m"
#define _BLUE_      "\x1b[34;40m"
#define _MAGENTA_   "\x1b[35;40m"
#define _CYAN_      "\x1b[36;40m"
#define _WHITE_     "\x1b[37;40m"

#define _BRED_      "\x1b[1;31;40m"
#define _BGREEN_    "\x1b[1;32;40m"
#define _BYELLOW_   "\x1b[1;33;40m"
#define _BBLUE_     "\x1b[1;34;40m"
#define _BMAGENTA_  "\x1b[1;35;40m"
#define _BCYAN_     "\x1b[1;36;40m"
#define _BWHITE_    "\x1b[1;37;40m"

#define DBG_MODE(x) (1ULL << (x))

#define DBG_ALL     (~0ULL)
#define DBG_ERROR   DBG_MODE(0)
#define DBG_DEFAULT DBG_ERROR

// ================== add debugging modes here ======================

#define DBG_INFO      DBG_MODE(1) /* foo */
#define DBG_IMU       DBG_MODE(3)
#define DBG_FEATURES       DBG_MODE(4)
#define DBG_BBF       DBG_MODE(5)
#define DBG_MISSION       DBG_MODE(6)
#define DBG_DISPLAY       DBG_MODE(7)
#define DBG_CLASS       DBG_MODE(8)
#define DBG_GRABBER       DBG_MODE(9)
#define DBG_VIEWER      DBG_MODE(10)
#define DBG_AUDIO      DBG_MODE(11)
#define DBG_12      DBG_MODE(12)
#define DBG_13      DBG_MODE(13)
#define DBG_14      DBG_MODE(14)
#define DBG_15      DBG_MODE(15)
#define DBG_16      DBG_MODE(16)

/// There can be no white space in these strings

#define DBG_NAMETAB \
{ "all", DBG_ALL }, \
{ "error", DBG_ERROR }, \
{ "info", DBG_INFO }, \
{ "xsens", DBG_IMU }, \
{ "sift", DBG_FEATURES }, \
{ "bbf", DBG_BBF }, \
{ "mission", DBG_MISSION }, \
{ "display", DBG_DISPLAY }, \
{ "class",  DBG_CLASS   }, \
{ "grabber",  DBG_GRABBER }, \
{ "ui", DBG_VIEWER }, \
{ "audio", DBG_AUDIO }, \
{ "12", DBG_12 }, \
{ "13", DBG_13 }, \
{ "14", DBG_14 }, \
{ "15", DBG_15 }, \
{ "16", DBG_16 }, \
{ NULL,     0 } 

#define DBG_COLORTAB \
{ DBG_INFO, _MAGENTA_ }, \
{ DBG_ERROR, _RED_ }, \
{ DBG_IMU, _GREEN_ }, \
{ DBG_FEATURES, _YELLOW_ }, \
{ DBG_BBF, _GREEN_ }, \
{ DBG_MISSION, _BLUE_ }, \
{ DBG_DISPLAY, _MAGENTA_ }, \
{ DBG_CLASS, _CYAN_ }, \
{ DBG_GRABBER, _WHITE_ }, \
{ DBG_VIEWER, _CYAN_ }, \
{ DBG_AUDIO, _BGREEN_ }, \
{ DBG_12, _BYELLOW_ }, \
{ DBG_13, _CYAN_ }, \
{ DBG_14, _BBLUE_ }, \
{ DBG_15, _BMAGENTA_ }, \
{ DBG_16, _BWHITE_ } \

#define DBG_ENV     "NAVGUIDE_DBG"


// ===================  do not modify after this line ==================

#define LOGBUFFSIZE 10000  // in bytes
#define MAXFILESIZE_MB 500 // in MB
#define DBG_UPDATERATE 5 // in seconds

static long long dbg_modes = 0;
static short dbg_initiated = 0;
//static long dbg_count = 0;
//static char dbg_logbuffer[LOGBUFFSIZE];
//static char dbg_tmpbuffer[512];
//static char dbg_filename[512];
//static short dbg_filenameset = 0;
//static long long dbg_lastutime = 0;

typedef struct dbg_mode {
    const char *d_name;
    unsigned long long d_mode;
    const char *color;
} dbg_mode_t;

typedef struct dbg_mode_color {
    unsigned long long d_mode;
    const char *color;
} dbg_mode_color_t;


static dbg_mode_color_t dbg_colortab[] = {
    DBG_COLORTAB
};

static dbg_mode_t dbg_nametab[] = {
    DBG_NAMETAB
};

/*
static inline
void RESETLOGBUFFER ()
{
    memset (dbg_logbuffer, '\0', LOGBUFFSIZE);
    dbg_count = 0;
}

static inline
void DUMPLOGBUFFER (struct timeval tv)
{
    FILE *fp;
    // reset file if it's too big
    struct stat buf;
    int status = stat (dbg_filename, &buf);
    int reset = 0;
    if (status == 0 && buf.st_size > MAXFILESIZE_MB * 1E6)
        reset = 1;
    
    // open file
    if (reset)
        fp = fopen (dbg_filename, "wb");
    else
        fp = fopen (dbg_filename, "ab");
    if (!fp) {
        printf ("Failed to open file %s to save log.", dbg_filename);
        return;
    }

    // write to file
    size_t sz = fwrite (dbg_logbuffer, dbg_count, sizeof(char), fp);
    assert (sz >= 0);

    // flush file
    fflush (fp);

    // update utime
    dbg_lastutime = (int64_t) tv.tv_sec * 1000000 + tv.tv_usec;

    // close file
    fclose (fp);
}

static inline
void SETLOGFILENAME ()
{
    if (dbg_filenameset)
        return;
    
    pid_t pid = getpid ();
    char procfile[512];
    char tmpprocessname[1024];
    char processname[512];
    sprintf (processname, "null");
    sprintf (procfile, "/proc/%d/cmdline", pid);
    FILE *fp = fopen (procfile, "r");
    if (fp) {
        size_t sz = fscanf (fp, "%s", tmpprocessname);
        assert (sz >= 0);
        // keep only the process name
        char *tok = strtok (tmpprocessname, "/");
        sprintf (processname, "%s", tok);
        while (tok) {
            tok = strtok (NULL, "/");
            if (tok)
                sprintf (processname, "%s", tok);
        }
        fclose (fp);
    } else {
        printf ("failed to open %s", procfile);
    }
    sprintf (dbg_filename, "/tmp/%s-%d.navguide.log", processname, pid);
    dbg_filenameset = 1;
}
    */
static inline 
const char* DCOLOR(unsigned long long d_mode)
{
    dbg_mode_color_t *mode;

    for (mode = dbg_colortab; mode->d_mode != 0; mode++)
    {
        if (mode->d_mode & d_mode)
            return mode->color;
    }

    return _BWHITE_;
}


static void dbg_init()
{
    const char *dbg_env;
    dbg_initiated = 1;

    dbg_modes = DBG_DEFAULT;

    dbg_env = getenv(DBG_ENV);
    if (!dbg_env) {
        return;
    } else {
        
        //RESETLOGBUFFER ();
        
        char env[256];
        char *name;

        strncpy(env, dbg_env, sizeof(env));
        for (name = strtok(env,","); name; name = strtok(NULL, ",")) {
            int cancel;
            dbg_mode_t *mode;

            if (*name == '-') {
                cancel = 1;
                name++;
            }
            else
                cancel = 0;

            for (mode = dbg_nametab; mode->d_name != NULL; mode++)
                if (strcmp(name, mode->d_name) == 0)
                    break;
            if (mode->d_name == NULL) {
                fprintf(stderr, "Warning: Unknown debug option: "
                        "\"%s\"\n", name);
                return;
            }

            if (cancel) 
            {
                dbg_modes &= ~mode->d_mode;
            }
            else
            {
                dbg_modes = dbg_modes | mode->d_mode;    
            }

        }
    }
}

#ifndef NO_DBG

#define dbg(mode, args...) { \
    if( !dbg_initiated) dbg_init(); \
    if( dbg_modes & (mode) ) { \
        printf("%s", DCOLOR(mode));             \
        printf(args);                           \
        printf("\n");                           \
        printf(_NORMAL_);                       \
        fflush(stdout);                         \
                                                \
    }                                                                   \
}
#define dbg_active(mode) (dbg_modes & (mode))

#else

#define dbg(mode, arg) 
#define dbg_active(mode) false
#define cdbg(mode,color,dtag,arg) 

#endif

#define cprintf(color, args...) { printf(color); printf(args); \
    printf(_NORMAL_); }



#ifdef __cplusplus
}
#endif
#endif
