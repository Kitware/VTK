/******************************************************************************
 * Project: PROJ.4
 * Purpose: Implementation of the HEALPix and rHEALPix projections.
 *          For background see <http://code.scenzgrid.org/index.php/p/scenzgrid-py/source/tree/master/docs/rhealpix_dggs.pdf>.
 * Authors: Alex Raichev (raichev@cs.auckland.ac.nz) 
 *          Michael Speth (spethm@landcareresearch.co.nz)
 * Notes:   Raichev implemented these projections in Python and 
 *          Speth translated them into C here.   
 ******************************************************************************
 * Copyright (c) 2001, Thomas Flemming, tf@ttqv.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substcounteral portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/
# define PROJ_PARMS__ \
    int north_square; \
    int south_square; \
    double qp; \
    double *apa;
# define PJ_LIB__    
# include	<projects.h>
PROJ_HEAD(healpix, "HEALPix") "\n\tSph., Ellps.";
PROJ_HEAD(rhealpix, "rHEALPix") "\n\tSph., Ellps.\n\tnorth_square= south_square=";
# include	<stdio.h>
/* Matrix for counterclockwise rotation by pi/2: */
# define R1 {{ 0,-1},{ 1, 0}}	
/* Matrix for counterclockwise rotation by pi: */
# define R2 {{-1, 0},{ 0,-1}}	
/* Matrix for counterclockwise rotation by 3*pi/2:  */
# define R3 {{ 0, 1},{-1, 0}}	
/* Identity matrix */
# define IDENT {{1, 0},{0, 1}}
/* IDENT, R1, R2, R3, R1 inverse, R2 inverse, R3 inverse:*/
# define ROT {IDENT, R1, R2, R3, R3, R2, R1}
/* Fuzz to handle rounding errors: */
# define EPS 1e-15
typedef struct {
    int cn; /* An integer 0--3 indicating the position of the polar cap. */
    double x, y;  /* Coordinates of the pole point (point of most extreme latitude on the polar caps). */
    enum Region {north, south, equatorial} region;
} CapMap;
typedef struct {
    double x, y;
} Point;
double rot[7][2][2] = ROT;

/**
 * Returns the sign of the double.
 * @param v the parameter whose sign is returned.
 * @return 1 for positive number, -1 for negative, and 0 for zero.
 **/
double pj_sign (double v) {
    return v > 0 ? 1 : (v < 0 ? -1 : 0);
}
/**
 * Return the index of the matrix in ROT.
 * @param index ranges from -3 to 3.
 */
static int get_rotate_index(int index) {
    switch(index) {
	case 0:
	    return 0;
	case 1:
	    return 1;
	case 2: 
	    return 2;
	case 3: 
	    return 3;
	case -1:
	    return 4;
	case -2:
	    return 5;
	case -3:
	    return 6;
    }
    return 0;
}
/**
 * Return 1 if point (testx, testy) lies in the interior of the polygon 
 * determined by the vertices in vert, and return 0 otherwise.
 * See http://paulbourke.net/geometry/polygonmesh/ for more details.
 * @param nvert the number of vertices in the polygon.
 * @param vert the (x, y)-coordinates of the polygon's vertices
 **/
static int pnpoly(int nvert, double vert[][2], double testx, double testy) {
    int i, c = 0;
    int counter = 0;
    double xinters;
    Point p1, p2;
    /* Check for boundrary cases */
    for (i = 0; i < nvert; i++) {
        if (testx == vert[i][0] && testy == vert[i][1]) {
            return 1;
        }
    }
    p1.x = vert[0][0];
    p1.y = vert[0][1];
    for (i = 1; i < nvert; i++) {
        p2.x = vert[i % nvert][0];
        p2.y = vert[i % nvert][1];
        if (testy > MIN(p1.y, p2.y)) {
            if (testy <= MAX(p1.y, p2.y)) {
                if (testx <= MAX(p1.x, p2.x)) {
                    if (p1.y != p2.y) {
                        xinters = (testy-p1.y)*(p2.x-p1.x)/(p2.y-p1.y)+p1.x;
                        if (p1.x == p2.x || testx <= xinters) {
                            counter++;
                        }
                    }
                }
            }
        }
        p1 = p2;
    }
    if (counter % 2 == 0) {
        return 0;
    } else {
        return 1;
    }
    return c;
}
/**
 * Return 1 if (x, y) lies in (the interior or boundary of) the image of the
 * HEALPix projection (in case proj=0) or in the image the rHEALPix projection 
 * (in case proj=1), and return 0 otherwise.
 * @param north_square the position of the north polar square (rHEALPix only)       
 * @param south_square the position of the south polar square (rHEALPix only)  
 **/
int in_image(double x, double y, int proj, int north_square, int south_square) {
    if (proj == 0) {
    	double healpixVertsJit[][2] = {
    	    {-1.0*PI- EPS, PI/4.0},
    	    {-3.0*PI/4.0, PI/2.0 + EPS},
    	    {-1.0*PI/2.0, PI/4.0 + EPS},
    	    {-1.0*PI/4.0, PI/2.0 + EPS},
    	    {0.0, PI/4.0 + EPS},
    	    {PI/4.0, PI/2.0 + EPS},
    	    {PI/2.0, PI/4.0 + EPS},
    	    {3.0*PI/4.0, PI/2.0 + EPS},
    	    {PI+ EPS, PI/4.0},
    	    {PI+ EPS, -1.0*PI/4.0},
    	    {3.0*PI/4.0, -1.0*PI/2.0 - EPS},
    	    {PI/2.0, -1.0*PI/4.0 - EPS},
    	    {PI/4.0, -1.0*PI/2.0 - EPS},
    	    {0.0, -1.0*PI/4.0 - EPS},
    	    {-1.0*PI/4.0, -1.0*PI/2.0 - EPS},
    	    {-1.0*PI/2.0, -1.0*PI/4.0 - EPS},
    	    {-3.0*PI/4.0, -1.0*PI/2.0 - EPS},
    	    {-1.0*PI - EPS, -1.0*PI/4.0}
    	};
    	return pnpoly((int)sizeof(healpixVertsJit)/
    	              sizeof(healpixVertsJit[0]), healpixVertsJit, x, y);
    } else {
    	double rhealpixVertsJit[][2] = {
    	    {-1.0*PI - EPS, PI/4.0 + EPS},
    	    {-1.0*PI + north_square*PI/2.0- EPS, PI/4.0 + EPS},
    	    {-1.0*PI + north_square*PI/2.0- EPS, 3*PI/4.0 + EPS},
    	    {-1.0*PI + (north_square + 1.0)*PI/2.0 + EPS, 3*PI/4.0 + EPS},
    	    {-1.0*PI + (north_square + 1.0)*PI/2.0 + EPS, PI/4.0 + EPS},
    	    {PI + EPS, PI/4.0 + EPS},
    	    {PI + EPS, -1.0*PI/4.0 - EPS},
    	    {-1.0*PI + (south_square + 1.0)*PI/2.0 + EPS, -1.0*PI/4.0 - EPS},
    	    {-1.0*PI + (south_square + 1.0)*PI/2.0 + EPS, -3.0*PI/4.0 - EPS},
    	    {-1.0*PI + south_square*PI/2.0 - EPS, -3.0*PI/4.0 - EPS},
    	    {-1.0*PI + south_square*PI/2.0 - EPS, -1.0*PI/4.0 - EPS},
    	    {-1.0*PI - EPS, -1.0*PI/4.0 - EPS}};
    	return pnpoly((int)sizeof(rhealpixVertsJit)/
    	              sizeof(rhealpixVertsJit[0]), rhealpixVertsJit, x, y);
    }
}
/**
 * Return the authalic latitude of latitude alpha (if inverse=0) or
 * return the approximate latitude of authalic latitude alpha (if inverse=1).    
 * P contains the relavent ellipsoid parameters. 
 **/
double auth_lat(PJ *P, double alpha, int inverse) {
    if (inverse == 0) {
        /* Authalic latitude. */
        double q = pj_qsfn(sin(alpha), P->e, 1.0 - P->es);
        double qp = P->qp; 
	    double ratio = q/qp;
	    if (fabsl(ratio) > 1) {
	        /* Rounding error. */
	        ratio = pj_sign(ratio);
	    }
	    return asin(ratio);
    } else {
        /* Approximation to inverse authalic latitude. */
        return pj_authlat(alpha, P->apa);
    }
}
/**
 * Return the HEALPix projection of the longitude-latitude point lp on
 * the unit sphere.
**/
XY healpix_sphere(LP lp) {
    double lam = lp.lam;
    double phi = lp.phi;
    double phi0 = asin(2.0/3.0);
    XY xy;
    /* equatorial region */
    if ( fabsl(phi) <= phi0) {
	    xy.x = lam;
	    xy.y = 3.0*PI/8.0*sin(phi);
    } else {
	    double lamc;
	    double sigma = sqrt(3.0*(1 - fabsl(sin(phi))));
	    double cn = floor(2*lam / PI + 2);
	    if (cn >= 4) {
	        cn = 3;
	    }
	    lamc = -3*PI/4 + (PI/2)*cn;
	    xy.x = lamc + (lam - lamc)*sigma;
	    xy.y = pj_sign(phi)*PI/4*(2 - sigma);
    }
    return xy;
}
/**
 * Return the inverse of healpix_sphere(). 
**/
LP healpix_sphere_inverse(XY xy) {
    LP lp; 
    double x = xy.x;
    double y = xy.y;
    double y0 = PI/4.0;
    /* Equatorial region. */
    if (fabsl(y) <= y0) {
	    lp.lam = x;
	    lp.phi = asin(8.0*y/(3.0*PI));	
    } else if (fabsl(y) < PI/2.0) {
	    double cn = floor(2.0*x/PI + 2.0);
        double xc, tau;
	    if (cn >= 4) {
	        cn = 3;
	    }
	    xc = -3.0*PI/4.0 + (PI/2.0)*cn;
	    tau = 2.0 - 4.0*fabsl(y)/PI;
	    lp.lam = xc + (x - xc)/tau;	
	    lp.phi = pj_sign(y)*asin(1.0 - pow(tau , 2.0)/3.0);
    } else {
	    lp.lam = -1.0*PI;
	    lp.phi = pj_sign(y)*PI/2.0;
    }
    return (lp);
}
/**
 * Return the vector sum a + b, where a and b are 2-dimensional vectors.
 * @param ret holds a + b.
 **/
static void vector_add(double a[2], double b[2], double *ret) {
    int i;
    for(i = 0; i < 2; i++) {
	    ret[i] = a[i] + b[i];
    }
}
/**
 * Return the vector difference a - b, where a and b are 2-dimensional vectors.
 * @param ret holds a - b.
 **/
static void vector_sub(double a[2], double b[2], double*ret) {
    int i;
    for(i = 0; i < 2; i++) { 
	    ret[i] = a[i] - b[i];
    }
}
/**
 * Return the 2 x 1 matrix product a*b, where a is a 2 x 2 matrix and 
 * b is a 2 x 1 matrix.
 * @param ret holds a*b.
 **/
static void dot_product(double a[2][2], double b[2], double *ret) {
    int i, j;
    int length = 2;
    for(i = 0; i < length; i++) {
	    ret[i] = 0;
	    for(j = 0; j < length; j++) {
	        ret[i] += a[i][j]*b[j];
	    }
    }
}
/**
 * Return the number of the polar cap, the pole point coordinates, and 
 * the region that (x, y) lies in.
 * If inverse=0, then assume (x,y) lies in the image of the HEALPix 
 * projection of the unit sphere.
 * If inverse=1, then assume (x,y) lies in the image of the 
 * (north_square, south_square)-rHEALPix projection of the unit sphere.
 **/
static CapMap get_cap(double x, double y, int north_square, int south_square,
                      int inverse) {
    CapMap capmap;
    double c;
    capmap.x = x;
    capmap.y = y;
    if (inverse == 0) {
    	if (y > PI/4.0) {
    	    capmap.region = north;
    	    c = PI/2.0; 
    	} else if (y < -1*PI/4.0) {
    	    capmap.region = south;
    	    c = -1*PI/2.0;
    	} else {
    	    capmap.region = equatorial;
    	    capmap.cn = 0;
    	    return capmap;
    	}
    	/* polar region */
    	if (x < -1*PI/2.0) {
    	    capmap.cn = 0;
    	    capmap.x = (-1*3.0*PI/4.0);
    	    capmap.y = c;
    	} else if (x >= -1*PI/2.0 && x < 0) {
    	    capmap.cn = 1;
    	    capmap.x = -1*PI/4.0;
    	    capmap.y = c;
    	} else if (x >= 0 && x < PI/2.0) {
    	    capmap.cn = 2;
    	    capmap.x = PI/4.0;
    	    capmap.y = c;
    	} else {
    	    capmap.cn = 3;
    	    capmap.x = 3.0*PI/4.0;
    	    capmap.y = c;
    	}
    	return capmap;
    } else {
    	double eps;
    	if (y > PI/4.0) {
    	    capmap.region = north;
    	    capmap.x = (-3.0*PI/4.0 + north_square*PI/2.0); 
    	    capmap.y = PI/2.0;
    	    x = x - north_square*PI/2.0;
    	} else if (y < -1*PI/4.0) {
    	    capmap.region = south;
    	    capmap.x = (-3.0*PI/4.0 + south_square*PI/2); 
    	    capmap.y = -1*PI/2.0;
    	    x = x - south_square*PI/2.0;
    	} else {
    	    capmap.region = equatorial;
    	    capmap.cn = 0;
    	    return capmap;
    	}
    	/* Polar Region, find the HEALPix polar cap number that 
    	   x, y moves to when rHEALPix polar square is disassembled. */
    	eps = 1e-15; /* Kludge.  Fuzz to avoid some rounding errors. */
    	if (capmap.region == north) {
    	    if (y >= -1*x - PI/4.0 - eps && y < x + 5.0*PI/4.0 - eps) {
    		    capmap.cn = (north_square + 1) % 4;
    	    } else if (y > -1*x -1*PI/4.0 + eps && y >= x + 5.0*PI/4.0 - eps) {
    		    capmap.cn = (north_square + 2) % 4;
    	    } else if (y <= -1*x -1*PI/4.0 + eps && y > x + 5.0*PI/4.0 + eps) {
    		    capmap.cn = (north_square + 3) % 4;
    	    } else {
    		    capmap.cn = north_square;
    	    }
    	} else if (capmap.region == south) {
    	    if (y <= x + PI/4.0 + eps && y > -1*x - 5.0*PI/4 + eps) {
    		    capmap.cn = (south_square + 1) % 4;
    	    } else if (y < x + PI/4.0 - eps && y <= -1*x - 5.0*PI/4.0 + eps) {
    		    capmap.cn = (south_square + 2) % 4;
    	    } else if (y >= x + PI/4.0 - eps && y < -1*x - 5.0*PI/4.0 - eps) {
    		    capmap.cn = (south_square + 3) % 4;
    	    } else {
    		    capmap.cn = south_square;
    	    }
    	}
    	return capmap;
    }
}
/**
 * Rearrange point (x, y) in the HEALPix projection by 
 * combining the polar caps into two polar squares.
 * Put the north polar square in position north_square and 
 * the south polar square in position south_square.
 * If inverse=1, then uncombine the polar caps.
 * @param north_square integer between 0 and 3.
 * @param south_square integer between 0 and 3.
 **/
static XY combine_caps(double x, double y, int north_square, int south_square,
                       int inverse) {
    XY xy;
    double v[2];
    double a[2];
    double vector[2];
    double v_min_c[2];
    double ret_dot[2];
    CapMap capmap = get_cap(x, y, north_square, south_square, inverse);
    if (capmap.region == equatorial) {
	    xy.x = capmap.x;
	    xy.y = capmap.y;
	    return xy;
    }
    v[0] = x;
    v[1] = y;
    if (inverse == 0) {
        /* Rotate (x, y) about its polar cap tip and then translate it to  
           north_square or south_square. */
    	int pole = 0;
    	double (*tmpRot)[2];
    	double c[2] = {capmap.x, capmap.y};
    	if (capmap.region == north) {
    	    pole = north_square;
    	    a[0] =  (-3.0*PI/4.0 + pole*PI/2);
    	    a[1] =  (PI/2.0 + pole*0);
    	    tmpRot = rot[get_rotate_index(capmap.cn - pole)];
    	    vector_sub(v, c, v_min_c);	
    	    dot_product(tmpRot, v_min_c, ret_dot);
    	    vector_add(ret_dot, a, vector);
    	} else {
    	    pole = south_square;
    	    a[0] =  (-3.0*PI/4.0 + pole*PI/2);
    	    a[1] =  (PI/-2.0 + pole*0);
    	    tmpRot = rot[get_rotate_index(-1*(capmap.cn - pole))];
    	    vector_sub(v, c, v_min_c);	
    	    dot_product(tmpRot, v_min_c, ret_dot);
    	    vector_add(ret_dot, a, vector);
    	}
    	xy.x = vector[0];
    	xy.y = vector[1];
    	return xy;
    } else {
        /* Inverse function.
         Unrotate (x, y) and then translate it back. */
    	int pole = 0;
    	double (*tmpRot)[2];
    	double c[2] = {capmap.x, capmap.y};
    	/* disassemble */
    	if (capmap.region == north) {
    	    pole = north_square;
    	    a[0] =  (-3.0*PI/4.0 + capmap.cn*PI/2);
    	    a[1] =  (PI/2.0 + capmap.cn*0);
    	    tmpRot = rot[get_rotate_index(-1*(capmap.cn - pole))];
    	    vector_sub(v, c, v_min_c);	
    	    dot_product(tmpRot, v_min_c, ret_dot);
    	    vector_add(ret_dot, a, vector);
    	} else {
    	    pole = south_square;
    	    a[0] =  (-3.0*PI/4.0 + capmap.cn*PI/2);
    	    a[1] =  (PI/-2.0 + capmap.cn*0);
    	    tmpRot = rot[get_rotate_index(capmap.cn - pole)];
    	    vector_sub(v, c, v_min_c);	
    	    dot_product(tmpRot, v_min_c, ret_dot);
    	    vector_add(ret_dot, a, vector);
    	}
    	xy.x = vector[0];
    	xy.y = vector[1];
    	return xy;
    }
}
FORWARD(s_healpix_forward); /* sphere  */
    (void) P;
    (void) xy;
    return healpix_sphere(lp);
}
FORWARD(e_healpix_forward); /* ellipsoid  */
    (void) xy;
    lp.phi = auth_lat(P, lp.phi, 0);
    return healpix_sphere(lp);
}
INVERSE(s_healpix_inverse); /* sphere */
    /* Check whether (x, y) lies in the HEALPix image */
    if (in_image(xy.x, xy.y, 0, 0, 0) == 0) {
	    lp.lam = HUGE_VAL;
	    lp.phi = HUGE_VAL;
	    pj_ctx_set_errno(P->ctx, -15);
	    return lp;
    }
    return healpix_sphere_inverse(xy);
}
INVERSE(e_healpix_inverse); /* ellipsoid */
    /* Check whether (x, y) lies in the HEALPix image. */
    if (in_image(xy.x, xy.y, 0, 0, 0) == 0) {
	    lp.lam = HUGE_VAL;
	    lp.phi = HUGE_VAL;
	    pj_ctx_set_errno(P->ctx, -15);
	    return lp;
    }
    lp = healpix_sphere_inverse(xy);
    lp.phi = auth_lat(P, lp.phi, 1);
    return (lp);
}
FORWARD(s_rhealpix_forward); /* sphere */
    xy = healpix_sphere(lp);
    return combine_caps(xy.x, xy.y, P->north_square, P->south_square, 0);
}
FORWARD(e_rhealpix_forward); /* ellipsoid */
    lp.phi = auth_lat(P, lp.phi, 0);
    xy = healpix_sphere(lp);
    return combine_caps(xy.x, xy.y, P->north_square, P->south_square, 0);
}
INVERSE(s_rhealpix_inverse); /* sphere */
    /* Check whether (x, y) lies in the rHEALPix image. */
    if (in_image(xy.x, xy.y, 1, P->north_square, P->south_square) == 0) {
	    lp.lam = HUGE_VAL;
	    lp.phi = HUGE_VAL;
	    pj_ctx_set_errno(P->ctx, -15);
	    return lp;
    }
    xy = combine_caps(xy.x, xy.y, P->north_square, P->south_square, 1);
    return healpix_sphere_inverse(xy);
}
INVERSE(e_rhealpix_inverse); /* ellipsoid */
    /* Check whether (x, y) lies in the rHEALPix image. */
    if (in_image(xy.x, xy.y, 1, P->north_square, P->south_square) == 0) {
	    lp.lam = HUGE_VAL;
	    lp.phi = HUGE_VAL;
	    pj_ctx_set_errno(P->ctx, -15);
	    return lp;
    }
    xy = combine_caps(xy.x, xy.y, P->north_square, P->south_square, 1);
    lp = healpix_sphere_inverse(xy);
    lp.phi = auth_lat(P, lp.phi, 1);
    return lp;
}
FREEUP;
	if (P) {
		if (P->apa)
			pj_dalloc(P->apa);
		pj_dalloc(P);
	}
}
ENTRY1(healpix, apa)
    if (P->es) {
        P->apa = pj_authset(P->es); /* For auth_lat(). */
        P->qp = pj_qsfn(1.0, P->e, P->one_es); /* For auth_lat(). */
    	P->a = P->a*sqrt(0.5*P->qp); /* Set P->a to authalic radius. */
        P->ra = 1.0/P->a;
    	P->fwd = e_healpix_forward;
    	P->inv = e_healpix_inverse; 
    } else {
    	P->fwd = s_healpix_forward;
    	P->inv = s_healpix_inverse; 
    }
ENDENTRY(P)
ENTRY1(rhealpix, apa)
    P->north_square = pj_param(P->ctx, P->params,"inorth_square").i;
    P->south_square = pj_param(P->ctx, P->params,"isouth_square").i;
    /* Check for valid north_square and south_square inputs. */
    if (P->north_square < 0 || P->north_square > 3) {
	    E_ERROR(-47);
    }
    if (P->south_square < 0 || P->south_square > 3) {
	    E_ERROR(-47);
    }
    if (P->es) {
        P->apa = pj_authset(P->es); /* For auth_lat(). */
        P->qp = pj_qsfn(1.0, P->e, P->one_es); /* For auth_lat(). */
	    P->a = P->a*sqrt(0.5*P->qp); /* Set P->a to authalic radius. */
        P->ra = 1.0/P->a;
	    P->fwd = e_rhealpix_forward;
	    P->inv = e_rhealpix_inverse; 
    } else {
	    P->fwd = s_rhealpix_forward;
	    P->inv = s_rhealpix_inverse; 
    }
ENDENTRY(P)
