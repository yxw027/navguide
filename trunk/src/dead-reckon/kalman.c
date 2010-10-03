#include <stdio.h>
#include <string.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_permutation.h>
#include <gsl/gsl_linalg.h>

#include <common/small_linalg.h>

#include "kalman.h"

struct _Kalman {
    double * x;
    double * P;
    int N;
    gsl_matrix_view Pv;
    gsl_vector_view xv;
    KalmanStateFunc state_func;
    KalmanStateJacobFunc state_jacob_func;
    KalmanStateCovFunc state_cov_func;
    void * user;
};

Kalman *
kalman_new (int N, KalmanStateFunc state_func,
        KalmanStateJacobFunc state_jacob_func,
        KalmanStateCovFunc state_cov_func, void * user)
{
    Kalman * k;

    k = malloc (sizeof (Kalman));
    k->N = N;
    k->x = malloc (N * sizeof (double));
    k->P = malloc (N * N * sizeof (double));
    k->state_func = state_func;
    k->state_jacob_func = state_jacob_func;
    k->state_cov_func = state_cov_func;
    k->user = user;

    k->Pv = gsl_matrix_view_array (k->P, N, N);
    k->xv = gsl_vector_view_array (k->x, N);

    return k;
}

void
kalman_free (Kalman * k)
{
    free (k->x);
    free (k->P);
    free (k);
}

int
kalman_set_state (Kalman * k, const double * x, const double * P, int N)
{
    if (N != k->N)
        return -1;
    memcpy (k->x, x, k->N * sizeof (double));
    memcpy (k->P, P, k->N * k->N * sizeof (double));
    return 0;
}

int
kalman_get_state (Kalman * k, double ** x, double ** P, int * N)
{
    if (x)
        *x = k->x;
    if (P)
        *P = k->P;
    if (N)
        *N = k->N;
    return 0;
}

int
kalman_pred (Kalman * k, double dt)
{
    double A[k->N * k->N];
    double Ptmp[k->N * k->N];
    k->state_func (k->x, k->N, dt, k->user);
    k->state_jacob_func (A, k->N, dt, k->user);
    gsl_matrix_view Av = gsl_matrix_view_array (A, k->N, k->N);
    gsl_matrix_view Ptmpv = gsl_matrix_view_array (Ptmp, k->N, k->N);

    /*  P_ = A*P*A' + Q  */
    gsl_blas_dgemm (CblasNoTrans, CblasNoTrans, 1.0, &Av.matrix,
            &k->Pv.matrix, 0.0, &Ptmpv.matrix);
    k->state_cov_func (k->P, k->N, dt, k->user);
    gsl_blas_dgemm (CblasNoTrans, CblasTrans, 1.0, &Ptmpv.matrix,
            &Av.matrix, 1.0, &k->Pv.matrix);
    return 0;
}

int
kalman_meas (Kalman * k, const double * z, int M, double dt,
        KalmanMeasFunc meas_func, KalmanMeasJacobFunc meas_jacob_func,
        KalmanMeasCovFunc meas_cov_func)
{
    kalman_pred (k, dt);

    double K[k->N * M];
    double PHt[k->N * M];
    double H[M * k->N];
    double R[M * M];
    double I[M * M];
    double h[M];

    gsl_matrix_view Kv = gsl_matrix_view_array (K, k->N, M);
    gsl_matrix_view PHtv = gsl_matrix_view_array (PHt, k->N, M);
    gsl_matrix_view Hv = gsl_matrix_view_array (H, M, k->N);
    gsl_matrix_view Rv = gsl_matrix_view_array (R, M, M);
    gsl_matrix_view Iv = gsl_matrix_view_array (I, M, M);
    gsl_vector_view hv = gsl_vector_view_array (h, M);

    meas_jacob_func (H, M, k->x, k->N, k->user);

    /*  K = P_*H'*inv(H*P_*H' + R)  */
    gsl_blas_dgemm (CblasNoTrans, CblasTrans, 1.0, &k->Pv.matrix,
            &Hv.matrix, 0.0, &PHtv.matrix);
    meas_cov_func (R, M, k->user);
    gsl_blas_dgemm (CblasNoTrans, CblasNoTrans, 1.0, &Hv.matrix,
            &PHtv.matrix, 1.0, &Rv.matrix);

    size_t permv[M];
    gsl_permutation perm = { M, permv };
    int signum;
    gsl_linalg_LU_decomp (&Rv.matrix, &perm, &signum);
    gsl_linalg_LU_invert (&Rv.matrix, &perm, &Iv.matrix);
    gsl_blas_dgemm (CblasNoTrans, CblasNoTrans, 1.0, &PHtv.matrix,
            &Iv.matrix, 0.0, &Kv.matrix);

    /*  x = x + K*(z - h(x))  */
    meas_func (h, M, k->x, k->N, k->user);
    vector_sub_nd (z, h, M, h);
    gsl_blas_dgemv (CblasNoTrans, 1.0, &Kv.matrix, &hv.vector, 1.0,
            &k->xv.vector);

    /*  P = P_ - K*H*P_  */
    gsl_blas_dgemm (CblasNoTrans, CblasTrans, -1.0, &Kv.matrix,
            &PHtv.matrix, 1.0, &k->Pv.matrix);
    return 0;
}
