/* transfrm.c,  forward / inverse transformation                            */

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
#include <math.h>
#include "config.h"
#include "global.h"

/* private prototypes*/
static void add_pred _ANSI_ARGS_((unsigned char *pred, unsigned char *cur,
  int lx, short *blk));
static void sub_pred _ANSI_ARGS_((unsigned char *pred, unsigned char *cur,
  int lx, short *blk));

/* subtract prediction and transform prediction error */
void MPEG2_transform( unsigned char *pred[], unsigned char *cur[],
  struct mbinfo *mbi,
  short blocks[][64])
{
  int i, j, i1, j1, k, n, cc, offs, lx;

  k = 0;

  for (j=0; j<vtkMPEG2WriterStr->height2; j+=16)
    for (i=0; i<vtkMPEG2WriterStr->width; i+=16)
    {
      for (n=0; n<vtkMPEG2WriterStr->block_count; n++)
      {
        cc = (n<4) ? 0 : (n&1)+1; /* color component index */
        if (cc==0)
        {
          /* luminance */
          if ((vtkMPEG2WriterStr->pict_struct==FRAME_PICTURE) && mbi[k].dct_type)
          {
            /* field DCT */
            offs = i + ((n&1)<<3) + vtkMPEG2WriterStr->width*(j+((n&2)>>1));
            lx = vtkMPEG2WriterStr->width<<1;
          }
          else
          {
            /* frame DCT */
            offs = i + ((n&1)<<3) + vtkMPEG2WriterStr->width2*(j+((n&2)<<2));
            lx = vtkMPEG2WriterStr->width2;
          }

          if (vtkMPEG2WriterStr->pict_struct==BOTTOM_FIELD)
            offs += vtkMPEG2WriterStr->width;
        }
        else
        {
          /* chrominance */

          /* scale coordinates */
          i1 = (vtkMPEG2WriterStr->chroma_format==CHROMA444) ? i : i>>1;
          j1 = (vtkMPEG2WriterStr->chroma_format!=CHROMA420) ? j : j>>1;

          if ((vtkMPEG2WriterStr->pict_struct==FRAME_PICTURE) && mbi[k].dct_type
              && (vtkMPEG2WriterStr->chroma_format!=CHROMA420))
          {
            /* field DCT */
            offs = i1 + (n&8) + vtkMPEG2WriterStr->chrom_width*(j1+((n&2)>>1));
            lx = vtkMPEG2WriterStr->chrom_width<<1;
          }
          else
          {
            /* frame DCT */
            offs = i1 + (n&8) + vtkMPEG2WriterStr->chrom_width2*(j1+((n&2)<<2));
            lx = vtkMPEG2WriterStr->chrom_width2;
          }

          if (vtkMPEG2WriterStr->pict_struct==BOTTOM_FIELD)
            offs += vtkMPEG2WriterStr->chrom_width;
        }

        sub_pred(pred[cc]+offs,cur[cc]+offs,lx,blocks[k*vtkMPEG2WriterStr->block_count+n]);
        MPEG2_fdct(blocks[k*vtkMPEG2WriterStr->block_count+n]);
      }

      k++;
    }
}

/* inverse transform prediction error and add prediction */
void MPEG2_itransform( unsigned char *pred[], unsigned char *cur[],
  struct mbinfo *mbi,
  short blocks[][64])
{
  int i, j, i1, j1, k, n, cc, offs, lx;

  k = 0;

  for (j=0; j<vtkMPEG2WriterStr->height2; j+=16)
    for (i=0; i<vtkMPEG2WriterStr->width; i+=16)
    {
      for (n=0; n<vtkMPEG2WriterStr->block_count; n++)
      {
        cc = (n<4) ? 0 : (n&1)+1; /* color component index */

        if (cc==0)
        {
          /* luminance */
          if ((vtkMPEG2WriterStr->pict_struct==FRAME_PICTURE) && mbi[k].dct_type)
          {
            /* field DCT */
            offs = i + ((n&1)<<3) + vtkMPEG2WriterStr->width*(j+((n&2)>>1));
            lx = vtkMPEG2WriterStr->width<<1;
          }
          else
          {
            /* frame DCT */
            offs = i + ((n&1)<<3) + vtkMPEG2WriterStr->width2*(j+((n&2)<<2));
            lx = vtkMPEG2WriterStr->width2;
          }

          if (vtkMPEG2WriterStr->pict_struct==BOTTOM_FIELD)
            offs += vtkMPEG2WriterStr->width;
        }
        else
        {
          /* chrominance */

          /* scale coordinates */
          i1 = (vtkMPEG2WriterStr->chroma_format==CHROMA444) ? i : i>>1;
          j1 = (vtkMPEG2WriterStr->chroma_format!=CHROMA420) ? j : j>>1;

          if ((vtkMPEG2WriterStr->pict_struct==FRAME_PICTURE) && mbi[k].dct_type
              && (vtkMPEG2WriterStr->chroma_format!=CHROMA420))
          {
            /* field DCT */
            offs = i1 + (n&8) + vtkMPEG2WriterStr->chrom_width*(j1+((n&2)>>1));
            lx = vtkMPEG2WriterStr->chrom_width<<1;
          }
          else
          {
            /* frame DCT */
            offs = i1 + (n&8) + vtkMPEG2WriterStr->chrom_width2*(j1+((n&2)<<2));
            lx = vtkMPEG2WriterStr->chrom_width2;
          }

          if (vtkMPEG2WriterStr->pict_struct==BOTTOM_FIELD)
            offs += vtkMPEG2WriterStr->chrom_width;
        }

        MPEG2_idct(blocks[k*vtkMPEG2WriterStr->block_count+n]);
        add_pred(pred[cc]+offs,cur[cc]+offs,lx,blocks[k*vtkMPEG2WriterStr->block_count+n]);
      }

      k++;
    }
}

/* add prediction and prediction error, saturate to 0...255 */
static void add_pred( unsigned char *pred, unsigned char *cur,
  int lx,
  short *blk)
{
  int i, j;

  for (j=0; j<8; j++)
  {
    for (i=0; i<8; i++)
      cur[i] = vtkMPEG2WriterStr->clp[blk[i] + pred[i]];
    blk+= 8;
    cur+= lx;
    pred+= lx;
  }
}

/* subtract prediction from block data */
static void sub_pred(pred,cur,lx,blk)
unsigned char *pred, *cur;
int lx;
short *blk;
{
  int i, j;

  for (j=0; j<8; j++)
  {
    for (i=0; i<8; i++)
      blk[i] = cur[i] - pred[i];
    blk+= 8;
    cur+= lx;
    pred+= lx;
  }
}

/*
 * select between frame and field DCT
 *
 * preliminary version: based on inter-field correlation
 */
void MPEG2_dct_type_estimation( unsigned char *pred, unsigned char *cur,
  struct mbinfo *mbi)
{
  short blk0[128], blk1[128];
  int i, j, i0, j0, k, offs, s0, s1, sq0, sq1, s01;
  double d, r;

  k = 0;

  for (j0=0; j0<vtkMPEG2WriterStr->height2; j0+=16)
    for (i0=0; i0<vtkMPEG2WriterStr->width; i0+=16)
    {
      if (vtkMPEG2WriterStr->frame_pred_dct || vtkMPEG2WriterStr->pict_struct!=FRAME_PICTURE)
        mbi[k].dct_type = 0;
      else
      {
        /* interlaced frame picture */
        /*
         * calculate prediction error (cur-pred) for top (blk0)
         * and bottom field (blk1)
         */
        for (j=0; j<8; j++)
        {
          offs = vtkMPEG2WriterStr->width*((j<<1)+j0) + i0;
          for (i=0; i<16; i++)
          {
            blk0[16*j+i] = cur[offs] - pred[offs];
            blk1[16*j+i] = cur[offs+vtkMPEG2WriterStr->width] - pred[offs+vtkMPEG2WriterStr->width];
            offs++;
          }
        }
        /* correlate fields */
        s0=s1=sq0=sq1=s01=0;

        for (i=0; i<128; i++)
        {
          s0+= blk0[i];
          sq0+= blk0[i]*blk0[i];
          s1+= blk1[i];
          sq1+= blk1[i]*blk1[i];
          s01+= blk0[i]*blk1[i];
        }

        d = (sq0-(s0*s0)/128.0)*(sq1-(s1*s1)/128.0);

        if (d>0.0)
        {
          r = (s01-(s0*s1)/128.0)/sqrt(d);
          if (r>0.5)
            mbi[k].dct_type = 0; /* frame DCT */
          else
            mbi[k].dct_type = 1; /* field DCT */
        }
        else
          mbi[k].dct_type = 1; /* field DCT */
      }
      k++;
    }
}
