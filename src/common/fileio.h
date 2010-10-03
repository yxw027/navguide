#ifndef _FILEIO_H__
#define _FILEIO_H__

/* From standard C library */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>

/* From the STL library */
//#include <vector>
//#include <algorithm>
using namespace std;

/* From the VL libary */
//#include "vl/VLf.h"

#include "dbg.h"

//#include "simcam/model.h"

void GetFileExtension( const char *filename, char *extension );
void WriteFilePos ( const char *dirname, const char *filename, long int data );
FILE* OpenFile ( const char *dirname, const char *filename, const char *mode );
void ResetFile ( const char *dirname, const char *filename );
long int ReadFilePos( const char *dirname, const char *filename, int id );
int ScanString( const char *copy, char *name, int n, float *data );
int ScanString( const char *copy, char *name, int n, int *data );
int get_file_dir ( const char *fullname, char *dirname );
void create_dir( const char *dirname );
unsigned int get_file_size (const char *filename);
int file_exists (const char *filename);
char *get_next_filename (const char *dirname, const char *basename);
char* get_name_from_fullname (const char *filename);
char *get_next_available_filename (const char *dirname, const char *basename, const char *suffix);
int dir_exists (const char *dirname);
char *get_next_available_dirname (const char *dirname, const char *basename, 
                                  const char *prefix, const char *suffix);

int count_files (const char *dirname);
int sort_file (const char *filename, int reverse);
int remove_dir (const char *dirname);
int is_dir (const char *dirname);

struct data_file_t { 
    char **file_name;
    int *file_count;
    int nfiles;
};
int
cmpstringpinv(const void *p1, const void *p2);
int
cmpstringp(const void *p1, const void *p2);

data_file_t *data_file_t_init ();
FILE *data_file_t_open (data_file_t *d, const char *filename);
FILE *data_file_t_open (data_file_t *d, const char *filename, int reset);
void data_file_t_printf (data_file_t *d, const char *filename, const char *string, ...);

#endif
