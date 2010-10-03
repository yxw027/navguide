#ifndef MISSION_PARSE_H
#define MISSION_PARSE_H

#include "route_definition.h"

#ifdef __cplusplus
extern "C" {
#endif

RouteNetwork* route_network_create ();
int parse_route_network (FILE * f, RouteNetwork *route);
void route_network_destroy ( RouteNetwork *route );

Mission *mission_create ();
int parse_mission( FILE *f, Mission *mission);
void mission_destroy();

int
link_mission_route ( Mission *mission, RouteNetwork *route );

void 
mission_printf ( Mission *mission );
void 
route_network_printf ( RouteNetwork *route );
int
add_checkpoint (RouteNetwork * route, int id, Waypoint * waypoint);
int 
mission_print ( Mission *mission, char *filename );
int 
route_network_print ( RouteNetwork *route, char *filename );

char *
get_waypoint_str (Waypoint * waypoint);
Segment *
find_segment_by_id (RouteNetwork * route, int id);
Zone *
find_zone_by_id (RouteNetwork * route, int id);
Waypoint *
find_waypoint_by_id (RouteNetwork * route, int id1, int id2, int id3);
Checkpoint *
find_checkpoint_by_id ( RouteNetwork *route, int id );
char *
get_next_line (FILE * f);

#ifdef __cplusplus
};
#endif

#endif
