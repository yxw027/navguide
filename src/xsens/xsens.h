#ifndef _XSENS_H
#define _XSENS_H

#include <stdint.h>
#include "imu.h"

// OR these together, (UNCALIBRATED_RAW mutually exclusive with other two flags)
#define XSENS_MODE_CALIBRATED       0x0002
#define XSENS_MODE_ORIENTATION      0x0004
#define XSENS_MODE_UNCALIBRATED_RAW 0x4000

// you can OR the timestamp flag with one of the modes below
#define XSENS_SETTING_TIMESTAMPS    0x0001

#define XSENS_SETTING_ORIENTATION_MASK  0x000c
#define XSENS_SETTING_QUATERNION    0x0000
#define XSENS_SETTING_EULER         0x0004
#define XSENS_SETTING_MATRIX        0x0008

#define XSENS_MID_GO_TO_CONFIG      0x30
#define XSENS_MID_SET_OUTPUT_MODE   0xD0
#define XSENS_MID_SET_OUTPUT_SETTINGS   0xD2
#define XSENS_MID_GO_TO_MEASUREMENT 0x10
#define XSENS_MID_MT_DATA           0x32

typedef struct xsens xsens_t;

typedef struct xsens_data xsens_data_t ;
struct xsens_data
{
    double linear_accel[3];
    double rotation_rate[3];
    double magnetometer[3];

    double quaternion[4];
};

typedef void (*xsens_callback_func) (xsens_t *, int64_t hosttime, uint16_t imu_time, 
                                     xsens_data_t *xd, void *user);

struct xsens
{
  int fd;
  int mode;
  int settings;

  double updatefrequency;

  uint64_t totalticks;
  int lastticks;

  uint64_t timeoffset;

  xsens_callback_func callback;
  void * user;
};

xsens_t *xsens_create(const char *portname);
void xsens_destroy(xsens_t *xs);
void xsens_change_state(xsens_t *xs, int configmode);
void xsens_change_output_mode(xsens_t *xs, int mode, int settings);
int xsens_get_firmware_rev (xsens_t * xs, uint8_t * ver);
int xsens_get_product_code (xsens_t * xs, char * str);
int xsens_get_device_id (xsens_t * xs, uint32_t * id);
int xsens_get_filter_settings (xsens_t * xs, float * gain,
        float * magnetometer);
int xsens_get_amd_setting (xsens_t * xs, uint16_t * val);
int xsens_set_amd_setting (xsens_t * xs, uint16_t val);
int xsens_set_filter_gain (xsens_t * xs, float gain);
int xsens_set_filter_magnetometer (xsens_t * xs, float weight);
void xsens_set_callback (xsens_t * xs, xsens_callback_func callback,
        void * user);
int xsens_run(xsens_t *xs);


#endif
