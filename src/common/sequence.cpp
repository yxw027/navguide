#include "sequence.h"

sequence_info read_sequence_info( const char *dname )
{
    sequence_info info;

    char fname[1024];
    
    sprintf(fname,"%s/%s", dname,"data.ini");
    
    FILE *fp = fopen( fname, "r" );
    
    if ( !fp ) {

        printlog("Failed to open file %s in read mode.", fname);
        
        return info;
    }

    char line[1024];

    while ( fgets(line,1024,fp) ) {

        char *header = strtok(line,"=");

        char *content = strtok(NULL,"=");

        if ( strstr(header,"DATE") != NULL ) {
            
            if ( sscanf(content,"%d",&info.date) != 1 ) {
                
                printlog("Failed to scan DATE info in %s", fname);
            }
        }

        if ( strstr(header,"WIDTH") != NULL ) {
            
            if ( sscanf(content,"%d",&info.width) != 1 ) {
                
                printlog("Failed to scan WIDTH info in %s", fname);
            }
        }
        
        if ( strstr(header,"HEIGHT") != NULL ) {
            
            if ( sscanf(content,"%d",&info.height) != 1 ) {
                
                printlog("Failed to scan HEIGHT info in %s", fname);
            }
        }
        
        if ( strstr(header,"FRAME RATE") != NULL ) {
            
            if ( sscanf(content,"%d",&info.frate) != 1 ) {
                
                printlog("Failed to scan FRAME RATE info in %s", fname);
            }
        }
        
        if ( strstr(header,"N_IMAGES") != NULL ) {
            
            if ( sscanf(content,"%d",&info.n_frames) != 1 ) {
                
                printlog("Failed to scan N_IMAGES info in %s", fname);
            }
        }
 
        if ( strstr(header,"CAMERA") != NULL ) {
            
            if ( strstr(content,"LADYBUG") ) {
                info.camera = LADYBUG;
            } else {
                printlog("Unknown camera type in %s.", fname);
            }
        }

        if ( strstr(header,"FILETYPE") != NULL ) {
            
            if ( strstr(content,"JPG") || strstr(content,"jpg") ) {
                info.format = EXT_JPG;
            } else if ( strstr(content,"PNG") || strstr(content,"png") ) {
                info.format = EXT_PNG;
            } else {
                printlog("Unknown file type in %s.", fname);
                info.format = EXT_NOK;
            }
        }
    }

    fclose(fp);

    info.dirname = strdup( dname );

    // guess number of sensors from camera type
    if ( info.camera == LADYBUG )
        info.n_sensors = 6;
    else
        info.n_sensors = 1;

    printlog("Sequence info:");
    printlog("# images: %d", info.n_frames);
    printlog("# sensors: %d", info.n_sensors);
    printlog("w x h: %d x %d", info.width, info.height);
    printlog("frame rate: %d hz", info.frate);
    printlog("date: %d", info.date);

    return info;
}

void clear_sequence_info( sequence_info &info ) 
{
    free( info.dirname );
    info.dirname = NULL;
    info.n_frames = 0;
    info.width = 0;
    info.height = 0;
    info.frate = 0;
    info.date = 0;
    info.n_sensors = 0;
}

void get_frame_filename( int frameid, int sensorid, 
                         sequence_info seqinfo, char *filename )
{
    if ( frameid < 0 || frameid >= seqinfo.n_frames ) {
        printlog("Error: frame id %d out of range [%d-%d]", frameid,
                 0, seqinfo.n_frames);
        return;
    }

    switch( seqinfo.format ) {
    case EXT_JPG:
        sprintf(filename,"%s/%06d-cam%d.jpg",seqinfo.dirname,frameid,sensorid);
        break;
    case EXT_PNG:
        sprintf(filename,"%s/%06d-cam%d.png",seqinfo.dirname,frameid,sensorid);
        break;
    default:
        printlog("Unknown image format!");
        break;
    }

}

