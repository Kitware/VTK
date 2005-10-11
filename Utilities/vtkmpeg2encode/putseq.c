/* putseq.c, sequence level routines                                        */

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
#include <string.h>
#include "mpeg2enc_config.h"
#include "mpeg2enc_global.h"

static void MPEG2_evaluate_frame_info(int cframe, int* f0, int* realframe, unsigned char* neworg[], unsigned char* newref[], int* sxf, int* syf, int* sxb, int* syb,struct MPEG2_structure *mpeg2_struct)
{
  int j, n, np, nb;

  /* f0: lowest frame number in current GOP
   *
   * first GOP contains N-(M-1) frames,
   * all other GOPs contain N frames
   */
  *f0 = mpeg2_struct->N_val*((cframe+(mpeg2_struct->M_val-1))/mpeg2_struct->N_val) - (mpeg2_struct->M_val-1);

  if (*f0<0)
    *f0=0;

  if (cframe==0 || (cframe-1)%mpeg2_struct->M_val==0)
    {
    /* I or P frame */
    for (j=0; j<3; j++)
      {
      /* shuffle reference frames */
      neworg[j] = mpeg2_struct->oldorgframe[j];
      newref[j] = mpeg2_struct->oldrefframe[j];
      mpeg2_struct->oldorgframe[j] = mpeg2_struct->neworgframe[j];
      mpeg2_struct->oldrefframe[j] = mpeg2_struct->newrefframe[j];
      mpeg2_struct->neworgframe[j] = neworg[j];
      mpeg2_struct->newrefframe[j] = newref[j];
      }

    /* f: frame number in display order */
    *realframe = (cframe==0) ? 0 : cframe+mpeg2_struct->M_val-1;
    if (*realframe>=mpeg2_struct->nframes)
      *realframe = mpeg2_struct->nframes - 1;

    if (cframe==*f0) /* first displayed frame in GOP is I */
      {
      /* I frame */
      mpeg2_struct->pict_type = I_TYPE;
      mpeg2_struct->forw_hor_f_code = mpeg2_struct->forw_vert_f_code = 15;
      mpeg2_struct->back_hor_f_code = mpeg2_struct->back_vert_f_code = 15;

      /* n: number of frames in current GOP
       *
       * first GOP contains (M-1) less (B) frames
       */
      n = (cframe==0) ? mpeg2_struct->N_val-(mpeg2_struct->M_val-1) : mpeg2_struct->N_val;

      /* last GOP may contain less frames */
      if (n > mpeg2_struct->nframes-*f0)
        n = mpeg2_struct->nframes-*f0;

      /* number of P frames */
      if (cframe==0)
        np = (n + 2*(mpeg2_struct->M_val-1))/mpeg2_struct->M_val - 1; /* first GOP */
      else
        np = (n + (mpeg2_struct->M_val-1))/mpeg2_struct->M_val - 1;

      /* number of B frames */
      nb = n - np - 1;

      MPEG2_rc_init_GOP(np,nb,mpeg2_struct);

      MPEG2_putgophdr(*f0,cframe==0,mpeg2_struct); /* set closed_GOP in first GOP only */
      }
    else
      {
      /* P frame */
      mpeg2_struct->pict_type = P_TYPE;
      mpeg2_struct->forw_hor_f_code = mpeg2_struct->motion_data[0].forw_hor_f_code;
      mpeg2_struct->forw_vert_f_code = mpeg2_struct->motion_data[0].forw_vert_f_code;
      mpeg2_struct->back_hor_f_code = mpeg2_struct->back_vert_f_code = 15;
      *sxf = mpeg2_struct->motion_data[0].sxf;
      *syf = mpeg2_struct->motion_data[0].syf;
      }
    }
  else
    {
    /* B frame */
    for (j=0; j<3; j++)
      {
      neworg[j] = mpeg2_struct->auxorgframe[j];
      newref[j] = mpeg2_struct->auxframe[j];
      }

    /* f: frame number in display order */
    *realframe = cframe - 1;
    mpeg2_struct->pict_type = B_TYPE;
    n = (cframe-2)%mpeg2_struct->M_val + 1; /* first B: n=1, second B: n=2, ... */
    mpeg2_struct->forw_hor_f_code = mpeg2_struct->motion_data[n].forw_hor_f_code;
    mpeg2_struct->forw_vert_f_code = mpeg2_struct->motion_data[n].forw_vert_f_code;
    mpeg2_struct->back_hor_f_code = mpeg2_struct->motion_data[n].back_hor_f_code;
    mpeg2_struct->back_vert_f_code = mpeg2_struct->motion_data[n].back_vert_f_code;
    *sxf = mpeg2_struct->motion_data[n].sxf;
    *syf = mpeg2_struct->motion_data[n].syf;
    *sxb = mpeg2_struct->motion_data[n].sxb;
    *syb = mpeg2_struct->motion_data[n].syb;
    }
}

VTK_MPEG2ENC_EXPORT int MPEG2_putseq_one(int cframe, int max,struct MPEG2_structure *mpeg2_struct)
{
  int j, k, realframe, f0, sxf, syf, sxb, syb;
  int ipflag;
  char name[256];
  unsigned char *neworg[3], *newref[3];
  static char ipb[5] = {' ','I','P','B','D'};
  if (!mpeg2_struct->quiet)
    {
    fprintf(stderr,"Encoding frame %d ",cframe);
    fflush(stderr);
    }

  MPEG2_evaluate_frame_info(cframe, &f0, &realframe, neworg, newref,
    &sxf, &syf, &sxb, &syb,mpeg2_struct);
  if ( realframe > max )
    {
    return -1;
    }

  mpeg2_struct->temp_ref = realframe - f0;
  mpeg2_struct->frame_pred_dct = mpeg2_struct->frame_pred_dct_tab[mpeg2_struct->pict_type-1];
  mpeg2_struct->q_scale_type = mpeg2_struct->qscale_tab[mpeg2_struct->pict_type-1];
  mpeg2_struct->intravlc = mpeg2_struct->intravlc_tab[mpeg2_struct->pict_type-1];
  mpeg2_struct->altscan = mpeg2_struct->altscan_tab[mpeg2_struct->pict_type-1];

  if ( mpeg2_struct->statfile )
    {
    fprintf(mpeg2_struct->statfile,"\nFrame %d (#%d in display order):\n",cframe,realframe);
    fprintf(mpeg2_struct->statfile," picture_type=%c\n",ipb[mpeg2_struct->pict_type]);
    fprintf(mpeg2_struct->statfile," temporal_reference=%d\n",mpeg2_struct->temp_ref);
    fprintf(mpeg2_struct->statfile," frame_pred_frame_dct=%d\n",mpeg2_struct->frame_pred_dct);
    fprintf(mpeg2_struct->statfile," q_scale_type=%d\n",mpeg2_struct->q_scale_type);
    fprintf(mpeg2_struct->statfile," intra_vlc_format=%d\n",mpeg2_struct->intravlc);
    fprintf(mpeg2_struct->statfile," alternate_scan=%d\n",mpeg2_struct->altscan);

    if (mpeg2_struct->pict_type!=I_TYPE)
      {
      fprintf(mpeg2_struct->statfile," forward search window: %d...%d / %d...%d\n",
        -sxf,sxf,-syf,syf);
      fprintf(mpeg2_struct->statfile," forward vector range: %d...%d.5 / %d...%d.5\n",
        -(4<<mpeg2_struct->forw_hor_f_code),(4<<mpeg2_struct->forw_hor_f_code)-1,
        -(4<<mpeg2_struct->forw_vert_f_code),(4<<mpeg2_struct->forw_vert_f_code)-1);
      }

    if (mpeg2_struct->pict_type==B_TYPE)
      {
      fprintf(mpeg2_struct->statfile," backward search window: %d...%d / %d...%d\n",
        -sxb,sxb,-syb,syb);
      fprintf(mpeg2_struct->statfile," backward vector range: %d...%d.5 / %d...%d.5\n",
        -(4<<mpeg2_struct->back_hor_f_code),(4<<mpeg2_struct->back_hor_f_code)-1,
        -(4<<mpeg2_struct->back_vert_f_code),(4<<mpeg2_struct->back_vert_f_code)-1);
      }
    }

  sprintf(name,mpeg2_struct->tplorg,realframe+mpeg2_struct->frame0);
  MPEG2_readframe(name,neworg,mpeg2_struct);

  if (mpeg2_struct->fieldpic)
    {
    if (!mpeg2_struct->quiet)
      {
      fprintf(stderr,"\nfirst field  (%s) ",mpeg2_struct->topfirst ? "top" : "bot");
      fflush(stderr);
      }

    mpeg2_struct->pict_struct = mpeg2_struct->topfirst ? TOP_FIELD : BOTTOM_FIELD;

    MPEG2_motion_estimation(mpeg2_struct->oldorgframe[0],mpeg2_struct->neworgframe[0],
      mpeg2_struct->oldrefframe[0],mpeg2_struct->newrefframe[0],
      neworg[0],newref[0],
      sxf,syf,sxb,syb,mpeg2_struct->mbinfo,0,0,mpeg2_struct);

    MPEG2_predict(mpeg2_struct->oldrefframe,mpeg2_struct->newrefframe,mpeg2_struct->predframe,0,mpeg2_struct->mbinfo,mpeg2_struct);
    MPEG2_dct_type_estimation(mpeg2_struct->predframe[0],neworg[0],mpeg2_struct->mbinfo,mpeg2_struct);
    MPEG2_transform(mpeg2_struct->predframe,neworg,mpeg2_struct->mbinfo,mpeg2_struct->blocks,mpeg2_struct);

    MPEG2_putpict(neworg[0],mpeg2_struct);

    for (k=0; k<mpeg2_struct->mb_height2*mpeg2_struct->mb_width; k++)
      {
      if (mpeg2_struct->mbinfo[k].mb_type & MB_INTRA)
        for (j=0; j<mpeg2_struct->block_count; j++)
          MPEG2_iquant_intra(mpeg2_struct->blocks[k*mpeg2_struct->block_count+j],mpeg2_struct->blocks[k*mpeg2_struct->block_count+j],
            mpeg2_struct->dc_prec,mpeg2_struct->intra_q,mpeg2_struct->mbinfo[k].mquant,mpeg2_struct);
      else
        for (j=0;j<mpeg2_struct->block_count;j++)
          MPEG2_iquant_non_intra(mpeg2_struct->blocks[k*mpeg2_struct->block_count+j],mpeg2_struct->blocks[k*mpeg2_struct->block_count+j],
            mpeg2_struct->inter_q,mpeg2_struct->mbinfo[k].mquant,mpeg2_struct);
      }

    MPEG2_itransform(mpeg2_struct->predframe,newref,mpeg2_struct->mbinfo,mpeg2_struct->blocks,mpeg2_struct);
    MPEG2_calcSNR(neworg,newref,mpeg2_struct);
    MPEG2_stats(mpeg2_struct);

    if (!mpeg2_struct->quiet)
      {
      fprintf(stderr,"second field (%s) ",mpeg2_struct->topfirst ? "bot" : "top");
      fflush(stderr);
      }

    mpeg2_struct->pict_struct = mpeg2_struct->topfirst ? BOTTOM_FIELD : TOP_FIELD;

    ipflag = (mpeg2_struct->pict_type==I_TYPE);
    if (ipflag)
      {
      /* first field = I, second field = P */
      mpeg2_struct->pict_type = P_TYPE;
      mpeg2_struct->forw_hor_f_code = mpeg2_struct->motion_data[0].forw_hor_f_code;
      mpeg2_struct->forw_vert_f_code = mpeg2_struct->motion_data[0].forw_vert_f_code;
      mpeg2_struct->back_hor_f_code = mpeg2_struct->back_vert_f_code = 15;
      sxf = mpeg2_struct->motion_data[0].sxf;
      syf = mpeg2_struct->motion_data[0].syf;
      }

    MPEG2_motion_estimation(mpeg2_struct->oldorgframe[0],mpeg2_struct->neworgframe[0],
      mpeg2_struct->oldrefframe[0],mpeg2_struct->newrefframe[0],
      neworg[0],newref[0],
      sxf,syf,sxb,syb,mpeg2_struct->mbinfo,1,ipflag,mpeg2_struct);

    MPEG2_predict(mpeg2_struct->oldrefframe,mpeg2_struct->newrefframe,mpeg2_struct->predframe,1,mpeg2_struct->mbinfo,mpeg2_struct);
    MPEG2_dct_type_estimation(mpeg2_struct->predframe[0],neworg[0],mpeg2_struct->mbinfo,mpeg2_struct);
    MPEG2_transform(mpeg2_struct->predframe,neworg,mpeg2_struct->mbinfo,mpeg2_struct->blocks,mpeg2_struct);

    MPEG2_putpict(neworg[0],mpeg2_struct);

    for (k=0; k<mpeg2_struct->mb_height2*mpeg2_struct->mb_width; k++)
      {
      if (mpeg2_struct->mbinfo[k].mb_type & MB_INTRA)
        for (j=0; j<mpeg2_struct->block_count; j++)
          MPEG2_iquant_intra(mpeg2_struct->blocks[k*mpeg2_struct->block_count+j],mpeg2_struct->blocks[k*mpeg2_struct->block_count+j],
            mpeg2_struct->dc_prec,mpeg2_struct->intra_q,mpeg2_struct->mbinfo[k].mquant,mpeg2_struct);
      else
        for (j=0;j<mpeg2_struct->block_count;j++)
          MPEG2_iquant_non_intra(mpeg2_struct->blocks[k*mpeg2_struct->block_count+j],mpeg2_struct->blocks[k*mpeg2_struct->block_count+j],
            mpeg2_struct->inter_q,mpeg2_struct->mbinfo[k].mquant,mpeg2_struct);
      }

    MPEG2_itransform(mpeg2_struct->predframe,newref,mpeg2_struct->mbinfo,mpeg2_struct->blocks,mpeg2_struct);
    MPEG2_calcSNR(neworg,newref,mpeg2_struct);
    MPEG2_stats(mpeg2_struct);
    }
  else
    {
    mpeg2_struct->pict_struct = FRAME_PICTURE;

    /* do MPEG2_motion_estimation
     *
     * uses source frames (...orgframe) for full pel search
     * and reconstructed frames (...refframe) for half pel search
     */

    MPEG2_motion_estimation(mpeg2_struct->oldorgframe[0],mpeg2_struct->neworgframe[0],
      mpeg2_struct->oldrefframe[0],mpeg2_struct->newrefframe[0],
      neworg[0],newref[0],
      sxf,syf,sxb,syb,mpeg2_struct->mbinfo,0,0,mpeg2_struct);

    MPEG2_predict(mpeg2_struct->oldrefframe,mpeg2_struct->newrefframe,mpeg2_struct->predframe,0,mpeg2_struct->mbinfo,mpeg2_struct);
    MPEG2_dct_type_estimation(mpeg2_struct->predframe[0],neworg[0],mpeg2_struct->mbinfo,mpeg2_struct);
    MPEG2_transform(mpeg2_struct->predframe,neworg,mpeg2_struct->mbinfo,mpeg2_struct->blocks,mpeg2_struct);

    MPEG2_putpict(neworg[0],mpeg2_struct);

    for (k=0; k<mpeg2_struct->mb_height*mpeg2_struct->mb_width; k++)
      {
      if (mpeg2_struct->mbinfo[k].mb_type & MB_INTRA)
        for (j=0; j<mpeg2_struct->block_count; j++)
          MPEG2_iquant_intra(mpeg2_struct->blocks[k*mpeg2_struct->block_count+j],mpeg2_struct->blocks[k*mpeg2_struct->block_count+j],
            mpeg2_struct->dc_prec,mpeg2_struct->intra_q,mpeg2_struct->mbinfo[k].mquant,mpeg2_struct);
      else
        for (j=0;j<mpeg2_struct->block_count;j++)
          MPEG2_iquant_non_intra(mpeg2_struct->blocks[k*mpeg2_struct->block_count+j],mpeg2_struct->blocks[k*mpeg2_struct->block_count+j],
            mpeg2_struct->inter_q,mpeg2_struct->mbinfo[k].mquant,mpeg2_struct);
      }

    MPEG2_itransform(mpeg2_struct->predframe,newref,mpeg2_struct->mbinfo,mpeg2_struct->blocks,mpeg2_struct);
    MPEG2_calcSNR(neworg,newref,mpeg2_struct);
    MPEG2_stats(mpeg2_struct);
    }

  sprintf(name,mpeg2_struct->tplref,realframe+mpeg2_struct->frame0);
  MPEG2_writeframe(name,newref,mpeg2_struct);
  return realframe;
}
