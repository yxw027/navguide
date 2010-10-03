#ifndef _APPLANIX_DECODE_H
#define _APPLANIX_DECODE_H

#include <stdint.h>


enum applanix_align_status_enum {
    APPLANIX_FULL_NAV=0,     // Full navigation (User accuracies are met)
    APPLANIX_FINE_ALIGN=1,   // Fine alignment is active (RMS heading error is less than 15 degrees)
    APPLANIX_GC_CHI2=2,       // GC CHI 2 (alignment with GPS, RMS heading error is greater than 15 degrees)
    APPLANIX_PC_CHI2=3,       // PC CHI 2 (alignment without GPS, RMS heading error is greater than 15 degrees)
    APPLANIX_GC_CHI1=4,       // GC CHI 1 (alignment with GPS, RMS heading error is greater than 45 degrees)
    APPLANIX_PC_CHI1=5,       // PC CHI 1 (alignment without GPS, RMS heading error is greater than 45 degrees)
    APPLANIX_COARSE_LEVEL=6, // Coarse leveling is active
    APPLANIX_INITIAL=7,      // Initial solution assigned
    APPLANIX_NO_VALID=8       // No valid solution
};

struct _applanix_timedist_t
{
    double time1;
    double time2;
    double dist_tag;
    uint8_t time_type;
    uint8_t dist_type;
};
typedef struct _applanix_timedist_t applanix_timedist_t;


struct _applanix_grp1_t
{
    applanix_timedist_t tidi; 
    double lat;               //30 Latitude 8 double (-90, 90] degrees
    double lon;               //38 Longitude 8 double (-180, 180] degrees
    double alt;               //46 Altitude 8 double (,) meters
    float vn;                 //54 North velocity 4 float (,) meters/second
    float ve;                 //58 East velocity 4 float (,) meters/second
    float vd;                 //62 Down velocity 4 float (,) meters/second
    double roll;              //66 Vehicle roll 8 double (-180, 180] degrees
    double pitch;             //74 Vehicle pitch 8 double (-90, 90] degrees
    double heading;           //82 Vehicle heading 8 double [0, 360) degrees
    double wander;            //90 Vehicle wander angle 8 double (-180, 180] degrees
    float track;              //98 Vehicle track angle 4 float [0, 360) degrees
    float speed;              //102 Vehicle speed 4 float [0, ) meters/second
    float rate_lon;           //106 Vehicle angular rate about longitudinal axis
    //    4 float (,) degrees/second
    float rate_trans;         //110 Vehicle angular rate about transverse axis
    //    4 float (,) degrees/second
    float rate_down;          //114 Vehicle angular rate about down axis
    //    4 float (,) degrees/second
    float acc_lon;            //118 Vehicle longitudinal acceleration 
    //    4 float (,) meters/second 2
    float acc_trans;          //122 Vehicle transverse acceleration
    //    4 float (,) meters/second 2
    float acc_down;           //126 Vehicle down acceleration
    //    4 float (,) meters/second 2
    uint8_t status;           //130 Alignment status  1 byte See Table 5
};
typedef struct _applanix_grp1_t applanix_grp1_t;


struct _applanix_grp2_t {
    applanix_timedist_t tidi;
    float np_err;
    float ep_err;
    float dp_err;
    float nv_err;
    float ev_err;
    float dv_err;
    float roll_err;
    float pitch_err;
    float heading_err;
    float ellipse_maj;
    float ellipse_min;
    float ellipse_ang;
};
typedef struct _applanix_grp2_t applanix_grp2_t;

struct _applanix_grp3_t
{
    applanix_timedist_t tidi;
    uint8_t solution_status;
    uint8_t num_sv;
    uint16_t status_bytes;
    struct {
        uint16_t prn;
        uint16_t status;
        float azimuth;
        float elevation;
        float L1_snr;
        float L2_snr;
    } channel[12];
    float hdop;
    float vdop;
    float dgps_latency;
    uint16_t dgps_reference_id;
    uint32_t gps_week;
    uint16_t gps_type;
    uint32_t gps_status;
};
typedef struct _applanix_grp3_t applanix_grp3_t;

struct _applanix_grp4_t
{
    applanix_timedist_t tidi; //4  Time/Distance 26 See Table 3
    int32_t dx;               //30 X incremental velocity 4 long (,) bits
    int32_t dy;               //34 Y incremental velocity 4 long (,) bits
    int32_t dz;               //38 Z incremental velocity 4 long (,) bits
    int32_t dthx;             //42 X incremental angle 4 long (,) bits
    int32_t dthy;             //46 Y incremental angle 4 long (,) bits
    int32_t dthz;             //50 Z incremental angle 4 long (,) bits
    uint8_t data_status;      //54 IMU stat 0=1bad, 1=2bad, 2=3bad IMU frames
    uint8_t imu_type;         //55 IMU type ours is type 17. Max rate 100Hz
    uint8_t imu_rate;         //56 IMU rate 0:50Hz 1:100Hz
    uint16_t imu_status;      //57 IMU status 
};
typedef struct _applanix_grp4_t applanix_grp4_t;

struct _applanix_grp10_t {
    applanix_timedist_t tidi;
    uint32_t status_A;
    uint32_t status_B;
    uint32_t status_C;
    uint32_t FDIR1;
    uint16_t FDIR1_imu;
    uint16_t FDIR2;
    uint16_t FDIR3;
    uint16_t FDIR4;
    uint16_t FDIR5;
};
typedef struct _applanix_grp10_t applanix_grp10_t;

struct _applanix_grp14_t {
    applanix_timedist_t tidi;
    uint16_t status;
    float primary_GPS[3];
    uint16_t primary_GPS_FOM;
    float aux_GPS1[3];
    uint16_t aux_GPS1_FOM;
    float aux_GPS2[3];
    uint16_t aux_GPS2_FOM;
    float DMI_lever[3];
    uint16_t DMI_lever_FOM;
    float DMI_scale;
    uint16_t DMI_scale_FOM;
};
typedef struct _applanix_grp14_t applanix_grp14_t;

struct _applanix_grp15_t {
    applanix_timedist_t tidi;
    double dist_signed;
    double dist_unsigned;
    uint16_t scale;
    uint8_t status;
    uint8_t type;
    uint8_t rate;
};
typedef struct _applanix_grp15_t applanix_grp15_t;

struct _applanix_grp10002_t {
    applanix_timedist_t tidi;
    uint8_t header[7];
    uint16_t length;
    uint8_t data[64];
    int16_t checksum;
};
typedef struct _applanix_grp10002_t applanix_grp10002_t;

struct _applanix_grp10006_t {
    applanix_timedist_t tidi;
    int32_t count_signed;
    uint32_t count_unsigned;
    int32_t event_count;
    uint32_t reserved_count;
};
typedef struct _applanix_grp10006_t applanix_grp10006_t;

struct _applanix_msg0_t {
    uint16_t transaction;
    uint16_t id;
    uint16_t response_code;
    uint8_t parameters_status;
    uint8_t parameter_name[32];
};
typedef struct _applanix_msg0_t applanix_msg0_t;

struct _applanix_msg20_t {
    uint16_t transaction;
    uint8_t time_types;
    uint8_t dist_type;
    uint8_t autostart;
    float ref_to_IMU[3];
    float ref_to_GPS[3];
    float ref_to_aux1_GPS[3];
    float ref_to_aux2_GPS[3];
    float IMU_wrt_ref_angles[3];
    float ref_wrt_vehicle_angles[3];
    uint8_t multipath;
};
typedef struct _applanix_msg20_t applanix_msg20_t;

struct _applanix_msg22_t {
    uint16_t transaction;
    float DMI_scale;
    float ref_to_DMI[3];
    float reserved[7];
};
typedef struct _applanix_msg22_t applanix_msg22_t;

struct _applanix_msg50_t {
    uint16_t transaction;
    uint8_t navigation_mode;
};
typedef struct _applanix_msg50_t applanix_msg50_t;

struct _applanix_msg52_t {
    uint16_t transaction;
    uint16_t num_groups;
    uint16_t groups[70];
    uint16_t rate;
};
typedef struct _applanix_msg52_t applanix_msg52_t;

enum {
    APPLANIX_MSG54_NO_OP=0,
    APPLANIX_MSG54_SAVE_NVM=1,
    APPLANIX_MSG54_RESTORE_NVM=2,
    APPLANIX_MSG54_RESTORE_FACTORY=3,
};
struct _applanix_msg54_t {
    uint16_t transaction;
    uint8_t control;
};
typedef struct _applanix_msg54_t applanix_msg54_t;

enum {
    APPLANIX_MSG57_NO_OP=0,
    APPLANIX_MSG57_STOP=1,
    APPLANIX_MSG57_START_MANUAL=2,
    APPLANIX_MSG57_START_AUTO=3,
    APPLANIX_MSG57_TRANSFER_NORMAL=4,
    APPLANIX_MSG57_TRANSFER_FORCED=5,
};
#define APPLANIX_MSG57_PRIMARY_GPS  (1<<0)
#define APPLANIX_MSG57_AUX1_GPS     (1<<1)
#define APPLANIX_MSG57_AUX2_GPS     (1<<2)
#define APPLANIX_MSG57_DMI_LEVER    (1<<3)
#define APPLANIX_MSG57_DMI_SCALE    (1<<4)
#define APPLANIX_MSG57_POSITION_FIX (1<<7)
struct _applanix_msg57_t {
    uint16_t transaction;
    uint8_t calib_action;
    uint8_t calib_select;
};
typedef struct _applanix_msg57_t applanix_msg57_t;

typedef enum {
    APPLANIX_DATA_GRP,
    APPLANIX_DATA_MSG,
} applanix_data_type_t;

struct _applanix_data_t {
    applanix_data_type_t type;
    int                  id;
    union {
        applanix_grp1_t      grp1;
        applanix_grp2_t      grp2;
        applanix_grp3_t      grp3;
        applanix_grp4_t      grp4;
        applanix_grp10_t     grp10;
        applanix_grp14_t     grp14;
        applanix_grp15_t     grp15;
        applanix_grp10006_t  grp10006;
        applanix_grp10002_t  grp10002;
        applanix_msg0_t      msg0;
        applanix_msg20_t     msg20;
        applanix_msg22_t     msg22;
        applanix_msg50_t     msg50;
        applanix_msg52_t     msg52;
    };
};
typedef struct _applanix_data_t applanix_data_t;



/** Decode a single message, depositing the result into the
 * applanix_data struct. **/

int applanix_decode (applanix_data_t * out, const uint8_t * data, int len);

int applanix_decode_timedist(applanix_timedist_t *grp, const uint8_t *data);
int applanix_decode_grp1(applanix_grp1_t *grp, const uint8_t *data);
int applanix_decode_grp2(applanix_grp2_t * grp, const uint8_t * data);
int applanix_decode_grp3(applanix_grp3_t * grp, const uint8_t * data);
int applanix_decode_grp4(applanix_grp4_t *grp, const uint8_t *data);
int applanix_decode_grp10 (applanix_grp10_t *grp, const uint8_t * data);
int applanix_decode_grp14 (applanix_grp14_t * grp, const uint8_t * data);
int applanix_decode_grp15 (applanix_grp15_t *grp, const uint8_t * data);
int applanix_decode_grp10002 (applanix_grp10002_t * grp, const uint8_t * data);
int applanix_decode_grp10006 (applanix_grp10006_t * grp, const uint8_t * data);
int applanix_decode_msg0 (applanix_msg0_t *msg, const uint8_t * data);
int applanix_decode_msg20 (applanix_msg20_t *msg, const uint8_t * data);
int applanix_decode_msg22 (applanix_msg22_t *msg, const uint8_t * data);
int applanix_decode_msg50 (applanix_msg50_t *msg, const uint8_t * data);
int applanix_decode_msg52 (applanix_msg52_t *msg, const uint8_t * data);

void applanix_print (FILE * f, const applanix_data_t * a);
void applanix_print_timedist (FILE * f, const applanix_timedist_t * tidi);
void applanix_print_grp1 (FILE * f, const applanix_grp1_t * grp);
void applanix_print_grp2 (FILE * f, const applanix_grp2_t * grp);
void applanix_print_grp3 (FILE * f, const applanix_grp3_t * grp);
void applanix_print_grp4 (FILE * f, const applanix_grp4_t * grp);
void applanix_print_grp10 (FILE * f, const applanix_grp10_t * grp);
void applanix_print_grp14 (FILE * f, const applanix_grp14_t * grp);
void applanix_print_grp15 (FILE * f, const applanix_grp15_t * grp);
void applanix_print_grp10002 (FILE * f, const applanix_grp10002_t * grp);
void applanix_print_grp10006 (FILE * f, const applanix_grp10006_t * grp);
void applanix_print_msg0 (FILE * f, const applanix_msg0_t * msg);
void applanix_print_msg20 (FILE * f, const applanix_msg20_t * msg);
void applanix_print_msg22 (FILE * f, const applanix_msg22_t * msg);
void applanix_print_msg50 (FILE * f, const applanix_msg50_t * msg);
void applanix_print_msg52 (FILE * f, const applanix_msg52_t * msg);

int applanix_encode_msg22 (const applanix_msg22_t *msg, uint8_t * data,
        int length);
int applanix_encode_msg50 (const applanix_msg50_t *msg, uint8_t * data,
        int length);
int applanix_encode_msg52 (const applanix_msg52_t *msg, uint16_t id,
        uint8_t * data, int length);
int applanix_encode_msg54 (const applanix_msg54_t *msg, uint8_t * data,
        int length);
int applanix_encode_msg57 (const applanix_msg57_t *msg, uint8_t * data,
        int length);

const char * applanix_grp_desc (int id);
const char * applanix_msg_desc (int id);
const char * applanix_nav_status_str (int status);

#if 0
int applanix_grp1_to_imu_t(const applanix_grp1_t *grp1, imu_t *imu, uint64_t utime);
int applanix_grp1_to_nmea_t(const applanix_grp1_t *grp1, nmea_t *nmea, uint64_t utime);
int applanix_grp1_to_speed(const applanix_grp1_t *grp1, double *speed);
#endif

int
applanix_nad83_itrf00_compute_constants(double R[3][3], double T[3], int64_t utime);
int 
applanix_nad83_2_itrf00(double nad83[3], double itrf00[3], int64_t utime);
int
applanix_itrf00_2_nad83(double itrf00[3], double nad83[3], int64_t utime);

int64_t
applanix_get_pos_time (const uint8_t * data);

uint16_t 
applanix_groupid(const uint8_t *data);

uint16_t
applanix_payload_length(const uint8_t *data);

uint16_t
applanix_checksum (const uint8_t * data);

uint16_t 
applanix_compute_checksum(const uint8_t *data);


#define APPLANIX_MSG90_ALIVE        000  // 000        Controller alive
#define APPLANIX_MSG90_CLOSE_TCPIP  001  // 001        Terminate TCP/IP connection
#define APPLANIX_MSG90_RESET_GAMS   100  // 100        Reset GAMS
#define APPLANIX_MSG90_RESET        101  // 101        Reset POS
#define APPLANIX_MSG90_SHUTDOWN     102  // 102        Shutdown POS

int 
applanix_encode_msg90(uint8_t *data, int cmd);

struct nav_applanix_data_t {
    double heading;
    int64_t utime;
};

#endif
