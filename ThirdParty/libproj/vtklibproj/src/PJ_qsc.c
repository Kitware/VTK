/*
 * This implements the Quadrilateralized Spherical Cube (QSC) projection.
 *
 * Copyright (c) 2011, 2012  Martin Lambers <marlam@marlam.de>
 *
 * The QSC projection was introduced in:
 * [OL76]
 * E.M. O'Neill and R.E. Laubscher, "Extended Studies of a Quadrilateralized
 * Spherical Cube Earth Data Base", Naval Environmental Prediction Research
 * Facility Tech. Report NEPRF 3-76 (CSC), May 1976.
 *
 * The preceding shift from an ellipsoid to a sphere, which allows to apply
 * this projection to ellipsoids as used in the Ellipsoidal Cube Map model,
 * is described in
 * [LK12]
 * M. Lambers and A. Kolb, "Ellipsoidal Cube Maps for Accurate Rendering of
 * Planetary-Scale Terrain Data", Proc. Pacfic Graphics (Short Papers), Sep.
 * 2012
 *
 * You have to choose one of the following projection centers,
 * corresponding to the centers of the six cube faces:
 * phi0 = 0.0, lam0 = 0.0       ("front" face)
 * phi0 = 0.0, lam0 = 90.0      ("right" face)
 * phi0 = 0.0, lam0 = 180.0     ("back" face)
 * phi0 = 0.0, lam0 = -90.0     ("left" face)
 * phi0 = 90.0                  ("top" face)
 * phi0 = -90.0                 ("bottom" face)
 * Other projection centers will not work!
 *
 * In the projection code below, each cube face is handled differently.
 * See the computation of the face parameter in the ENTRY0(qsc) function
 * and the handling of different face values (FACE_*) in the forward and
 * inverse projections.
 *
 * Furthermore, the projection is originally only defined for theta angles
 * between (-1/4 * PI) and (+1/4 * PI) on the current cube face. This area
 * of definition is named AREA_0 in the projection code below. The other
 * three areas of a cube face are handled by rotation of AREA_0.
 */

#define PROJ_PARMS__ \
        int face; \
        double a_squared; \
        double b; \
        double one_minus_f; \
        double one_minus_f_squared;
#define PJ_LIB__
#include        <projects.h>
PROJ_HEAD(qsc, "Quadrilateralized Spherical Cube") "\n\tAzi, Sph.";
#define EPS10 1.e-10

/* The six cube faces. */
#define FACE_FRONT  0
#define FACE_RIGHT  1
#define FACE_BACK   2
#define FACE_LEFT   3
#define FACE_TOP    4
#define FACE_BOTTOM 5

/* The four areas on a cube face. AREA_0 is the area of definition,
 * the other three areas are counted counterclockwise. */
#define AREA_0 0
#define AREA_1 1
#define AREA_2 2
#define AREA_3 3

/* Helper function for forward projection: compute the theta angle
 * and determine the area number. */
static double
qsc_fwd_equat_face_theta(double phi, double y, double x, int *area) {
        double theta;
        if (phi < EPS10) {
            *area = AREA_0;
            theta = 0.0;
        } else {
            theta = atan2(y, x);
            if (fabs(theta) <= FORTPI) {
                *area = AREA_0;
            } else if (theta > FORTPI && theta <= HALFPI + FORTPI) {
                *area = AREA_1;
                theta -= HALFPI;
            } else if (theta > HALFPI + FORTPI || theta <= -(HALFPI + FORTPI)) {
                *area = AREA_2;
                theta = (theta >= 0.0 ? theta - PI : theta + PI);
            } else {
                *area = AREA_3;
                theta += HALFPI;
            }
        }
        return (theta);
}

/* Helper function: shift the longitude. */
static double
qsc_shift_lon_origin(double lon, double offset) {
        double slon = lon + offset;
        if (slon < -PI) {
            slon += TWOPI;
        } else if (slon > +PI) {
            slon -= TWOPI;
        }
        return slon;
}

/* Forward projection, ellipsoid */
FORWARD(e_forward);
        double lat, lon;
        double theta, phi;
        double t, mu; /* nu; */
        int area;

        /* Convert the geodetic latitude to a geocentric latitude.
         * This corresponds to the shift from the ellipsoid to the sphere
         * described in [LK12]. */
        if (P->es) {
            lat = atan(P->one_minus_f_squared * tan(lp.phi));
        } else {
            lat = lp.phi;
        }

        /* Convert the input lat, lon into theta, phi as used by QSC.
         * This depends on the cube face and the area on it.
         * For the top and bottom face, we can compute theta and phi
         * directly from phi, lam. For the other faces, we must use
         * unit sphere cartesian coordinates as an intermediate step. */
        lon = lp.lam;
        if (P->face == FACE_TOP) {
            phi = HALFPI - lat;
            if (lon >= FORTPI && lon <= HALFPI + FORTPI) {
                area = AREA_0;
                theta = lon - HALFPI;
            } else if (lon > HALFPI + FORTPI || lon <= -(HALFPI + FORTPI)) {
                area = AREA_1;
                theta = (lon > 0.0 ? lon - PI : lon + PI);
            } else if (lon > -(HALFPI + FORTPI) && lon <= -FORTPI) {
                area = AREA_2;
                theta = lon + HALFPI;
            } else {
                area = AREA_3;
                theta = lon;
            }
        } else if (P->face == FACE_BOTTOM) {
            phi = HALFPI + lat;
            if (lon >= FORTPI && lon <= HALFPI + FORTPI) {
                area = AREA_0;
                theta = -lon + HALFPI;
            } else if (lon < FORTPI && lon >= -FORTPI) {
                area = AREA_1;
                theta = -lon;
            } else if (lon < -FORTPI && lon >= -(HALFPI + FORTPI)) {
                area = AREA_2;
                theta = -lon - HALFPI;
            } else {
                area = AREA_3;
                theta = (lon > 0.0 ? -lon + PI : -lon - PI);
            }
        } else {
            double q, r, s;
            double sinlat, coslat;
            double sinlon, coslon;

            if (P->face == FACE_RIGHT) {
                lon = qsc_shift_lon_origin(lon, +HALFPI);
            } else if (P->face == FACE_BACK) {
                lon = qsc_shift_lon_origin(lon, +PI);
            } else if (P->face == FACE_LEFT) {
                lon = qsc_shift_lon_origin(lon, -HALFPI);
            }
            sinlat = sin(lat);
            coslat = cos(lat);
            sinlon = sin(lon);
            coslon = cos(lon);
            q = coslat * coslon;
            r = coslat * sinlon;
            s = sinlat;

            if (P->face == FACE_FRONT) {
                phi = acos(q);
                theta = qsc_fwd_equat_face_theta(phi, s, r, &area);
            } else if (P->face == FACE_RIGHT) {
                phi = acos(r);
                theta = qsc_fwd_equat_face_theta(phi, s, -q, &area);
            } else if (P->face == FACE_BACK) {
                phi = acos(-q);
                theta = qsc_fwd_equat_face_theta(phi, s, -r, &area);
            } else if (P->face == FACE_LEFT) {
                phi = acos(-r);
                theta = qsc_fwd_equat_face_theta(phi, s, q, &area);
            } else {
                /* Impossible */
                phi = theta = 0.0;
                area = AREA_0;
            }
        }

        /* Compute mu and nu for the area of definition.
         * For mu, see Eq. (3-21) in [OL76], but note the typos:
         * compare with Eq. (3-14). For nu, see Eq. (3-38). */
        mu = atan((12.0 / PI) * (theta + acos(sin(theta) * cos(FORTPI)) - HALFPI));
        t = sqrt((1.0 - cos(phi)) / (cos(mu) * cos(mu)) / (1.0 - cos(atan(1.0 / cos(theta)))));
        /* nu = atan(t);        We don't really need nu, just t, see below. */

        /* Apply the result to the real area. */
        if (area == AREA_1) {
            mu += HALFPI;
        } else if (area == AREA_2) {
            mu += PI;
        } else if (area == AREA_3) {
            mu += HALFPI + PI;
        }

        /* Now compute x, y from mu and nu */
        /* t = tan(nu); */
        xy.x = t * cos(mu);
        xy.y = t * sin(mu);
        return (xy);
}

/* Inverse projection, ellipsoid */
INVERSE(e_inverse);
        double mu, nu, cosmu, tannu;
        double tantheta, theta, cosphi, phi;
        double t;
        int area;

        /* Convert the input x, y to the mu and nu angles as used by QSC.
         * This depends on the area of the cube face. */
        nu = atan(sqrt(xy.x * xy.x + xy.y * xy.y));
        mu = atan2(xy.y, xy.x);
        if (xy.x >= 0.0 && xy.x >= fabs(xy.y)) {
            area = AREA_0;
        } else if (xy.y >= 0.0 && xy.y >= fabs(xy.x)) {
            area = AREA_1;
            mu -= HALFPI;
        } else if (xy.x < 0.0 && -xy.x >= fabs(xy.y)) {
            area = AREA_2;
            mu = (mu < 0.0 ? mu + PI : mu - PI);
        } else {
            area = AREA_3;
            mu += HALFPI;
        }

        /* Compute phi and theta for the area of definition.
         * The inverse projection is not described in the original paper, but some
         * good hints can be found here (as of 2011-12-14):
         * http://fits.gsfc.nasa.gov/fitsbits/saf.93/saf.9302
         * (search for "Message-Id: <9302181759.AA25477 at fits.cv.nrao.edu>") */
        t = (PI / 12.0) * tan(mu);
        tantheta = sin(t) / (cos(t) - (1.0 / sqrt(2.0)));
        theta = atan(tantheta);
        cosmu = cos(mu);
        tannu = tan(nu);
        cosphi = 1.0 - cosmu * cosmu * tannu * tannu * (1.0 - cos(atan(1.0 / cos(theta))));
        if (cosphi < -1.0) {
            cosphi = -1.0;
        } else if (cosphi > +1.0) {
            cosphi = +1.0;
        }

        /* Apply the result to the real area on the cube face.
         * For the top and bottom face, we can compute phi and lam directly.
         * For the other faces, we must use unit sphere cartesian coordinates
         * as an intermediate step. */
        if (P->face == FACE_TOP) {
            phi = acos(cosphi);
            lp.phi = HALFPI - phi;
            if (area == AREA_0) {
                lp.lam = theta + HALFPI;
            } else if (area == AREA_1) {
                lp.lam = (theta < 0.0 ? theta + PI : theta - PI);
            } else if (area == AREA_2) {
                lp.lam = theta - HALFPI;
            } else /* area == AREA_3 */ {
                lp.lam = theta;
            }
        } else if (P->face == FACE_BOTTOM) {
            phi = acos(cosphi);
            lp.phi = phi - HALFPI;
            if (area == AREA_0) {
                lp.lam = -theta + HALFPI;
            } else if (area == AREA_1) {
                lp.lam = -theta;
            } else if (area == AREA_2) {
                lp.lam = -theta - HALFPI;
            } else /* area == AREA_3 */ {
                lp.lam = (theta < 0.0 ? -theta - PI : -theta + PI);
            }
        } else {
            /* Compute phi and lam via cartesian unit sphere coordinates. */
            double q, r, s, t;
            q = cosphi;
            t = q * q;
            if (t >= 1.0) {
                s = 0.0;
            } else {
                s = sqrt(1.0 - t) * sin(theta);
            }
            t += s * s;
            if (t >= 1.0) {
                r = 0.0;
            } else {
                r = sqrt(1.0 - t);
            }
            /* Rotate q,r,s into the correct area. */
            if (area == AREA_1) {
                t = r;
                r = -s;
                s = t;
            } else if (area == AREA_2) {
                r = -r;
                s = -s;
            } else if (area == AREA_3) {
                t = r;
                r = s;
                s = -t;
            }
            /* Rotate q,r,s into the correct cube face. */
            if (P->face == FACE_RIGHT) {
                t = q;
                q = -r;
                r = t;
            } else if (P->face == FACE_BACK) {
                q = -q;
                r = -r;
            } else if (P->face == FACE_LEFT) {
                t = q;
                q = r;
                r = -t;
            }
            /* Now compute phi and lam from the unit sphere coordinates. */
            lp.phi = acos(-s) - HALFPI;
            lp.lam = atan2(r, q);
            if (P->face == FACE_RIGHT) {
                lp.lam = qsc_shift_lon_origin(lp.lam, -HALFPI);
            } else if (P->face == FACE_BACK) {
                lp.lam = qsc_shift_lon_origin(lp.lam, -PI);
            } else if (P->face == FACE_LEFT) {
                lp.lam = qsc_shift_lon_origin(lp.lam, +HALFPI);
            }
        }

        /* Apply the shift from the sphere to the ellipsoid as described
         * in [LK12]. */
        if (P->es) {
            int invert_sign;
            double tanphi, xa;
            invert_sign = (lp.phi < 0.0 ? 1 : 0);
            tanphi = tan(lp.phi);
            xa = P->b / sqrt(tanphi * tanphi + P->one_minus_f_squared);
            lp.phi = atan(sqrt(P->a * P->a - xa * xa) / (P->one_minus_f * xa));
            if (invert_sign) {
                lp.phi = -lp.phi;
            }
        }
        return (lp);
}
FREEUP; if (P) pj_dalloc(P); }
ENTRY0(qsc)
        P->inv = e_inverse;
        P->fwd = e_forward;
        /* Determine the cube face from the center of projection. */
        if (P->phi0 >= HALFPI - FORTPI / 2.0) {
            P->face = FACE_TOP;
        } else if (P->phi0 <= -(HALFPI - FORTPI / 2.0)) {
            P->face = FACE_BOTTOM;
        } else if (fabs(P->lam0) <= FORTPI) {
            P->face = FACE_FRONT;
        } else if (fabs(P->lam0) <= HALFPI + FORTPI) {
            P->face = (P->lam0 > 0.0 ? FACE_RIGHT : FACE_LEFT);
        } else {
            P->face = FACE_BACK;
        }
        /* Fill in useful values for the ellipsoid <-> sphere shift
         * described in [LK12]. */
        if (P->es) {
            P->a_squared = P->a * P->a;
            P->b = P->a * sqrt(1.0 - P->es);
            P->one_minus_f = 1.0 - (P->a - P->b) / P->a;
            P->one_minus_f_squared = P->one_minus_f * P->one_minus_f;
        }
ENDENTRY(P)
