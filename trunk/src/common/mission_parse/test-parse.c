#include <stdio.h>
#include <stdlib.h>

#include <common/mission_edit.h>
#include <common/mission_parse.h>

int main (int argc, char ** argv)
{
    FILE * f;
    RouteNetwork * route = route_network_create();

    if (argc != 3) {
        fprintf (stderr, "Usage: %s filename_RNDF filename_MDF\n", argv[0]);
        return 1;
    }

    f = fopen (argv[1], "r");
    if (!f) {
        fprintf (stderr, "Failed to open %s\n", argv[1]);
        return 1;
    }

    if (parse_route_network (f, route) < 0) {
        fclose (f);
        route_network_destroy (route);
        return 1;
    }
    fclose (f);

    // print out the route
    route_network_printf( route );

    // read a mission
    Mission *mission = mission_create();

    FILE *f2 = fopen(argv[2], "r");

    if (!f2 ) {
        fprintf(stderr, "could not find file %s\n", argv[2]);
    } else {
        if ( parse_mission( f2, mission) < 0 ) {
            fprintf(stderr, "failed to parse mission.\n");
        }
        mission_printf(mission);
    }

   // insert a waypoint
    //if ( route_insert_waypoint( route, 1, 0, 0, 1, 44.0, 55.0 ) != 0 ) {
//	    fprintf( stderr, "failed to insert point.\n");
    //}	
  //  
    // remove a waypoint
   //  if ( route_remove_waypoint( route, 10, 0, 0 ) != 0 ) {
     //   fprintf(stderr, "failed to remove point.\n");
    //}
    //
    // remove a lane
    //if ( route_remove_lane( route, 2, 0 ) != 0 ) {
//	    fprintf(stderr, "failed to remove lane.\n");
  //  }
  
    // remove a segment
   //if ( route_remove_segment( route, 10 ) != 0 ) {
//	    fprintf(stderr, "failed to remove segment.\n");
  //  }

    // insert a segment
    double **gps;
    gps = (double**)malloc(2*sizeof(double*));
    gps[0] = (double*)malloc(3*sizeof(double));
    gps[1] = (double*)malloc(3*sizeof(double));

    gps[0][0] = gps[0][1] = gps[0][2] = 0.0;
    gps[1][0] = gps[1][1] = gps[1][2] = 1.0;

    //if ( route_insert_segment( route, 2, gps, "New_Segment", 12, BOUND_BROKEN_WHITE, BOUND_DBL_YELLOW ) != 0 ) {
//	    fprintf(stderr, "failed to insert new segment.\n");
  //  }

    // insert a lane
    //if ( route_insert_lane( route, route->segments + 13, 1, BOUND_DBL_YELLOW, BOUND_BROKEN_WHITE, 14 ) != 0 ) {
//	   fprintf(stderr, "failed to add lane.\n");
  //  }

    // remove a spot
    //if ( route_remove_spot( route, route->zones, 2 ) != 0 ) {
//	    fprintf(stderr, "failed to remove spot.\n");
  //  }
	
    // insert a spot
    //if ( route_insert_spot( route, route->zones, gps[0], gps[1], 12 ) != 0 ) {
//	    	fprintf(stderr, "failed to insert spot.\n");
  //  }

    // add an exit
 //   if ( route_insert_exit( route->segments[0].lanes[0].waypoints, \
//			 route->segments[2].lanes[1].waypoints+1 ) != 0 ) {
//	    fprintf(stderr,"failed to add exit.\n");
  //  }

    // // print out the route in a RNDF file
    char filename_rndf_out[256];
    sprintf(filename_rndf_out, "%s.out", argv[1]);
    
    printf("printing route...\n");
    if ( route_network_print( route, filename_rndf_out ) == -1 )
      fprintf(stderr, "failed to write route into file %s\n", filename_rndf_out);
    printf("done.\n");
    
    // print out the mission in a MDF file
    char filename_mdf_out[256];
    sprintf(filename_mdf_out, "%s.out", argv[2]);
    if ( mission_print( mission, filename_mdf_out ) < 0 ) {
        fprintf(stderr, "failed to write mission file into %s.\n", filename_mdf_out);
    }

    // link the route against the mission
    link_mission_route ( mission, route );

    
    // destroy structures

    mission_destroy (mission);

    route_network_destroy ( route );

    return 0;
}
