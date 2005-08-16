/* putbits.c, bit-level output                                              */

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
#include "config.h"
#include "global.h"

/* private data */
static unsigned char outbfr;
static int outcnt;
static int bytecnt;

/* initialize buffer, call once before first putbits or alignbits */
void MPEG2_initbits()
{
  outcnt = 8;
  bytecnt = 0;
}

/* write rightmost n (0<=n<=32) bits of val to outfile */
void MPEG2_putbits(val,n)
int val;
int n;
{
  int i;
  unsigned int mask;

  mask = 1 << (n-1); /* selects first (leftmost) bit */

  for (i=0; i<n; i++)
  {
    outbfr <<= 1;

    if (val & mask)
      outbfr|= 1;

    mask >>= 1; /* select next bit */
    outcnt--;

    if (outcnt==0) /* 8 bit buffer full */
    {
      putc(outbfr,vtkMPEG2WriterStr->outfile);
      outcnt = 8;
      bytecnt++;
    }
  }
}

/* zero bit stuffing to next byte boundary (5.2.3, 6.2.1) */
void MPEG2_alignbits()
{
  if (outcnt!=8)
    MPEG2_putbits(0,outcnt);
}

/* return total number of generated bits */
int MPEG2_bitcount()
{
  return 8*bytecnt + (8-outcnt);
}
