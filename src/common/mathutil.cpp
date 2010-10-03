#include "mathutil.h"

#define TWOPI_INV (0.5/PI)
#define TWOPI (2*PI)

/** valid only for v > 0 **/
static inline double mod2pi_positive(double vin)
{
    double q = vin * TWOPI_INV + 0.5;
    int qi = (int) q;

    return vin - qi*TWOPI;
}

/** Map v to [-PI, PI] **/
inline double mod2pi(double vin)
{
    if (vin < 0)
        return -mod2pi_positive(-vin);
    else
        return mod2pi_positive(vin);
}

/** Return vin such that it is within PI degrees of ref **/
double mod2pi_ref(double ref, double vin)
{
    return ref + mod2pi(vin - ref);
}

void
quat_mult (double * c, const double * a, const double * b)
{
    c[0] = a[0]*b[0] - a[1]*b[1] - a[2]*b[2] - a[3]*b[3];
    c[1] = a[0]*b[1] + a[1]*b[0] + a[2]*b[3] - a[3]*b[2];
    c[2] = a[0]*b[2] - a[1]*b[3] + a[2]*b[0] + a[3]*b[1];
    c[3] = a[0]*b[3] + a[1]*b[2] - a[2]*b[1] + a[3]*b[0];
}

double
angle_clamp (double theta)
{
    if (theta > 2*PI)
        return theta - M_PI;
    if (theta < -2*PI)
        return theta + 2*PI;
    return theta;
}

int math_round (double a)
{
    int ab = (int)(floor (a));
    if (a-ab > .5)
        return ab+1;

    return ab;
}

double vect_length_double (double *a, int n)
{
    double l = 0.0;
    for (int i=0;i<n;i++)
        l += a[i] * a[i];
    return sqrt (l);
}

float vect_length_float (float *a, int n)
{
    float l = 0.0;
    for (int i=0;i<n;i++)
        l += a[i] * a[i];
    return sqrt (l);
}

void vect_norm (double *a, int n)
{
    double length = 0.0;
    for (int i=0;i<n;i++)
        length += a[i] * a[i];
    length = sqrt (length);
    if (length > 1E-6)
        for (int i=0;i<n;i++)
            a[i] = a[i] / length;
}

void quat_angle_unit_testing (int n)
{
    srand (time (NULL));

    for (int i=0;i<n;i++) {
        
        // pick an axis at random
        double u[3];
        u[0] = 1.0;
        u[1] = 1.0*rand()/RAND_MAX;
        u[2] = 1.0*rand()/RAND_MAX;
        
        // normalize it
        double len = sqrt (u[0]*u[0]+u[1]*u[1]+u[2]*u[2]);
        u[0] /= len;
        u[1] /= len;
        u[2] /= len;

        // pick an angle at random in [-pi, pi]
        double a1 = 1.0*rand()/RAND_MAX * 2 * PI - PI;

        // create a quaternion
        double q1[4];
        q1[0] = cos(a1/2);
        q1[1] = sin(a1/2)*u[0];
        q1[2] = sin(a1/2)*u[1];
        q1[3] = sin(a1/2)*u[2];

        // pick another angle at random
        double a2 = 1.0*rand()/RAND_MAX * 2 * PI - PI;

        // create a quaternion
        double q2[4];
        q2[0] = cos(a2/2);
        q2[1] = sin(a2/2)*u[0];
        q2[2] = sin(a2/2)*u[1];
        q2[3] = sin(a2/2)*u[2];

        // measure the angle between the two quaternions
        double ang = quat_angle (q1, q2);
        
        // compare it with the ground truth angle
        double gt_ang = diff_angle_plus_minus_pi (a1, a2);

        // print out
        double error = diff_angle_plus_minus_pi (ang, gt_ang);

        printf ("angle: %.4f  true angle: %.4f  error: %.4f\n", ang, gt_ang, error);
    }
}

/** return the angle of rotation involved between two quaternions **/
double quat_angle (double q1[4], double q2[4])
{
    double dot = 0.0;
    for (int i=0;i<4;i++)
        dot += q1[i]*q2[i];
    
    double a = 2 * acos (dot);
    
    if (a > PI)
        return 2*PI-a;
    else
        return a;
}
/** return the angle of rotation involved between two quaternions **/
double quat_angle_2 (double q1[4], double q2[4])
{
    double dot = 0.0;
    for (int i=0;i<4;i++)
        dot += q1[i]*q2[i];
    
    double a = 2 * acos (dot);
    
    return a;
}

void quat_bar (double *q, double *r)
{
    r[0] =  q[0];
    r[1] = -q[1];
    r[2] = -q[2];
    r[3] = -q[3];
}

// rotate 3D point p with quaternion q
//
void quat_rot (double *q, double *p, double *p2)
{
    double qp[4];
    qp[0] = 0;
    qp[1] = p[0];
    qp[2] = p[1];
    qp[3] = p[2];

    double qbar[4];
    quat_bar (q, qbar);
    
    double qc[4];
    quat_mult (qc, qp, qbar);
    
    double qd[4];
    quat_mult(qd, q, qc);

    p2[0] = qd[1];
    p2[1] = qd[2];
    p2[2] = qd[3];
}

/* cross product o = q x r */
void math_cross_product (double *q, double *r, double *o)
{
    o[0] = q[1]*r[2] - q[2]*r[1];
    o[1] = -q[0]*r[2] + r[0]*q[2];
    o[2] = q[0]*r[1] - r[0]*q[1];
}

void quat_rot_axis (double *ax, double angle, double *p, double *p2)
{
    double q[4];
    q[0] = cos(angle/2);
    q[1] = sin(angle/2)*ax[0];
    q[2] = sin(angle/2)*ax[1];
    q[3] = sin(angle/2)*ax[2];

    quat_rot (q, p, p2);
}

// clip value
//
double clip_value (double val, double min, double max, double eps)
{
    double d = max-min;

    while (val > max - eps) {
        val -= d;
    }
    while (val < min + eps) {
        val += d;
    }

    return val;
}

double diff_angle_plus_minus_180 (double a1, double a2)
{
    double ca1 = clip_value (a1 * PI/180.0, -PI, PI, 1E-6);
    double ca2 = clip_value (a2 * PI/180.0, -PI, PI, 1E-6);
    double res = diff_angle_plus_minus_pi (ca1, ca2);
    return res * 180.0 / PI;
}

// diff between two angles [-pi,pi]
//
double diff_angle_plus_minus_pi (double a1, double a2)
{
    if (a1 > 0 && a2 > 0)
        return fabs (a2-a1);

    if (a1 < 0 && a2 < 0)
        return fabs (a2-a1);

    if (a1 < 0 && a2 > 0)
        return fmin (a2-a1, 2*PI-a2+a1);

    if (a2 < 0 && a1 > 0)
        return fmin (a1-a2, 2*PI-a1+a2);
        
    return fabs (a1-a2);
}

// compute sum
//
int sum (int *val, int n)
{
    int count = 0;
    for (int i=0;i<n;i++)
        count += val[i];

    return count;
}

int unsigned_char_comp (const void *a, const void *b)
{
    const unsigned char *aa = (const unsigned char *)a;
    const unsigned char *bb = (const unsigned char *)b;
    
    if (*aa == *bb)
        return 0;
    if (*aa < *bb)
        return -1;
    return 1;
}

// sort a list of <n> unsigned char
//
void math_sort (unsigned char *val, int n)
{
    qsort (val, n, sizeof(unsigned char), unsigned_char_comp);
}

// sort a list of <n> double numbers
//
void math_sort (double *val, int n)
{
    int done = 0;
    
    while (!done) {
        done = 1;
        for (int i=0;i<n-1;i++) {
            for (int j=i+1;j<n;j++) {
                if (val[i] > val[j]) {
                    double tmp = val[i];
                    val[i] = val[j];
                    val[j] = tmp;
                    done = 0;
                }
            }
        }
    }
}

// smoothing in the 0-2pi space
//
double *math_smooth_2pi (double *data, int size, int window)
{
    double *out = (double*)malloc(size*sizeof(double));
    for (int i=0;i<size;i++) {
        if (i < window || size - 1 - window < i) 
            out[i] = data[i];
        double ssin = 0.0, scos = 0.0;
        for (int j=0;j<2*window;j++) {
            ssin += sin(data[i + j - window]);
            scos += cos(data[i + j - window]);
        }
        out[i] = atan2 (ssin, scos);
    }

    return out;
}

// gaussian smooth
double* math_smooth_double (double *data, int size, int window)
{
    double *nd = (double*)malloc(size*sizeof(double));
    
    assert (window % 2 == 1);

    double sigma = 1.0 * window / 5;
    double *kernel = (double*)malloc(window*sizeof(double));
    int radius = window/2;
    double a = sqrt(1.0 / (2 * PI * sigma * sigma));

    // precompute kernel
    for (int i=-radius;i<=radius;i++) {
        double d = exp ( - ( 1.0 * i * i / (2 * sigma * sigma)));
        kernel[i+radius] = d;
    }

    // apply kernel
    for (int i=0;i<size;i++) {
        
        double val = 0.0;
        double sum = 0.0;

        for (int j=0;j<window;j++) {
            if (i+j>=radius && i-radius+j<size) {
                val += a * data[i-radius+j] * kernel[j];
                sum += a * kernel[j];
            }
        }
        if (fabs (sum) > 1E-6)
            val /= sum;
        
        nd[i] = val;
    }
     
    return nd;
}

int *math_smooth_int (int *data, int size, int window)
{
    double *datad = (double*)malloc(size*sizeof(double));
    for (int i=0;i<size;i++)
        datad[i] = 1.0 * data[i];

    datad = math_smooth_double (datad, size, window);

    int *out = (int*)malloc(size*sizeof(int));
    for (int i=0;i<size;i++)
        out[i] = math_round (datad[i]);

    free (datad);

    return out;
}

// gaussian smooth
unsigned char* math_smooth (unsigned char *data, int size, int window)
{
    unsigned char *nd = (unsigned char*)malloc(size);
    
    assert (window % 2 == 1);

    double sigma = 1.0 * window / 5;
    double kernel[window];
    int radius = window/2;
    double a = sqrt (1.0 / (2 * PI * sigma * sigma));

    // precompute kernel
    for (int i=-radius;i<=radius;i++) {
        double d = exp ( - ( 1.0 * i * i / (2 * sigma * sigma)));
        kernel[i+radius] = d;
    }

    // apply kernel
    for (int i=0;i<size;i++) {
        
        double val = 0.0;
        double sum = 0.0;

        for (int j=0;j<window;j++) {
            if (i+j>=radius && i-radius+j<size) {
                val += a * data[i-radius+j] * kernel[j];
                sum += a * kernel[j];
            }
        }
        if (fabs (sum) > 1E-6)
            val /= sum;
        
        nd[i] = math_round (val);
    }
     
    return nd;
}


gint pair_comp_key (gconstpointer d1, gconstpointer d2, gpointer data)
{
    pair_t *p1 = (pair_t*)d1;
    pair_t *p2 = (pair_t*)d2;

    if (p1->key < p2->key) return -1;
    if (p2->key < p1->key) return 1;
    return 0;
}

gint pair_comp_val (gconstpointer d1, gconstpointer d2, gpointer data)
{
    pair_t *p1 = (pair_t*)d1;
    pair_t *p2 = (pair_t*)d2;

    if (p1->val < p2->val) return -1;
    if (p2->val < p1->val) return 1;
    return 0;
}

gint pair_int_comp_key (gconstpointer d1, gconstpointer d2, gpointer data)
{
    pair_int_t *p1 = (pair_int_t*)d1;
    pair_int_t *p2 = (pair_int_t*)d2;

    if (p1->key < p2->key) return -1;
    if (p2->key < p1->key) return 1;
    return 0;
}

gint pair_int_comp_val (gconstpointer d1, gconstpointer d2, gpointer data)
{
    pair_int_t *p1 = (pair_int_t*)d1;
    pair_int_t *p2 = (pair_int_t*)d2;

    if (p1->val < p2->val) return -1;
    if (p2->val < p1->val) return 1;
    return 0;
}

// compute median of a set of <n> values
//
double math_median_double (double *val, int n)
{
    if (n==0 || !val)
        return 0.0;

    if (n==1)
        return val[0];

    // sort the numbers
    math_sort (val, n);

    // check sort
    for (int i=0;i<n-1;i++) 
        assert (val[i] <= val[i+1]);

    // if n is odd, return middle value
    if (n%2==1)
        return val[n/2];
    else
        return (val[n/2]+val[n/2-1])/2.0;
}

unsigned char math_median (unsigned char *val, int n)
{
    // sort data
    math_sort (val, n);

    // check
    for (int i=0;i<n-1;i++)
        assert (val[i] <= val[i+1]);

    // return middle value
    return val[n/2];
}

#define ELEM_SWAP(a,b) { register double t=(a);(a)=(b);(b)=t; }

/*
 *  This Quickselect routine is based on the algorithm described in
 *  "Numerical recipes in C", Second Edition,
 *  Cambridge University Press, 1992, Section 8.5, ISBN 0-521-43108-5
 *  This code by Nicolas Devillard - 1998. Public domain.
 */
double math_quick_select(double *vals, int n) 
{
    // make a copy of the array
    double *arr = (double*)malloc(n*sizeof(double));
    for (int i=0;i<n;i++)
        arr[i] = vals[i];

    int low, high ;
    int median;
    int middle, ll, hh;

    low = 0 ; high = n-1 ; median = (low + high) / 2;
    for (;;) {
        if (high <= low) { /* One element only */
            double res = arr[median];
        free (arr);
        return res;
        }
        if (high == low + 1) {  /* Two elements only */
            if (arr[low] > arr[high])
                ELEM_SWAP(arr[low], arr[high]) ;
            double res = arr[median];
            free (arr);
            return res;
        }

        /* Find median of low, middle and high items; swap into position low */
        middle = (low + high) / 2;
        if (arr[middle] > arr[high])    ELEM_SWAP(arr[middle], arr[high]) ;
        if (arr[low] > arr[high])       ELEM_SWAP(arr[low], arr[high]) ;
        if (arr[middle] > arr[low])     ELEM_SWAP(arr[middle], arr[low]) ;

        /* Swap low item (now in position middle) into position (low+1) */
        ELEM_SWAP(arr[middle], arr[low+1]) ;

        /* Nibble from each end towards middle, swapping items when stuck */
        ll = low + 1;
        hh = high;
        for (;;) {
            do ll++; while (arr[low] > arr[ll]) ;
            do hh--; while (arr[hh]  > arr[low]) ;

            if (hh < ll)
                break;

            ELEM_SWAP(arr[ll], arr[hh]) ;
        }

        /* Swap middle item (in position low) back into correct position */
        ELEM_SWAP(arr[low], arr[hh]) ;

        /* Re-set active partition */
        if (hh <= median)
            low = ll;
        if (hh >= median)
            high = hh - 1;
    }
}

#undef ELEM_SWAP

// compute the 2D reprojection of a 2D point (x,y) on a 2D line segment (x1, y1, x2, y2)
// the result is returned as the distance from (x1,y1) along the line segment <lambda>
// as well as the 2D position (xo, yo)
// return TRUE on success
//
gboolean math_project_2d (double x, double y, double x1, double y1, double x2, double y2, 
        double *lambda, double *xo, double *yo)
{
    double norm = sqrt ((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
    if (norm < 1E-6) return FALSE;

    double ux = (x2-x1)/norm;
    double uy = (y2-y1)/norm;
    double px = (x-x1);
    double py = (y-y1);

    double l = ux * px + uy * py;

    if (lambda) *lambda = l;
    if (xo) *xo = x1 + l * ux;
    if (yo) *yo = y1 + l * uy;

    return TRUE;
}

// project a 2D point onto a segment. if the projection lies outside of the segment,
// it is clipped to the nearest point on the segment
// input: point to project (x,y), segment (x1,y1,x2,y2)
// output: the projected point (xo, y0) and the distance to the segment <dist>
//
gboolean math_project_2d_segment (double x, double y, double x1, double y1, double x2, double y2,
        double *xo, double *yo, double *dist)
{
    double lambda, xp, yp;

    // project the 2D point on the line
    if (!math_project_2d (x, y, x1, y1, x2, y2, &lambda, &xp, &yp)) {
        return FALSE;
    }

    double norm = sqrt ((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));

    // clip to the nearest point on the segment
    if (lambda < 0) {
        xp = x1; yp = y1;
    }
    if (norm < lambda) {
        xp = x2; yp = y2;
    }

    if (xo) *xo = xp;
    if (yo) *yo = yp;

    if (dist) {
        *dist = sqrt ((xp-x)*(xp-x)+(yp-y)*(yp-y));
    }

    return TRUE;
}





// compute the interesection of two 2D segments (x1,y1-x2,y2) and (x3,y3-x4,y4)
// return true if discrimant close to zero
//
int intersect (double x1, double y1, double x2, double y2, 
        double x3, double y3, double x4, double y4,
        double err, double *x, double *y, double *l1, double *l2)
{
    double delta = (y4-y3)*(x2-x1)-(y2-y1)*(x4-x3);
    if (fabs (delta) < err) 
        return 0;

    double comp = (y4-y3)*(x2-x1)*y1+(y4-y3)*(y2-y1)*(x3-x1)-(y2-y1)*(x4-x3)*y3;

    *y = comp / delta;

    if (fabs (y4-y3) < err)
        *x = x1 + (*y-y1)*(x2-x1)/(y2-y1);
    else
        *x = x3 + (*y-y3)*(x4-x3)/(y4-y3);

    // compute lambdas
    if (fabs (x2-x1)<err)
        *l1 = (*y-y2)/(y2-y1);
    else
        *l1 = (*x-x2)/(x2-x1);

    if (fabs (x4-x3)<err)
        *l2 = (*y-y4)/(y4-y3);
    else
        *l2 = (*x-x4)/(x4-x3);

    return 1;
}

double math_gaussian (double x, double mu, double sigma)
{
    return exp (-(x-mu)*(x-mu)/(sigma*sigma));
}

double math_square_distance_double (double x1, double y1, double x2, double y2)
{
    return (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2);
}

void histogram_min_max (double *data, int ndata, double *min, double *max)
{
    if (!data || ndata == 0)
        return;

    // determine data range
    double maxval = data[0], minval = data[0];
    for (int i=0;i<ndata;i++) {
        maxval = fmax (maxval, data[i]);
        minval = fmin (minval, data[i]);
    }

    *min = minval;
    *max = maxval;
}

// build a histogram of <size> buckets
//
int *histogram_build (double *data, int ndata, int size,
        double *minval, double *maxval)
{
    if (!data || ndata == 0 || size == 0)
        return NULL;

    // determine data range
    double maxv = data[0], minv = data[0];
    for (int i=0;i<ndata;i++) {
        maxv = fmax (maxv, data[i]);
        minv = fmin (minv, data[i]);
    }

    if (maxv-minv < 1E-6)
        return NULL;

    int *hist = (int*)malloc(size*sizeof(int));
    for (int i=0;i<size;i++)
        hist[i] = 0;

    for (int i=0;i<ndata;i++) {
        int bucket = math_round((data[i]-minv) * size / (maxv-minv));
        bucket = MIN (size-1, MAX (0, bucket));
        hist[bucket]++;
    }

    if (maxval)
        *maxval = maxv;
    if (minval)
        *minval = minv;

    return hist;
}

void histogram_write_to_file (int *hist, int size, const char *filename)
{
    FILE *fp = fopen (filename, "w");
    if (!fp) {
        fprintf (stderr, "common/histogram_write_to_file: failed to open %s in write mode", filename);
        return;
    }

    for (int i=0;i<size;i++) {
        fprintf (fp, "%d %d\n", i, hist[i]);
    }

    fclose (fp);
}
void histogram_write_to_file_full (int *hist, int size, const char *filename, double minval, double maxval)
{
    FILE *fp = fopen (filename, "w");
    if (!fp) {
        fprintf (stderr, "common/histogram_write_to_file: failed to open %s in write mode", filename);
        return;
    }

    for (int i=0;i<size;i++) {
        fprintf (fp, "%.3f %d\n", minval + i*(maxval-minval)/size, hist[i]);
    }

    fclose (fp);
}
void histogram_write_to_file_double (double *hist, int size, const char *filename)
{
    FILE *fp = fopen (filename, "w");
    if (!fp) {
        fprintf (stderr, "common/histogram_write_to_file: failed to open %s in write mode", filename);
        return;
    }

    for (int i=0;i<size;i++) {
        fprintf (fp, "%d %.4f\n", i, hist[i]);
    }

    fclose (fp);
}
void histogram_write_to_file_double_full (double *hist, int size, const char *filename, double minval, double maxval)
{
    FILE *fp = fopen (filename, "w");
    if (!fp) {
        fprintf (stderr, "common/histogram_write_to_file: failed to open %s in write mode", filename);
        return;
    }

    for (int i=0;i<size;i++) {
        fprintf (fp, "%.3f %.4f\n", minval + i*(maxval-minval)/size, hist[i]);
    }

    fclose (fp);
}

int math_max_index_double (double *data, int n)
{
    assert (data);
    int best_index = 0;
    double maxval = data[0];
    for (int i=1;i<n;i++) {
        if (maxval < data[i]) {
            maxval = data[i];
            best_index = i;
        }
    }

    return best_index;
}

int histogram_peak (int *data, int ndata, int *peak)
{
    int best_index = 0;
    int best_val = data[0];
    for (int i=0;i<ndata;i++) {
        if (data[i] > best_val) {
            best_val = data[i];
            best_index = i;
        }
    }

    if (peak)
        *peak = best_val;

    return best_index;
}

void math_normalize (double *data, int ndata)
{
    // compute square sum
    double sq = 0.0;
    for (int i=0;i<ndata;i++)
        sq += data[i]*data[i];

    sq = sqrt (sq);

    if (sq < 1E-6)
        return;

    for (int i=0;i<ndata;i++)
        data[i] /= sq;
}

void math_normalize_float (float *data, int ndata)
{
    // compute square sum
    float sq = 0.0;
    for (int i=0;i<ndata;i++)
        sq += data[i]*data[i];

    sq = sqrt (sq);

    if (sq < 1E-6)
        return;

    for (int i=0;i<ndata;i++)
        data[i] /= sq;
}

int histogram_find_orientation (unsigned char *data, int size)
{
    // map the histogram to [0,360] and consider each bin as an angle
    //
    double ssin = 0.0, scos = 0.0;

    for (int i=0;i<size;i++) {
        double angle = 2 * PI * i / size;
        ssin += data[i]*sin (angle);
        scos += data[i]*cos (angle);
    }

    double a = atan2 (ssin, scos);

    int index = math_round ((a + PI) / (2 * PI) * (size-1));

    index = MIN(size-1, MAX(0, index));

    return index;
}

double math_mean_char (unsigned char *data, int n)
{
    double avg = 0.0;

    for (int i=0;i<n;i++)
        avg += data[i];

    return avg/n;
}


double math_mean_double (double *data, int size)
{
    if (size == 0 || !data)
        return 0;

    double m = 0.0;

    for (int i=0;i<size;i++)
        m += data[i];

    return m / size;
}

double math_stdev (double *data, int size)
{
    double m = math_mean_double (data, size);

    double v = 0.0;

    for (int i=0;i<size;i++)
        v += (data[i] - m)*(data[i] - m);

    return sqrt (v / size);
}

double histogram_variance (double *data, int ndata, double mean)
{
    double num = 0.0;
    double den = 0.0;

    for (int i=0;i<ndata;i++) {
        num += data[i]*(i-mean)*(i-mean);
        den += data[i];
    }

    if (fabs(den) < 1E-6)
        return -1;
    else
        return num/den;
}

double histogram_mean (double *data, int ndata)
{
    double num = 0.0;
    double den = 0.0;

    for (int i=0;i<ndata;i++) {
        num += data[i]*i;
        den += data[i];
    }

    if (fabs(den) < 1E-6)
        return -1;
    else
        return num/den;
}

void histogram_min_threshold (double *data, int ndata, double thresh)
{
    for (int i=0;i<ndata;i++)
        if (data[i] < thresh)
            data[i] = 0.0;
}

int histogram_min (double *data, int ndata, double *val)
{
    if (ndata == 0)
        return -1;

    double minval = data[0];
    int index = 0;

    for (int i=1;i<ndata;i++) {
        if (data[i] < minval) {
            minval = data[i];
            index = i;
        }
    }

    *val = minval;

    return index;
}
int histogram_max (double *data, int ndata, double *val)
{
    if (ndata == 0)
        return -1;

    double maxval = data[0];
    int index = 0;

    for (int i=1;i<ndata;i++) {
        if (data[i] > maxval) {
            maxval = data[i];
            index = i;
        }
    }

    *val = maxval;

    return index;
}

// least-square fitting of a 2D line on a set of <n> 2D points <pts> [x1, y1, ..., xn, yn]
// the method returns y = ax + b and theta = atan(a)
// source: mathworld.com least square fitting
//
int math_line_fit (double *pts, int n, double *a, double *b, double *theta)
{
    double xbar = 0.0, ybar = 0.0;
    double covxy = 0.0;

    if (n == 0)
        return -1;

    for (int i=0;i<n;i++) {
        xbar += pts[2*i];
        ybar += pts[2*i+1];
        covxy += pts[2*i]*pts[2*i+1];
    }

    xbar /= n;
    ybar /= n;

    double ssxy = covxy - n*xbar*ybar;
    double ssxx = 0.0;
    for (int i=0;i<n;i++)
        ssxx += (pts[2*i]-xbar)*(pts[2*i]-xbar);

    if (fabs(ssxx) < 1E-6) {
        *a = *b = 0.0;
        *theta = PI/2;
        return 0;
    }

    *a = ssxy/ssxx;

    *b = ybar - *a * xbar;

    *theta = atan2 (*a, 1); // in [-pi, pi]

    // clip within [-pi/2, pi/2]
    if (*theta > PI/2)
        *theta = *theta - PI;
    if (*theta < -PI/2)
        *theta = *theta + PI;

    return 0;
}


int rand_int (int min, int max)
{
    int a = MIN (max, MAX (min, (int)(min+1.0*(max-min)*rand()/RAND_MAX)));

    return a;
}

int math_find_index (int *indices, int n, int index)
{
    for (int i=0;i<n;i++) {
        if (indices[i] == index)
            return i;
    }

    return -1;
}

// generate a set of <n> different integers between <min> and <max>
//
int *math_random_indices (int n, int min, int max)
{
    if (max < min || max - min < n)
        return NULL;

    // init
    int *data = (int*)malloc(n*sizeof(int));
    for (int i=0;i<n;i++)
        data[i] = -1;

    // pick random numbers until done
    int count = 0;

    while (1) {
        int index = rand_int (min, max);
        if (math_find_index (data, n, index) >= 0)
            continue;
        data[count] = index;
        count++;
        if (count == n)
            break;
    }

    return data;
}

// n-size vectors
//
double *vect_read (FILE *fp, int *nval)
{
    if (!fp) {
        *nval = 0;
        return NULL;
    }

    double *vals=NULL;

    while (!feof (fp)) {
        double v;
        if (fscanf (fp, "%lf", &v) == 1) {
            vals = (double*)realloc (vals, (*nval+1)*sizeof(double));
            vals[*nval] = v;
            *nval = *nval + 1;
        }
    }

    return vals;
}

void vect_write (FILE *fp, double *vals, int nval)
{
    if (!fp) {
        return;
    }

    for (int i=0;i<nval;i++) {
        fprintf (fp, "%f\n", vals[i]);
    }
}

double vect_variance (double *data, int ndata, double mean)
{
    double variance = 0.0;

    for (int i=0;i<ndata;i++)
        variance += (data[i]-mean)*(data[i]-mean);

    variance /= ndata;

    return variance;
}

double vect_stdev (double *data, int ndata, double mean)
{
    double var = vect_variance (data, ndata, mean);

    return sqrt (var);
}

double vect_mean_double (double *data, int ndata)
{
    double mean = 0.0;

    for (int i=0;i<ndata;i++)
        mean += data[i];

    mean /= ndata;

    return mean;
}

unsigned char vect_mean_char (unsigned char *data, int ndata)
{
    int mean = 0;

    for (int i=0;i<ndata;i++)
        mean += data[i];

    mean = MIN(255, MAX (0, math_round (mean / ndata)));

    return (unsigned char)mean;
}

double vect_mean_sincos (double *data, int ndata)
{
    double ssin = 0.0, scos = 0.0;

    for (int i=0;i<ndata;i++) {
        ssin += sin (data[i]);
        scos += cos (data[i]);
    }

    return atan2 (ssin, scos);
}

/* assume that weights are in range 0 - 1
 */
double vect_weighted_mean_sincos (double *data, double *weight, int ndata)
{
    double ssin = 0.0, scos = 0.0;

    for (int i=0;i<ndata;i++) {
        ssin += weight[i] * sin (data[i]);
        scos += weight[i] * cos (data[i]);
    }

    return atan2 (ssin, scos);
}

// assumes that <mean> is between -PI and PI
//
double vect_variance_sincos (double *data, int ndata, double mean)
{
    double variance = 0.0;

    for (int i=0;i<ndata;i++) {

        double ang = clip_value (data[i], -PI, PI, 1E-6);

        double dff = diff_angle_plus_minus_pi (ang, mean);

        variance += dff * dff;
    }

    return variance / ndata;
}

double vect_stdev_sincos (double *data, int ndata, double mean)
{
    return sqrt (vect_variance_sincos (data, ndata, mean));
}

float vect_sqdist_char (float *d1, unsigned char *d2, int size)
{
    float sqdist = 0.0;
    float d = 0.0;
    for (int i=0;i<size;i++) {
        d = d1[i]-d2[i];
        sqdist += d * d;
    }

    return sqdist;
}

double vect_sqdist_double (double *d1, unsigned char *d2, int size)
{
    double sqdist = 0.0;
    double d = 0.0;
    for (int i=0;i<size;i++) {
        d = d1[i]-d2[i];
        sqdist += d * d;
    }

    return sqdist;
}

double vect_sqdist_double_double (double *d1, double *d2, int size)
{
    double sqdist = 0.0;
    double d = 0.0;
    for (int i=0;i<size;i++) {
        d = d1[i]-d2[i];
        sqdist += d * d;
    }

    return sqdist;
}

float vect_sqdist_float (float *d1, float *d2, int size)
{
    float sqdist = 0.0;
    float d = 0.0;
    for (int i=0;i<size;i++) {
        d = d1[i]-d2[i];
        sqdist += d * d;
    }

    return sqdist;
}

double vect_dot_double (double *d1, double *d2, int size)
{
    double d = .0;
    for (int i=0;i<size;i++)
        d += d1[i] * d2[i];
    return d;
}
float vect_dot_float (float *d1, float *d2, int size)
{
    double d = .0;
    for (int i=0;i<size;i++)
        d += d1[i] * d2[i];
    return d;
}


double vect_sqdist_double (double *d1, double *d2, int size)
{
    double sqdist = 0.0;
    double d = 0.0;
    for (int i=0;i<size;i++) {
        d = d1[i]-d2[i];
        sqdist += d * d;
    }

    return sqdist;
}

int vect_sqdist_char (unsigned char *d1, unsigned char *d2, int size)
{
    int sqdist = 0;
    int d = 0;
    for (int i=0;i<size;i++) {
        d = d1[i]-d2[i];
        sqdist += d * d;
    }

    return sqdist;
}

/* compute the minimum NCC between two patches with a max offset in (col, row)
*/
double vect_dist_min_ncc (double *d1, double *d2, int size, int max_col, int max_row)
{
    int w = math_round (sqrt (1.0*size));
    assert (w*w == size);

    int width = MIN (w - 2*max_col, w - 2*max_row);

    double min_ncc = 2.0;
    for (int offset_row = -max_row; offset_row <= max_row; offset_row++) {
        for (int offset_col = -max_col; offset_col <= max_col; offset_col++) {
            double ncc = vect_dist_ncc_offset (d1, d2, size, width, offset_col, offset_row);
            if (ncc < min_ncc)
                min_ncc = ncc;
        }
    }

    return min_ncc;
}

/* compute the NCC between two patches with an offset in (col, row)
 * size:  length of the descriptor vector (e.g. 49 for a 7x7 patch)
 * width: width of the inside patch used for NCC computation (e.g. 5 for a 7x7 patch with an offset of maximum 1 pixel)
 */
double vect_dist_ncc_offset (double *d1, double *d2, int size, int width, int offset_col, int offset_row)
{
    // check boundaries condition'
    assert (size % 2 == 1 && width % 2 == 1);
    int w = math_round (sqrt (1.0*size));
    assert (w*w == size);
    int col0 = (w-width)/2;
    int row0 = (w-width)/2;
    //printf ("size: %d  width: %d  off_col: %d  off_row: %d  col0: %d  row0: %d\n",
    //        size, width, offset_col, offset_row, col0, row0);
    assert (col0 + offset_col >= 0);
    assert (row0 + offset_row >= 0);
    assert ((col0 + offset_col + width - 1)*(col0 + offset_col + width - 1) < size);
    assert ((row0 + offset_row + width - 1)*(row0 + offset_row + width - 1) < size);
    col0 += offset_col;
    row0 += offset_row;

    // create the vectors for a patch of size width x width
    unsigned char *r1 = (unsigned char*)malloc(width*width);
    unsigned char *r2 = (unsigned char*)malloc(width*width);

    for (int row=0;row<width;row++) {
        for (int col=0;col<width;col++) {
            r1[row*width+col] = d1[(row0+row)*width+(col0+col)];
            r2[row*width+col] = d2[(row0+row)*width+(col0+col)];
        }
    }   

    // compute NCC
    double ncc = vect_dist_ncc (r1, r2, width*width);

    // free and return
    free (r1);
    free (r2);
    return ncc;
}

double vect_dist_ncc (unsigned char *d1, unsigned char *d2, int size)
{
    // compute mean
    double av1 = 1.0 * math_mean_char (d1, size);
    double av2 = 1.0 * math_mean_char (d2, size);

    // compute denominator
    double den1 = 0.0, den2 = 0.0;
    for (int i=0;i<size;i++) { 
        den1 += (1.0*d1[i]-av1)*(1.0*d1[i]-av1);
        den2 += (1.0*d2[i]-av2)*(1.0*d2[i]-av2);
    }
    double den = sqrt (den1 * den2);

    // compute nominator
    double num=0.0;
    for (int i=0;i<size;i++) {
        num += (1.0*d1[i]-av1)*(1.0*d2[i]-av2);
    }

    double res = fabs(den)>1E-6 ? num / den : 0;

    assert (-1.0 - 1E-6 <= res && res <= 1.0 + 1E-6 );

    return 1.0 - res;
}

double vect_sqdist (unsigned char *d1, double *d2, int size)
{
    double sqdist = 0;
    double d = 0;
    for (int i=0;i<size;i++) {
        d = 1.0*d1[i]-d2[i];
        sqdist += d * d;
    }

    return sqdist;
}      

int *vect_append (int *data, int *size, int val)
{
    data = (int*)realloc(data, (*size+1)*sizeof(int));
    data[*size] = val;
    *size = *size + 1;
    return data;
}

int *vect_remove_binary (int *data, int *size, int val)
{
    int index = vect_search_binary (data, *size, val);

    assert (index >= 0 && index < *size-1);

    for (int i=index;i<*size-1;i++) data[i] = data[i+1];

    *size = *size-1;
    data = (int*)realloc(data, (*size)*sizeof(int));
    return data;
}

// assume that the vector is sorted
// 
int vect_search_binary (int *data, int size, int val)
{
    if (size == 0) return -1;

    int start = 0, end = size-1;
    if (val < data[start] || data[end] < val) return -1;

    while (1) {
        if (data[start] == val) return start;
        if (data[end] == val)   return end;
        if (end-start <= 1) return -1;
        int middle = (start+end)/2;
        if (data[middle] < val) start = middle;
        else end = middle;
    }
    return -1;
}

quartet_t* quartet_new (int a, int b, int c, int d)
{
    quartet_t *q = (quartet_t*)malloc(sizeof(quartet_t));
    q->a = a;
    q->b = b;
    q->c = c;
    q->d = d;
    return q;
}

guint m_quartet_hash (gconstpointer key)
{
    quartet_t *q = (quartet_t*)key;

    guint ha = g_int_hash (&q->a);
    guint hb = g_int_hash (&q->b);
    guint hc = g_int_hash (&q->c);
    guint hd = g_int_hash (&q->d);

    return ha ^ hb ^ hc ^ hd;
}

gboolean m_quartet_equal (gconstpointer v1, gconstpointer v2)
{
    quartet_t *q1 = (quartet_t*)v1;
    quartet_t *q2 = (quartet_t*)v2;

    return (q1->a == q2->a && q1->b == q2->b && q1->c == q2->c && q1->d == q2->d);
}

gpointer m_quartetdup (const quartet_t *q)
{
    quartet_t *r = g_slice_new (quartet_t);
    r->a = q->a;
    r->b = q->b;
    r->c = q->c;
    r->d = q->d;

    return r;
}

void m_quartet_free (quartet_t *q)
{
    g_slice_free(quartet_t, q);
}

void g_str_free (char *q)
{
    free (q);
}

double math_overlap (double a1, double a2, double b1, double b2)
{
    assert (a1 <= a2 && b1 <= b2);

    if (fabs(a2-a1) < 1E-6 || fabs(b2-b1) < 1E-6) return 0.0;
    if (fabs(a2-a1) < 1E-6 && fabs(b2-b1) > 1E-6) return 1.0;
    if (fabs(b2-b1) < 1E-6 && fabs(a2-a1) > 1E-6) return 1.0;

    // full overlap
    //
    if (b1 <= a1 && a2 <= b2) return (a2-a1)/(b2-b1);
    if (a1 <= b1 && b2 <= a2) return (b2-b1)/(a2-a1);

    // full exclusion
    //
    if (a2 <= b1 || b2 <= a1) return 0.0;

    // partial overlap
    //
    if (a1 < b1) return (a2-b1)/(b2-a1);
    if (b1 < a1) return (b2-a1)/(a2-b1);

    printf ("%.4f %.4f  %.4f %.4f\n", a1, a2, b1, b2); fflush(stdout);
    assert (false);
    return 0.0;
}

int * g_intdup(gint value)
{
    gint * tmp;
    tmp = g_new(gint,1);
    *tmp = value;
    return tmp;
}

float *g_floatdup (float value)
{
    float * tmp;
    tmp = g_new (float,1);
    *tmp = value;
    return tmp;
}
gdouble *g_doubledup (gdouble value)
{
    gdouble * tmp;
    tmp = g_new (gdouble,1);
    *tmp = value;
    return tmp;
}
// compute the distance between a 2d point (x,y) and a 2D segment (a,b)-(c,d)
//
gboolean math_dist_2d (double x, double y, double a, double b, double c, double d, double *oxh, double *oyh, double *res)
{
    double ux = c-a;
    double uy = d-b;
    double len = sqrt(pow(ux,2)+pow(uy,2));
    if (fabs(len) < 1E-6)
        return FALSE;

    ux /= len;
    uy /= len;

    double lambda = ux * (x - a) + uy * (y - b);
    if (lambda < 0 || lambda > len) return FALSE;

    double xh = a + lambda * ux;
    double yh = b + lambda * uy;

    if (res) *res = sqrt (pow(xh-x,2) + pow(yh-y,2));
    if (oxh) *oxh = xh;
    if (oyh) *oyh = yh;
    return TRUE;
}

void vect_normalize (double *data, int size, double val)
{
    // normalize 
    double total = .0;
    for (int i=0;i<size;i++)
        total += data[i];
    if (total > 1E-6) {
        for (int i=0;i<size;i++) 
            data[i] *= val/total;
    }

}

// subtract the principal point (cc) and divide by focal length (fc)
//
void calib_normalize (double *x, double *y, double *fc, double *cc)
{
    *x = (*x-cc[0])/fc[0];
    *y = (*y-cc[1])/fc[1];
}

// subtract the principal point (cc) and divide by focal length (fc)
//
void calib_denormalize (double *x, double *y, double *fc, double *cc)
{
    *x = fc[0] * (*x) + cc[0];
    *y = fc[1] * (*y) + cc[1];
}

// distortion (first-order)
//
void calib_distort_1st_order (double *x, double *y, double k)
{
    double r2 = pow(*x,2) + pow(*y,2);
    *x = (1.0 + k * r2) * (*x);
    *y = (1.0 + k * r2) * (*y);
}

// undistortion (first-order approximation)
//
void calib_undistort_1st_order (double *x, double *y, double k)
{
    double rd[2], rc;
    double r2 = pow(*x,2) + pow(*y,2);
    rd[0] = 1.0 + k * r2;
    rd[1] = 1.0 + k * r2;
    rc = r2 / rd[0];
    rd[0] = 1.0 + rc * k;
    rd[1] = 1.0 + rc * k;
    *x = *x / rd[0];
    *y = *y / rd[0];
}

// undistortion (recursive)
//
void calib_undistort (double *x, double *y, double *k)
{
    double k1 = k[0];
    double k2 = k[1];
    double k3 = k[4];
    double p1 = k[2];
    double p2 = k[3];
    double dx[2];
    double x0 = *x;
    double y0 = *y;

    for (int kk=0;kk<20;kk++) {
        double r2 = pow(*x,2) + pow(*y,2);
        double kr = 1.0 + k1 * r2 + k2 * pow(r2,2) + k3 * pow(r2,3);
        dx[0] =  2*p1*(*x)*(*y) + p2 * (r2 + 2 * pow((*x),2));
        dx[1] =  2*p2*(*x)*(*y) + p1 * (r2 + 2 * pow((*y),2));
        (*x) = (x0 - dx[0])/(kr);
        (*y) = (y0 - dx[1])/(kr);
    }
}

float **math_init_mat_float (int m, int n, float val)
{
    float **p = (float**)malloc(m*sizeof(float*));
    for (int i=0;i<m;i++) {
        p[i] = (float*)malloc(n*sizeof(float));
        for (int j=0;j<n;j++)
            p[i][j] = val;
    }
    return p;
}

int **math_init_mat_int (int m, int n, int val)
{
    int **p = (int**)malloc(m*sizeof(int*));
    for (int i=0;i<m;i++) {
        p[i] = (int*)malloc(n*sizeof(int));
        for (int j=0;j<n;j++)
            p[i][j] = val;
    }
    return p;
}


double *math_3d_array_alloc_double (int p, int q, int r)
{
    double *a = (double*)calloc(p*q*r, sizeof(double));

    return a;
}

double math_3d_array_get_double (double *a, int p, int q, int r, int i, int j, int k)
{
    int index = i * q * r + j * r + k;
    
    assert (0 <= i && i < p && 0 <= j && j < q && 0 <= k && k < r);
    
    return a[index];
}

void math_3d_array_set_double (double *a, int p, int q, int r, int i, int j, int k, double val)
{
    int index = i * q * r + j * r + k;
    
    assert (0 <= i && i < p && 0 <= j && j < q && 0 <= k && k < r);
    
    a[index] = val;
}

int *math_3d_array_alloc_int (int p, int q, int r)
{
    int *a = (int*)calloc(p*q*r, sizeof(int));

    return a;
}

int math_3d_array_get_int (int *a, int p, int q, int r, int i, int j, int k)
{
    int index = i * q * r + j * r + k;
    
    assert (0 <= i && i < p && 0 <= j && j < q && 0 <= k && k < r);
    
    return a[index];
}

void math_3d_array_set_int (int *a, int p, int q, int r, int i, int j, int k, int val)
{
    int index = i * q * r + j * r + k;

    assert (0 <= i && i < p && 0 <= j && j < q && 0 <= k && k < r);

    a[index] = val;
}

void secs_to_hms (int isecs, int *hours, int *mins, int *secs)
{
    *secs = isecs;
    *mins = (int)(isecs/60);
    *hours = (int)(isecs/3600);
    *mins -= 60 * *hours;
    *secs -= 3600 * *hours + 60 * *mins;
}

