/*
** libproj -- library of cartographic projections
**
** Copyright (c) 2003, 2006   Gerald I. Evenden
*/
static const char
LIBPROJ_ID[] = "Id";
/*
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files (the
** "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to
** permit persons to whom the Software is furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#define PROJ_LIB__
#include  <lib_proj.h>
PROJ_HEAD(robin, "Robinson") "\n\tPCyl., Sph.";
#define V(C,z) (C.c0 + z * (C.c1 + z * (C.c2 + z * C.c3)))
#define DV(C,z) (C.c1 + z * (C.c2 + C.c2 + z * 3. * C.c3))
/* note: following terms based upon 5 deg. intervals in degrees. */
static struct COEFS {
  float c0, c1, c2, c3;
} X[] = {
{1.0f,  -5.67239e-12f,  -7.15511e-05f,  3.11028e-06f},
{0.9986f,  -0.000482241f,  -2.4897e-05f,  -1.33094e-06f},
{0.9954f,  -0.000831031f,  -4.4861e-05f,  -9.86588e-07f},
{0.99f,  -0.00135363f,  -5.96598e-05f,  3.67749e-06f},
{0.9822f,  -0.00167442f,  -4.4975e-06f,  -5.72394e-06f},
{0.973f,  -0.00214869f,  -9.03565e-05f,  1.88767e-08f},
{0.96f,  -0.00305084f,  -9.00732e-05f,  1.64869e-06f},
{0.9427f,  -0.00382792f,  -6.53428e-05f,  -2.61493e-06f},
{0.9216f,  -0.00467747f,  -0.000104566f,  4.8122e-06f},
{0.8962f,  -0.00536222f,  -3.23834e-05f,  -5.43445e-06f},
{0.8679f,  -0.00609364f,  -0.0001139f,  3.32521e-06f},
{0.835f,  -0.00698325f,  -6.40219e-05f,  9.34582e-07f},
{0.7986f,  -0.00755337f,  -5.00038e-05f,  9.35532e-07f},
{0.7597f,  -0.00798325f,  -3.59716e-05f,  -2.27604e-06f},
{0.7186f,  -0.00851366f,  -7.0112e-05f,  -8.63072e-06f},
{0.6732f,  -0.00986209f,  -0.000199572f,  1.91978e-05f},
{0.6213f,  -0.010418f,  8.83948e-05f,  6.24031e-06f},
{0.5722f,  -0.00906601f,  0.000181999f,  6.24033e-06f},
{0.5322f, 0.f,0.f,0.f}  },
Y[] = {
{0.0f,  0.0124f,  3.72529e-10f,  1.15484e-09f},
{0.062f,  0.0124001f,  1.76951e-08f,  -5.92321e-09f},
{0.124f,  0.0123998f,  -7.09668e-08f,  2.25753e-08f},
{0.186f,  0.0124008f,  2.66917e-07f,  -8.44523e-08f},
{0.248f,  0.0123971f,  -9.99682e-07f,  3.15569e-07f},
{0.31f,  0.0124108f,  3.73349e-06f,  -1.1779e-06f},
{0.372f,  0.0123598f,  -1.3935e-05f,  4.39588e-06f},
{0.434f,  0.0125501f,  5.20034e-05f,  -1.00051e-05f},
{0.4968f,  0.0123198f,  -9.80735e-05f,  9.22397e-06f},
{0.5571f,  0.0120308f,  4.02857e-05f,  -5.2901e-06f},
{0.6176f,  0.0120369f,  -3.90662e-05f,  7.36117e-07f},
{0.6769f,  0.0117015f,  -2.80246e-05f,  -8.54283e-07f},
{0.7346f,  0.0113572f,  -4.08389e-05f,  -5.18524e-07f},
{0.7903f,  0.0109099f,  -4.86169e-05f,  -1.0718e-06f},
{0.8435f,  0.0103433f,  -6.46934e-05f,  5.36384e-09f},
{0.8936f,  0.00969679f,  -6.46129e-05f,  -8.54894e-06f},
{0.9394f,  0.00840949f,  -0.000192847f,  -4.21023e-06f},
{0.9761f,  0.00616525f,  -0.000256001f,  -4.21021e-06f},
{1.0f, 0.0f,0.0f,0.0f}  };
#define FXC  0.8487
#define FYC  1.3523
#define C1  11.45915590261646417544
#define RC1  0.08726646259971647884
#define NODES  18
#define ONEEPS  1.000001
#define EPS  1e-8
FORWARD(s_forward); /* spheroid */
  int i;
  double dphi;
  (void) P; /* avoid warning */

  i = (int)floor((dphi = fabs(lp.phi)) * C1);
  if (i >= NODES) i = NODES - 1;
  dphi = RAD_TO_DEG * (dphi - RC1 * i);
  xy.x = V(X[i], dphi) * FXC * lp.lam;
  xy.y = V(Y[i], dphi) * FYC;
  if (lp.phi < 0.) xy.y = -xy.y;
  return (xy);
}
INVERSE(s_inverse); /* spheroid */
  int i;
  double t, t1;
  struct COEFS T;
  (void) P; /* avoid warning */

  lp.lam = xy.x / FXC;
  lp.phi = fabs(xy.y / FYC);
  if (lp.phi >= 1.) { /* simple pathologic cases */
    if (lp.phi > ONEEPS) I_ERROR
    else {
      lp.phi = xy.y < 0. ? -HALFPI : HALFPI;
      lp.lam /= X[NODES].c0;
    }
  } else { /* general problem */
    /* in Y space, reduce to table interval */
    for (i = (int)floor(lp.phi * NODES);;) {
      if (Y[i].c0 > lp.phi) --i;
      else if (Y[i+1].c0 <= lp.phi) ++i;
      else break;
    }
    T = Y[i];
    /* first guess, linear interp */
    t = 5. * (lp.phi - T.c0)/(Y[i+1].c0 - T.c0);
    /* make into root */
    T.c0 -= (float)lp.phi;
    for (;;) { /* Newton-Raphson reduction */
      t -= t1 = V(T,t) / DV(T,t);
      if (fabs(t1) < EPS)
        break;
    }
    lp.phi = (5 * i + t) * DEG_TO_RAD;
    if (xy.y < 0.) lp.phi = -lp.phi;
    lp.lam /= V(X[i], t);
  }
  return (lp);
}
FREEUP; if (P) free(P); }
ENTRY0(robin) P->es = 0.; P->inv = s_inverse; P->fwd = s_forward; ENDENTRY(P)
/*
** Log: proj_robin.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
