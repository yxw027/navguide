#ifndef _ESCAPE_CONSTANTS_H
#define _ESCAPE_CONSTANTS_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif


// define vehicle contants for Ford Escape
// and other useful EMC parameters

#define TIME_STEP            .000001 // time step conversion factor
#define A_CROSS             2.685    // frontal cross-sectional area
                                     // in meters squared
#define Cd                   .40     // drag coeficient
#define LENGTH              2.620    // Vehicle length in meters
#define WIDTH               1.780    // Vehicle width in meters
#define AIR_DENSITY         1.2      // kg/m^3 - used for drag calcs
#define WEIGHT_DIST          .564    // Percentage of weight on front wheels
#define MASS             1741.34     // Vehicle Mass in kilograms
#define ROLLING_FRIC         .0015   // coeficient of rolling resistance
#define GRAVITY             9.81     // m/s^2
#define THROTTLE_MAX        1.2      // Maximum throttle voltage
#define BRAKE_MIN            .4      // Minimum brake voltage
#define STEERING_RATIO     17.9      // steering column ratio
#define LENGTH_REAR         1.478    // length from rear axle to COM
#define LENGTH_FRONT        1.142    // length from front axle to COM
#define MAX_STEER            .579    // Maximum steer angle for escape (rads)
#define MIN_RADIUS          5.25     // Minimum turning radius

#ifdef __cplusplus
}
#endif

#endif 
