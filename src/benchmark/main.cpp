#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <assert.h>
//#include <mkl_cblas.h>
#include<cblas.h>
#include <math.h>

/* c = a x b, where a = [ m x k ], b = [ k x n], c = [ m x n ]
 * 
 * using naive approach
 */
void math_matrix_mult_naive_double (int m, int n, int k, double *a, double *b, double *c)
{
    for (int row=0;row<m;row++) {
        for (int col=0;col<n;col++) {
            c[row*n+col] = .0;
            double *ptra = a + row*k;
            double *ptrb = b + col;
            for (int kk=0;kk<k;kk++) {
                c[row*n+col] += (*ptra) * (*ptrb);
                ptra++;
                ptrb+=n;
            }
        }
    }
}

/* c = a x b, where a = [ m x k ], b = [ k x n], c = [ m x n ]
 * 
 * using Intel Math Kernel Library
 */
void math_matrix_mult_mkl_double (int m, int n, int k, const double *a, const double *b, double *c)
{
    cblas_dgemm (CblasRowMajor, CblasNoTrans, CblasNoTrans, m, n, k, 1.0, a, m, b, k, 0.0, c, m);
}

/* alloc memory for a matrix
 */
double * math_matrix_alloc_double (int nrows, int ncols)
{
    double *a = (double*)malloc(nrows*ncols*sizeof(double));
    return a;
}

/* free matrix
 */
void math_matrix_free_double (double *a)
{
    free (a);
}

/* generates a random square matrix of size N x N
 */
double * math_matrix_rand_double (int N)
{
    double *a = math_matrix_alloc_double (N, N);

    for (int row=0;row<N;row++) {
        for (int col=0;col<N;col++) {
            a[row*N+col] = 1.0*rand()/RAND_MAX;
        }
    }

    return a;
}

/* matrix multiplication unit testing
 */
void math_matrix_mult_unit_testing_double ()
{
    double *a, *b, *c;

    printf ("double-precision:\n\n");

    for (int N = 100; N <= 2000; N += 100) {

        printf ("N = %d\t", N);

        a = math_matrix_rand_double (N);
        b = math_matrix_rand_double (N);

        c = math_matrix_alloc_double (N, N);

        // naive multiplication
        GTimer *t1 = g_timer_new ();
        //math_matrix_mult_naive_double (N, N, N, a, b, c);
        printf ("naive: %.5f secs.\t", g_timer_elapsed (t1, NULL));

        // KML multiplication
        GTimer *t2 = g_timer_new ();
        math_matrix_mult_mkl_double (N, N, N, a, b, c);
        printf ("kml  : %.5f secs.\n", g_timer_elapsed (t2, NULL));

        math_matrix_free_double (a);
        math_matrix_free_double (b);
        math_matrix_free_double (c);

        g_timer_destroy (t1);
        g_timer_destroy (t2);

    }
}

/* c = a x b, where a = [ m x k ], b = [ k x n], c = [ m x n ]
 * 
 * using naive approach
 */
void math_matrix_mult_naive_float (int m, int n, int k, float *a, float *b, float *c)
{
    for (int row=0;row<m;row++) {
        for (int col=0;col<n;col++) {
            c[row*n+col] = .0;
            float *ptra = a + row*k;
            float *ptrb = b + col;
            for (int kk=0;kk<k;kk++) {
                c[row*n+col] += (*ptra) * (*ptrb);
                ptra++;
                ptrb+=n;
                //a[row*k+kk] * b[kk*n+col];
            }
        }
    }
}

/* c = a x b, where a = [ m x k ], b = [ k x n], c = [ m x n ]
 * 
 * using Intel Math Kernel Library
 */
void math_matrix_mult_mkl_float (int m, int n, int k, const float *a, const float *b, float *c)
{
    cblas_sgemm (CblasRowMajor, CblasNoTrans, CblasNoTrans, m, n, k, 1.0, a, k, b, n, 0.0, c, n);
}

/* alloc memory for a matrix
 */
float * math_matrix_alloc_float (int nrows, int ncols)
{
    float *a = (float*)malloc(nrows*ncols*sizeof(float));
    return a;
}

/* free matrix
 */
void math_matrix_free_float (float *a)
{
    free (a);
}

/* generates a random square matrix of size N x N
 */
float * math_matrix_rand_float (int N, int P)
{
    float *a = math_matrix_alloc_float (N, P);

    for (int row=0;row<N;row++) {
        for (int col=0;col<P;col++) {
            a[row*P+col] = 1.0*rand()/RAND_MAX;
        }
    }

    return a;
}

/* matrix multiplication unit testing
 */
void math_matrix_mult_unit_testing_float ()
{
    float *a, *b, *c1, *c2;

    printf ("single-precision:\n\n");

    for (int N = 100; N <= 2000; N += 100) {

        printf ("N = %d\t", N);

        a = math_matrix_rand_float (N, N);
        b = math_matrix_rand_float (N, N);

        c1 = math_matrix_alloc_float (N, N);
        c2 = math_matrix_alloc_float (N, N);

        // naive multiplication
        GTimer *t1 = g_timer_new ();
        //math_matrix_mult_naive_float (N, N, N, a, b, c1);
        printf ("naive: %.5f secs. ", g_timer_elapsed (t1, NULL));

        // KML multiplication
        GTimer *t2 = g_timer_new ();
        math_matrix_mult_mkl_float (N, N, N, a, b, c2);
        printf ("kml  : %.5f secs.\n", g_timer_elapsed (t2, NULL));

        // sanity check
        for (int i=0;i<N*N;i++) {
          //  assert (fabs ((c1[i]-c2[i])/N) < 1E-6);
        }

        // free
        math_matrix_free_float (a);
        math_matrix_free_float (b);
        math_matrix_free_float (c1);
        math_matrix_free_float (c2);


        g_timer_destroy (t1);
        g_timer_destroy (t2);

    }
}

int main (int argc, char *argv[]) 
{
    // float precision
    math_matrix_mult_unit_testing_float ();

    // float precision
    math_matrix_mult_unit_testing_double ();


}

