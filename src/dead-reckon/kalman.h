#ifndef _KALMAN_H_
#define _KALMAN_H_

typedef struct _Kalman Kalman;

typedef void (*KalmanStateFunc) (double * x, int N, double dt, void * user);
typedef void (*KalmanStateJacobFunc) (double * A, int N, double dt,
        void * user);
typedef void (*KalmanStateCovFunc) (double * Q, int N, double dt, void * user);

typedef void (*KalmanMeasFunc) (double * z, int M, const double * x,
        int N, void * user);
typedef void (*KalmanMeasJacobFunc) (double * H, int M, const double * x,
        int N, void * user);
typedef void (*KalmanMeasCovFunc) (double * R, int M, void * user);

#endif
