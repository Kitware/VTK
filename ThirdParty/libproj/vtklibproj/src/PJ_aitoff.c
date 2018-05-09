/******************************************************************************
 * Project:  PROJ.4
 * Purpose:  Implementation of the aitoff (Aitoff) and wintri (Winkel Tripel)
 *           projections.
 * Author:   Gerald Evenden
 *
 ******************************************************************************
 * Copyright (c) 1995, Gerald Evenden
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

#define PROJ_PARMS__ \
	double	cosphi1; \
	int		mode;
#define PJ_LIB__
#include <projects.h>

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#  define M_PI_2 1.57079632679489661923
#endif

PROJ_HEAD(aitoff, "Aitoff") "\n\tMisc Sph";
PROJ_HEAD(wintri, "Winkel Tripel") "\n\tMisc Sph\n\tlat_1";

FORWARD(s_forward); /* spheroid */
	double c, d;

	if((d = acos(cos(lp.phi) * cos(c = 0.5 * lp.lam)))) {/* basic Aitoff */
		xy.x = 2. * d * cos(lp.phi) * sin(c) * (xy.y = 1. / sin(d));
		xy.y *= d * sin(lp.phi);
	} else
		xy.x = xy.y = 0.;
	if (P->mode) { /* Winkel Tripel */
		xy.x = (xy.x + lp.lam * P->cosphi1) * 0.5;
		xy.y = (xy.y + lp.phi) * 0.5;
	}
	return (xy);
}

/***********************************************************************************
*
* Inverse functions added by Drazen Tutic and Lovro Gradiser based on paper:
*           
* I.Özbug Biklirici and Cengizhan Ipbüker. A General Algorithm for the Inverse 
* Transformation of Map Projections Using Jacobian Matrices. In Proceedings of the
* Third International Symposium Mathematical & Computational Applications,
* pages 175{182, Turkey, September 2002.
*
* Expected accuracy is defined by EPSILON = 1e-12. Should be appropriate for 
* most applications of Aitoff and Winkel Tripel projections.
*
* Longitudes of 180W and 180E can be mixed in solution obtained.
*
* Inverse for Aitoff projection in poles is undefined, longitude value of 0 is assumed.
*
* Contact : dtutic@geof.hr
* Date: 2015-02-16
*
************************************************************************************/

INVERSE(s_inverse); /* sphere */  
        int iter, MAXITER = 10, round = 0, MAXROUND = 20;
	double EPSILON = 1e-12, D, C, f1, f2, f1p, f1l, f2p, f2l, dp, dl, sl, sp, cp, cl, x, y;

	if ((fabs(xy.x) < EPSILON) && (fabs(xy.y) < EPSILON )) { lp.phi = 0.; lp.lam = 0.; return (lp); }

	/* intial values for Newton-Raphson method */
	lp.phi = xy.y; lp.lam = xy.x; 
	do {
		iter = 0; 
		do { 
			sl = sin(lp.lam * 0.5); cl = cos(lp.lam * 0.5); 
			sp = sin(lp.phi); cp = cos(lp.phi); 
			D = cp * cl;
 		      	C = 1. - D * D;
			D = acos(D) / pow(C, 1.5);
	       		f1 = 2. * D * C * cp * sl;
		       	f2 = D * C * sp;
		       	f1p = 2.* (sl * cl * sp * cp / C - D * sp * sl);
		       	f1l = cp * cp * sl * sl / C + D * cp * cl * sp * sp;
		    	f2p = sp * sp * cl / C + D * sl * sl * cp;
		      	f2l = 0.5 * (sp * cp * sl / C - D * sp * cp * cp * sl * cl);
		      	if (P->mode) { /* Winkel Tripel */
				f1 = 0.5 * (f1 + lp.lam * P->cosphi1);
				f2 = 0.5 * (f2 + lp.phi);
				f1p *= 0.5;
				f1l = 0.5 * (f1l + P->cosphi1);
				f2p = 0.5 * (f2p + 1.);
				f2l *= 0.5;
			}
			f1 -= xy.x; f2 -= xy.y;
			dl = (f2 * f1p - f1 * f2p) / (dp = f1p * f2l - f2p * f1l);
			dp = (f1 * f2l - f2 * f1l) / dp;
			while (dl > M_PI) dl -= M_PI; /* set to interval [-M_PI, M_PI]  */
			while (dl < -M_PI) dl += M_PI; /* set to interval [-M_PI, M_PI]  */
			lp.phi -= dp;	lp.lam -= dl;
		} while ((fabs(dp) > EPSILON || fabs(dl) > EPSILON) && (iter++ < MAXITER));
		if (lp.phi > M_PI_2) lp.phi -= 2.*(lp.phi-M_PI_2); /* correct if symmetrical solution for Aitoff */ 
		if (lp.phi < -M_PI_2) lp.phi -= 2.*(lp.phi+M_PI_2); /* correct if symmetrical solution for Aitoff */ 
		if ((fabs(fabs(lp.phi) - M_PI_2) < EPSILON) && (!P->mode)) lp.lam = 0.; /* if pole in Aitoff, return longitude of 0 */ 

		/* calculate x,y coordinates with solution obtained */
		if((D = acos(cos(lp.phi) * cos(C = 0.5 * lp.lam)))) {/* Aitoff */
			x = 2. * D * cos(lp.phi) * sin(C) * (y = 1. / sin(D));
			y *= D * sin(lp.phi);
		} else
			x = y = 0.;
		if (P->mode) { /* Winkel Tripel */
			x = (x + lp.lam * P->cosphi1) * 0.5;
			y = (y + lp.phi) * 0.5;
		}
	/* if too far from given values of x,y, repeat with better approximation of phi,lam */
	} while (((fabs(xy.x-x) > EPSILON) || (fabs(xy.y-y) > EPSILON)) && (round++ < MAXROUND));

	if (iter == MAXITER && round == MAXROUND) fprintf(stderr, "Warning: Accuracy of 1e-12 not reached. Last increments: dlat=%e and dlon=%e\n", dp, dl);

	return (lp);
}

FREEUP; if (P) pj_dalloc(P); }
	static PJ *
setup(PJ *P) {
	P->inv = s_inverse;
	P->fwd = s_forward;
	P->es = 0.;
	return P;
}
ENTRY0(aitoff)
	P->mode = 0;
ENDENTRY(setup(P))
ENTRY0(wintri)
	P->mode = 1;
	if (pj_param(P->ctx, P->params, "tlat_1").i)
        {
		if ((P->cosphi1 = cos(pj_param(P->ctx, P->params, "rlat_1").f)) == 0.)
			E_ERROR(-22)
        }
	else /* 50d28' or acos(2/pi) */
		P->cosphi1 = 0.636619772367581343;
ENDENTRY(setup(P))
