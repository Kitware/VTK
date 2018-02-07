/*********************************************************************
 *   Copyright 2009, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *********************************************************************/

#ifndef NCOFFSETS_H
#define NCOFFSETS_H 1

typedef struct Alignment {
    char* typename;
    unsigned int alignment;
} Alignment;

/* Define indices for every primitive C type */
/* NAT => NOT-A-TYPE*/
#define NATINDEX       0
#define CHARINDEX      1
#define UCHARINDEX     2
#define SHORTINDEX     3
#define USHORTINDEX    4
#define INTINDEX       5
#define UINTINDEX      6
#define LONGINDEX      7
#define ULONGINDEX     8
#define LONGLONGINDEX  9
#define ULONGLONGINDEX 10
#define FLOATINDEX     11
#define DOUBLEINDEX    12
#define PTRINDEX       13
#define NCVLENINDEX    14

#define NCTYPES        15

/* Capture in struct and in a vector*/
typedef struct Typealignset {
    Alignment charalign;	/* char*/
    Alignment ucharalign;	/* unsigned char*/
    Alignment shortalign;	/* short*/
    Alignment ushortalign;	/* unsigned short*/
    Alignment intalign;		/* int*/
    Alignment uintalign;	/* unsigned int*/
    Alignment longalign;	/* long*/
    Alignment ulongalign;	/* unsigned long*/
    Alignment longlongalign;	/* long long*/
    Alignment ulonglongalign;	/* unsigned long long*/
    Alignment floatalign;	/* float*/
    Alignment doublealign;	/* double*/
    Alignment ptralign;		/* void**/
    Alignment ncvlenalign;	/* nc_vlen_t*/
} Typealignset;

typedef Alignment Typealignvec;

EXTERNL unsigned int nctypealignment(nc_type nctype);

#endif /*NCOFFSETS_H*/
