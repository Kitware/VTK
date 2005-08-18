/* predict.c, motion compensated prediction                                 */

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

/* private prototypes */
static void MPEG2_predict_mb _ANSI_ARGS_((
  unsigned char *oldref[], unsigned char *newref[], unsigned char *cur[],
  int lx, int bx, int by, int pict_type, int pict_struct, int mb_type,
  int motion_type, int secondfield,
  int PMV[2][2][2], int mv_field_sel[2][2], int dmvector[2],
  struct MPEG2_structure *mpeg2_struct));

static void pred _ANSI_ARGS_((unsigned char *src[], int sfield,
  unsigned char *dst[], int dfield,
  int lx, int w, int h, int x, int y, int dx, int dy, int addflag,
  struct MPEG2_structure *mpeg2_struct));

static void pred_comp _ANSI_ARGS_((unsigned char *src, unsigned char *dst,
  int lx, int w, int h, int x, int y, int dx, int dy, int addflag));

static void calc_DMV _ANSI_ARGS_((int DMV[][2], int *dmvector, int mvx,
  int mvy,struct MPEG2_structure *mpeg2_struct));

static void clearblock _ANSI_ARGS_((unsigned char *cur[], int i0, int j0,struct MPEG2_structure *mpeg2_struct));


/* form prediction for a complete picture (frontend for predict_mb)
 *
 * reff: reference frame for forward prediction
 * refb: reference frame for backward prediction
 * cur:  destination (current) frame
 * secondfield: predict second field of a frame
 * mbi:  macroblock info
 *
 * Notes:
 * - cf. predict_mb
 */

void MPEG2_predict( unsigned char *reff[], unsigned char *refb[], unsigned char *cur[3],
  int secondfield,
  struct mbinfo *mbi, 
  struct MPEG2_structure *mpeg2_struct)
{
  int i, j, k;

  k = 0;

  /* loop through all macroblocks of the picture */
  for (j=0; j<mpeg2_struct->height2; j+=16)
    for (i=0; i<mpeg2_struct->width; i+=16)
    {
      MPEG2_predict_mb(reff,refb,cur,mpeg2_struct->width,i,j,mpeg2_struct->pict_type,mpeg2_struct->pict_struct,
                 mbi[k].mb_type,mbi[k].motion_type,secondfield,
                 mbi[k].MV,mbi[k].mv_field_sel,mbi[k].dmvector,mpeg2_struct);

      k++;
    }
}

/* form prediction for one macroblock
 *
 * oldref: reference frame for forward prediction
 * newref: reference frame for backward prediction
 * cur:    destination (current) frame
 * lx:     frame width (identical to global var `width')
 * bx,by:  picture (field or frame) coordinates of macroblock to be predicted
 * pict_type: I, P or B
 * pict_struct: FRAME_PICTURE, TOP_FIELD, BOTTOM_FIELD
 * mb_type:     MB_FORWARD, MB_BACKWARD, MB_INTRA
 * motion_type: MC_FRAME, MC_FIELD, MC_16X8, MC_DMV
 * secondfield: predict second field of a frame
 * PMV[2][2][2]: motion vectors (in half pel picture coordinates)
 * mv_field_sel[2][2]: motion vertical field selects (for field predictions)
 * dmvector: differential motion vectors (for dual prime)
 *
 * Notes:
 * - when predicting a P type picture which is the second field of
 *   a frame, the same parity reference field is in oldref, while the
 *   opposite parity reference field is assumed to be in newref!
 * - intra macroblocks are modelled to have a constant prediction of 128
 *   for all pels; this results in a DC DCT coefficient symmetric to 0
 * - vectors for field prediction in frame pictures are in half pel frame
 *   coordinates (vertical component is twice the field value and always
 *   even)
 *
 * already covers dual prime (not yet used)
 */

static void MPEG2_predict_mb(
unsigned char *oldref[], unsigned char *newref[],unsigned char *cur[],
int lx,
int bx, int by,
int pict_type,
int pict_struct,
int mb_type,
int motion_type,
int secondfield,
int PMV[2][2][2], int mv_field_sel[2][2], int dmvector[2],
struct MPEG2_structure *mpeg2_struct)
{
  int addflag, currentfield;
  unsigned char **predframe;
  int DMV[2][2];

  if (mb_type&MB_INTRA)
  {
    clearblock(cur,bx,by,mpeg2_struct);
    return;
  }

  addflag = 0; /* first prediction is stored, second is added and averaged */

  if ((mb_type & MB_FORWARD) || (pict_type==P_TYPE))
  {
    /* forward prediction, including zero MV in P pictures */

    if (pict_struct==FRAME_PICTURE)
    {
      /* frame picture */

      if ((motion_type==MC_FRAME) || !(mb_type & MB_FORWARD))
      {
        /* frame-based prediction in frame picture */
        pred(oldref,0,cur,0,
          lx,16,16,bx,by,PMV[0][0][0],PMV[0][0][1],0,mpeg2_struct);
      }
      else if (motion_type==MC_FIELD)
      {
        /* field-based prediction in frame picture
         *
         * note scaling of the vertical coordinates (by, PMV[][0][1])
         * from frame to field!
         */

        /* top field prediction */
        pred(oldref,mv_field_sel[0][0],cur,0,
          lx<<1,16,8,bx,by>>1,PMV[0][0][0],PMV[0][0][1]>>1,0,mpeg2_struct);

        /* bottom field prediction */
        pred(oldref,mv_field_sel[1][0],cur,1,
          lx<<1,16,8,bx,by>>1,PMV[1][0][0],PMV[1][0][1]>>1,0,mpeg2_struct);
      }
      else if (motion_type==MC_DMV)
      {
        /* dual prime prediction */

        /* calculate derived motion vectors */
        calc_DMV(DMV,dmvector,PMV[0][0][0],PMV[0][0][1]>>1,mpeg2_struct);

        /* predict top field from top field */
        pred(oldref,0,cur,0,
          lx<<1,16,8,bx,by>>1,PMV[0][0][0],PMV[0][0][1]>>1,0,mpeg2_struct);

        /* predict bottom field from bottom field */
        pred(oldref,1,cur,1,
          lx<<1,16,8,bx,by>>1,PMV[0][0][0],PMV[0][0][1]>>1,0,mpeg2_struct);

        /* predict and add to top field from bottom field */
        pred(oldref,1,cur,0,
          lx<<1,16,8,bx,by>>1,DMV[0][0],DMV[0][1],1,mpeg2_struct);

        /* predict and add to bottom field from top field */
        pred(oldref,0,cur,1,
          lx<<1,16,8,bx,by>>1,DMV[1][0],DMV[1][1],1,mpeg2_struct);
      }
      else
      {
        /* invalid motion_type in frame picture */
        if (!mpeg2_struct->quiet)
          fprintf(stderr,"invalid motion_type\n");
      }
    }
    else /* TOP_FIELD or BOTTOM_FIELD */
    {
      /* field picture */

      currentfield = (pict_struct==BOTTOM_FIELD);

      /* determine which frame to use for prediction */
      if ((pict_type==P_TYPE) && secondfield
          && (currentfield!=mv_field_sel[0][0]))
        predframe = newref; /* same frame */
      else
        predframe = oldref; /* previous frame */

      if ((motion_type==MC_FIELD) || !(mb_type & MB_FORWARD))
      {
        /* field-based prediction in field picture */
        pred(predframe,mv_field_sel[0][0],cur,currentfield,
          lx<<1,16,16,bx,by,PMV[0][0][0],PMV[0][0][1],0,mpeg2_struct);
      }
      else if (motion_type==MC_16X8)
      {
        /* 16 x 8 motion compensation in field picture */

        /* upper half */
        pred(predframe,mv_field_sel[0][0],cur,currentfield,
          lx<<1,16,8,bx,by,PMV[0][0][0],PMV[0][0][1],0,mpeg2_struct);

        /* determine which frame to use for lower half prediction */
        if ((pict_type==P_TYPE) && secondfield
            && (currentfield!=mv_field_sel[1][0]))
          predframe = newref; /* same frame */
        else
          predframe = oldref; /* previous frame */

        /* lower half */
        pred(predframe,mv_field_sel[1][0],cur,currentfield,
          lx<<1,16,8,bx,by+8,PMV[1][0][0],PMV[1][0][1],0,mpeg2_struct);
      }
      else if (motion_type==MC_DMV)
      {
        /* dual prime prediction */

        /* determine which frame to use for prediction */
        if (secondfield)
          predframe = newref; /* same frame */
        else
          predframe = oldref; /* previous frame */

        /* calculate derived motion vectors */
        calc_DMV(DMV,dmvector,PMV[0][0][0],PMV[0][0][1],mpeg2_struct);

        /* predict from field of same parity */
        pred(oldref,currentfield,cur,currentfield,
          lx<<1,16,16,bx,by,PMV[0][0][0],PMV[0][0][1],0,mpeg2_struct);

        /* predict from field of opposite parity */
        pred(predframe,!currentfield,cur,currentfield,
          lx<<1,16,16,bx,by,DMV[0][0],DMV[0][1],1,mpeg2_struct);
      }
      else
      {
        /* invalid motion_type in field picture */
        if (!mpeg2_struct->quiet)
          fprintf(stderr,"invalid motion_type\n");
      }
    }
    addflag = 1; /* next prediction (if any) will be averaged with this one */
  }

  if (mb_type & MB_BACKWARD)
  {
    /* backward prediction */

    if (pict_struct==FRAME_PICTURE)
    {
      /* frame picture */

      if (motion_type==MC_FRAME)
      {
        /* frame-based prediction in frame picture */
        pred(newref,0,cur,0,
          lx,16,16,bx,by,PMV[0][1][0],PMV[0][1][1],addflag,mpeg2_struct);
      }
      else
      {
        /* field-based prediction in frame picture
         *
         * note scaling of the vertical coordinates (by, PMV[][1][1])
         * from frame to field!
         */

        /* top field prediction */
        pred(newref,mv_field_sel[0][1],cur,0,
          lx<<1,16,8,bx,by>>1,PMV[0][1][0],PMV[0][1][1]>>1,addflag,mpeg2_struct);

        /* bottom field prediction */
        pred(newref,mv_field_sel[1][1],cur,1,
          lx<<1,16,8,bx,by>>1,PMV[1][1][0],PMV[1][1][1]>>1,addflag,mpeg2_struct);
      }
    }
    else /* TOP_FIELD or BOTTOM_FIELD */
    {
      /* field picture */

      currentfield = (pict_struct==BOTTOM_FIELD);

      if (motion_type==MC_FIELD)
      {
        /* field-based prediction in field picture */
        pred(newref,mv_field_sel[0][1],cur,currentfield,
          lx<<1,16,16,bx,by,PMV[0][1][0],PMV[0][1][1],addflag,mpeg2_struct);
      }
      else if (motion_type==MC_16X8)
      {
        /* 16 x 8 motion compensation in field picture */

        /* upper half */
        pred(newref,mv_field_sel[0][1],cur,currentfield,
          lx<<1,16,8,bx,by,PMV[0][1][0],PMV[0][1][1],addflag,mpeg2_struct);

        /* lower half */
        pred(newref,mv_field_sel[1][1],cur,currentfield,
          lx<<1,16,8,bx,by+8,PMV[1][1][0],PMV[1][1][1],addflag,mpeg2_struct);
      }
      else
      {
        /* invalid motion_type in field picture */
        if (!mpeg2_struct->quiet)
          fprintf(stderr,"invalid motion_type\n");
      }
    }
  }
}

/* predict a rectangular block (all three components)
 *
 * src:     source frame (Y,U,V)
 * sfield:  source field select (0: frame or top field, 1: bottom field)
 * dst:     destination frame (Y,U,V)
 * dfield:  destination field select (0: frame or top field, 1: bottom field)
 *
 * the following values are in luminance picture (frame or field) dimensions
 * lx:      distance of vertically adjacent pels (selects frame or field pred.)
 * w,h:     width and height of block (only 16x16 or 16x8 are used)
 * x,y:     coordinates of destination block
 * dx,dy:   half pel motion vector
 * addflag: store or add (= average) prediction
 */
static void pred(src,sfield,dst,dfield,lx,w,h,x,y,dx,dy,addflag, mpeg2_struct)
unsigned char *src[];
int sfield;
unsigned char *dst[];
int dfield;
int lx;
int w, h;
int x, y;
int dx, dy;
int addflag;
struct MPEG2_structure *mpeg2_struct;
{
  int cc;

  for (cc=0; cc<3; cc++)
  {
    if (cc==1)
    {
      /* scale for color components */
      if (mpeg2_struct->chroma_format==CHROMA420)
      {
        /* vertical */
        h >>= 1; y >>= 1; dy /= 2;
      }
      if (mpeg2_struct->chroma_format!=CHROMA444)
      {
        /* horizontal */
        w >>= 1; x >>= 1; dx /= 2;
        lx >>= 1;
      }
    }
    pred_comp(src[cc]+(sfield?lx>>1:0),dst[cc]+(dfield?lx>>1:0),
      lx,w,h,x,y,dx,dy,addflag);
  }
}

/* low level prediction routine
 *
 * src:     prediction source
 * dst:     prediction destination
 * lx:      line width (for both src and dst)
 * x,y:     destination coordinates
 * dx,dy:   half pel motion vector
 * w,h:     size of prediction block
 * addflag: store or add prediction
 */

static void pred_comp(src,dst,lx,w,h,x,y,dx,dy,addflag)
unsigned char *src;
unsigned char *dst;
int lx;
int w, h;
int x, y;
int dx, dy;
int addflag;
{
  int xint, xh, yint, yh;
  int i, j;
  unsigned char *s, *d;

  /* half pel scaling */
  xint = dx>>1; /* integer part */
  xh = dx & 1;  /* half pel flag */
  yint = dy>>1;
  yh = dy & 1;

  /* origins */
  s = src + lx*(y+yint) + (x+xint); /* motion vector */
  d = dst + lx*y + x;

  if (!xh && !yh)
    if (addflag)
      for (j=0; j<h; j++)
      {
        for (i=0; i<w; i++)
          d[i] = (unsigned int)(d[i]+s[i]+1)>>1;
        s+= lx;
        d+= lx;
      }
    else
      for (j=0; j<h; j++)
      {
        for (i=0; i<w; i++)
          d[i] = s[i];
        s+= lx;
        d+= lx;
      }
  else if (!xh && yh)
    if (addflag)
      for (j=0; j<h; j++)
      {
        for (i=0; i<w; i++)
          d[i] = (d[i] + ((unsigned int)(s[i]+s[i+lx]+1)>>1)+1)>>1;
        s+= lx;
        d+= lx;
      }
    else
      for (j=0; j<h; j++)
      {
        for (i=0; i<w; i++)
          d[i] = (unsigned int)(s[i]+s[i+lx]+1)>>1;
        s+= lx;
        d+= lx;
      }
  else if (xh && !yh)
    if (addflag)
      for (j=0; j<h; j++)
      {
        for (i=0; i<w; i++)
          d[i] = (d[i] + ((unsigned int)(s[i]+s[i+1]+1)>>1)+1)>>1;
        s+= lx;
        d+= lx;
      }
    else
      for (j=0; j<h; j++)
      {
        for (i=0; i<w; i++)
          d[i] = (unsigned int)(s[i]+s[i+1]+1)>>1;
        s+= lx;
        d+= lx;
      }
  else /* if (xh && yh) */
    if (addflag)
      for (j=0; j<h; j++)
      {
        for (i=0; i<w; i++)
          d[i] = (d[i] + ((unsigned int)(s[i]+s[i+1]+s[i+lx]+s[i+lx+1]+2)>>2)+1)>>1;
        s+= lx;
        d+= lx;
      }
    else
      for (j=0; j<h; j++)
      {
        for (i=0; i<w; i++)
          d[i] = (unsigned int)(s[i]+s[i+1]+s[i+lx]+s[i+lx+1]+2)>>2;
        s+= lx;
        d+= lx;
      }
}


/* calculate derived motion vectors (DMV) for dual prime prediction
 * dmvector[2]: differential motion vectors (-1,0,+1)
 * mvx,mvy: motion vector (for same parity)
 *
 * DMV[2][2]: derived motion vectors (for opposite parity)
 *
 * uses global variables pict_struct and topfirst
 *
 * Notes:
 *  - all vectors are in field coordinates (even for frame pictures)
 */

static void calc_DMV(DMV,dmvector,mvx,mvy,mpeg2_struct)
int DMV[][2];
int *dmvector;
int mvx, mvy;
struct MPEG2_structure *mpeg2_struct;
{
  if (mpeg2_struct->pict_struct==FRAME_PICTURE)
  {
    if (mpeg2_struct->topfirst)
    {
      /* vector for prediction of top field from bottom field */
      DMV[0][0] = ((mvx  +(mvx>0))>>1) + dmvector[0];
      DMV[0][1] = ((mvy  +(mvy>0))>>1) + dmvector[1] - 1;

      /* vector for prediction of bottom field from top field */
      DMV[1][0] = ((3*mvx+(mvx>0))>>1) + dmvector[0];
      DMV[1][1] = ((3*mvy+(mvy>0))>>1) + dmvector[1] + 1;
    }
    else
    {
      /* vector for prediction of top field from bottom field */
      DMV[0][0] = ((3*mvx+(mvx>0))>>1) + dmvector[0];
      DMV[0][1] = ((3*mvy+(mvy>0))>>1) + dmvector[1] - 1;

      /* vector for prediction of bottom field from top field */
      DMV[1][0] = ((mvx  +(mvx>0))>>1) + dmvector[0];
      DMV[1][1] = ((mvy  +(mvy>0))>>1) + dmvector[1] + 1;
    }
  }
  else
  {
    /* vector for prediction from field of opposite 'parity' */
    DMV[0][0] = ((mvx+(mvx>0))>>1) + dmvector[0];
    DMV[0][1] = ((mvy+(mvy>0))>>1) + dmvector[1];

    /* correct for vertical field shift */
    if (mpeg2_struct->pict_struct==TOP_FIELD)
      DMV[0][1]--;
    else
      DMV[0][1]++;
  }
}

static void clearblock(cur,i0,j0,mpeg2_struct)
unsigned char *cur[];
int i0, j0;
struct MPEG2_structure *mpeg2_struct;
{
  int i, j, w, h;
  unsigned char *p;

  p = cur[0] + ((mpeg2_struct->pict_struct==BOTTOM_FIELD) ? mpeg2_struct->width : 0) + i0 + mpeg2_struct->width2*j0;

  for (j=0; j<16; j++)
  {
    for (i=0; i<16; i++)
      p[i] = 128;
    p+= mpeg2_struct->width2;
  }

  w = h = 16;

  if (mpeg2_struct->chroma_format!=CHROMA444)
  {
    i0>>=1; w>>=1;
  }

  if (mpeg2_struct->chroma_format==CHROMA420)
  {
    j0>>=1; h>>=1;
  }

  p = cur[1] + ((mpeg2_struct->pict_struct==BOTTOM_FIELD) ? mpeg2_struct->chrom_width : 0) + i0
             + mpeg2_struct->chrom_width2*j0;

  for (j=0; j<h; j++)
  {
    for (i=0; i<w; i++)
      p[i] = 128;
    p+= mpeg2_struct->chrom_width2;
  }

  p = cur[2] + ((mpeg2_struct->pict_struct==BOTTOM_FIELD) ? mpeg2_struct->chrom_width : 0) + i0
             + mpeg2_struct->chrom_width2*j0;

  for (j=0; j<h; j++)
  {
    for (i=0; i<w; i++)
      p[i] = 128;
    p+= mpeg2_struct->chrom_width2;
  }
}
