#include "util.h"

void read_imu_data (FILE *fp, navlcm_imu_t *imu)
{
    if (!fp)
        return;

    fread (&imu->utime, sizeof (int64_t), 1, fp);
    fread (&imu->linear_accel[0], sizeof(double), 1, fp);
    fread (&imu->linear_accel[1], sizeof(double), 1, fp);
    fread (&imu->linear_accel[2], sizeof(double), 1, fp);
    fread (&imu->rotation_rate[0], sizeof(double), 1, fp);
    fread (&imu->rotation_rate[1], sizeof(double), 1, fp);
    fread (&imu->rotation_rate[2], sizeof(double), 1, fp);
    fread (&imu->q[0], sizeof(double), 1, fp);
    fread (&imu->q[1], sizeof(double), 1, fp);
    fread (&imu->q[2], sizeof(double), 1, fp);
    fread (&imu->q[3], sizeof(double), 1, fp);

}

void write_imu_data (FILE *fp, const navlcm_imu_t *imu)
{
    if (!fp)
        return;

    fwrite (&imu->utime, sizeof (int64_t), 1, fp);
    fwrite (&imu->linear_accel[0], sizeof(double), 1, fp);
    fwrite (&imu->linear_accel[1], sizeof(double), 1, fp);
    fwrite (&imu->linear_accel[2], sizeof(double), 1, fp);
    fwrite (&imu->rotation_rate[0], sizeof(double), 1, fp);
    fwrite (&imu->rotation_rate[1], sizeof(double), 1, fp);
    fwrite (&imu->rotation_rate[2], sizeof(double), 1, fp);
    fwrite (&imu->q[0], sizeof(double), 1, fp);
    fwrite (&imu->q[1], sizeof(double), 1, fp);
    fwrite (&imu->q[2], sizeof(double), 1, fp);
    fwrite (&imu->q[3], sizeof(double), 1, fp);

}

void write_imu_data (FILE *fp, int64_t utime, xsens_data_t *xd)
{
    if (!fp)
        return;

    fwrite (&utime, sizeof (int64_t), 1, fp);
    fwrite (&xd->linear_accel[0], sizeof(double), 1, fp);
    fwrite (&xd->linear_accel[1], sizeof(double), 1, fp);
    fwrite (&xd->linear_accel[2], sizeof(double), 1, fp);
    fwrite (&xd->rotation_rate[0], sizeof(double), 1, fp);
    fwrite (&xd->rotation_rate[1], sizeof(double), 1, fp);
    fwrite (&xd->rotation_rate[2], sizeof(double), 1, fp);
    fwrite (&xd->quaternion[0], sizeof(double), 1, fp);
    fwrite (&xd->quaternion[1], sizeof(double), 1, fp);
    fwrite (&xd->quaternion[2], sizeof(double), 1, fp);
    fwrite (&xd->quaternion[3], sizeof(double), 1, fp);

}
