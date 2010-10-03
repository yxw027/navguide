#ifndef _global_trans_h_
#define _global_trans_h_

typedef struct _GlobalTrans GlobalTrans;

GlobalTrans *
global_trans_create (lc_t * lc);
void 
global_trans_destroy( GlobalTrans *s );
int 
global_trans_new_pose (GlobalTrans * s, const pose_state_t * ps,
        const double * gps_body_position, int has_direct_heading,
        double direct_heading, double direct_heading_var);

#endif
