#include <stdio.h>
#include <math.h>

#include "bessel.h"

static float
bessi0 (float x)
{
    float ax, ans;
    double y;

    if ((ax=fabs(x)) < 3.75) {
        y = x/3.75;
        y *= y;
        ans = 1.0 + y * (3.5156229 + y * (3.0899424 + y * (1.2067492
                        + y * (0.2659732 + y * (0.360768e-1
                                + y * 0.45813e-2)))));
    }
    else {
        y = 3.75/ax;
        ans = (exp(ax)/sqrt(ax)) * (0.39894228 + y * (0.1328592e-1
                    + y * (0.225319e-2 + y * (-0.157565e-2 + y * (0.916281e-2
                    + y * (-0.2057706e-1 + y * (0.2635537e-1 + y *(-0.1647633e-1
                    + y * 0.392377e-2))))))));
    }
    return ans;
}

#define ACC 40.0
#define BIGNO   1.0e10
#define BIGNI   1.0e-10

void
besseli (float * I, int n, float x)
{
    int j, k;
    float bi, bim, bip, tox;

    I[0] = bessi0(x);
    if (n == 0)
        return;

    if (x == 0.0) {
        for (j = 1; j <= n; j++)
            I[j] = 0.0;
        return;
    }

    tox = 2.0/fabs(x);
    bip = 0.0;
    bi = 1.0;
    for (j = 2*(n + (int) sqrt(ACC*n)); j > 0; j--) {
        bim = bip + j*tox*bi;
        bip = bi;
        bi = bim;
        if (fabs (bi) > BIGNO) {
            for (k = j+1; k <= n; k++)
                I[k] *= BIGNI;
            bi *= BIGNI;
            bip *= BIGNI;
        }
        if (j <= n)
            I[j] = bip;
    }
    for (j = 1; j <= n; j++) {
        I[j] *= (x < 0.0 && (n & 1) ? -1 : 1) * I[0] / bi;
    }
}
