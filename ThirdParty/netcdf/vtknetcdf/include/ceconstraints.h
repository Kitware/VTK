/*********************************************************************
  *   Copyright 1993, UCAR/Unidata
  *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
  *********************************************************************/
/* $Header$ */

#ifndef CECONSTRAINTS_H
#define CECONSTRAINTS_H

#ifndef NC_MAX_VAR_DIMS
#define NC_MAX_VAR_DIMS 1024
#endif

typedef enum CEops {
CEO_NIL=0,CEO_EQ=1,CEO_NEQ=2,CEO_GE=3,CEO_GT=4,CEO_LE=5,CEO_LT=6,CEO_RE=7
} CEops;

/* Must match CEops */
#define OPSTRINGS {"?","=","!=",">=",">","<=","<","=~"}

typedef enum CEsort {
CES_NIL=0,
CES_STR=8,CES_INT=9,CES_FLOAT=10,
CES_VAR=11,CES_FCN=12,CES_CONST=13,
CES_SELECT=14, CES_PROJECT=15,
CES_SEGMENT=16, CES_CONSTRAINT=17,
CES_VALUE=18, CES_SLICE=19
} CEsort;

#endif /*CECONSTRAINTS_H*/

