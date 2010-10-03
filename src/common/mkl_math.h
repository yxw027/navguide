#ifndef _MKL_MATH_H
#define _MKL_MATH_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <glib.h>

/* from intel mkl */
#include <mkl_cblas.h>

void math_matrix_mult_unit_testing ();
void math_matrix_mult_unit_testing_float ();
void math_matrix_mult_naive_float (int m, int n, int k, float *a, float *b, float *c);
void math_matrix_mult_mkl_float (int m, int n, int k, const float *a, const float *b, float *c);
void math_matrix_mult_unit_testing_float_2 ();

#endif
