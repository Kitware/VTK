#define PROJ_PARMS__ \
        struct PJconsts* pj[12]; \
        double dy0;
#define PJ_LIB__
#include	<projects.h>
PROJ_HEAD(igh, "Interrupted Goode Homolosine") "\n\tPCyl, Sph.";
	C_NAMESPACE PJ
*pj_sinu(PJ *), *pj_moll(PJ *);

static const double d4044118 = (40 + 44/60. + 11.8/3600.) * DEG_TO_RAD; // 40d 44' 11.8" [degrees]

static const double d10  =  10 * DEG_TO_RAD;
static const double d20  =  20 * DEG_TO_RAD;
static const double d30  =  30 * DEG_TO_RAD;
static const double d40  =  40 * DEG_TO_RAD;
static const double d50  =  50 * DEG_TO_RAD;
static const double d60  =  60 * DEG_TO_RAD;
static const double d80  =  80 * DEG_TO_RAD;
static const double d90  =  90 * DEG_TO_RAD;
static const double d100 = 100 * DEG_TO_RAD;
static const double d140 = 140 * DEG_TO_RAD;
static const double d160 = 160 * DEG_TO_RAD;
static const double d180 = 180 * DEG_TO_RAD;

static const double EPSLN = 1.e-10; // allow a little 'slack' on zone edge positions

FORWARD(s_forward); /* spheroid */
        int z;
        if (lp.phi >=  d4044118) {          // 1|2
          z = (lp.lam <= -d40 ? 1: 2);
        }
        else if (lp.phi >=  0) {            // 3|4
          z = (lp.lam <= -d40 ? 3: 4);
        }
        else if (lp.phi >= -d4044118) {     // 5|6|7|8
               if (lp.lam <= -d100) z =  5; // 5
          else if (lp.lam <=  -d20) z =  6; // 6
          else if (lp.lam <=   d80) z =  7; // 7
          else z = 8;                       // 8
        }
        else {                              // 9|10|11|12
               if (lp.lam <= -d100) z =  9; // 9
          else if (lp.lam <=  -d20) z = 10; // 10
          else if (lp.lam <=   d80) z = 11; // 11
          else z = 12;                      // 12
        }

        lp.lam -= P->pj[z-1]->lam0;
        xy = P->pj[z-1]->fwd(lp, P->pj[z-1]);
        xy.x += P->pj[z-1]->x0;
        xy.y += P->pj[z-1]->y0;

        return (xy);
}
INVERSE(s_inverse); /* spheroid */
        const double y90 = P->dy0 + sqrt(2); // lt=90 corresponds to y=y0+sqrt(2)

        int z = 0;
        if (xy.y > y90+EPSLN || xy.y < -y90+EPSLN) // 0
          z = 0;
        else if (xy.y >=  d4044118)       // 1|2
          z = (xy.x <= -d40? 1: 2);
        else if (xy.y >=  0)              // 3|4
          z = (xy.x <= -d40? 3: 4);
        else if (xy.y >= -d4044118) {     // 5|6|7|8
               if (xy.x <= -d100) z =  5; // 5
          else if (xy.x <=  -d20) z =  6; // 6
          else if (xy.x <=   d80) z =  7; // 7
          else z = 8;                     // 8
        }
        else {                            // 9|10|11|12
               if (xy.x <= -d100) z =  9; // 9
          else if (xy.x <=  -d20) z = 10; // 10
          else if (xy.x <=   d80) z = 11; // 11
          else z = 12;                    // 12
        }

        if (z)
        {
          int ok = 0;

          xy.x -= P->pj[z-1]->x0;
          xy.y -= P->pj[z-1]->y0;
          lp = P->pj[z-1]->inv(xy, P->pj[z-1]);
          lp.lam += P->pj[z-1]->lam0;

          switch (z) {
            case  1: ok = (lp.lam >= -d180-EPSLN && lp.lam <=  -d40+EPSLN) ||
                         ((lp.lam >=  -d40-EPSLN && lp.lam <=  -d10+EPSLN) &&
                          (lp.phi >=   d60-EPSLN && lp.phi <=   d90+EPSLN)); break;
            case  2: ok = (lp.lam >=  -d40-EPSLN && lp.lam <=  d180+EPSLN) ||
                         ((lp.lam >= -d180-EPSLN && lp.lam <= -d160+EPSLN) &&
                          (lp.phi >=   d50-EPSLN && lp.phi <=   d90+EPSLN)) ||
                         ((lp.lam >=  -d50-EPSLN && lp.lam <=  -d40+EPSLN) &&
                          (lp.phi >=   d60-EPSLN && lp.phi <=   d90+EPSLN)); break;
            case  3: ok = (lp.lam >= -d180-EPSLN && lp.lam <=  -d40+EPSLN); break;
            case  4: ok = (lp.lam >=  -d40-EPSLN && lp.lam <=  d180+EPSLN); break;
            case  5: ok = (lp.lam >= -d180-EPSLN && lp.lam <= -d100+EPSLN); break;
            case  6: ok = (lp.lam >= -d100-EPSLN && lp.lam <=  -d20+EPSLN); break;
            case  7: ok = (lp.lam >=  -d20-EPSLN && lp.lam <=   d80+EPSLN); break;
            case  8: ok = (lp.lam >=   d80-EPSLN && lp.lam <=  d180+EPSLN); break;
            case  9: ok = (lp.lam >= -d180-EPSLN && lp.lam <= -d100+EPSLN); break;
            case 10: ok = (lp.lam >= -d100-EPSLN && lp.lam <=  -d20+EPSLN); break;
            case 11: ok = (lp.lam >=  -d20-EPSLN && lp.lam <=   d80+EPSLN); break;
            case 12: ok = (lp.lam >=   d80-EPSLN && lp.lam <=  d180+EPSLN); break;
          }

          z = (!ok? 0: z); // projectable?
        }
     // if (!z) pj_errno = -15; // invalid x or y
        if (!z) lp.lam = HUGE_VAL;
        if (!z) lp.phi = HUGE_VAL;
        return (lp);
}
FREEUP;
        if (P) {
                int i;
                for (i = 0; i < 12; ++i)
		{
			if (P->pj[i]) 
				(*(P->pj[i]->pfree))(P->pj[i]); 
                }
                pj_dalloc(P);
        }
}
ENTRY0(igh)
/*
  Zones:

    -180            -40                       180
      +--------------+-------------------------+    Zones 1,2,9,10,11 & 12:
      |1             |2                        |      Mollweide projection
      |              |                         |
      +--------------+-------------------------+    Zones 3,4,5,6,7 & 8:
      |3             |4                        |      Sinusoidal projection
      |              |                         |
    0 +-------+------+-+-----------+-----------+
      |5      |6       |7          |8          |
      |       |        |           |           |
      +-------+--------+-----------+-----------+
      |9      |10      |11         |12         |
      |       |        |           |           |
      +-------+--------+-----------+-----------+
    -180    -100      -20         80          180
*/

#define SETUP(n, proj, x_0, y_0, lon_0) \
    if (!(P->pj[n-1] = pj_##proj(0))) E_ERROR_0; \
    if (!(P->pj[n-1] = pj_##proj(P->pj[n-1]))) E_ERROR_0; \
    P->pj[n-1]->x0 = x_0; \
    P->pj[n-1]->y0 = y_0; \
    P->pj[n-1]->lam0 = lon_0; 

        LP lp = { 0, d4044118 };
        XY xy1;
        XY xy3;

        // sinusoidal zones
        SETUP(3, sinu, -d100, 0, -d100);
        SETUP(4, sinu,   d30, 0,   d30);
        SETUP(5, sinu, -d160, 0, -d160);
        SETUP(6, sinu,  -d60, 0,  -d60);
        SETUP(7, sinu,   d20, 0,   d20);
        SETUP(8, sinu,  d140, 0,  d140);

        // mollweide zones
        SETUP(1, moll, -d100, 0, -d100);

        // y0 ?
        xy1 = P->pj[0]->fwd(lp, P->pj[0]); // zone 1
        xy3 = P->pj[2]->fwd(lp, P->pj[2]); // zone 3
        // y0 + xy1.y = xy3.y for lt = 40d44'11.8"
        P->dy0 = xy3.y - xy1.y;

        P->pj[0]->y0 = P->dy0;

        // mollweide zones (cont'd)
        SETUP( 2, moll,   d30,  P->dy0,   d30);
        SETUP( 9, moll, -d160, -P->dy0, -d160);
        SETUP(10, moll,  -d60, -P->dy0,  -d60);
        SETUP(11, moll,   d20, -P->dy0,   d20);
        SETUP(12, moll,  d140, -P->dy0,  d140);

        P->inv = s_inverse;
        P->fwd = s_forward;
        P->es = 0.;
ENDENTRY(P)



