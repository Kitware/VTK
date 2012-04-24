/*
** libproj -- library of cartographic projections
**
** Copyright (c) 2004, 2006   Gerald I. Evenden
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
  static struct
BARANYI {
  short ypa[11];
  short xpa[20];
  struct SEG {
    union {
      struct {double X, Y, R2; } arc;
      struct {double A, B, dum; } lin;
    } tipe;
    double limit;
  } seg[3];
} baranyi[] = {
{{00,100,205,315,430,550,675,805,940,1080,1080},
 {00,100,200,300,400,500,600,700,800,900,
  1000,1100,1200,1300,1400,1500,1600,1700,1800,1800},
 {  {{{80.0,0.0,10000.0}},81.241411756},
  {{{0.0,111.465034594,56264.9014095}},9999},
  {{{0,0,0}},0} } },

{{00,100,210,330,460,600,750,910,1080,1260,1260},
 {00,100,200,300,400,500,600,700,800,900,
  1000,1100,1200,1300,1400,1500,1600,1700,1800,1800},
 {  {{{75.0,0.0,11025.00000000}},89.732937686},
    {{{0.0,123.428571429,62214.612245111755}},9999},
  {{{0,0,0}},0} } },

{{00,120,240,360,490,620,750,860,970,1080,1080},
 {00,120,240,350,460,570,680,780,880,980,
  1080,1180,1280,1380,1480,1570,1660,1750,1840,1840},
 {  {{{94.0,0.0,8100.0000000}},78.300539425},
  {{{0.0,165.869652378,78766.3642715}},9999},
  {{{0,0,0}},0} } },

{{00,120,240,360,490,620,750,870,990,1110,1110},
 {00,120,240,350,460,570,680,780,880,980,
  1080,1180,1280,1380,1480,1570,1660,1750,1840,1840},
 {  {{{84.0,0.0,10000.0}},94.323113828},
  {{{0.0,315.227272727,181669.688016296}},9999},
  {{{0,0,0}},0} } },

{{00,100,205,315,440,580,705,815,920,1020,1020},
 {00,105,210,315,420,525,625,725,825,925,
  1025,1125,1225,1325,1425,1510,1595,1680,1765,1765},
 {  {{{86.5,0.0,8100.0}},89.129742863},
  {{{102.995921508,-0.140082858,0}},101.013708578},
  {{{0.0,0.0,10404.000}},9999} } },

{{00,100,205,315,435,565,705,850,1000,1155,1155},
 {00,105,210,315,420,525,625,725,825,925,
  1025,1125,1225,1325,1425,1515,1605,1695,1785,1785},
 {  {{{83.5,0.0,9025.000000}},92.807743792},
  {{{115.5,-0.218634245,0}},9999},
  {{{0,0,0}},0} } },

{{00,120,240,355,470,585,695,805,905,995,995},
 {00,120,240,355,470,580,690,795,900,1000,
  1100,1200,1300,1400,1500,1590,1680,1760,1840,1840},
 {  {{{94.0,0.0,8100.0}},87.968257449},
  {{{0.0,460.302631579,313378.98632277}},9999},
  {{{0,0,0}},0} } },
};
  static struct
VOXC {
  double a1, a2;
} voxc[] = {
  {0.975, 0.0025},
  {0.95,  0.005}
};
#define PROJ_PARMS__ \
  struct BARANYI *p; \
  struct VOXC *vox; \
  int vopt; \
  int mode;
#define PROJ_LIB__
# include   <lib_proj.h>
PROJ_HEAD(brny_1, "Baranyi 1") "\n\tPCyl., Sph., NoInv.";
PROJ_HEAD(brny_2, "Baranyi 2") "\n\tPCyl., Sph., NoInv.";
PROJ_HEAD(brny_3, "Baranyi 3") "\n\tPCyl., Sph., NoInv.";
PROJ_HEAD(brny_4, "Baranyi 4") "\n\tPCyl., Sph., NoInv.";
PROJ_HEAD(brny_5, "Baranyi 5") "\n\tPCyl., Sph., NoInv.";
PROJ_HEAD(brny_6, "Baranyi 6") "\n\tPCyl., Sph., NoInv.";
PROJ_HEAD(brny_7, "Baranyi 7") "\n\tPCyl., Sph., NoInv.";
  static double
xyp(double lp, short *pa) {
  int i;

  lp = fabs(lp) * RAD_TO_DEG * 0.1f;
  i = (int)lp;
  return ((pa[i] + (lp - i)*(pa[i+1]-pa[i]))*0.1);
}
FORWARD(s_forward); /* spheroid */
  struct SEG *s;
  double xl;
  
  xy.x = xyp(lp.lam, P->p->xpa);
  if (P->vopt) { /* do Voxland's method? */
    xy.y = lp.phi * RAD_TO_DEG;
    xy.y = xy.y * (P->vox->a1 + P->vox->a2 * xy.y);
  } else
    xy.y = xyp(lp.phi, P->p->ypa);
  for (s = P->p->seg; xy.y > s->limit; ++s) ;
  if (s->tipe.arc.R2 > 0.) { /* for arc, R obviously non-zero */
    double dy = xy.y + s->tipe.arc.Y;
    xl = (s->tipe.arc.X + sqrt(fabs(s->tipe.arc.R2 - dy * dy)));
  } else  /* for line, R==0 */
    xl = (xy.y - s->tipe.lin.A) / s->tipe.lin.B;
  xy.x *= DEG_TO_RAD * xl * 10. / P->p->xpa[19];
  xy.y *= DEG_TO_RAD;
  if (lp.phi < 0.) xy.y = - xy.y;
  if (lp.lam < 0.) xy.x = - xy.x;
  return (xy);
}
  static PROJ *
setup(PROJ *P, int n) {
  if (((P->mode = n--) <= 2) && (P->vopt = proj_param(P->params, "tvopt").i))
    P->vox = voxc + n;
  else
    P->vopt = 0;
  P->p = baranyi + n;
  P->es = 0.;
  P->fwd = s_forward;
  return P;
}
FREEUP; if (P) free(P); }
ENTRY0(brny_1) ENDENTRY(setup(P, 1))
ENTRY0(brny_2) ENDENTRY(setup(P, 2))
ENTRY0(brny_3) ENDENTRY(setup(P, 3))
ENTRY0(brny_4) ENDENTRY(setup(P, 4))
ENTRY0(brny_5) ENDENTRY(setup(P, 5))
ENTRY0(brny_6) ENDENTRY(setup(P, 6))
ENTRY0(brny_7) ENDENTRY(setup(P, 7))
/*
** Log: proj_baranyi.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
