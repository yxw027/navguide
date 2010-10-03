#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <inttypes.h>

#include <lcm/lcm.h>
#include <common/getopt.h>
#include <common/timestamp.h>
#include <lcmtypes/navlcm_imu_t.h>
#include "xsens.h"

static int verbose;
uint32_t device_id;

static void imu_callback (xsens_t * xs, int64_t utime, uint16_t navlcm_imu_time, 
                          xsens_data_t *xd, void *user)
{
    navlcm_imu_t imu;

    (void) xs;
    (void) navlcm_imu_time;

    // copy data into the navlcm_imu_t
    imu.utime = utime;
    imu.imu_serial_number = device_id;
    for (int i = 0; i < 3; i++) {
        imu.linear_accel[i] = xd->linear_accel[i];
        imu.rotation_rate[i] = xd->rotation_rate[i];
    }

    for (int i = 0; i < 4; i++) {
        imu.q[i] = xd->quaternion[i];
    }

    imu.has_heading = 0;
    imu.heading = 0;

    // and transmit.
    lcm_t *lcm = (lcm_t*)user;
    if (!lcm)
        return;

    navlcm_imu_t_publish (lcm, "IMU", &imu);

    if (verbose) {
        int64_t t = timestamp_now ();
        printf ("%ld %ld", t / 1000000, t % 1000000);
        printf ("orientation: %8.4f %8.4f %8.4f %8.4f\n",
                imu.q[0], imu.q[1], imu.q[2], imu.q[3]);
    }

}


int main(int argc, char *argv[])
{
    getopt_t *gopt = getopt_create();

    getopt_add_bool  (gopt, 'h',   "help",    0,        "Show this help");
    getopt_add_string(gopt, 'd',   "device", "/dev/xsens", "Device file");
    getopt_add_string(gopt, 'g',   "gain",   "0.1",       "Cross-over frequency between gyro/accelerometer");
    getopt_add_string(gopt, 'm',   "magnetometer", "0", "Magnetometer gain");
    getopt_add_bool  (gopt, 'v',   "verbose",    0,     "Be verbose");
    getopt_add_bool  (gopt, 's',   "status",   0,  "Report status and exit");
    getopt_add_bool  (gopt, 0,     "amd",      0,       "Enable 'Adjust for Magnetic Disturbances'");

    if (!getopt_parse(gopt, argc, argv, 1) || getopt_get_bool(gopt,"help")) {
        printf("Usage: %s [options]\n\n", argv[0]);
        getopt_do_usage(gopt);
        return 0;
    }
    
    verbose = getopt_get_bool(gopt, "verbose");

    xsens_t *xs = xsens_create(getopt_get_string(gopt, "device"));
    if (!xs)
        return 1;
    
    /// say hello to the device
    int res;
    uint8_t ver[3];
    char str[256];
    float gain, magnetometer;
    uint16_t amd;

    xsens_get_product_code (xs, str);
    xsens_get_firmware_rev (xs, ver);
    fprintf (stderr, "XSens is model \"%s\" with firmware %d.%d.%d\n",
             str, ver[0], ver[1], ver[2]);
    xsens_get_device_id (xs, &device_id);
    fprintf (stderr, "Serial number %08x\n", device_id);
    if (getopt_get_bool(gopt, "status")) return 0;
      

    xsens_set_filter_gain (xs, strtod(getopt_get_string(gopt, "gain"), NULL));

    xsens_set_filter_magnetometer (xs, strtod(getopt_get_string(gopt, "magnetometer"), NULL));

    xsens_get_filter_settings (xs, &gain, &magnetometer);
    fprintf (stderr, "Filter settings: gain is %.3f, magnetometer weight is %.3f\n",
             gain, magnetometer);

    xsens_get_amd_setting (xs, &amd);
    fprintf (stderr, "Adjust for Magnetic Disturbances (AMD) is %s\n",
             amd ? "on" : "off");
    int set_amd = getopt_get_bool(gopt, "amd");

    if (set_amd >= 0 && set_amd != amd) {
        fprintf (stderr, "Changing AMD to %d\n", set_amd);
        xsens_set_amd_setting (xs, set_amd);
    }

    // XXX if read-back settings don't match, loop?
    lcm_t *lcm = lcm_create(NULL);
    if (!lcm)
        return 1;

    // setup event loop

    xsens_set_callback (xs, imu_callback, lcm);
    xsens_change_output_mode(xs, XSENS_MODE_CALIBRATED | XSENS_MODE_ORIENTATION,
                             XSENS_SETTING_TIMESTAMPS | XSENS_SETTING_QUATERNION);
    
    res = xsens_run (xs);
    if (res < 0) {
        xsens_destroy (xs);
        sleep (1);
    }

    // XXX be able to recover if connection is lost

    return 0;
}
