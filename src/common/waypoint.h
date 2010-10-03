#ifndef __waypoint_h__
#define __waypoint_h__

#define WAYPOINT_STATUS_INVALID     0
#define WAYPOINT_STATUS_REACHED     1
#define WAYPOINT_STATUS_EN_ROUTE    2
#define WAYPOINT_STATUS_PENDING     3
#define WAYPOINT_STATUS_UNREACHABLE 4

#ifdef __cplusplus
extern "C" {
#endif

const char *waypoint_status_to_string( int status );

#ifdef __cplusplus
}
#endif

#endif
