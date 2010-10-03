#include "fileio.h"

void
GetFileExtension( const char *filename, char *ext )
{

    //char *ext = (char*)malloc(5*sizeof(char));

    char *copy = strdup( filename );

    // get the extension
    char *pch;
    pch = strtok( copy, "." );

    while ( pch != NULL ) {
        
        memset (ext, 0, strlen(ext));

        if ( strlen(pch)<5 )
            sprintf(ext,"%s",pch);

        pch = strtok(NULL, ".");
    }

    delete copy;

    //return ext;
}

void WriteFilePos ( const char *dirname, const char *filename, long int data )
{
    char fname[1024];
    sprintf(fname,"%s/%s", dirname, filename );

    FILE *f = fopen( fname, "a" );

    fprintf( f, "%ld ", data );

    fclose(f);
}

void ResetFile ( const char *dirname, const char *filename )
{
    char fname[2048];
    sprintf(fname,"%s/%s", dirname, filename );

    printf("reseting file %s\n", fname);

    FILE *f = fopen( fname, "w" );
    
    fclose(f);
}

FILE* OpenFile ( const char *dirname, const char *filename, const char *mode )
{
    char fname[1024];
    sprintf(fname,"%s/%s", dirname, filename );

    printf("opening file %s\n", fname);

    FILE *f = fopen( fname, mode );
    
    return f;
}

long int ReadFilePos( const char *dirname, const char *filename, int id )
{
    long int pos = -1;

    char fname[1024];
    sprintf(fname,"%s/%s", dirname, filename );

    FILE *f = fopen( fname, "r" );

    int counter = 0;

    while ( fscanf(f,"%ld", &pos ) == 1 ) {
        if ( counter == id ) {
            fclose(f);
            return pos;
        }
        counter++;
    }

    // this should not happen
    fclose(f);

    return pos;
}

/* scan a string */
int ScanString( const char *input, char *name, int n, float *data )
{
    char *copy = strdup( input );

    char *pch;
    pch = strtok( copy, " " );
    
    float d;
    int count = 0;
    int mode = -1;
    int ok = -1;

    while ( pch != NULL ) {
        
        if ( count == 0 && strstr(pch, name) ) {
            mode = 1;
            if ( n == 0 ) {
                ok = 0;
                break;
            }
        } 
        else if ( mode == 1 ) {
            if ( n == 0 ) {
                ok = 0;
                break;
            }
            if ( count <= n ) {
               
                if ( sscanf(pch,"%f",&d) != 1 )
                    break;
                data[count-1] = d;
                if ( count == n ) {
                    ok = 0;
                    break;
                }
            }
        }

        pch = strtok(NULL, " ");
        count++;
    }

    delete copy;

    return ok;
}

/* scan a string */
int ScanString( const char *input, char *name, int n, int *data )
{
    
    char *copy = strdup( input );

    char *pch;
    pch = strtok( copy, " " );
    
    int d;
    int count = 0;
    int mode = -1;
    int ok = -1;

    while ( pch != NULL ) {
        
        if ( count == 0 && strstr(pch, name) ) {
            
            mode = 1;
            if ( n == 0 ) {
                ok = 0;
                break;
            }
        } 
        else if ( mode == 1 ) {
            if ( n == 0 ) {
                ok = 0;
                break;
            }
            if ( count <= n ) {
               
                if ( sscanf(pch,"%d",&d) != 1 )
                    break;
                
                data[count-1] = d;
                if ( count == n ) {
                    ok = 0;
                    break;
                }
            }
        }

        pch = strtok(NULL, " ");
        count++;
    }

    delete copy;

    return ok;
}

/* extract the dirname from a full path name */
int get_file_dir ( const char *fullname, char *dirname )
{
    char *copy = strdup( fullname );

    // get the extension
    char *pch;
    pch = strrchr( copy, '/' );
    if (!pch) {
        free (copy);
        return -1;
    }

    int pos = pch - copy + 1;

    char *name = (char*)malloc((pos+1)*sizeof(char));

    strncpy( name, fullname, pos );

    name[pos] = '\0';

    sprintf(dirname, "%s", name );

    free( copy );
    free( name );

    return 0;
}

char* get_name_from_fullname (const char *filename)
{
    char *copy = strdup( filename );

    // get the extension
    char *pch;
    pch = strrchr( copy, '/' );
    if (!pch) {
        return copy;
    } else {
        free (copy);
        return strdup (pch+1);
    }

    return NULL;
}

/* return true if dir exists */
int dir_exists (const char *dirname)
{
    DIR *dir = opendir (dirname);
    if (dir != NULL) {
        closedir (dir);
        return 1;
    }

    return 0;
}

/* remove a directory, even if non empty */
int remove_dir (const char *dirname)
{
    int status = 0;
    DIR *dir = NULL;

    if (unlink (dirname) != 0) {
        status = -1;
        switch (errno) {
        case EISDIR:
            // remove elements in the subdirectory
            dir = opendir (dirname);
            if (!dir) {
                dbg (DBG_ERROR, "error opening directory %s in remove_dir", dirname);
                break;
            }
            struct dirent *dt;
            while ((dt = readdir (dir)) != NULL) {
                if (strlen (dt->d_name) == 1 && dt->d_name[0] == '.')
                    continue;
                if (strlen (dt->d_name) == 2 && strncmp ("..", dt->d_name, 2) == 0)
                    continue;
                char fullname[512];
                sprintf (fullname, "%s/%s", dirname, dt->d_name);
                remove_dir (fullname);
            }
            closedir (dir);
            status = 0;
            break;
        case EPERM:
        case EACCES:
            dbg (DBG_ERROR, "permission error unlink %s", dirname);
            break;
        case ENOTDIR:
            dbg (DBG_ERROR, "invalid path unlink %s", dirname);
            break;
        default:
            dbg (DBG_ERROR, "error %d unlink %s", errno, dirname);
            break;
        }

        // remove empty directory
        if (rmdir (dirname) != 0) {
            status = -1;
            switch (errno) {
            case EACCES:
                dbg (DBG_ERROR, "access denied on rmdir %s", dirname);
                break;
            case EPERM:
                dbg (DBG_ERROR, "permission error on rmdir %s", dirname);
                break;
            case ENOTEMPTY:
                dbg (DBG_ERROR, "calling rmdir on non-empty directory %s", dirname);
                break;
            default:
                dbg (DBG_ERROR, "unexpected error %d rmdir %s", errno, dirname);
                break;
            }
        }
    }

    return status;
}

/* create a directory if it does not exist */
void create_dir( const char *dirname )
{
    DIR *dir = opendir(dirname);
    
    if ( dir != NULL ) {
        
        // dir already exists
        closedir(dir);
        return;
    }

    // create directory
    mkdir( dirname, 0777 );

}

int is_dir (const char *dirname)
{
    DIR *dir = opendir (dirname);
    
    if (dir != NULL) {
        closedir (dir);
        return 1;
    }

    return 0;
}

int count_files (const char *dirname)
{
    DIR *dir = opendir (dirname);
    if (!dir)
        return 0;

    struct dirent *dt;

    int count = 0;
    while ((dt = readdir (dir))!=NULL)
        count++;

    closedir (dir);
    
    return count;
}

int
cmpstringp(const void *p1, const void *p2)
{
    /* The actual arguments to this function are "pointers to
       pointers to char", but strcmp(3) arguments are "pointers
       to char", hence the following cast plus dereference */
    
    return strcmp(* (char * const *) p1, * (char * const *) p2);
}

int
cmpstringpinv(const void *p1, const void *p2)
{
    /* The actual arguments to this function are "pointers to
       pointers to char", but strcmp(3) arguments are "pointers
       to char", hence the following cast plus dereference */
    
    return strcmp(* (char * const *) p2, * (char * const *) p1);
}

int sort_file (const char *filename, int reverse)
{
    FILE *fp = fopen (filename, "r");
    
    if (!fp) {
        dbg (DBG_ERROR, "failed to open %s in read mode", filename);
        return -1;
    }

    // read
    char **data = NULL;
    char buffer[256];
    int nel = 0;
    while (fgets (buffer, 256, fp) != NULL) {
        data = (char**)realloc (data, (nel+1)*sizeof(char*));
        data[nel] = strdup (buffer);
        nel++;
    }

    fclose (fp);

    // sort
    if (reverse)
        qsort (data, nel, sizeof(char*), cmpstringpinv);
    else
        qsort (data, nel, sizeof(char*), cmpstringp);

    // write
    fp = fopen (filename, "w");
    if (!fp) {
        dbg (DBG_ERROR, "failed to open %s in write mode.", filename);
        return -1;
    }
    
    for (int i=0;i<nel;i++) {
        fprintf (fp, "%s", data[i]);
    }

    fclose (fp);

    for (int i=0;i<nel;i++) free (data[i]);
    free (data);
    return 0;
}

int file_exists (const char *filename)
{
    if (strlen (filename) < 1)
        return 0;

    FILE *fp = fopen (filename, "r");
    if (fp) {
        fclose (fp);
        return 1;
    }

    return 0;
}

/* return file size in bytes */
unsigned int get_file_size (const char *filename)
{
    struct stat buf;
    
    if (stat (filename, &buf) < 0)
        return 0;
    
    return (unsigned int)buf.st_size;
}

char *get_next_filename (const char *dirname, const char *basename)
{
    char *name = (char*)malloc(1024*sizeof(char));
    int ext = 0;

    while (1) {
        memset(name, '\0', 1024);
        sprintf(name, "%s/%s.%02d", dirname, basename, ext);
        if (!file_exists (name))
            return name;
        ext++;
    }

    return NULL;
}

char *get_next_available_filename (const char *dirname, const char *basename, const char *suffix)
{
    char *name = (char*)malloc(1024*sizeof(char));
    int ext = 0;

    while (1) {
        memset(name, '\0', 1024);
        sprintf(name, "%s/%s_%02d", dirname, basename, ext);
        char logname[1024];
        sprintf (logname, "%s%s", name, suffix);
        if (!file_exists (logname))
            return name;
        ext++;
    }
    return NULL;
}

char *get_next_available_dirname (const char *dirname, const char *basename, 
                                  const char *prefix, const char *suffix)
{
    char *name = (char*)malloc(1024*sizeof(char));
    int ext = 0;

    while (1) {
        memset(name, '\0', 1024);
        sprintf(name, "%s-%02d", basename, ext);
        char logname[1024];
        sprintf (logname, "%s/%s%s%s", dirname, prefix, name, suffix);
        if (!dir_exists (logname))
            return name;
        ext++;
    }
    return NULL;
}

data_file_t *data_file_t_init ()
{
    data_file_t *d = (data_file_t*)malloc(sizeof(data_file_t));

    d->file_name = NULL;
    d->file_count = NULL;
    d->nfiles = 0;

    return d;
}

int data_file_t_index (data_file_t *d, const char *filename)
{
    for (int i=0;i<d->nfiles;i++) {
        if (strcmp (d->file_name[i], filename) == 0)
            return i;
    }
    
    return -1;
}

FILE *data_file_t_open (data_file_t *d, const char *filename, int reset)
{
    int index = data_file_t_index (d, filename);
    
    char fullname[512];
    sprintf (fullname, "%s", filename);
    FILE *fp = NULL;

    if (index < 0) {
        d->file_name = (char**)realloc (d->file_name, (d->nfiles+1)*sizeof(char*));
        d->file_count = (int*)realloc (d->file_count, (d->nfiles+1)*sizeof(int));
        d->file_name[d->nfiles] = strdup (filename);
        d->file_count[d->nfiles] = 0;
        fp = reset ? fopen (fullname, "w") : fopen (fullname, "a");
        d->nfiles++;
    } else {
        d->file_count[index]++;
        fp = fopen (fullname, "a");
    }

    return fp;
}

FILE *data_file_t_open (data_file_t *d, const char *filename)
{
    return data_file_t_open (d, filename, 1);
}

void data_file_t_printf (data_file_t *d, const char *filename, const char *string, ...)
{
    FILE *fp = data_file_t_open (d, filename);
    
    if (!fp) {
        dbg (DBG_ERROR, "Failed to open file %s in write mode", filename);
        return;
    }

    char buf[512];
    va_list arg_ptr;
    va_start (arg_ptr,string);
    vsprintf(buf,string,arg_ptr);
    va_end (arg_ptr);

    int count = d->file_count[data_file_t_index (d, filename)];

    fprintf (fp, "%d %s\n", count, buf);

    fclose (fp);
}

