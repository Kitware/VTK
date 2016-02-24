/******************************************************************************
 * Project:  PROJ.4
 * Purpose:  Implementation of the aeqd (Azimuthal Equidistant) projection.
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
	double	sinph0; \
	double	cosph0; \
	double	*en; \
	double	M1; \
	double	N1; \
	double	Mp; \
	double	He; \
	double	G; \
	int		mode; \
	struct geod_geodesic g;
#define PJ_LIB__
#include	"geodesic.h"
#include	<projects.h>

PROJ_HEAD(aeqd, "Azimuthal Equidistant") "\n\tAzi, Sph&Ell\n\tlat_0 guam";

#define EPS10 1.e-10
#define TOL 1.e-14
#define RHO 57.295779513082320876798154814105

#define N_POLE	0
#define S_POLE	1
#define EQUIT	2
#define OBLIQ	3

FORWARD(e_guam_fwd); /* Guam elliptical */
	double  cosphi, sinphi, t;

	cosphi = cos(lp.phi);
	sinphi = sin(lp.phi);
	t = 1. / sqrt(1. - P->es * sinphi * sinphi);
	xy.x = lp.lam * cosphi * t;
	xy.y = pj_mlfn(lp.phi, sinphi, cosphi, P->en) - P->M1 +
		.5 * lp.lam * lp.lam * cosphi * sinphi * t;
	return (xy);
}
FORWARD(e_forward); /* elliptical */
	double  coslam, cosphi, sinphi, rho;
	double azi1, azi2, s12;
	double lam1, phi1, lam2, phi2;

	coslam = cos(lp.lam);
	cosphi = cos(lp.phi);
	sinphi = sin(lp.phi);
	switch (P->mode) {
	case N_POLE:
		coslam = - coslam;
	case S_POLE:
		xy.x = (rho = fabs(P->Mp - pj_mlfn(lp.phi, sinphi, cosphi, P->en))) *
			sin(lp.lam);
		xy.y = rho * coslam;
		break;
	case EQUIT:
	case OBLIQ:
		if (fabs(lp.lam) < EPS10 && fabs(lp.phi - P->phi0) < EPS10) {
			xy.x = xy.y = 0.;
			break;
		}

		phi1 = P->phi0*RHO; lam1 = P->lam0*RHO;
		phi2 = lp.phi*RHO;  lam2 = (lp.lam+P->lam0)*RHO;

		geod_inverse(&P->g, phi1, lam1, phi2, lam2, &s12, &azi1, &azi2);
		azi1 /= RHO;
		xy.x = s12 * sin(azi1) / P->a;
		xy.y = s12 * cos(azi1) / P->a;
		break;
	}
	return (xy);
}
FORWARD(s_forward); /* spherical */
	double  coslam, cosphi, sinphi;

	sinphi = sin(lp.phi);
	cosphi = cos(lp.phi);
	coslam = cos(lp.lam);
	switch (P->mode) {
	case EQUIT:
		xy.y = cosphi * coslam;
		goto oblcon;
	case OBLIQ:
		xy.y = P->sinph0 * sinphi + P->cosph0 * cosphi * coslam;
oblcon:
		if (fabs(fabs(xy.y) - 1.) < TOL)
			if (xy.y < 0.)
				F_ERROR
			else
				xy.x = xy.y = 0.;
		else {
			xy.y = acos(xy.y);
			xy.y /= sin(xy.y);
			xy.x = xy.y * cosphi * sin(lp.lam);
			xy.y *= (P->mode == EQUIT) ? sinphi :
		   		P->cosph0 * sinphi - P->sinph0 * cosphi * coslam;
		}
		break;
	case N_POLE:
		lp.phi = -lp.phi;
		coslam = -coslam;
	case S_POLE:
		if (fabs(lp.phi - HALFPI) < EPS10) F_ERROR;
		xy.x = (xy.y = (HALFPI + lp.phi)) * sin(lp.lam);
		xy.y *= coslam;
		break;
	}
	return (xy);
}
INVERSE(e_guam_inv); /* Guam elliptical */
	double x2, t;
	int i;

	x2 = 0.5 * xy.x * xy.x;
	lp.phi = P->phi0;
	for (i = 0; i < 3; ++i) {
		t = P->e * sin(lp.phi);
		lp.phi = pj_inv_mlfn(P->ctx, P->M1 + xy.y -
			x2 * tan(lp.phi) * (t = sqrt(1. - t * t)), P->es, P->en);
	}
	lp.lam = xy.x * t / cos(lp.phi);
	return (lp);
}
INVERSE(e_inverse); /* elliptical */
	double c;
	double azi1, azi2, s12, x2, y2, lat1, lon1, lat2, lon2;

	if ((c = hypot(xy.x, xy.y)) < EPS10) {
		lp.phi = P->phi0;
		lp.lam = 0.;
		return (lp);
	}
	if (P->mode == OBLIQ || P->mode == EQUIT) {

		x2 = xy.x * P->a;
		y2 = xy.y * P->a;
		lat1 = P->phi0 * RHO;
		lon1 = P->lam0 * RHO;
		azi1 = atan2(x2, y2) * RHO;
		s12 = sqrt(x2 * x2 + y2 * y2);
		geod_direct(&P->g, lat1, lon1, azi1, s12, &lat2, &lon2, &azi2);
		lp.phi = lat2 / RHO;
		lp.lam = lon2 / RHO;
		lp.lam -= P->lam0;
	} else { /* Polar */
		lp.phi = pj_inv_mlfn(P->ctx, P->mode == N_POLE ? P->Mp - c : P->Mp + c,
			P->es, P->en);
		lp.lam = atan2(xy.x, P->mode == N_POLE ? -xy.y : xy.y);
	}
	return (lp);
}
INVERSE(s_inverse); /* spherical */
	double cosc, c_rh, sinc;

	if ((c_rh = hypot(xy.x, xy.y)) > PI) {
		if (c_rh - EPS10 > PI) I_ERROR;
		c_rh = PI;
	} else if (c_rh < EPS10) {
		lp.phi = P->phi0;
		lp.lam = 0.;
		return (lp);
	}
	if (P->mode == OBLIQ || P->mode == EQUIT) {
		sinc = sin(c_rh);
		cosc = cos(c_rh);
		if (P->mode == EQUIT) {
                        lp.phi = aasin(P->ctx, xy.y * sinc / c_rh);
			xy.x *= sinc;
			xy.y = cosc * c_rh;
		} else {
			lp.phi = aasin(P->ctx,cosc * P->sinph0 + xy.y * sinc * P->cosph0 /
				c_rh);
			xy.y = (cosc - P->sinph0 * sin(lp.phi)) * c_rh;
			xy.x *= sinc * P->cosph0;
		}
		lp.lam = xy.y == 0. ? 0. : atan2(xy.x, xy.y);
	} else if (P->mode == N_POLE) {
		lp.phi = HALFPI - c_rh;
		lp.lam = atan2(xy.x, -xy.y);
	} else {
		lp.phi = c_rh - HALFPI;
		lp.lam = atan2(xy.x, xy.y);
	}
	return (lp);
}
FREEUP;
    if (P) {
		if (P->en)
			pj_dalloc(P->en);
		pj_dalloc(P);
	}
}
ENTRY1(aeqd, en)
	geod_init(&P->g, P->a, P->es / (1 + sqrt(P->one_es)));
	P->phi0 = pj_param(P->ctx, P->params, "rlat_0").f;
	if (fabs(fabs(P->phi0) - HALFPI) < EPS10) {
		P->mode = P->phi0 < 0. ? S_POLE : N_POLE;
		P->sinph0 = P->phi0 < 0. ? -1. : 1.;
		P->cosph0 = 0.;
	} else if (fabs(P->phi0) < EPS10) {
		P->mode = EQUIT;
		P->sinph0 = 0.;
		P->cosph0 = 1.;
	} else {
		P->mode = OBLIQ;
		P->sinph0 = sin(P->phi0);
		P->cosph0 = cos(P->phi0);
	}
	if (! P->es) {
		P->inv = s_inverse; P->fwd = s_forward;
	} else {
		if (!(P->en = pj_enfn(P->es))) E_ERROR_0;
		if (pj_param(P->ctx, P->params, "bguam").i) {
			P->M1 = pj_mlfn(P->phi0, P->sinph0, P->cosph0, P->en);
			P->inv = e_guam_inv; P->fwd = e_guam_fwd;
		} else {
			switch (P->mode) {
			case N_POLE:
				P->Mp = pj_mlfn(HALFPI, 1., 0., P->en);
				break;
			case S_POLE:
				P->Mp = pj_mlfn(-HALFPI, -1., 0., P->en);
				break;
			case EQUIT:
			case OBLIQ:
				P->inv = e_inverse; P->fwd = e_forward;
				P->N1 = 1. / sqrt(1. - P->es * P->sinph0 * P->sinph0);
				P->G = P->sinph0 * (P->He = P->e / sqrt(P->one_es));
				P->He *= P->cosph0;
				break;
			}
			P->inv = e_inverse; P->fwd = e_forward;
		}
	}
ENDENTRY(P)
