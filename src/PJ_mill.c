#define PJ_LIB__
# include	<projects.h>
PROJ_HEAD(mill, "Miller Cylindrical") "\n\tCyl, Sph";
FORWARD(s_forward); /* spheroid */
	(void) P;
	xy.x = lp.lam;
	xy.y = log(tan(FORTPI + lp.phi * .4)) * 1.25;
	return (xy);
}
INVERSE(s_inverse); /* spheroid */
	(void) P;
	lp.lam = xy.x;
	lp.phi = 2.5 * (atan(exp(.8 * xy.y)) - FORTPI);
	return (lp);
}
FREEUP; if (P) pj_dalloc(P); }
ENTRY0(mill) P->es = 0.; P->inv = s_inverse; P->fwd = s_forward; ENDENTRY(P)
