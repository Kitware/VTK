#define PROJ_PARMS__ \
	double rc;
#define PJ_LIB__
# include	<projects.h>
PROJ_HEAD(eqc, "Equidistant Cylindrical (Plate Caree)")
	"\n\tCyl, Sph\n\tlat_ts=[, lat_0=0]";
FORWARD(s_forward); /* spheroid */
	xy.x = P->rc * lp.lam;
	xy.y = lp.phi - P->phi0;
	return (xy);
}
INVERSE(s_inverse); /* spheroid */
	lp.lam = xy.x / P->rc;
	lp.phi = xy.y + P->phi0;
	return (lp);
}
FREEUP; if (P) pj_dalloc(P); }
ENTRY0(eqc)
	if ((P->rc = cos(pj_param(P->ctx, P->params, "rlat_ts").f)) <= 0.) E_ERROR(-24);
	P->inv = s_inverse;
	P->fwd = s_forward;
	P->es = 0.;
ENDENTRY(P)
