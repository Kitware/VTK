/* General projections header file */
/*
** libproj4 -- library of cartographic projections
**
** Id
**
** Copyright (c) 2003, 2005, 2006   Gerald I. Evenden
**
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

#ifndef PROJECTS_H
#define PROJECTS_H 1

#include "proj_config.h"

#if defined(__BORLANDC__)
  // Disable Borland compiler warning messages that often occur in valid code.
# if !defined(VTK_DISPLAY_WIN32_WARNINGS)
#  pragma warn -8004 /* assigned a value that is never used */
#  pragma warn -8008 /* condition is always false */
#  pragma warn -8026 /* funcs w/class-by-value args not expanded inline */
#  pragma warn -8027 /* functions w/ do/for/while not expanded inline */
#  pragma warn -8060 /* possibly incorrect assignment */
#  pragma warn -8066 /* unreachable code */
#  pragma warn -8072 /* suspicious pointer arithmetic */
# endif
#endif

#if defined(_MSC_VER)
  // Disable MSVC compiler warning messages that often occur in valid code.
# if !defined(VTK_DISPLAY_WIN32_WARNINGS)
#  pragma warning ( disable : 4706 ) /* assignment in conditional expression */
# endif
#endif

    /* standard inclusions */
#include <math.h>
#include <stdlib.h>

  /* depending who's compiling */
#ifdef __cplusplus
#define BEGIN_C_DECLS extern "C" {
#define END_C_DECLS  }
#else
#define BEGIN_C_DECLS
#define END_C_DECLS
#endif

  /* some useful constants */
#define HALFPI    1.5707963267948966
#define FORTPI    0.78539816339744833
#ifndef PI
#define PI    3.14159265358979323846
#endif
#define TWOPI    6.2831853071795864769
#define RAD_TO_DEG  57.29577951308232
#define DEG_TO_RAD  .0174532925199432958

typedef struct { double u, v; }  PROJ_UV;
typedef struct { double r, i; }  PROJ_COMPLEX;

#ifdef PROJ_UV_TYPE
#define PROJ_XY PROJ_UV
#define PROJ_LP PROJ_UV
#else
typedef struct { double x, y; }     PROJ_XY;
typedef struct { double lam, phi; } PROJ_LP;
#endif

PROJ_EXPORT int * proj_errno_loc(void);
#define proj_errno (*proj_errno_loc())

typedef struct {int errnum; char * name; } PROJ_ERR_LIST;
typedef union { double  f; int  i; const char *s; } PROJ_PVALUE;

struct PROJ_ELLPS {
  char  *id;  /* ellipse keyword name */
  char  *major;  /* a= value */
  char  *ell;  /* elliptical parameter */
  char  *name;  /* comments */
};
struct PROJ_UNITS {
  char  *id;  /* units keyword */
  char  *to_meter;  /* multiply by value to get meters */
  char  *name;  /* comments */
};
struct PROJ_DERIVS {
    double x_l, x_p; /* derivatives of x for lambda-phi */
    double y_l, y_p; /* derivatives of y for lambda-phi */
};
struct PROJ_FACTORS {
  struct PROJ_DERIVS der;
  double h, k;  /* meridinal, parallel scales */
  double omega, thetap;  /* angular distortion, theta prime */
  double conv;  /* convergence */
  double s;    /* areal scale factor */
  double a, b;  /* max-min scale error */
  int code;    /* info as to analytics, see following */
};
#define IS_ANAL_XL_YL 01  /* derivatives of lon analytic */
#define IS_ANAL_XP_YP 02  /* derivatives of lat analytic */
#define IS_ANAL_HK  04    /* h and k analytic */
#define IS_ANAL_CONV 010  /* convergence analytic */
    /* parameter list struct */
typedef struct ARG_list {
  struct ARG_list *next;
  char used;
  char param[1]; } paralist;
  /* base projection data structure */
typedef struct PROJconsts {
  PROJ_XY  (*fwd)(PROJ_LP, struct PROJconsts *);
  PROJ_LP  (*inv)(PROJ_XY, struct PROJconsts *);
  void (*spc)(PROJ_LP, struct PROJconsts *, struct PROJ_FACTORS *);
  void (*pfree)(struct PROJconsts *);
  const char *descr;
  paralist *params;   /* parameter list */
  int over;   /* over-range flag */
  int geoc;   /* geocentric latitude flag */
  double
    a,  /* major axis or radius if es==0 */
    e,  /* eccentricity */
    es, /* e ^ 2 */
    ra, /* 1/A */
    one_es, /* 1 - e^2 */
    rone_es, /* 1/one_es */
    lam0, phi0, /* central longitude, latitude */
    x0, y0, /* easting and northing */
    k0,  /* general scaling factor */
    to_meter, fr_meter; /* cartesian scaling */
#ifdef PROJ_PARMS__
PROJ_PARMS__
#endif /* end of optional extensions */
} PROJ;

struct PROJ_LIST {
  const char  *id;    /* projection keyword */
  PROJ  *(*proj)(PROJ *);  /* projection entry point */
  char   * const *descr;  /* description text */
};

/* Generate proj_list external or make list from include file */
BEGIN_C_DECLS
#ifndef PROJ_LIST_H
extern PROJ_EXPORT const struct PROJ_LIST proj_list[];
#else
#  define PROJ_HEAD(id, name) \
     extern PROJ_EXPORT PROJ *proj_##id(PROJ *); extern PROJ_EXPORT char * const proj_s_##id;
#  include PROJ_LIST_H
#  undef PROJ_HEAD
#  define PROJ_HEAD(id, name) {#id, proj_##id, &proj_s_##id},
   PROJ_EXPORT const struct PROJ_LIST
   proj_list[] = {
#    include PROJ_LIST_H
     {0,     0,  0},
   };
#  undef PROJ_HEAD
#endif

#if defined ( _MSC_VER )
#pragma warning ( disable : 4132 )
  // const object should be initialized...
  // these two are initialized in the .c files...
#endif

#ifndef PROJ_ELLPS__
extern
#endif
PROJ_EXPORT const struct PROJ_ELLPS proj_ellps[];

#ifndef PROJ_UNITS__
extern
#endif
PROJ_EXPORT const struct PROJ_UNITS proj_units[];

#if defined ( _MSC_VER )
#pragma warning ( default : 4132 )
#endif

#ifdef PROJ_LIB__
    /* repeatative projection code */
#define PROJ_HEAD(id, name) static const char des_##id [] = name
#define ENTRYA(name) const char * const proj_s_##name = des_##name; \
  PROJ *proj_##name(PROJ *P) { if (!P) { \
  if ((P = (PROJ *)malloc(sizeof(PROJ)))) { \
  P->pfree = freeup; P->fwd = 0; P->inv = 0; \
  P->spc = 0; P->descr = des_##name;
#define ENTRYX } return P; } else {
#define ENTRY0(name) ENTRYA(name) ENTRYX
#define ENTRY1(name, a) ENTRYA(name) P->a = 0; ENTRYX
#define ENTRY2(name, a, b) ENTRYA(name) P->a = 0; P->b = 0; ENTRYX
#define ENDENTRY(p) } return (p); }
#define E_ERROR(err) { proj_errno = err; freeup(P); return(0); }
#define E_ERROR_0 { freeup(P); return(0); }
#define F_ERROR { proj_errno = -20; return(xy); }
#define I_ERROR { proj_errno = -20; return(lp); }
#define FORWARD(name) static PROJ_XY name(PROJ_LP lp,PROJ*P) {PROJ_XY xy={0.,0.}
#define INVERSE(name) static PROJ_LP name(PROJ_XY xy,PROJ*P) {PROJ_LP lp={0.,0.}
#define FREEUP static void freeup(PROJ *P) {
#define SPECIAL(name) static void name(PROJ_LP lp, PROJ *P, struct PROJ_FACTORS *fac)
#endif

  /* procedure prototypes */
PROJ_EXPORT double  proj_dmstor(const char *, char **);
PROJ_EXPORT void proj_set_rtodms(int, int);
PROJ_EXPORT char *proj_rad2dms(char *, double, const char *);
PROJ_EXPORT char *proj_rtodms(char *, double, const char *);
PROJ_EXPORT double proj_adjlon(double);
PROJ_EXPORT double proj_acos(double);
PROJ_EXPORT double proj_asin(double);
PROJ_EXPORT double proj_sqrt(double);
PROJ_EXPORT double proj_atan2(double, double);
PROJ_EXPORT PROJ_PVALUE proj_param(paralist *, const char *);
PROJ_EXPORT paralist *proj_mkparam(char *);
PROJ_EXPORT int proj_ell_set(paralist *, double *, double *);
PROJ_EXPORT void *proj_mdist_ini(double);
PROJ_EXPORT double proj_mdist(double, double, double, const void *);
PROJ_EXPORT double proj_inv_mdist(double, const void *);
PROJ_EXPORT void *proj_gauss_ini(double, double, double *,double *);
PROJ_EXPORT PROJ_LP proj_gauss(PROJ_LP, const void *);
PROJ_EXPORT PROJ_LP proj_inv_gauss(PROJ_LP, const void *);
PROJ_EXPORT PROJ_LP proj_translate(PROJ_LP, const void *);
PROJ_EXPORT PROJ_LP proj_inv_translate(PROJ_LP, const void *);
PROJ_EXPORT void *proj_translate_ini(double, double);
PROJ_EXPORT double proj_tsfn(double, double, double);
PROJ_EXPORT double proj_msfn(double, double, double);
PROJ_EXPORT double proj_phi2(double, double);
PROJ_EXPORT double proj_qsfn(double, const void *);
PROJ_EXPORT double proj_psi(double, double, double);
PROJ_EXPORT double proj_apsi(double, double);
PROJ_EXPORT void *proj_auth_ini(double, double *);
PROJ_EXPORT double proj_auth_lat(double, const void *);
PROJ_EXPORT double proj_auth_inv(double, const void *);
PROJ_EXPORT PROJ_COMPLEX proj_zpoly1(PROJ_COMPLEX, PROJ_COMPLEX *, int);
PROJ_EXPORT PROJ_COMPLEX proj_zpolyd1(PROJ_COMPLEX, PROJ_COMPLEX *, int, PROJ_COMPLEX *);
PROJ_EXPORT int proj_deriv(PROJ_LP, double, PROJ *, struct PROJ_DERIVS *);
PROJ_EXPORT int proj_factors(PROJ_LP, PROJ *, double, struct PROJ_FACTORS *);
PROJ_EXPORT PROJ_XY proj_fwd(PROJ_LP, PROJ *);
PROJ_EXPORT PROJ_LP proj_inv(PROJ_XY, PROJ *);
PROJ_EXPORT void proj_pr_list(PROJ *);
PROJ_EXPORT void proj_free(PROJ *);
PROJ_EXPORT PROJ *proj_init(int, char **);
PROJ_EXPORT char *proj_strerrno(int);
PROJ_EXPORT int proj_strerror_r(int, char *, int);
END_C_DECLS

#endif /* end of basic projections header */
/*
** Log: lib_proj.h
** Revision 1.2  2008-11-10 20:40:20  jeff
** COMP: Ignoring assignment in conditional expression warning.
**
** Revision 1.1  2008-11-07 16:41:13  jeff
** ENH: Adding a 2D geoview. Adding the geographic projection library libproj4
** to Utilities. Updating the architecture of the geospatial views. All
** multi-resolution sources are now subclasses of vtkGeoSource. Each source
** has its own worker thread for fetching refined images or geometry.
** On the 3D side, vtkGeoGlobeSource is an appropriate source for vtkGeoTerrain,
** and vtkGeoAlignedImageSource is an appropriate source for
** vtkGeoAlignedImageRepresentation. On the 2D side, vtkGeoProjectionSource is an
** appropriate source for vtkGeoTerrain2D, and the image source is the same.
**
** Revision 3.3  2008/06/04 17:15:22  gie
** new material
**
** Revision 3.2  2006/01/24 01:17:22  gie
** updates
**
** Revision 3.1  2006/01/11 02:41:14  gie
** Initial
**
**
*/
