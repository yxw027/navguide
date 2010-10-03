#ifndef MISSION_EDIT_H
#define MISSION_EDIT_H

#include "route_definition.h"
#include "mission_parse.h"

int
route_check_consistency ( RouteNetwork *route );

int
route_insert_waypoint ( RouteNetwork *route, int sid, int lid, int wp1id, int wp2id, double lat, double lon );
int
route_remove_waypoint ( RouteNetwork *route, int sid, int lid, int wpid );
int
route_move_waypoint( RouteNetwork *route, int sid, int lid, int wpid, double lat, double lon );
int
route_remove_lane( RouteNetwork *route, int sid, int lid );
int
route_remove_segment( RouteNetwork *route, int sid );
int
route_insert_segment( RouteNetwork *route, int n, double **gps, const char *name, int lane_width, \
                      BoundaryType left_boundary, BoundaryType right_boundary);
int
route_insert_lane ( RouteNetwork *route, Segment *s, int lane_id, BoundaryType b_left, \
                    BoundaryType b_right, int lane_width, int same_direction );

int
route_remove_spot( RouteNetwork *route, Zone *zone, int spot_id ) ;
int
route_insert_spot( RouteNetwork *route, Zone *zone, double *gps1, double *gps2, int spot_width );
int
route_insert_exit (Waypoint *w1, Waypoint *w2 );
int
route_remove_exit ( Waypoint *w1, Waypoint *w2 ) ;
int
route_add_rem_stop ( Waypoint *w );
int
route_toggle_checkpoint( RouteNetwork *route, Waypoint *w );
int
same_waypoint (Waypoint *w1, Waypoint *w2 );

int
begin_in_lane( Waypoint *w );
int
end_in_lane( Waypoint *w );
Waypoint*
next_in_lane( Waypoint *w ); 
Waypoint*
prev_in_lane( Waypoint *w );
int
route_reset_speed_limits ( RouteNetwork *route );

int
route_insert_zone( RouteNetwork *route, int n, double **gps, const char *name );
int
route_remove_zone ( RouteNetwork *route, int idx );

int
route_is_checkpoint ( RouteNetwork *route, Waypoint *w ) ;

int
mission_move_down_checkpoint( Mission *mission, int idx ) ;
int
mission_move_up_checkpoint( Mission *mission, int idx ) ;
int
mission_remove_checkpoint( Mission *mission, int idx );
int
mission_add_checkpoint( Mission *mission, int id );
void
mission_add_speed_limit( Mission *mission, int id, int min_speed, int max_speed );
int
get_ppid ( Waypoint *w );
int
route_remove_obstacle( RouteNetwork *route, int index );
int
route_add_obstacle( RouteNetwork *route, Obstacle o);

int
route_remove_all_checkpoints ( RouteNetwork *route );
int
route_add_all_checkpoints ( RouteNetwork *route );

#endif
