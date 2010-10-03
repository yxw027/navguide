#ifndef ROUTE_QUERY_H
#define ROUTE_QUERY_H


char *
get_waypoint_str (Waypoint * waypoint);
Segment *
find_segment_by_id (RouteNetwork * route, int id);
Zone *
find_zone_by_id (RouteNetwork * route, int id);
Waypoint *
find_waypoint_by_id (RouteNetwork * route, int id1, int id2, int id3);

#endif
