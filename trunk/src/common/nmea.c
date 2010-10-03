#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "nmea.h"

/** does string s2 begin with s1? **/
static
int strpcmp(const char *s1, const char *s2)
{
	return strncmp(s1, s2, strlen(s1));
}

//static double
//pdouble (char *tok, double prev)
//{
//    if (tok[0]==0)
//        return prev;
//
//    return strtod(tok, NULL);
//}

// returns 1 for north/east, -1 for south/west
static double
nsew (char a)
{
    char c = toupper(a);
    if (c=='W' || c=='S')
        return -1;
    return 1;
}

gps_nmea_t *
nmea_new (void)
{
    gps_nmea_t * gn;

    gn = malloc (sizeof (gps_nmea_t));
    memset (gn, 0, sizeof (gps_nmea_t));

    return gn;
}

void
nmea_free (gps_nmea_t * gn)
{
    free (gn);
}

#define GPS_MESSAGE_MAXLEN 512

void
nmea_state_clear (gps_state_t * gd)
{
    int i;
    gd->is_valid = 0;
    for (i = 0; i < GPS_MAX_SATS; i++)
        gd->sat_snr[i] = -1;
}

static int
nmea_handler (const char * msgtype, const nmea_t * nmea, gps_nmea_t * gn)
{
    nmea_parse (&gn->state_pending, nmea);
    if (gn->state_pending.is_valid) {
        memcpy (&gn->state, &gn->state_pending, sizeof (gps_state_t));
        nmea_state_clear (&gn->state_pending);
        if (gn->handler)
            gn->handler (gn, &gn->state, gn->userdata);
    }
    return 0;
}

int
nmea_subscribe (gps_nmea_t * gn, lc_t * lc, const char * channel,
        gps_state_handler_t handler, void * userdata)
{
    gn->handler = handler;
    gn->userdata = userdata;
    return nmea_t_lc_subscribe( lc, channel,
            (nmea_t_lc_handler_t) nmea_handler, gn );
}

int
nmea_unsubscribe (gps_nmea_t * gn, lc_t * lc, const char * channel,
        gps_state_handler_t handler, void * userdata)
{
    return nmea_t_lc_unsubscribe( lc, channel,
            (nmea_t_lc_handler_t) nmea_handler, gn );
}

void
nmea_parse (gps_state_t *gd, const nmea_t *_nmea)
{
    nmea_sentence_t n;
    memset( &n, 0, sizeof(n) );

    if( 0 != nmea_parse_sentence( _nmea->nmea, &n ) ) return;

    switch( n.type ) {
        case NMEA_TYPE_GPRMC:
            /* GPRMC is the first message from the GPS-18 for a given fix,
             * so we use it for timestamp purposes. */
            nmea_state_clear (gd);
            gd->recv_utime = _nmea->utime;
            break;
        case NMEA_TYPE_GPGGA:
            gd->utc_hours   = n.data.gpgga.utc_hours;
            gd->utc_minutes = n.data.gpgga.utc_minutes;
            gd->utc_seconds = n.data.gpgga.utc_seconds;
            gd->lat = n.data.gpgga.lat;
            gd->lon = n.data.gpgga.lon;
            gd->elev = n.data.gpgga.antenna_height;
            switch (n.data.gpgga.fix) {
                case NMEA_GGA_FIX:
                    gd->status = GPS_STATUS_LOCK;
                    break;
                case NMEA_GGA_FIX_DIFFERENTIAL:
                    gd->status = GPS_STATUS_DGPS_LOCK;
                    break;
                default: 
                    gd->status = GPS_STATUS_NO_LOCK;
                    break;
            }
            break;
        case NMEA_TYPE_GPGSV:
            {
                nmea_gpgsv_t *gsv = &n.data.gpgsv;
                int i;
                for( i=0; i<gsv->num_sats_here; i++ ) {
                    int sat = gsv->prn[i];
                    if( sat < 0 || sat >= GPS_MAX_SATS ) {
                        fprintf(stderr, "bad satellite! (%d)\n", sat);
                        continue;
                    }

                    gd->sat_elevation_b[sat] = gsv->elevation[i];
                    gd->sat_azimuth_b[sat] = gsv->azimuth[i];
                    gd->sat_snr_b[sat] = gsv->snr[i];
                }
                
                if( gsv->sentence == gsv->num_sentences ) {
                    gd->sats_visible = 0;
                    for (i = 0; i < GPS_MAX_SATS; i++) {
                        gd->sat_elevation[i] = gd->sat_elevation_b[i];
                        gd->sat_azimuth[i] = gd->sat_azimuth_b[i];
                        gd->sat_snr[i] = gd->sat_snr_b[i];
                        if (gd->sat_snr[i] > 0.001)
                            gd->sats_visible++;
                    }
                }
            }
            break;
        case NMEA_TYPE_PGRMV:
            gd->ve = n.data.pgrmv.vel_east;
            gd->vn = n.data.pgrmv.vel_north;
            gd->vu = n.data.pgrmv.vel_up;
            /* PGRMV is the last sentence we are interested in from the GPS-18
             * so we use it to mark the state as valid. */
            if (gd->recv_utime)
                gd->is_valid = 1;
            break;
        case NMEA_TYPE_PGRME:
            gd->err_horiz = n.data.pgrme.err_horizontal;
            gd->err_vert  = n.data.pgrme.err_vertical;
            gd->err_pos   = n.data.pgrme.err_position;
            break;
        case NMEA_TYPE_GPGLL:
#if 0
            gd->utc_hours   = n.data.gpgll.utc_hours;
            gd->utc_minutes = n.data.gpgll.utc_minutes;
            gd->utc_seconds = n.data.gpgll.utc_seconds;
            gd->lat = n.data.gpgll.lat;
            gd->lon = n.data.gpgll.lon;
            if( NMEA_GLL_STATUS_VALID == n.data.gpgll.status ) {
                switch( n.data.gpgll.mode ) {
                    case NMEA_GLL_MODE_AUTONOMOUS:
                        gd->status = GPS_STATUS_LOCK;
                        break;
                    case NMEA_GLL_MODE_DIFFERENTIAL:
                        gd->status = GPS_STATUS_DGPS_LOCK;
                        break;
                    default: 
                        gd->status = GPS_STATUS_NO_LOCK;
                        break;
                }
            } else {
                gd->status = GPS_STATUS_NO_LOCK;
            }
#endif
            break;
        case NMEA_TYPE_UNRECOGNIZED:
            break;
    }
}

int nmea_parse_sentence( const char *sentence, nmea_sentence_t *p )
{
    char nmea_cpy[NMEA_SENTENCE_MAXLEN+1];
    strncpy(nmea_cpy, sentence, NMEA_SENTENCE_MAXLEN);
    char *nmea;

    // get rid of any extra stuff at the beginning
    nmea = strchr(nmea_cpy, '$');
    if (nmea == NULL)
        return -1;

    // tokenize and verify checksum
    char *toks[100];
    int  ntoks = 0;
    int  pos = 0;
    int checksum = 0;
    while (nmea[pos]!=0 && ntoks < 100) {
        if (nmea[pos]=='*') {
            char *endptr = NULL;
            int tocheck = strtol( nmea+pos+1, &endptr, 16 );

            if( endptr == nmea+pos+1 || tocheck != checksum ) return -1;

            nmea[pos]=0;
            break;
        }

        if( pos > 0) checksum ^= nmea[pos];

        if (nmea[pos]==',') {
            nmea[pos] = 0;
            toks[ntoks++]=&nmea[pos+1];
        }
        pos++;
    }

    memset( p, 0, sizeof(nmea_sentence_t) );
    p->type = NMEA_TYPE_UNRECOGNIZED;

    if (!strpcmp("$GPGGA", nmea) && ntoks==14) { // GPS system fix data
        p->type = NMEA_TYPE_GPGGA;
        nmea_gpgga_t *gga = &p->data.gpgga;

        gga->utc_hours   = (toks[0][0]-'0') * 10 + (toks[0][1]-'0') * 1;
        gga->utc_minutes = (toks[0][2]-'0') * 10 + (toks[0][3]-'0') * 1;
        gga->utc_seconds = strtod(toks[0] + 4, NULL);

        gga->lat = ( (toks[1][0] - '0') * 10 + (toks[1][1] - '0') + 
            strtod(toks[1] + 2, NULL) / 60.0 ) * nsew( toks[2][0] );
        gga->lon = ( (toks[3][0] - '0') * 100 + 
                     (toks[3][1] - '0') * 10 + 
                     (toks[3][2] - '0') * 1 + 
            strtod(toks[3] + 3, NULL) / 60.0 ) * nsew( toks[4][0] );

        int qual = atoi(toks[5]);
        switch (qual)
        {
            case 1: gga->fix = NMEA_GGA_FIX; break;
            case 2: gga->fix = NMEA_GGA_FIX_DIFFERENTIAL; break;
            case 6: gga->fix = NMEA_GGA_ESTIMATED; break;
            default: 
            case 0: gga->fix = NMEA_GGA_NO_FIX; break;
        }

        gga->num_satellites = atoi( toks[6] );
        gga->horizontal_dilution = strtod( toks[7], NULL );
        gga->antenna_height = strtod( toks[8], NULL );
        gga->geoidal_height = strtod( toks[10], NULL );
    } else if (!strpcmp("$GPGSV", nmea) && ntoks>2) { // satellites in view
        p->type = NMEA_TYPE_GPGSV; 
        nmea_gpgsv_t *gsv = &p->data.gpgsv;

        gsv->num_sentences = atoi(toks[0]);
        gsv->sentence = atoi(toks[1]);
        gsv->in_view = atoi(toks[2]);

        gsv->num_sats_here = (ntoks-3)/4;
        int i;
        for( i=0; i<gsv->num_sats_here; i++ ) {
            gsv->prn[i] = atoi( toks[4*i+3] );
            gsv->elevation[i] = strtod( toks[4*i+4], NULL );
            gsv->azimuth[i] = strtod( toks[4*i+5], NULL );
            gsv->snr[i] = strtod( toks[4*i+6], NULL );
        }
    } else if (!strpcmp("$PGRMV", nmea) && ntoks >= 3) { // 3D velocity info
        p->type = NMEA_TYPE_PGRMV;
        p->data.pgrmv.vel_east  = strtod(toks[0], NULL);
        p->data.pgrmv.vel_north = strtod(toks[1], NULL);
        p->data.pgrmv.vel_up    = strtod(toks[2], NULL);
    } else if (!strpcmp("$PGRME", nmea)) { // estimated error information
        p->type = NMEA_TYPE_PGRME;
        p->data.pgrme.err_horizontal = strtod(toks[0], NULL);
        p->data.pgrme.err_vertical   = strtod(toks[1], NULL);
        p->data.pgrme.err_position   = strtod(toks[2], NULL);
    }
#if 0
    else if (!strpcmp("$GPGLL", nmea)) { // geographic position
        p->type = NMEA_TYPE_GPGLL;
        nmea_gpgll_t *gll = &p->data.gpgll;

        gll->lat = ( (toks[0][0] - '0') * 10 + (toks[0][1] - '0') + 
            strtod(toks[0] + 2, NULL) / 60.0 ) * nsew( toks[1][0] );
        gll->lon = ( (toks[2][0] - '0') * 100 + 
                     (toks[2][1] - '0') * 10 + 
                     (toks[2][2] - '0') * 1 + 
            strtod(toks[2] + 3, NULL) / 60.0 ) * nsew( toks[3][0] );

        gll->utc_hours   = (toks[4][0]-'0') * 10 + (toks[4][1]-'0') * 1;
        gll->utc_minutes = (toks[4][2]-'0') * 10 + (toks[4][3]-'0') * 1;
        gll->utc_seconds = strtod(toks[4] + 4, NULL);

        //gll->status = toks[5][0];
        //gll->mode   = toks[6][0];
#endif
    else if (!strpcmp("$GPRMC", nmea)) { // recommended minimum specific data
        p->type = NMEA_TYPE_GPRMC;
        strncpy( p->data.raw, sentence, sizeof(p->data.raw) );
    }
    else {
        p->type = NMEA_TYPE_UNRECOGNIZED;
        strncpy( p->data.raw, sentence, sizeof(p->data.raw) );
    }
//    else if (!strpcmp("$GPALM", nmea)) {  // almanac data
//    }
//    else if (!strpcmp("$GPGSA", nmea)) { // DOP and active satellites
//    }
//    else if (!strpcmp("$GPVTG", nmea)) { // track made good and ground speed
//    }
//    else if (!strpcmp("$PGRMF", nmea)) { // fix data 
//    }
//    else if (!strpcmp("$PGRMT", nmea)) { // sensor status information (GARMIN)
//    }
//    else if (!strpcmp("$PGRMB", nmea)) { // dgps beacon info
//    }
//    else if (!strpcmp("$PGRMM", nmea)) { // ??? e.g. "$PGRMM,WGS 84*06"
//    }
    return 0;
}
