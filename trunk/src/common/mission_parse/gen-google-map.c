#include <stdio.h>
#include <common/mission_parse.h>

int main (int argc, char ** argv)
{
    FILE * f;
    RouteNetwork * route = route_network_create();
    int i, j, k, l;
    float maxlat, minlat, maxlon, minlon;

    if (argc != 2) {
        fprintf (stderr, "Usage: %s filename\n", argv[0]);
        return 1;
    }

    f = fopen (argv[1], "r");
    if (!f) {
        fprintf (stderr, "Failed to open %s\n", argv[1]);
        return 1;
    }

    /* Get the route network */
    parse_route_network (f, route);
    fclose (f);
    if (!route)
        return 1;

    /* Emit javascript to initialize the map */
    printf ("function load_dgc() {\n\
  if (GBrowserIsCompatible()) {\n\
     var map = new GMap2(document.getElementById(\"map\"));\n\
     map.setCenter(new GLatLng(37.4419, -122.1419), 16);\n\
     map.setMapType (G_SATELLITE_MAP);\n\
     map.addControl(new GLargeMapControl());\n\
     map.addControl(new GMapTypeControl());\n\
     map.addControl(new GScaleControl());\n\
     map.addControl(new GOverviewMapControl());\n\
     var points = [];\n"
    );
 
    maxlat = minlat = route->segments[0].lanes[0].waypoints[0].lat;
    maxlon = minlon = route->segments[0].lanes[0].waypoints[0].lon;

    /* Draw all the lanes contained within segments */
    for (i = 0; i < route->num_segments; i++) {
        Segment * segment = route->segments + i;
        for (j = 0; j < segment->num_lanes; j++) {
            Lane * lane = segment->lanes + j;
            printf ("points = [];\n");
            for (k = 0; k < lane->num_waypoints; k++) {
                Waypoint * waypoint = lane->waypoints + k;
                printf ("points.push (new GLatLng (%f, %f));\n",
                        waypoint->lat, waypoint->lon);

                /* Keep track of the waypoints extent for map centering */
                if (waypoint->lat > maxlat)
                    maxlat = waypoint->lat;
                if (waypoint->lat < minlat)
                    minlat = waypoint->lat;
                if (waypoint->lon > maxlon)
                    maxlon = waypoint->lon;
                if (waypoint->lon < minlon)
                    maxlon = waypoint->lon;

                /* Draw the exits */
                for (l = 0; l < waypoint->num_exits; l++) {
                    Waypoint * entry = waypoint->exits[l];
                    printf ("map.addOverlay (new GPolyline "
                        "([new GLatLng (%f, %f), new GLatLng (%f, %f)], "
                        "\"#00ff00\", 1));\n",
                        waypoint->lat, waypoint->lon, entry->lat, entry->lon);
                }
            }
            printf ("map.addOverlay (new GPolyline (points, \"#0000ff\", 2));\n");
        }
    }

    /* Draw all zones */
    for (i = 0; i < route->num_zones; i++) {
        Zone * zone = route->zones + i;
        printf ("points = [];\n");
        for (j = 0; j < zone->num_peripoints; j++) {
            Waypoint * peripoint = zone->peripoints + j;
            printf ("points.push (new GLatLng (%f, %f));\n",
                    peripoint->lat, peripoint->lon);
            for (l = 0; l < peripoint->num_exits; l++) {
                Waypoint * entry = peripoint->exits[l];
                printf ("map.addOverlay (new GPolyline "
                    "([new GLatLng (%f, %f), new GLatLng (%f, %f)], "
                    "\"#00ff00\", 1));\n",
                    peripoint->lat, peripoint->lon, entry->lat, entry->lon);
            }
        }
        printf ("points.push (points[0]);\n");
        printf ("map.addOverlay (new GPolyline (points, \"#ff0000\", 2));\n");

        /* Draw any parking spaces */
        for (j = 0; j < zone->num_spots; j++) {
            Spot * spot = zone->spots + j;
            printf ("points = [new GLatLng (%f, %f), new GLatLng (%f, %f)];\n",
                    spot->waypoints[0].lat, spot->waypoints[0].lon,
                    spot->waypoints[1].lat, spot->waypoints[1].lon);
            printf ("map.addOverlay (new GPolyline (points, \"#0000ff\", 2));\n");
        }
    }

    /* Label the checkpoints */
    for (i = 0; i < route->num_checkpoints; i++) {
        Waypoint * checkpoint = route->checkpoints[i].waypoint;
        printf ("map.addOverlay (new GMarker (new GLatLng (%f, %f), {title: \"Checkpoint %d\"}));\n",
                checkpoint->lat, checkpoint->lon, route->checkpoints[i].id);
    }

    printf ("map.panTo(new GLatLng(%f, %f));\n",
            (maxlat + minlat)/2, (maxlon + minlon)/2);
    printf ("}}\n"); 

    return 0;
}
