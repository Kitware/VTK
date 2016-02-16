#define PROJ_PARMS__ \
	double lamc;\
	double phic;\
	double c;\
	double n1;\
	double n2;\
    double XS;\
    double YS;

#define PJ_LIB__
# include	<projects.h>
PROJ_HEAD(gstmerc, "Gauss-Schreiber Transverse Mercator (aka Gauss-Laborde Reunion)")
	"\n\tCyl, Sph&Ell\n\tlat_0= lon_0= k_0=";
FORWARD(s_forward); /* spheroid */
	double L, Ls, sinLs1, Ls1;
	L= P->n1*lp.lam;
    Ls= P->c+P->n1*log(pj_tsfn(-1.0*lp.phi,-1.0*sin(lp.phi),P->e));
    sinLs1= sin(L)/cosh(Ls);
    Ls1= log(pj_tsfn(-1.0*asin(sinLs1),0.0,0.0));
    xy.x= (P->XS + P->n2*Ls1)*P->ra;
    xy.y= (P->YS + P->n2*atan(sinh(Ls)/cos(L)))*P->ra;
    /*fprintf(stderr,"fwd:\nL      =%16.13f\nLs     =%16.13f\nLs1    =%16.13f\nLP(%16.13f,%16.13f)=XY(%16.4f,%16.4f)\n",L,Ls,Ls1,lp.lam+P->lam0,lp.phi,(xy.x*P->a + P->x0)*P->to_meter,(xy.y*P->a + P->y0)*P->to_meter);*/
	return (xy);
}
INVERSE(s_inverse); /* spheroid */
	double L, LC, sinC;
	L= atan(sinh((xy.x*P->a - P->XS)/P->n2)/cos((xy.y*P->a - P->YS)/P->n2));
    sinC= sin((xy.y*P->a - P->YS)/P->n2)/cosh((xy.x*P->a - P->XS)/P->n2);
    LC= log(pj_tsfn(-1.0*asin(sinC),0.0,0.0));
    lp.lam= L/P->n1;
    lp.phi= -1.0*pj_phi2(P->ctx, exp((LC-P->c)/P->n1),P->e);
    /*fprintf(stderr,"inv:\nL      =%16.13f\nsinC   =%16.13f\nLC     =%16.13f\nXY(%16.4f,%16.4f)=LP(%16.13f,%16.13f)\n",L,sinC,LC,((xy.x/P->ra)+P->x0)/P->to_meter,((xy.y/P->ra)+P->y0)/P->to_meter,lp.lam+P->lam0,lp.phi);*/
	return (lp);
}
FREEUP; if (P) pj_dalloc(P); }
ENTRY0(gstmerc)
    P->lamc= P->lam0;
    P->n1= sqrt(1.0+P->es*pow(cos(P->phi0),4.0)/(1.0-P->es));
    P->phic= asin(sin(P->phi0)/P->n1);
    P->c=       log(pj_tsfn(-1.0*P->phic,0.0,0.0))
         -P->n1*log(pj_tsfn(-1.0*P->phi0,-1.0*sin(P->phi0),P->e));
    P->n2= P->k0*P->a*sqrt(1.0-P->es)/(1.0-P->es*sin(P->phi0)*sin(P->phi0));
    P->XS= 0;/* -P->x0 */
    P->YS= -1.0*P->n2*P->phic;/* -P->y0 */
	P->inv= s_inverse;
	P->fwd= s_forward;
    /*fprintf(stderr,"a  (m) =%16.4f\ne      =%16.13f\nl0(rad)=%16.13f\np0(rad)=%16.13f\nk0     =%16.4f\nX0  (m)=%16.4f\nY0  (m)=%16.4f\n\nlC(rad)=%16.13f\npC(rad)=%16.13f\nc      =%16.13f\nn1     =%16.13f\nn2 (m) =%16.4f\nXS (m) =%16.4f\nYS (m) =%16.4f\n", P->a, P->e, P->lam0, P->phi0, P->k0, P->x0, P->y0, P->lamc, P->phic, P->c, P->n1, P->n2, P->XS +P->x0, P->YS + P->y0);*/
ENDENTRY(P)
