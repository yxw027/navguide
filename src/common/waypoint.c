#include "waypoint.h"

static const char *waypoint_status_strings[] = {
    "invalid",
    "reached",
    "en route",
    "pending",
    "unreachable",
};

const char *waypoint_status_to_string( int status ) {
    if( status < 0 || status > 4 ) return 0;
    return waypoint_status_strings[status];
}
