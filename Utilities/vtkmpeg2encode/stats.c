/* stats.c, coding statistics                                               */

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
#include "mpeg2enc_config.h"
#include "mpeg2enc_global.h"

/* private prototypes */
static void MPEG2_calcSNR1 _ANSI_ARGS_((unsigned char *org, unsigned char *rec,
  int lx, int w, int h, double *pv, double *pe));


void MPEG2_calcSNR(org,rec,mpeg2_struct)
unsigned char *org[3];
unsigned char *rec[3];
struct MPEG2_structure *mpeg2_struct;
{
  int w,h,offs;
  double v,e;

  w = mpeg2_struct->horizontal_size;
  h = (mpeg2_struct->pict_struct==FRAME_PICTURE) ? mpeg2_struct->vertical_size : (mpeg2_struct->vertical_size>>1);
  offs = (mpeg2_struct->pict_struct==BOTTOM_FIELD) ? mpeg2_struct->width : 0;

  MPEG2_calcSNR1(org[0]+offs,rec[0]+offs,mpeg2_struct->width2,w,h,&v,&e);
  if ( mpeg2_struct->statfile )
    {
    fprintf(mpeg2_struct->statfile,"Y: variance=%4.4g, MSE=%3.3g (%3.3g dB), SNR=%3.3g dB\n",
      v, e, 10.0*log10(255.0*255.0/e), 10.0*log10(v/e));
    }

  if (mpeg2_struct->chroma_format!=CHROMA444)
  {
    w >>= 1;
    offs >>= 1;
  }

  if (mpeg2_struct->chroma_format==CHROMA420)
    h >>= 1;

  MPEG2_calcSNR1(org[1]+offs,rec[1]+offs,mpeg2_struct->chrom_width2,w,h,&v,&e);
  if ( mpeg2_struct->statfile )
    {
    fprintf(mpeg2_struct->statfile,"U: variance=%4.4g, MSE=%3.3g (%3.3g dB), SNR=%3.3g dB\n",
      v, e, 10.0*log10(255.0*255.0/e), 10.0*log10(v/e));
    }

  MPEG2_calcSNR1(org[2]+offs,rec[2]+offs,mpeg2_struct->chrom_width2,w,h,&v,&e);
  if ( mpeg2_struct->statfile )
    {
    fprintf(mpeg2_struct->statfile,"V: variance=%4.4g, MSE=%3.3g (%3.3g dB), SNR=%3.3g dB\n",
      v, e, 10.0*log10(255.0*255.0/e), 10.0*log10(v/e));
    }
}

static void MPEG2_calcSNR1(org,rec,lx,w,h,pv,pe)
unsigned char *org;
unsigned char *rec;
int lx,w,h;
double *pv,*pe;
{
  int i, j;
  double v1, s1, s2, e2;

  s1 = s2 = e2 = 0.0;

  for (j=0; j<h; j++)
  {
    for (i=0; i<w; i++)
    {
      v1 = org[i];
      s1+= v1;
      s2+= v1*v1;
      v1-= rec[i];
      e2+= v1*v1;
    }
    org += lx;
    rec += lx;
  }

  s1 /= w*h;
  s2 /= w*h;
  e2 /= w*h;

  /* prevent division by zero in calcSNR() */
  if(e2==0.0)
    e2 = 0.00001;

  *pv = s2 - s1*s1; /* variance */
  *pe = e2;         /* MSE */
}

void MPEG2_stats(mpeg2_struct)
  struct MPEG2_structure *mpeg2_struct;
{
  int i, j, k, nmb, mb_type;
  int n_skipped, n_intra, n_ncoded, n_blocks, n_interp, n_forward, n_backward;
  struct mbinfo *mbi;

  nmb = mpeg2_struct->mb_width*mpeg2_struct->mb_height2;

  n_skipped=n_intra=n_ncoded=n_blocks=n_interp=n_forward=n_backward=0;

  for (k=0; k<nmb; k++)
  {
    mbi = mpeg2_struct->mbinfo+k;
    if (mbi->skipped)
      n_skipped++;
    else if (mbi->mb_type & MB_INTRA)
      n_intra++;
    else if (!(mbi->mb_type & MB_PATTERN))
      n_ncoded++;

    for (i=0; i<mpeg2_struct->block_count; i++)
      if (mbi->cbp & (1<<i))
        n_blocks++;

    if (mbi->mb_type & MB_FORWARD)
    {
      if (mbi->mb_type & MB_BACKWARD)
        n_interp++;
      else
        n_forward++;
    }
    else if (mbi->mb_type & MB_BACKWARD)
      n_backward++;
  }

  if ( mpeg2_struct->statfile )
    {
    fprintf(mpeg2_struct->statfile,"\npicture statistics:\n");
    fprintf(mpeg2_struct->statfile," # of intra coded macroblocks:  %4d (%.1f%%)\n",
      n_intra,100.0*(double)n_intra/nmb);
    fprintf(mpeg2_struct->statfile," # of coded blocks:             %4d (%.1f%%)\n",
      n_blocks,100.0*(double)n_blocks/(mpeg2_struct->block_count*nmb));
    fprintf(mpeg2_struct->statfile," # of not coded macroblocks:    %4d (%.1f%%)\n",
      n_ncoded,100.0*(double)n_ncoded/nmb);
    fprintf(mpeg2_struct->statfile," # of skipped macroblocks:      %4d (%.1f%%)\n",
      n_skipped,100.0*(double)n_skipped/nmb);
    fprintf(mpeg2_struct->statfile," # of forw. pred. macroblocks:  %4d (%.1f%%)\n",
      n_forward,100.0*(double)n_forward/nmb);
    fprintf(mpeg2_struct->statfile," # of backw. pred. macroblocks: %4d (%.1f%%)\n",
      n_backward,100.0*(double)n_backward/nmb);
    fprintf(mpeg2_struct->statfile," # of interpolated macroblocks: %4d (%.1f%%)\n",
      n_interp,100.0*(double)n_interp/nmb);

    fprintf(mpeg2_struct->statfile,"\nmacroblock_type map:\n");

    k = 0;

    for (j=0; j<mpeg2_struct->mb_height2; j++)
      {
      for (i=0; i<mpeg2_struct->mb_width; i++)
        {
        mbi = mpeg2_struct->mbinfo + k;
        mb_type = mbi->mb_type;
        if (mbi->skipped)
          putc('S',mpeg2_struct->statfile);
        else if (mb_type & MB_INTRA)
          putc('I',mpeg2_struct->statfile);
        else switch (mb_type & (MB_FORWARD|MB_BACKWARD))
          {
        case MB_FORWARD:
          putc(mbi->motion_type==MC_FIELD ? 'f' :
            mbi->motion_type==MC_DMV   ? 'p' :
            'F',mpeg2_struct->statfile); break;
        case MB_BACKWARD:
          putc(mbi->motion_type==MC_FIELD ? 'b' :
            'B',mpeg2_struct->statfile); break;
        case MB_FORWARD|MB_BACKWARD:
          putc(mbi->motion_type==MC_FIELD ? 'd' :
            'D',mpeg2_struct->statfile); break;
        default:
          putc('0',mpeg2_struct->statfile); break;
          }

        if (mb_type & MB_QUANT)
          putc('Q',mpeg2_struct->statfile);
        else if (mb_type & (MB_PATTERN|MB_INTRA))
          putc(' ',mpeg2_struct->statfile);
        else
          putc('N',mpeg2_struct->statfile);

        putc(' ',mpeg2_struct->statfile);

        k++;
        }
      putc('\n',mpeg2_struct->statfile);
      }

    fprintf(mpeg2_struct->statfile,"\nmquant map:\n");

    k=0;
    for (j=0; j<mpeg2_struct->mb_height2; j++)
      {
      for (i=0; i<mpeg2_struct->mb_width; i++)
        {
        if (i==0 || mpeg2_struct->mbinfo[k].mquant!=mpeg2_struct->mbinfo[k-1].mquant)
          fprintf(mpeg2_struct->statfile,"%3d",mpeg2_struct->mbinfo[k].mquant);
        else
          fprintf(mpeg2_struct->statfile,"   ");

        k++;
        }
      putc('\n',mpeg2_struct->statfile);
      }

#if 0
    fprintf(mpeg2_struct->statfile,"\ncbp map:\n");

    k=0;
    for (j=0; j<mpeg2_struct->mb_height2; j++)
      {
      for (i=0; i<mpeg2_struct->mb_width; i++)
        {
        fprintf(mpeg2_struct->statfile,"%02x ",mpeg2_struct->mbinfo[k].cbp);

        k++;
        }
      putc('\n',mpeg2_struct->statfile);
      }

    if (mpeg2_struct->pict_struct==FRAME_PICTURE && !mpeg2_struct->frame_pred_dct)
      {
      fprintf(mpeg2_struct->statfile,"\ndct_type map:\n");

      k=0;
      for (j=0; j<mpeg2_struct->mb_height2; j++)
        {
        for (i=0; i<mpeg2_struct->mb_width; i++)
          {
          if (mpeg2_struct->mbinfo[k].mb_type & (MB_PATTERN|MB_INTRA))
            fprintf(mpeg2_struct->statfile,"%d  ",mpeg2_struct->mbinfo[k].dct_type);
          else
            fprintf(mpeg2_struct->statfile,"   ");

          k++;
          }
        putc('\n',mpeg2_struct->statfile);
        }
      }

    if (mpeg2_struct->pict_type!=I_TYPE)
      {
      fprintf(mpeg2_struct->statfile,"\nforward motion vectors (first vector, horizontal):\n");

      k=0;
      for (j=0; j<mpeg2_struct->mb_height2; j++)
        {
        for (i=0; i<mpeg2_struct->mb_width; i++)
          {
          if (mpeg2_struct->mbinfo[k].mb_type & MB_FORWARD)
            fprintf(mpeg2_struct->statfile,"%4d",mpeg2_struct->mbinfo[k].MV[0][0][0]);
          else
            fprintf(mpeg2_struct->statfile,"   .");

          k++;
          }
        putc('\n',mpeg2_struct->statfile);
        }

      fprintf(mpeg2_struct->statfile,"\nforward motion vectors (first vector, vertical):\n");

      k=0;
      for (j=0; j<mpeg2_struct->mb_height2; j++)
        {
        for (i=0; i<mpeg2_struct->mb_width; i++)
          {
          if (mpeg2_struct->mbinfo[k].mb_type & MB_FORWARD)
            fprintf(mpeg2_struct->statfile,"%4d",mpeg2_struct->mbinfo[k].MV[0][0][1]);
          else
            fprintf(mpeg2_struct->statfile,"   .");

          k++;
          }
        putc('\n',mpeg2_struct->statfile);
        }

      fprintf(mpeg2_struct->statfile,"\nforward motion vectors (second vector, horizontal):\n");

      k=0;
      for (j=0; j<mpeg2_struct->mb_height2; j++)
        {
        for (i=0; i<mpeg2_struct->mb_width; i++)
          {
          if (mpeg2_struct->mbinfo[k].mb_type & MB_FORWARD
            && ((mpeg2_struct->pict_struct==FRAME_PICTURE && mpeg2_struct->mbinfo[k].motion_type==MC_FIELD) ||
              (mpeg2_struct->pict_struct!=FRAME_PICTURE && mpeg2_struct->mbinfo[k].motion_type==MC_16X8)))
            fprintf(mpeg2_struct->statfile,"%4d",mpeg2_struct->mbinfo[k].MV[1][0][0]);
          else
            fprintf(mpeg2_struct->statfile,"   .");

          k++;
          }
        putc('\n',mpeg2_struct->statfile);
        }

      fprintf(mpeg2_struct->statfile,"\nforward motion vectors (second vector, vertical):\n");

      k=0;
      for (j=0; j<mpeg2_struct->mb_height2; j++)
        {
        for (i=0; i<mpeg2_struct->mb_width; i++)
          {
          if (mpeg2_struct->mbinfo[k].mb_type & MB_FORWARD
            && ((mpeg2_struct->pict_struct==FRAME_PICTURE && mpeg2_struct->mbinfo[k].motion_type==MC_FIELD) ||
              (mpeg2_struct->pict_struct!=FRAME_PICTURE && mpeg2_struct->mbinfo[k].motion_type==MC_16X8)))
            fprintf(mpeg2_struct->statfile,"%4d",mpeg2_struct->mbinfo[k].MV[1][0][1]);
          else
            fprintf(mpeg2_struct->statfile,"   .");

          k++;
          }
        putc('\n',mpeg2_struct->statfile);
        }


      }

    if (mpeg2_struct->pict_type==B_TYPE)
      {
      fprintf(mpeg2_struct->statfile,"\nbackward motion vectors (first vector, horizontal):\n");

      k=0;
      for (j=0; j<mpeg2_struct->mb_height2; j++)
        {
        for (i=0; i<mpeg2_struct->mb_width; i++)
          {
          if (mpeg2_struct->mbinfo[k].mb_type & MB_BACKWARD)
            fprintf(mpeg2_struct->statfile,"%4d",mpeg2_struct->mbinfo[k].MV[0][1][0]);
          else
            fprintf(mpeg2_struct->statfile,"   .");

          k++;
          }
        putc('\n',mpeg2_struct->statfile);
        }

      fprintf(mpeg2_struct->statfile,"\nbackward motion vectors (first vector, vertical):\n");

      k=0;
      for (j=0; j<mpeg2_struct->mb_height2; j++)
        {
        for (i=0; i<mpeg2_struct->mb_width; i++)
          {
          if (mpeg2_struct->mbinfo[k].mb_type & MB_BACKWARD)
            fprintf(mpeg2_struct->statfile,"%4d",mpeg2_struct->mbinfo[k].MV[0][1][1]);
          else
            fprintf(mpeg2_struct->statfile,"   .");

          k++;
          }
        putc('\n',mpeg2_struct->statfile);
        }

      fprintf(mpeg2_struct->statfile,"\nbackward motion vectors (second vector, horizontal):\n");

      k=0;
      for (j=0; j<mpeg2_struct->mb_height2; j++)
        {
        for (i=0; i<mpeg2_struct->mb_width; i++)
          {
          if (mpeg2_struct->mbinfo[k].mb_type & MB_BACKWARD
            && ((mpeg2_struct->pict_struct==FRAME_PICTURE && mpeg2_struct->mbinfo[k].motion_type==MC_FIELD) ||
              (mpeg2_struct->pict_struct!=FRAME_PICTURE && mpeg2_struct->mbinfo[k].motion_type==MC_16X8)))
            fprintf(mpeg2_struct->statfile,"%4d",mpeg2_struct->mbinfo[k].MV[1][1][0]);
          else
            fprintf(mpeg2_struct->statfile,"   .");

          k++;
          }
        putc('\n',mpeg2_struct->statfile);
        }

      fprintf(mpeg2_struct->statfile,"\nbackward motion vectors (second vector, vertical):\n");

      k=0;
      for (j=0; j<mpeg2_struct->mb_height2; j++)
        {
        for (i=0; i<mpeg2_struct->mb_width; i++)
          {
          if (mpeg2_struct->mbinfo[k].mb_type & MB_BACKWARD
            && ((mpeg2_struct->pict_struct==FRAME_PICTURE && mpeg2_struct->mbinfo[k].motion_type==MC_FIELD) ||
              (mpeg2_struct->pict_struct!=FRAME_PICTURE && mpeg2_struct->mbinfo[k].motion_type==MC_16X8)))
            fprintf(mpeg2_struct->statfile,"%4d",mpeg2_struct->mbinfo[k].MV[1][1][1]);
          else
            fprintf(mpeg2_struct->statfile,"   .");

          k++;
          }
        putc('\n',mpeg2_struct->statfile);
        }


      }
#endif

#if 0
    /* useful for debugging */
    fprintf(mpeg2_struct->statfile,"\nmacroblock info dump:\n");

    k=0;
    for (j=0; j<mpeg2_struct->mb_height2; j++)
      {
      for (i=0; i<mpeg2_struct->mb_width; i++)
        {
        fprintf(mpeg2_struct->statfile,"%d: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
          k,
          mpeg2_struct->mbinfo[k].mb_type,
          mpeg2_struct->mbinfo[k].motion_type,
          mpeg2_struct->mbinfo[k].dct_type,
          mpeg2_struct->mbinfo[k].mquant,
          mpeg2_struct->mbinfo[k].cbp,
          mpeg2_struct->mbinfo[k].skipped,
          mpeg2_struct->mbinfo[k].MV[0][0][0],
          mpeg2_struct->mbinfo[k].MV[0][0][1],
          mpeg2_struct->mbinfo[k].MV[0][1][0],
          mpeg2_struct->mbinfo[k].MV[0][1][1],
          mpeg2_struct->mbinfo[k].MV[1][0][0],
          mpeg2_struct->mbinfo[k].MV[1][0][1],
          mpeg2_struct->mbinfo[k].MV[1][1][0],
          mpeg2_struct->mbinfo[k].MV[1][1][1],
          mpeg2_struct->mbinfo[k].mv_field_sel[0][0],
          mpeg2_struct->mbinfo[k].mv_field_sel[0][1],
          mpeg2_struct->mbinfo[k].mv_field_sel[1][0],
          mpeg2_struct->mbinfo[k].mv_field_sel[1][1]);

        k++;
        }
      }
#endif
    }
}
