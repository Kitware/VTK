/* putmpg.c, block and motion vector encoding routines                      */

/* Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis.  The MPEG Software Simulation Group disclaims
 * any and all warranties, whether express, implied, or statuary, including any
 * implied warranties or merchantability or of fitness for a particular
 * purpose.  In no event shall the copyright-holder be liable for any
 * incidental, punitive, or consequential damages of any kind whatsoever
 * arising from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs and user's
 * customers, employees, agents, transferees, successors, and assigns.
 *
 * The MPEG Software Simulation Group does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any third-party
 * patents.
 *
 * Commercial implementations of MPEG-1 and MPEG-2 video, including shareware,
 * are subject to royalty fees to patent holders.  Many of these patents are
 * general enough such that they are unavoidable regardless of implementation
 * design.
 *
 */

#include <stdio.h>
#include "mpeg2enc_config.h"
#include "mpeg2enc_global.h"

/* generate variable length codes for an intra-coded block (6.2.6, 6.3.17) */
void MPEG2_putintrablk(blk,cc,mpeg2_struct)
short *blk;
int cc;
struct MPEG2_structure *mpeg2_struct;
{
  int n, dct_diff, run, signed_level;

  /* DC coefficient (7.2.1) */
  dct_diff = blk[0] - mpeg2_struct->dc_dct_pred[cc]; /* difference to previous block */
  mpeg2_struct->dc_dct_pred[cc] = blk[0];

  if (cc==0)
    MPEG2_putDClum(dct_diff,mpeg2_struct);
  else
    MPEG2_putDCchrom(dct_diff,mpeg2_struct);

  /* AC coefficients (7.2.2) */
  run = 0;
  for (n=1; n<64; n++)
  {
    /* use appropriate entropy scanning pattern */
    signed_level = blk[(mpeg2_struct->altscan ? MPEG2_alternate_scan : MPEG2_zig_zag_scan)[n]];
    if (signed_level!=0)
    {
      MPEG2_putAC(run,signed_level,mpeg2_struct->intravlc,mpeg2_struct);
      run = 0;
    }
    else
      run++; /* count zero coefficients */
  }

  /* End of Block -- normative block punctuation */
  if (mpeg2_struct->intravlc)
    MPEG2_putbits(6,4,mpeg2_struct); /* 0110 (Table B-15) */
  else
    MPEG2_putbits(2,2,mpeg2_struct); /* 10 (Table B-14) */
}

/* generate variable length codes for a non-intra-coded block (6.2.6, 6.3.17) */
void MPEG2_putnonintrablk(blk,mpeg2_struct)
short *blk;
struct MPEG2_structure *mpeg2_struct;
{
  int n, run, signed_level, first;

  run = 0;
  first = 1;

  for (n=0; n<64; n++)
  {
    /* use appropriate entropy scanning pattern */
    signed_level = blk[(mpeg2_struct->altscan ? MPEG2_alternate_scan : MPEG2_zig_zag_scan)[n]];

    if (signed_level!=0)
    {
      if (first)
      {
        /* first coefficient in non-intra block */
        MPEG2_putACfirst(run,signed_level,mpeg2_struct);
        first = 0;
      }
      else
        MPEG2_putAC(run,signed_level,0,mpeg2_struct);

      run = 0;
    }
    else
      run++; /* count zero coefficients */
  }

  /* End of Block -- normative block punctuation  */
  MPEG2_putbits(2,2,mpeg2_struct);
}

/* generate variable length code for a motion vector component (7.6.3.1) */
void MPEG2_putmv(dmv,f_code,mpeg2_struct)
int dmv,f_code;
struct MPEG2_structure *mpeg2_struct;
{
  int r_size, f, vmin, vmax, dv, temp, motion_code, motion_residual;

  r_size = f_code - 1; /* number of fixed length code ('residual') bits */
  f = 1<<r_size;
  vmin = -16*f; /* lower range limit */
  vmax = 16*f - 1; /* upper range limit */
  dv = 32*f;

  /* fold vector difference into [vmin...vmax] */
  if (dmv>vmax)
    dmv-= dv;
  else if (dmv<vmin)
    dmv+= dv;

  /* check value */
  if (dmv<vmin || dmv>vmax)
    if (!mpeg2_struct->quiet)
      fprintf(stderr,"invalid motion vector\n");

  /* split dmv into motion_code and motion_residual */
  temp = ((dmv<0) ? -dmv : dmv) + f - 1;
  motion_code = temp>>r_size;
  if (dmv<0)
    motion_code = -motion_code;
  motion_residual = temp & (f-1);

  MPEG2_putmotioncode(motion_code,mpeg2_struct); /* variable length code */

  if (r_size!=0 && motion_code!=0)
    MPEG2_putbits(motion_residual,r_size,mpeg2_struct); /* fixed length code */
}
