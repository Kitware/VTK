#define PJ_LIB__
# include	<projects.h>
PROJ_HEAD(wag2, "Wagner II") "\n\tPCyl., Sph.";
#define C_x 0.92483
#define C_y 1.38725
#define C_p1 0.88022
#define C_p2 0.88550
FORWARD(s_forward); /* spheroid */
	lp.phi = aasin(P->ctx,C_p1 * sin(C_p2 * lp.phi));
	xy.x = C_x * lp.lam * cos(lp.phi);
	xy.y = C_y * lp.phi;
	return (xy);
}
INVERSE(s_inverse); /* spheroid */
	lp.phi = xy.y / C_y;
	lp.lam = xy.x / (C_x * cos(lp.phi));
	lp.phi = aasin(P->ctx,sin(lp.phi) / C_p1) / C_p2;
	return (lp);
}
FREEUP; if (P) pj_dalloc(P); }
ENTRY0(wag2) P->es = 0.; P->inv = s_inverse; P->fwd = s_forward; ENDENTRY(P)
