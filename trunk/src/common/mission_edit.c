/**
 * mission_edit.c
 *
 * Code for editing a Route Network Definition (RNDF) and a Mission Definition
 * (MDF) and placing the result inside the structure defined by
 * route-definition.h.
 *
 * The structure are read/written using the mission_parse.c methods.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "mission_edit.h"

/* check the consistency of a route */
int
route_check_consistency ( RouteNetwork *route ) 
{
    int i,j,k;

    printf("checking consistency...\n");

    for (i=0;i<route->num_segments;i++) {
        Segment *s = route->segments + i;

        if ( s->id != i+1 ) {
            printf("Error: segment %d has ID %d.\n", i, s->id );
        }

        for (j=0;j<s->num_lanes;j++) {
            
            Lane *l = s->lanes + j;

            if ( l->id != j+1 ) {
                printf("Error: lane %d has ID %d.%d.\n", j, s->id, l->id );
            }

            for (k=0;k<l->num_waypoints;k++) {
                
                Waypoint *w = l->waypoints + k;

                if ( w->id != k+1 ) {
                    printf("Error: waypoint %d has ID %d.%d.%d\n", k, s->id, l->id, w->id);
                }
            }
        }
    }

    printf("done.\n");

    return 0;
}


/* return 1 if two waypoints are the same (ids) */
int
same_waypoint (Waypoint *w1, Waypoint *w2 )
{
    if ( !w1 || !w2 )
        return 0;

    if ( w1->type != w2->type )
        return 0;
    
    if ( w1->id != w2->id )
        return 0;

    int w1_parent_parent_id = -1;
    int w2_parent_parent_id = -1;
    int w1_parent_id=-1;
    int w2_parent_id=-1;
    
    switch ( w1->type ) {
    case POINT_WAYPOINT:
        w1_parent_id = w1->parent.lane->id;
        w1_parent_parent_id = w1->parent.lane->parent->id;
        break;
    case POINT_PERIMETER:
        w1_parent_id = 0;
        w1_parent_parent_id = w1->parent.zone->id;
        break;
    case POINT_SPOT:
        w1_parent_id = w1->parent.spot->id;
        w1_parent_parent_id = w1->parent.spot->parent->id;
        break;
    default:
        fprintf(stderr, "unknown waypoint type!\n");
    }
    switch ( w2->type ) {
    case POINT_WAYPOINT:
        w2_parent_id = w2->parent.lane->id;
        w2_parent_parent_id = w2->parent.lane->parent->id;
        break;
    case POINT_PERIMETER:
        w2_parent_id = 0;
        w2_parent_parent_id = w2->parent.zone->id;
        break;
    case POINT_SPOT:
        w2_parent_id = w2->parent.spot->id;
        w2_parent_parent_id = w2->parent.spot->parent->id;
        break;
    default:
        fprintf(stderr, "unknown waypoint type!\n");
    }

    if ( w1_parent_parent_id == w2_parent_parent_id && w1_parent_id == w2_parent_id ) 
        return 1;

    return 0;
}

/* remove pointers to a given waypoint (exits, checkpoints) */
void
route_remove_pointers( RouteNetwork *route, int segment_id, int lane_id, int wp_id  )
{
    if ( !route )
        return;

    //printf("removing pointers %d.%d.%d\n", segment_id, lane_id, wp_id);

    // remove exits in the lanes
    int i,j,k,m,p;
    for (i=0;i<route->num_segments;i++) {
        Segment *s = &route->segments[i];
        for (j=0;j<s->num_lanes;j++) {
            Lane *l = &s->lanes[j];
            for (k=0;k<l->num_waypoints;k++) {
                Waypoint *w = &l->waypoints[k];
                int done = 0;
                while ( done != 1 ) {
                    done = 1;
                    
                    for (m=0;m<w->num_exits;m++) {
                        Waypoint *exit = w->exits[m];
                        
                        if ( exit->type == POINT_WAYPOINT && ( exit->parent.lane->parent->id != segment_id || 
                                                            exit->parent.lane->id != lane_id || exit->id != wp_id ) )
                            continue;
                        if ( exit->type == POINT_PERIMETER && ( exit->parent.zone->id != segment_id || 
                                                                lane_id != 0 || exit->id != wp_id ) )
                            continue;

                        if ( exit->type == POINT_SPOT )
                            continue;
                        //printf("removing exit (%d exits)...\n", w->num_exits);
                        
                        // remove the waypoint from the list of exits
                        
                        Waypoint **nexits = malloc((w->num_exits-1)*sizeof(Waypoint*));
                        int counter=0;
                        for (p=0;p<w->num_exits;p++) {
                            if ( p != m ) {
                                nexits[counter] = w->exits[p];
                                counter++;
                            }
                        }
                        
                        free( w->exits );
                        w->exits = nexits;
                        w->num_exits--;
                        done = 0;
                        //printf("removed exit %d.%d.%d (%d exits) (%d,%d,%d,%d,%d)\n", segment_id, lane_id, exit->id, w->num_exits+1, i, j, k, m, p );
                        break;
                    }
                }
            }
        }
    }
    
    printf(" # zones: %d\n", route->num_zones);
    // removing exits in the zones
    for (i=0;i<route->num_zones;i++) {
        Zone *z = &route->zones[i];
        for (j=0;j<z->num_peripoints;j++) {
            Waypoint *w = &z->peripoints[j];
            int done = 0;
            while ( done != 1 ) {
                done = 1;
                
                //printf("%d.%d.%d has %d exits\n", w->parent.zone->id, 0, w->id, w->num_exits);

                for (m=0;m<w->num_exits;m++) {
                    Waypoint *exit = w->exits[m];
                   
                    if ( exit->type == POINT_WAYPOINT && ( exit->parent.lane->parent->id != segment_id || \
                                                           exit->parent.lane->id != lane_id || exit->id != wp_id ) )
                        continue;
                    
                    if ( exit->type == POINT_PERIMETER && ( exit->parent.zone->id != segment_id || 
                                                            lane_id != 0 || exit->id != wp_id ) )
                        continue;

                    if ( exit->type == POINT_SPOT )
                        continue;
                    // remove the waypoint from the list of exits
                    
                    Waypoint **nexits = malloc((w->num_exits-1)*sizeof(Waypoint*));
                    int counter=0;
                    for (p=0;p<w->num_exits;p++) {
                        if ( p != m ) {
                            nexits[counter] = w->exits[p];
                            counter++;
                        }
                    }
                    
                    free( w->exits );
                    w->exits = nexits;
                    w->num_exits--;
                    done = 0;
                    //printf("removed exit %d.%d.%d (%d exits) (%d,%d,%d,%d,%d)\n", segment_id, lane_id, exit->id, w->num_exits+1, i, j, k, m, p );
                    break;
                }
            }
        }
    }


    printf("removing checkpoint pointers %d.%d.%d\n", segment_id, lane_id, wp_id);

    // remove the checkpoint pointers
    for (i=0;i<route->num_checkpoints;i++) {
        //printf("\t%d/%d\n",i,route->num_checkpoints);
        Waypoint *w = route->checkpoints[i].waypoint;
        
        if ( ( w->type == POINT_WAYPOINT && w->id == wp_id &&
               w->parent.lane->id == lane_id && w->parent.lane->parent->id == segment_id ) || 
             ( w->type == POINT_SPOT && w->id == wp_id &&
               w->parent.spot->id == lane_id && w->parent.spot->parent->id == segment_id ) )
            
            {
            
            int c_id = i;

            //printf("removing checkpoint %d.%d.%d\n",w->parent.lane->parent->id, w->parent.lane->id, w->id);
            printf("removing checkpoint %d\n", i);

            // remove the checkpoint
            Checkpoint * nc = malloc( (route->num_checkpoints-1)*sizeof(Checkpoint));
            int counter = 0;
            for (p=0;p<route->num_checkpoints;p++) {
                Checkpoint c = route->checkpoints[p];
                if ( p != c_id ) {
                    nc[counter] = c;
                    counter++;
                }
            }
            if ( counter != route->num_checkpoints-1 ) {
                fprintf(stderr, "error rewriting route checkpoints. wrote %d instead of %d checkpoints.\n",
                        counter, route->num_checkpoints-1 );
            }
            free(route->checkpoints);
            route->checkpoints = nc;
            route->num_checkpoints--;
            //printf("removed checkpoint %d.%d.%d\n", segment_id, lane_id, wp_id);
            break;
        }
    }              
//printf("done.\n");
}

/* update the exit and checkpoint pointers when data has been shifted */
void
route_update_pointers ( RouteNetwork *route, int segment_id, int lane_id, int wp_id, Waypoint *wn )
{
    //printf("update pointers: %d\n", wn->id);
    if ( !route )
        return;

    int i,j,k,m;

    // update exits in the lanes
    for (i=0;i<route->num_segments;i++) {
        Segment *s = &route->segments[i];
        
        for (j=0;j<s->num_lanes;j++) {
            Lane *l = &s->lanes[j];
            
            for (k=0;k<l->num_waypoints;k++) {
                Waypoint *w = &l->waypoints[k];
                                
                for (m=0;m<w->num_exits;m++) {
                    Waypoint *exit = w->exits[m];
                     if ( exit->type == POINT_WAYPOINT && ( exit->parent.lane->parent->id != segment_id || \
                                                            exit->parent.lane->id != lane_id || exit->id != wp_id ) )
                        continue;
                    
                    if ( exit->type == POINT_PERIMETER && ( exit->parent.zone->id != segment_id || 
                                                            lane_id != 0 || exit->id != wp_id ) )
                        continue;
                    
                    if ( exit->type == POINT_SPOT )
                        continue;
                    
                    //if ( exit->type == POINT_WAYPOINT && exit->id == wp_id &&
                    //   exit->parent.lane->id == lane_id && exit->parent.lane->parent->id == segment_id ) {
                    
                    w->exits[m] = wn;
                    printf("exit: replaced pointer (%d) to %d.%d.%d\n", exit->type, segment_id, lane_id, wp_id);
                       
                }
            }
        }
    }

    // update exits in the zones
    for (i=0;i<route->num_zones;i++) {
        Zone *z = &route->zones[i];
        for (j=0;j<z->num_peripoints;j++) {
            Waypoint *w = &z->peripoints[j];
            for (m=0;m<w->num_exits;m++) {
                Waypoint *exit = w->exits[m];
                if ( exit->type == POINT_WAYPOINT && ( exit->parent.lane->parent->id != segment_id || \
                                                       exit->parent.lane->id != lane_id || exit->id != wp_id ) )
                    continue;
                
                if ( exit->type == POINT_PERIMETER && ( exit->parent.zone->id != segment_id || 
                                                        lane_id != 0 || exit->id != wp_id ) )
                    continue;
                
                if ( exit->type == POINT_SPOT )
                    continue;
                
                //if ( exit->type == POINT_WAYPOINT && exit->id == wp_id &&
                // exit->parent.lane->id == lane_id && exit->parent.lane->parent->id == segment_id ) {
                
                w->exits[m] = wn;
                printf("exit: replaced pointer to %d.%d.%d\n", segment_id, lane_id, wp_id);
            }
        }
    }

    // update route checkpoints
    for (i=0;i<route->num_checkpoints;i++) {
        Waypoint *w = route->checkpoints[i].waypoint;
        if ( w->type == POINT_WAYPOINT && w->id == wp_id &&
             w->parent.lane->id == lane_id && w->parent.lane->parent->id == segment_id ) {
            
            route->checkpoints[i].waypoint = wn;
            printf("checkpoint: replaced pointer %d.%d.%d\n", segment_id, lane_id, wp_id);
        }
        if ( w->type == POINT_SPOT && w->id == wp_id &&
             w->parent.spot->id == lane_id && w->parent.spot->parent->id == segment_id ) {
            
            route->checkpoints[i].waypoint = wn;
            printf("checkpoint: replaced spot pointer %d.%d.%d\n", segment_id, lane_id, wp_id);
        }
    }
}


/* insert a waypoint in the route given the segment id, lane id and two neighbor waypoint ids */
int
route_insert_waypoint ( RouteNetwork *route, int sid, int lid, int wp1id, int wp2id, double lat, \
                        double lon)
{
    if ( !route ) 
        return -1;

    // first check consistency
    if ( sid >= route->num_segments ) {
        fprintf(stderr,"invalid segment position %d\n", sid);
        return -1;
    }

    Segment *s = route->segments + sid;

    if ( lid >= s->num_lanes ) {
        fprintf(stderr, "invalid lane position %d (num_lanes = %d)\n", lid, s->num_lanes );
        return -1;
    }

    Lane *l = s->lanes + lid;

    if ( wp1id >= l->num_waypoints || wp2id >= l->num_waypoints ) {
        fprintf(stderr, "invalid waypoint positions %d %d\n", wp1id, wp2id);
        return -1;
    }

    // then realloc memory
    Waypoint *nl = malloc( (l->num_waypoints+1) * sizeof(Waypoint));

    Waypoint w;
    w.id = wp2id+1;
    w.lat = lat;
    w.lon = lon;
    w.type = POINT_WAYPOINT;
    w.is_stop = 0;
    w.num_exits = 0;
    w.exits = NULL;
    w.parent.lane = l;

    int i;
 
    for (i=l->num_waypoints;i>=0;i--) {
        Waypoint w2;
        
        if ( i < wp2id ) {
            w2 = l->waypoints[i];
            route_update_pointers( route, s->id, l->id, w2.id, nl+i);
        } else if ( i == wp2id ) {
            w2 = w; 
        } else {
            w2 = l->waypoints[i-1];
            route_update_pointers( route, s->id, l->id, w2.id, nl+i);
            w2.id = i+1;
        }
        
        nl[i] = w2;
    }
    
    // free memory
    free( l->waypoints );

    // set pointer to the new allocated space
    l->waypoints = nl;
    
    l->num_waypoints++;

    return 0;

}

/* remove a waypoint in the route given the segment id, lane id and two neighbor waypoint ids */
int
route_remove_waypoint ( RouteNetwork *route, int sid, int lid, int wpid )
{
    int i;

    if ( !route ) 
        return -1;

    // first check consistency
    if ( sid >= route->num_segments ) {
        fprintf(stderr,"invalid segment position %d\n", sid);
        return -1;
    }

    Segment *s = route->segments + sid;

    if ( lid >= s->num_lanes ) {
        fprintf(stderr, "invalid lane position %d (num_lanes = %d)\n", lid, s->num_lanes );
        return -1;
    }

    Lane *l = s->lanes + lid;

    // if the lane has only two waypoints left, don't do anything
    // we do not want to end up with lanes having less than two waypoints
    // use remove_lane instead
    if ( l->num_waypoints == 2 ) {
        printf("lane %d.%d has only two waypoints. skipping.\n", s->id, l->id);
        return 0;
    }

    Waypoint w = l->waypoints[wpid];

    if ( wpid >= l->num_waypoints  ) {
        fprintf(stderr, "invalid waypoint positions %d \n", wpid);
        return -1;
    }

   // remove the pointers to the removed waypoint
    printf("removing pointers to %d.%d.%d\n", s->id, l->id, w.id);
    route_remove_pointers( route, s->id, l->id, w.id) ;

    // realloc memory
    Waypoint *nl = malloc( (l->num_waypoints-1) * sizeof(Waypoint));

    for (i=0;i<l->num_waypoints;i++) {
           //for (i=l->num_waypoints-1;i>=0;i--) {
        Waypoint w2;
        if ( i < wpid ) {
            w2 = l->waypoints[i];
            nl[i] = w2;
             printf("[%d] processing %d\n", i, w2.id);
             route_update_pointers( route, s->id, l->id, w2.id, nl+i);
        } else if ( i == wpid ) {
            // skip it
        } else {
            w2 = l->waypoints[i];
            nl[i-1] = w2;
            printf("processing %d\n", w2.id);
            route_update_pointers( route, s->id, l->id, w2.id, nl+i-1);
            //w2.id = i;
        }
    }
        
    // free memory
    free( l->waypoints );

    // set pointer to the new allocated space
    l->waypoints = nl;
    
    l->num_waypoints--;

    // renumber the waypoints on the parent lane
    for (i=0;i<l->num_waypoints;i++) {
        l->waypoints[i].id = i+1;
    }

    return 0;

}

/* move a waypoint */
int
route_move_waypoint( RouteNetwork *route, int sid, int lid, int wpid, double lat, double lon )
{
    if ( !route )
        return -1;
    
        // first check consistency
    if ( sid >= route->num_segments ) {
        fprintf(stderr,"invalid segment position %d\n", sid);
        return -1;
    }
    
    Segment *s = route->segments + sid;
    
    if ( lid >= s->num_lanes ) {
        fprintf(stderr, "invalid lane position %d (num_lanes = %d)\n", lid, s->num_lanes );
        return -1;
    }

    Lane *l = s->lanes + lid;

    if ( wpid >= l->num_waypoints ) {
        fprintf(stderr, "invalid waypoint position %d\n", wpid);
    }

    Waypoint *w = l->waypoints + wpid;
    
    w->lat = lat;
    w->lon = lon;
    
    return 0;
}

/* remove a lane given a segment position and a lane position */
int
route_remove_lane( RouteNetwork *route, int sid, int lid )
{
    if ( !route )
        return -1;
    
    // first check consistency
    if ( sid >= route->num_segments ) {
        fprintf(stderr,"invalid segment position %d\n", sid);
        return -1;
    }
    
    Segment *s = route->segments + sid;
    
    if ( lid >= s->num_lanes ) {
        fprintf(stderr, "invalid lane position %d (num_lanes = %d)\n", lid, s->num_lanes );
        return -1;
    }

    if ( s->num_lanes == 1 ) {
        // do not remove the last lane of a segment
        // we do not want to end up with empty segments
        // use remove_segment instead
        printf("cannot remove single lane in a segment. Remove whole segment instead.\n");
        return -1;
    }

    Lane *l = s->lanes + lid;

    // allocate new space
    Lane *new_lanes = (Lane*)malloc((s->num_lanes-1)*sizeof(Lane));

    int i,j;

    // remove all the pointers to the waypoints of the lane
    //printf("%d waypoints to remove.", l->num_waypoints);

    for (i=0;i<l->num_waypoints;i++) {
        Waypoint *w = &l->waypoints[i];
        printf("removing waypoint %d.%d.%d\n", s->id, l->id, w->id);
        route_remove_pointers( route, s->id, l->id, w->id);
    }
    
    // free memory
    free( l->waypoints );

    // copy data into the new space, skipping one lane
    for (i=0;i<s->num_lanes;i++) {
        if ( i < lid ) {
            // update the lane pointers in the waypoint
            for (j=0;j<s->lanes[i].num_waypoints;j++) {
                s->lanes[i].waypoints[j].parent.lane = new_lanes + i;
            }
            new_lanes[i] = s->lanes[i];
            printf("restoring lane %d\n",i);
        } else if ( i > lid ) {
            // update the lane pointers in the waypoint
            for (j=0;j<s->lanes[i].num_waypoints;j++) {
                s->lanes[i].waypoints[j].parent.lane = new_lanes + i - 1;
            }
            new_lanes[i-1] = s->lanes[i];
        }
    }    

    // free memory
    free( s->lanes );

    // copy new memory
    s->lanes = new_lanes;

    s->num_lanes--;

    // renumber the lanes on the segment
    for (i=0;i<s->num_lanes;i++) {
        s->lanes[i].id = i+1;
    }

    return 0;
}

/* remove a segment given a segment position */
int
route_remove_segment( RouteNetwork *route, int sid )
{
    if ( !route )
        return -1;
    
    // first check consistency
    if ( sid >= route->num_segments ) {
        fprintf(stderr,"invalid segment position %d\n", sid);
        return -1;
    }
    
    // set this flag to 1 if this is the last segment
    int last_segment = 0;
    if ( route->num_segments == 1 ) {
        last_segment = 1;
    }

    Segment *s = route->segments + sid;
    
    // realloc new space in memory
    Segment *new_segments = NULL;

    if ( !last_segment ) {
        new_segments = malloc( (route->num_segments-1) * sizeof(Segment));
    }

    // remove all pointers to this segment
    int j,k;
    for (j=0;j<s->num_lanes;j++) {
        Lane *l = &s->lanes[j];
        for (k=0;k<l->num_waypoints;k++) {
            Waypoint *w = &l->waypoints[k];
            printf("removing waypoint %d.%d.%d\n", s->id, l->id, w->id);
            route_remove_pointers( route, s->id, l->id, w->id);
        }
    }

    // free the space occupied by the segment
    if (s->name)
        free(s->name);

    for (j=0;j<s->num_lanes;j++) {
        free( s->lanes[j].waypoints);
    }
    
    free( s->lanes );

    // copy the previous segments in new space
    int i;
    for (i=0;i<route->num_segments;i++) {
        if ( i < sid ) {
            new_segments[i] = route->segments[i];
            // update pointers to this segment in the lanes
            for (j=0;j<route->segments[i].num_lanes;j++) {
                route->segments[i].lanes[j].parent = new_segments + i;
            }
        } else if ( i > sid ) {
            new_segments[i-1] = route->segments[i];
            // update pointers to this segment in the lanes
            for (j=0;j<route->segments[i].num_lanes;j++) {
                route->segments[i].lanes[j].parent = new_segments + i - 1;
            }
        }
    }

    // free memory
    free( route->segments );
    
    // copy new memory
    route->segments = new_segments;

    route->num_segments--;

    // renumber all the segments on the route
    for (i=0;i<route->num_segments;i++) {
        route->segments[i].id = i+1;
    }

    return 0;
}

/* remove a zone */
int
route_remove_zone ( RouteNetwork *route, int idx )
{
    if ( !route || route->num_zones <= idx )
        return -1;

    Zone *z = route->zones + idx;

    // remove all the checkpoints
    int i;
    for (i=0;i<route->num_checkpoints;i++) {
        Waypoint *w = route->checkpoints[i].waypoint;
        int j;
        for (j=0;j<z->num_spots;j++) {
            Waypoint *w1 = z->spots[j].waypoints;
            if ( same_waypoint(w,w1) )
                route_toggle_checkpoint( route, w );
            Waypoint *w2 = z->spots[j].waypoints + 1;
            if ( same_waypoint(w,w2) )
                route_toggle_checkpoint( route, w );
        }
    }

    // remove all the spots (spots are renumbered each time)
    for (i=0;i<z->num_spots;i++) {
        printf("removing spot\n");
        route_remove_spot( route, z, 1 ); // spot ID = 1
    }

    // then remove all the exits coming out from the zone
    for (i=0;i<z->num_peripoints;i++) {
        Waypoint *w1 = z->peripoints + i;
        int j;
        for (j=0;j<w1->num_exits;j++) {
            Waypoint *w2 = w1->exits[j];
            if ( route_remove_exit( w1, w2 ) < 0 ) {
                printf( "Error removing exit during zone removal [1].");
            } else {
                printf( "Removed exit %s -> %s\n", get_waypoint_str( w1 ),
                        get_waypoint_str( w2 ) );
            }
        }
    }

    // then remove all the exits coming in to the zone
    for (i=0;i<route->num_segments;i++) {
        Segment *s = route->segments + i;
        int j;
        for (j=0;j<s->num_lanes;j++) {
            Lane *l = s->lanes + j;
            int k;
            for (k=0;k<l->num_waypoints;k++) {
                Waypoint *w1 = l->waypoints + k;
                int m;
                for (m=0;m<w1->num_exits;m++) {
                    Waypoint *w2 = w1->exits[m];
                    
                    if ( w2->type != POINT_PERIMETER )
                        continue;
                    
                    if ( w2->parent.zone->id != z->id ) 
                        continue;

                    if ( route_remove_exit( w1, w2 ) < 0 ) {
                        printf( "Error removing exit during zone removal [2].");
                    } else {
                        printf( "Removed exit %s -> %s\n", get_waypoint_str( w1 ),
                                get_waypoint_str( w2 ) );
                    }
                }
            }
        }
    }

    // finally remove the zone
    for (i=0;i<route->num_zones-1;i++) {
        if ( i >= idx ) {
            route->zones[i] = route->zones[i+1];
        }
    }

    // realloc memory
    if ( route->num_zones == 1 ) {
        free( route->zones );
        route->zones = NULL;
    } else {
        route->zones = realloc( route->zones, (route->num_zones-1)*sizeof(Zone));
    }

    route->num_zones--;

    // update pointers
    for (i=0;i<route->num_zones;i++) {
        Zone *z = route->zones + i;
        int j;
        for (j=0;j<z->num_peripoints;j++)
            z->peripoints[j].parent.zone = z;
        for (j=0;j<z->num_spots;j++)
            z->spots[j].parent = z;
    }    return 0;
}

/* add a segment given segment name and <n> waypoints and a lane width in feet */
int
route_insert_segment( RouteNetwork *route, int n, double **gps, const char *name, int lane_width, \
                      BoundaryType left_boundary, BoundaryType right_boundary)
{
    if ( !route )
        return -1;

    Segment *new_segments = malloc( (route->num_segments+1)*sizeof(Segment));
    
    int i,j;
    for (i=0;i<route->num_segments;i++) {
        new_segments[i] = route->segments[i];
        // update pointers to segment in the lanes
        for (j=0;j<route->segments[i].num_lanes;j++) 
            route->segments[i].lanes[j].parent = new_segments + i;
    }

    Segment *s = new_segments + route->num_segments;
    s->id = route->num_segments+1;
    s->parent = route;
    s->num_lanes = 1;
    s->max_speed = s->min_speed = -1;
    s->name = strdup( name );
    s->lanes = malloc(sizeof(Lane));

    // create the lane
    Lane *lane = s->lanes;
    lane->parent = s;
    lane->id = 1;
    lane->num_waypoints = n;
    lane->waypoints = malloc(n*sizeof(Waypoint));
    for (i=0;i<n;i++) {
        
        Waypoint w;
        w.lat = gps[i][0];
        w.lon = gps[i][1];
        w.type = POINT_WAYPOINT;
        w.id = i+1;
        w.is_stop = 0;
        w.num_exits = 0;
        w.exits = NULL;
        w.parent.lane = lane;
        lane->waypoints[i] = w;
    }

    lane->lane_width = lane_width;
    lane->left_boundary = left_boundary;
    lane->right_boundary = right_boundary;

    route->num_segments++;

    // free the memory

    free(route->segments);

    route->segments = new_segments;

    // shift all zone IDs by one
    for (i=0;i<route->num_zones;i++)
        route->zones[i].id++;
    return 0;
}

/* insert a zone */
int
route_insert_zone( RouteNetwork *route, int n, double **gps, const char *name )
{
    if ( !route )
        return -1;

    // determine the ID of the new zone
    int id = 0;
    int i;
    for (i=0;i<route->num_zones;i++) {
        if ( route->zones[i].id )
            id = route->zones[i].id;
    }
    for (i=0;i<route->num_segments;i++) {
        if ( route->segments[i].id > id )
            id = route->segments[i].id;
    }
    id++;

    // realloc the memory
    route->zones = realloc( route->zones, (route->num_zones+1)*sizeof(Zone));

    Zone *z = route->zones + route->num_zones;

    z->id = id;

    // copy the zone name
    z->name = strdup( name );

    z->parent = route;

    z->num_spots = 0;
    z->spots = NULL;

    z->max_speed = z->min_speed = 0;

    // copy the peripoints
    z->num_peripoints = n;

    z->peripoints = (Waypoint*)malloc( n*sizeof(Waypoint) );

    for (i=0;i<n;i++) {
        
        Waypoint *w = z->peripoints  + i;

        w->exits = NULL;
        w->num_exits = 0;
        w->lat = gps[i][0];
        w->lon = gps[i][1];
        w->is_stop = 0;
        w->parent.zone = z;
        w->id = i;
        w->type = POINT_PERIMETER;
    }
    
    route->num_zones++;

    // update pointers
    int j;
    for (i=0;i<route->num_zones;i++) {
        Zone *z = route->zones + i;
        for (j=0;j<z->num_peripoints;j++)
            z->peripoints[j].parent.zone = z;
        for (j=0;j<z->num_spots;j++)
            z->spots[j].parent = z;
    }

    return 0;
    
}

int
route_insert_lane ( RouteNetwork *route, Segment *s, int lane_id, BoundaryType b_left, \
                    BoundaryType b_right, int lane_width, int same_direction )
{
    if ( !route || !s )
        return -1;

    // check validity of lane ID
    // lane ID must be <= num_lanes+1
    if ( lane_id <= 0 || lane_id > s->num_lanes + 1 ) {
        fprintf(stderr, "Invalid lane id for insertion : %d must be [%d-%d]\n", lane_id, 1, s->num_lanes+1);
        return -1;
    }

    // find another lane to replicate the waypoints
    if ( s->num_lanes == 0 ) {
        fprintf(stderr, "Invalid segment: has 0 lanes!\n");
        return -1;
    } else {
        printf("Segment has %d lane(s).\n", s->num_lanes);
    }

    int i;

    int nlane_id;
    int higher_id = 0; // true if the new lane has a higher ID
    if ( lane_id == 1 ) {
        nlane_id = 0;
    } else {
        nlane_id = lane_id-2;
        higher_id = 1;
    }
        
    // replicate the waypoints
    // and move them eastward-southward if higher_id is true
    // westward-northward otherwise
    double delta = 0.000050;

    int num_waypoints = s->lanes[nlane_id].num_waypoints;
    printf("replicating %d waypoints.\n", num_waypoints);

    Waypoint *waypoints = malloc(num_waypoints*sizeof(Waypoint));
    for (i=0;i<num_waypoints;i++) {
        Waypoint *w1;
        if ( same_direction ) {
            w1 = &s->lanes[nlane_id].waypoints[i];
        } else {
            w1 = &s->lanes[nlane_id].waypoints[num_waypoints-1-i];
        }            
        Waypoint w;
        w.type = POINT_WAYPOINT;
        w.id = i+1;
        w.lat = w1->lat + delta * ( higher_id ? -1.0 : 1.0 );
        w.lon = w1->lon + delta * ( higher_id ? 1.0 : -1.0 );
        w.is_stop = 0;
        w.num_exits = 0;
        w.exits = NULL;
        waypoints[i] = w;
    }

    // insert the lane
    Lane *lanes = malloc( (s->num_lanes+1)*sizeof(Lane));

    // create a new lane
    Lane *new_lane = lanes + lane_id-1;
    new_lane->parent = s;
    new_lane->id = lane_id;
    new_lane->num_waypoints = num_waypoints;
    new_lane->waypoints = waypoints;
    new_lane->lane_width = lane_width;
    new_lane->left_boundary = b_left;
    new_lane->right_boundary = b_right;

    int j;
    for (i=0;i<s->num_lanes+1;i++) {
        if ( i+1 < lane_id ) {
            lanes[i] = s->lanes[i];
            // update pointers to the lane
            for (j=0;j<s->lanes[i].num_waypoints;j++) 
                lanes[i].waypoints[j].parent.lane = lanes + i;
            
        } else if ( i+1 == lane_id ) {
            for (j=0;j<num_waypoints;j++)
                lanes[i].waypoints[j].parent.lane = lanes + i;
        } else if (i+1 > lane_id) {
            lanes[i] = s->lanes[i-1];
            lanes[i].id++;
            // update pointers to the lane
            for (j=0;j<s->lanes[i-1].num_waypoints;j++) 
                lanes[i].waypoints[j].parent.lane = lanes + i;
        }
    }

    // free memory

    free( s->lanes );
    
    //return 0;

    s->lanes = lanes;
    
    s->num_lanes++;

    return 0;
    
}

/* remove a spot given zone and spot ID */
int
route_remove_spot( RouteNetwork *route, Zone *zone, int spot_id ) 
{
    if ( !route || !zone )
        return -1;

    if ( zone->num_spots < spot_id ) {
        fprintf(stderr, "Invalid spot ID\n");
        return -1;
    }

    //printf("zone name: %s\n", route->zones[0].name);

    // remove checkpoint on second waypoint of the spot
    if ( route_is_checkpoint( route, &(zone->spots[spot_id-1].waypoints[1]) )) {
        route_toggle_checkpoint( route, &(zone->spots[spot_id-1].waypoints[1]) );
    }

    //zone = route->zones;

    // alloc new memory space
    Spot *spots = malloc((zone->num_spots-1)*sizeof(Spot));
    
    int i;
    for (i=0;i<zone->num_spots;i++) {
        if ( i+1 < spot_id ) {
            spots[i] = zone->spots[i];
            // upadte pointers to the spot end point
            route_update_pointers( route, zone->id, i+1 , 1, spots[i].waypoints  );
            route_update_pointers( route, zone->id, i+1 , 2, spots[i].waypoints + 1 );
            // update pointers to the parent spot
            spots[i].waypoints[0].parent.spot = spots + i;
            spots[i].waypoints[1].parent.spot = spots + i;
        } else if ( i+1 > spot_id ) {
            spots[i-1] = zone->spots[i];
            // update pointers
            route_update_pointers( route, zone->id, i+1, 1, spots[i-1].waypoints );
            route_update_pointers( route, zone->id, i+1, 2, spots[i-1].waypoints + 1 );
            // update pointers to the parent spot
            spots[i-1].waypoints[0].parent.spot = spots + i - 1;
            spots[i-1].waypoints[1].parent.spot = spots + i - 1;
        }
    }

    // remove pointers to the deleted spot waypoints
    route_remove_pointers( route, zone->id, spot_id, 1 );
    route_remove_pointers( route, zone->id, spot_id, 2 );

    free(zone->spots);

    zone->spots = spots;
    
    zone->num_spots--;

    // renumber spot IDs
    for (i=0;i<zone->num_spots;i++)
        zone->spots[i].id = i+1;
    
    return 0;
}

/* add a spot */
int
route_insert_spot( RouteNetwork *route, Zone *zone, double *gps1, double *gps2, int spot_width )
{
    if ( !route || !zone )
        return -1;

    Spot *spots = malloc( (zone->num_spots+1)*sizeof(Spot));

    int i;
    for (i=0;i<zone->num_spots;i++) {
        spots[i] = zone->spots[i];
        // upadte pointers to the spot end point
        route_update_pointers( route, zone->id, i+1 , 1, spots[i].waypoints  );
        route_update_pointers( route, zone->id, i+1 , 2, spots[i].waypoints + 1 );
        // update pointers to the parent spot
        spots[i].waypoints[0].parent.spot = spots + i;
        spots[i].waypoints[1].parent.spot = spots + i;
    }

    Spot *new_spot = spots + zone->num_spots;

    Waypoint w1;
    w1.lat = gps1[0];
    w1.lon = gps1[1];
    w1.type = POINT_SPOT;
    w1.parent.spot = new_spot;
    w1.id = 1;
    w1.is_stop = 0;
    w1.num_exits = 0;
    w1.exits = NULL;

    Waypoint w2;
    w2.lat = gps2[0];
    w2.lon = gps2[1];
    w2.type = POINT_SPOT;
    w2.parent.spot = new_spot;
    w2.id = 2;
    w2.is_stop = 0;
    w2.num_exits = 0;
    w2.exits = NULL;
  
    new_spot->parent = zone;
    new_spot->id = zone->num_spots+1;
    new_spot->spot_width = spot_width;
    new_spot->waypoints[0] = w1;
    new_spot->waypoints[1] = w2;
    new_spot->checkpoint_id = -1;

    // add checkpoint on second waypoint
    add_checkpoint( route, route->max_checkpoint_id+1, new_spot->waypoints + 1 );

    // free memory
    free( zone->spots );

    zone->spots = spots;

    zone->num_spots++;

    return 0;
}

/* insert an exit given exit point and entry point
*/

int
route_insert_exit (Waypoint *w1, Waypoint *w2 )
{
    if ( !w1 || !w2 )
        return -1;

    int i;

    // look whether the exit already exists
    for (i=0;i<w1->num_exits;i++) {
        Waypoint *w = w1->exits[i];
        if ( same_waypoint( w, w2 ) ) {
            printf("exit already exists!\n");
            return -1;
        }
    }

    //printf("adding exit between %d and %d (%d exits already)\n", w1->id, w2->id,w1->num_exits );

    if ( w1->exits != NULL && w1->num_exits == 0 )
        printf("problem...\n");

    w1->exits = realloc( w1->exits, (w1->num_exits+1)*sizeof(Waypoint*));

    w1->exits[w1->num_exits] = w2;

    w1->num_exits++;

    return 0;
}

/* remove exit given two waypoints */
int
route_remove_exit ( Waypoint *w1, Waypoint *w2 ) 
{
    if ( !w1 || !w2 )
        return -1;

    int i;
    int index = -1;
    for (i=0;i<w1->num_exits;i++) {
        if ( same_waypoint( w1->exits[i], w2 ) ) {
            index = i;
            break;
        }
    }

    if ( index < 0 ) {
        printf("exit not found!\n");
        return -1;
    }

    if ( w1->num_exits == 1 ) {
        free(w1->exits);
        w1->exits = NULL;
        w1->num_exits = 0;
        return 0;
    }

    // exit exists and will be removed
    Waypoint **new_exits = malloc( (w1->num_exits-1)*sizeof(Waypoint*));
    for (i=0;i<w1->num_exits;i++) {
        if ( i < index ) {
            new_exits[i] = w1->exits[i];
        } else if ( i > index ) {
            new_exits[i-1] = w1->exits[i];
        }
    }

    free( w1->exits );
    w1->exits = new_exits;

    w1->num_exits--;

    return 0;
}

/* add or remove a stop */
int
route_add_rem_stop ( Waypoint *w ) 
{
    if ( !w )
        return -1;
    
    w->is_stop = 1 - w->is_stop;

    return 0;
}

int
route_is_checkpoint ( RouteNetwork *route, Waypoint *w ) 
{
    int i;
    if ( !route || !w )
        return -1;

    for (i=0;i<route->num_checkpoints;i++) {
        Waypoint *w2 = route->checkpoints[i].waypoint;
        if ( same_waypoint(w,w2) ) {
            return route->checkpoints[i].id;
        }
    }
    
    return -1;
}
   
/* toggle a checkpoint */
int
route_toggle_checkpoint( RouteNetwork *route, Waypoint *w )
{
    if ( !w )
        return -1;

    // look whether the point is a checkpoint

    int i;
    int index=-1;
    for (i=0;i<route->num_checkpoints;i++) {
        Waypoint *w2 = route->checkpoints[i].waypoint;
        if ( same_waypoint(w,w2) ) {
            index = i;
            break;
        }
    }

    //printf("ok so far\n");

    // if it is not a checkpoint, make it a checkpoint
    if ( index < 0 ) {
        add_checkpoint( route, route->max_checkpoint_id+1, w );

        // if on a spot, update spot checkpoint id
        if ( w->type == POINT_SPOT) {
            w->parent.spot->checkpoint_id = route->max_checkpoint_id+1;
        }
    
        return 0;
    } else {

        //otherwise, remove the checkpoint
   
        // if on a spot, update spot checkpoint id
        if ( w->type == POINT_SPOT) {
            w->parent.spot->checkpoint_id = -1;
        }
        
        if ( route->num_checkpoints == 1 ) {
            free(route->checkpoints);
            route->checkpoints = NULL;
            route->num_checkpoints = 0;
            return 0;
        } else {

            Checkpoint *new_checkpoints = malloc( (route->num_checkpoints-1)*sizeof(Checkpoint));
            for (i=0;i<route->num_checkpoints;i++) {
                if ( i < index ) {
                    new_checkpoints[i] = route->checkpoints[i];
                } else if ( i > index ) {
                    new_checkpoints[i-1] = route->checkpoints[i];
                }
            }

            free( route->checkpoints );
            route->checkpoints = new_checkpoints;
            route->num_checkpoints--;
            return 0;
        }
    }

    return 0;
}

/* return true if the waypoint is at the beginning of the lane */

int
begin_in_lane( Waypoint *w )
{
    if ( !w )
        return 0;

    if ( w->type != POINT_WAYPOINT )
        return 0;

    if ( w->id == 1 )
        return 1;

    return 0;
}

/* return true if the waypoint is at the end of the lane */

int
end_in_lane( Waypoint *w )
{
    if ( !w )
        return 0;

    if ( w->type != POINT_WAYPOINT )
        return 0;

    if ( w->id == w->parent.lane->num_waypoints )
        return 1;

    return 0;
}

/* return next waypoint in the lane */
Waypoint*
next_in_lane( Waypoint *w ) 
{
    if ( !w || w->type != POINT_WAYPOINT )
        return NULL;

    if ( end_in_lane(w) )
        return w;

    return w->parent.lane->waypoints + w->id;
}

/* return prev waypoint in the lane */
Waypoint*
prev_in_lane( Waypoint *w ) 
{
    if ( !w || w->type != POINT_WAYPOINT )
        return NULL;

    if ( begin_in_lane(w) )
        return w;

    return w->parent.lane->waypoints + w->id-2;
}

/* add a checkpoint in a mission */
int
mission_add_checkpoint( Mission *mission, int id )
{
    if ( !mission )
        return -1;

    mission->checkpoint_ids = (int*)realloc( mission->checkpoint_ids, \
                                             (mission->num_checkpoints+1) * sizeof(int));


    mission->checkpoint_ids[mission->num_checkpoints] = id;

    mission->num_checkpoints++;

    return 0;
}

/* remove a checkpoint */
int
mission_remove_checkpoint( Mission *mission, int idx )
{
    if ( !mission || idx < 0 )
        return -1;

    int n = mission->num_checkpoints;

    if ( idx >= n )
        return -1;

    int *cs = (int*)malloc((n-1)*sizeof(int));

    int i;
    for (i=0;i<n;i++) {
        if ( i < idx ) 
            cs[i] = mission->checkpoint_ids[i];
        else if ( i > idx ) {
            cs[i-1] = mission->checkpoint_ids[i];
        }
    }

    free( mission->checkpoint_ids );
    
    mission->checkpoint_ids = cs;

    mission->num_checkpoints--;

    return 0;
}

/* move up a checkpoint in the mission file */
int
mission_move_up_checkpoint( Mission *mission, int idx ) 
{
    if ( !mission || idx < 0 )
        return -1;

    int n = mission->num_checkpoints;

    if ( idx >= n )
        return -1;

    if ( idx == 0 )
        return 0; // nothing to do

    int id = mission->checkpoint_ids[idx-1];
    mission->checkpoint_ids[idx-1] = mission->checkpoint_ids[idx];
    mission->checkpoint_ids[idx] = id;

    return 0;
}

/* move down a checkpoint in the mission file */
int
mission_move_down_checkpoint( Mission *mission, int idx ) 
{
    if ( !mission || idx < 0 )
        return -1;

    int n = mission->num_checkpoints;

    if ( idx >= n )
        return -1;

    if ( idx == n-1 )
        return 0; // nothing to do

    int id = mission->checkpoint_ids[idx+1];
    mission->checkpoint_ids[idx+1] = mission->checkpoint_ids[idx];
    mission->checkpoint_ids[idx] = id;

    return 0;
}

int
get_ppid ( Waypoint *w )
{
    int pid = -1;
    if ( w->type == POINT_WAYPOINT ) 
        pid = w->parent.lane->parent->id;
    else if ( w->type == POINT_SPOT ) 
        pid = w->parent.spot->parent->id;
    else if ( w->type == POINT_PERIMETER )
        pid = w->parent.zone->id;

    return pid;
}

void
mission_add_speed_limit( Mission *mission, int id, int min_speed, int max_speed )
{
    if ( !mission ) 
        return;

    mission->speed_limits = (Speedlimit*)realloc(mission->speed_limits, \
                                                 (mission->num_speed_limits+1)*sizeof(Speedlimit));

    Speedlimit sl;
    sl.id = id;
    sl.min_speed = min_speed;
    sl.max_speed = max_speed;

    mission->speed_limits[mission->num_speed_limits] = sl;

    mission->num_speed_limits++;
}

/* add an obstacle */
int
route_add_obstacle( RouteNetwork *route, Obstacle o)
{
    if ( !route )
        return -1;

    // determine new obstacle id
    int id = 0;
    if ( route->num_obstacles > 0 ) {
        id = route->obstacles[route->num_obstacles-1].id + 1;
    }

    o.id = id;

    // realloc
    route->obstacles = (Obstacle*)realloc( route->obstacles, (route->num_obstacles+1)*sizeof(Obstacle));

    route->obstacles[route->num_obstacles] = o;

    route->num_obstacles++;
    
    return 0;
}

/* remove an obstacle */
int
route_remove_obstacle( RouteNetwork *route, int index )
{
    if ( !route || index < 0 || index > route->num_obstacles-1)
        return -1;

    // if this is the last obstacle, clear
    if ( route->num_obstacles == 0 ) {
        free( route->obstacles );
        route->obstacles = NULL;
        route->num_obstacles = 0;
        return 0;
    }

    // otherwise, shift obstacles in the list
    int i;
    for (i=index;i<route->num_obstacles-1;i++) 
        route->obstacles[i] = route->obstacles[i+1];

    // realloc
    route->obstacles = realloc( route->obstacles, (route->num_obstacles-1)*sizeof(Obstacle));
    
    route->num_obstacles--;

    return 0;
}

/* remove all checkpoints */
int
route_remove_all_checkpoints ( RouteNetwork *route ) 
{
    if ( !route )
        return -1;
    
    free( route->checkpoints );
    
    route->num_checkpoints =  0;

    route->checkpoints = NULL;

    route->max_checkpoint_id = 0;

    return 0;
}

/* add all checkpoints */
int
route_add_all_checkpoints ( RouteNetwork *route )
{
    if ( !route )
        return -1;

    // first remove the existing checkpoints
    route_remove_all_checkpoints( route );

    // add checkpoints everywhere
    int i;
    for (i=0;i<route->num_segments;i++) {
        Segment *s = route->segments + i;
        int j;
        for (j=0;j<s->num_lanes;j++) {
            Lane *l = s->lanes + j;
            int k;
            for (k=0;k<l->num_waypoints;k++) {
                Waypoint *w = l->waypoints + k;
                
                add_checkpoint( route, route->max_checkpoint_id+1, w );
            }
        }
    }

    return 0;
}

