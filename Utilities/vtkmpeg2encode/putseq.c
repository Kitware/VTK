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
#include "config.h"
#include "global.h"

static void MPEG2_evaluate_frame_info(int cframe, int* f0, int* realframe, unsigned char* neworg[], unsigned char* newref[], int* sxf, int* syf, int* sxb, int* syb)
{
  int j, n, np, nb;

  /* f0: lowest frame number in current GOP
   *
   * first GOP contains N-(M-1) frames,
   * all other GOPs contain N frames
   */
  *f0 = vtkMPEG2WriterStr->N_val*((cframe+(vtkMPEG2WriterStr->M_val-1))/vtkMPEG2WriterStr->N_val) - (vtkMPEG2WriterStr->M_val-1);

  if (*f0<0)
    *f0=0;

  if (cframe==0 || (cframe-1)%vtkMPEG2WriterStr->M_val==0)
    {
    /* I or P frame */
    for (j=0; j<3; j++)
      {
      /* shuffle reference frames */
      neworg[j] = vtkMPEG2WriterStr->oldorgframe[j];
      newref[j] = vtkMPEG2WriterStr->oldrefframe[j];
      vtkMPEG2WriterStr->oldorgframe[j] = vtkMPEG2WriterStr->neworgframe[j];
      vtkMPEG2WriterStr->oldrefframe[j] = vtkMPEG2WriterStr->newrefframe[j];
      vtkMPEG2WriterStr->neworgframe[j] = neworg[j];
      vtkMPEG2WriterStr->newrefframe[j] = newref[j];
      }

    /* f: frame number in display order */
    *realframe = (cframe==0) ? 0 : cframe+vtkMPEG2WriterStr->M_val-1;
    if (*realframe>=vtkMPEG2WriterStr->nframes)
      *realframe = vtkMPEG2WriterStr->nframes - 1;

    if (cframe==*f0) /* first displayed frame in GOP is I */
      {
      /* I frame */
      vtkMPEG2WriterStr->pict_type = I_TYPE;
      vtkMPEG2WriterStr->forw_hor_f_code = vtkMPEG2WriterStr->forw_vert_f_code = 15;
      vtkMPEG2WriterStr->back_hor_f_code = vtkMPEG2WriterStr->back_vert_f_code = 15;

      /* n: number of frames in current GOP
       *
       * first GOP contains (M-1) less (B) frames
       */
      n = (cframe==0) ? vtkMPEG2WriterStr->N_val-(vtkMPEG2WriterStr->M_val-1) : vtkMPEG2WriterStr->N_val;

      /* last GOP may contain less frames */
      if (n > vtkMPEG2WriterStr->nframes-*f0)
        n = vtkMPEG2WriterStr->nframes-*f0;

      /* number of P frames */
      if (cframe==0)
        np = (n + 2*(vtkMPEG2WriterStr->M_val-1))/vtkMPEG2WriterStr->M_val - 1; /* first GOP */
      else
        np = (n + (vtkMPEG2WriterStr->M_val-1))/vtkMPEG2WriterStr->M_val - 1;

      /* number of B frames */
      nb = n - np - 1;

      MPEG2_rc_init_GOP(np,nb);

      MPEG2_putgophdr(*f0,cframe==0); /* set closed_GOP in first GOP only */
      }
    else
      {
      /* P frame */
      vtkMPEG2WriterStr->pict_type = P_TYPE;
      vtkMPEG2WriterStr->forw_hor_f_code = vtkMPEG2WriterStr->motion_data[0].forw_hor_f_code;
      vtkMPEG2WriterStr->forw_vert_f_code = vtkMPEG2WriterStr->motion_data[0].forw_vert_f_code;
      vtkMPEG2WriterStr->back_hor_f_code = vtkMPEG2WriterStr->back_vert_f_code = 15;
      *sxf = vtkMPEG2WriterStr->motion_data[0].sxf;
      *syf = vtkMPEG2WriterStr->motion_data[0].syf;
      }
    }
  else
    {
    /* B frame */
    for (j=0; j<3; j++)
      {
      neworg[j] = vtkMPEG2WriterStr->auxorgframe[j];
      newref[j] = vtkMPEG2WriterStr->auxframe[j];
      }

    /* f: frame number in display order */
    *realframe = cframe - 1;
    vtkMPEG2WriterStr->pict_type = B_TYPE;
    n = (cframe-2)%vtkMPEG2WriterStr->M_val + 1; /* first B: n=1, second B: n=2, ... */
    vtkMPEG2WriterStr->forw_hor_f_code = vtkMPEG2WriterStr->motion_data[n].forw_hor_f_code;
    vtkMPEG2WriterStr->forw_vert_f_code = vtkMPEG2WriterStr->motion_data[n].forw_vert_f_code;
    vtkMPEG2WriterStr->back_hor_f_code = vtkMPEG2WriterStr->motion_data[n].back_hor_f_code;
    vtkMPEG2WriterStr->back_vert_f_code = vtkMPEG2WriterStr->motion_data[n].back_vert_f_code;
    *sxf = vtkMPEG2WriterStr->motion_data[n].sxf;
    *syf = vtkMPEG2WriterStr->motion_data[n].syf;
    *sxb = vtkMPEG2WriterStr->motion_data[n].sxb;
    *syb = vtkMPEG2WriterStr->motion_data[n].syb;
    }
}

int MPEG2_putseq_one(int cframe, int max)
{
  int j, k, realframe, f0, sxf, syf, sxb, syb;
  int ipflag;
  char name[256];
  unsigned char *neworg[3], *newref[3];
  static char ipb[5] = {' ','I','P','B','D'};
  if (!vtkMPEG2WriterStr->quiet)
    {
    fprintf(stderr,"Encoding frame %d ",cframe);
    fflush(stderr);
    }

  MPEG2_evaluate_frame_info(cframe, &f0, &realframe, neworg, newref,
    &sxf, &syf, &sxb, &syb);
  if ( realframe > max )
    {
    return -1;
    }

  vtkMPEG2WriterStr->temp_ref = realframe - f0;
  vtkMPEG2WriterStr->frame_pred_dct = vtkMPEG2WriterStr->frame_pred_dct_tab[vtkMPEG2WriterStr->pict_type-1];
  vtkMPEG2WriterStr->q_scale_type = vtkMPEG2WriterStr->qscale_tab[vtkMPEG2WriterStr->pict_type-1];
  vtkMPEG2WriterStr->intravlc = vtkMPEG2WriterStr->intravlc_tab[vtkMPEG2WriterStr->pict_type-1];
  vtkMPEG2WriterStr->altscan = vtkMPEG2WriterStr->altscan_tab[vtkMPEG2WriterStr->pict_type-1];

  if ( vtkMPEG2WriterStr->statfile )
    {
    fprintf(vtkMPEG2WriterStr->statfile,"\nFrame %d (#%d in display order):\n",cframe,realframe);
    fprintf(vtkMPEG2WriterStr->statfile," picture_type=%c\n",ipb[vtkMPEG2WriterStr->pict_type]);
    fprintf(vtkMPEG2WriterStr->statfile," temporal_reference=%d\n",vtkMPEG2WriterStr->temp_ref);
    fprintf(vtkMPEG2WriterStr->statfile," frame_pred_frame_dct=%d\n",vtkMPEG2WriterStr->frame_pred_dct);
    fprintf(vtkMPEG2WriterStr->statfile," q_scale_type=%d\n",vtkMPEG2WriterStr->q_scale_type);
    fprintf(vtkMPEG2WriterStr->statfile," intra_vlc_format=%d\n",vtkMPEG2WriterStr->intravlc);
    fprintf(vtkMPEG2WriterStr->statfile," alternate_scan=%d\n",vtkMPEG2WriterStr->altscan);

    if (vtkMPEG2WriterStr->pict_type!=I_TYPE)
      {
      fprintf(vtkMPEG2WriterStr->statfile," forward search window: %d...%d / %d...%d\n",
        -sxf,sxf,-syf,syf);
      fprintf(vtkMPEG2WriterStr->statfile," forward vector range: %d...%d.5 / %d...%d.5\n",
        -(4<<vtkMPEG2WriterStr->forw_hor_f_code),(4<<vtkMPEG2WriterStr->forw_hor_f_code)-1,
        -(4<<vtkMPEG2WriterStr->forw_vert_f_code),(4<<vtkMPEG2WriterStr->forw_vert_f_code)-1);
      }

    if (vtkMPEG2WriterStr->pict_type==B_TYPE)
      {
      fprintf(vtkMPEG2WriterStr->statfile," backward search window: %d...%d / %d...%d\n",
        -sxb,sxb,-syb,syb);
      fprintf(vtkMPEG2WriterStr->statfile," backward vector range: %d...%d.5 / %d...%d.5\n",
        -(4<<vtkMPEG2WriterStr->back_hor_f_code),(4<<vtkMPEG2WriterStr->back_hor_f_code)-1,
        -(4<<vtkMPEG2WriterStr->back_vert_f_code),(4<<vtkMPEG2WriterStr->back_vert_f_code)-1);
      }
    }

  sprintf(name,vtkMPEG2WriterStr->tplorg,realframe+vtkMPEG2WriterStr->frame0);
  MPEG2_readframe(name,neworg);

  if (vtkMPEG2WriterStr->fieldpic)
    {
    if (!vtkMPEG2WriterStr->quiet)
      {
      fprintf(stderr,"\nfirst field  (%s) ",vtkMPEG2WriterStr->topfirst ? "top" : "bot");
      fflush(stderr);
      }

    vtkMPEG2WriterStr->pict_struct = vtkMPEG2WriterStr->topfirst ? TOP_FIELD : BOTTOM_FIELD;

    MPEG2_motion_estimation(vtkMPEG2WriterStr->oldorgframe[0],vtkMPEG2WriterStr->neworgframe[0],
      vtkMPEG2WriterStr->oldrefframe[0],vtkMPEG2WriterStr->newrefframe[0],
      neworg[0],newref[0],
      sxf,syf,sxb,syb,vtkMPEG2WriterStr->mbinfo,0,0);

    MPEG2_predict(vtkMPEG2WriterStr->oldrefframe,vtkMPEG2WriterStr->newrefframe,vtkMPEG2WriterStr->predframe,0,vtkMPEG2WriterStr->mbinfo);
    MPEG2_dct_type_estimation(vtkMPEG2WriterStr->predframe[0],neworg[0],vtkMPEG2WriterStr->mbinfo);
    MPEG2_transform(vtkMPEG2WriterStr->predframe,neworg,vtkMPEG2WriterStr->mbinfo,vtkMPEG2WriterStr->blocks);

    MPEG2_putpict(neworg[0]);

    for (k=0; k<vtkMPEG2WriterStr->mb_height2*vtkMPEG2WriterStr->mb_width; k++)
      {
      if (vtkMPEG2WriterStr->mbinfo[k].mb_type & MB_INTRA)
        for (j=0; j<vtkMPEG2WriterStr->block_count; j++)
          MPEG2_iquant_intra(vtkMPEG2WriterStr->blocks[k*vtkMPEG2WriterStr->block_count+j],vtkMPEG2WriterStr->blocks[k*vtkMPEG2WriterStr->block_count+j],
            vtkMPEG2WriterStr->dc_prec,vtkMPEG2WriterStr->intra_q,vtkMPEG2WriterStr->mbinfo[k].mquant);
      else
        for (j=0;j<vtkMPEG2WriterStr->block_count;j++)
          MPEG2_iquant_non_intra(vtkMPEG2WriterStr->blocks[k*vtkMPEG2WriterStr->block_count+j],vtkMPEG2WriterStr->blocks[k*vtkMPEG2WriterStr->block_count+j],
            vtkMPEG2WriterStr->inter_q,vtkMPEG2WriterStr->mbinfo[k].mquant);
      }

    MPEG2_itransform(vtkMPEG2WriterStr->predframe,newref,vtkMPEG2WriterStr->mbinfo,vtkMPEG2WriterStr->blocks);
    MPEG2_calcSNR(neworg,newref);
    MPEG2_stats();

    if (!vtkMPEG2WriterStr->quiet)
      {
      fprintf(stderr,"second field (%s) ",vtkMPEG2WriterStr->topfirst ? "bot" : "top");
      fflush(stderr);
      }

    vtkMPEG2WriterStr->pict_struct = vtkMPEG2WriterStr->topfirst ? BOTTOM_FIELD : TOP_FIELD;

    ipflag = (vtkMPEG2WriterStr->pict_type==I_TYPE);
    if (ipflag)
      {
      /* first field = I, second field = P */
      vtkMPEG2WriterStr->pict_type = P_TYPE;
      vtkMPEG2WriterStr->forw_hor_f_code = vtkMPEG2WriterStr->motion_data[0].forw_hor_f_code;
      vtkMPEG2WriterStr->forw_vert_f_code = vtkMPEG2WriterStr->motion_data[0].forw_vert_f_code;
      vtkMPEG2WriterStr->back_hor_f_code = vtkMPEG2WriterStr->back_vert_f_code = 15;
      sxf = vtkMPEG2WriterStr->motion_data[0].sxf;
      syf = vtkMPEG2WriterStr->motion_data[0].syf;
      }

    MPEG2_motion_estimation(vtkMPEG2WriterStr->oldorgframe[0],vtkMPEG2WriterStr->neworgframe[0],
      vtkMPEG2WriterStr->oldrefframe[0],vtkMPEG2WriterStr->newrefframe[0],
      neworg[0],newref[0],
      sxf,syf,sxb,syb,vtkMPEG2WriterStr->mbinfo,1,ipflag);

    MPEG2_predict(vtkMPEG2WriterStr->oldrefframe,vtkMPEG2WriterStr->newrefframe,vtkMPEG2WriterStr->predframe,1,vtkMPEG2WriterStr->mbinfo);
    MPEG2_dct_type_estimation(vtkMPEG2WriterStr->predframe[0],neworg[0],vtkMPEG2WriterStr->mbinfo);
    MPEG2_transform(vtkMPEG2WriterStr->predframe,neworg,vtkMPEG2WriterStr->mbinfo,vtkMPEG2WriterStr->blocks);

    MPEG2_putpict(neworg[0]);

    for (k=0; k<vtkMPEG2WriterStr->mb_height2*vtkMPEG2WriterStr->mb_width; k++)
      {
      if (vtkMPEG2WriterStr->mbinfo[k].mb_type & MB_INTRA)
        for (j=0; j<vtkMPEG2WriterStr->block_count; j++)
          MPEG2_iquant_intra(vtkMPEG2WriterStr->blocks[k*vtkMPEG2WriterStr->block_count+j],vtkMPEG2WriterStr->blocks[k*vtkMPEG2WriterStr->block_count+j],
            vtkMPEG2WriterStr->dc_prec,vtkMPEG2WriterStr->intra_q,vtkMPEG2WriterStr->mbinfo[k].mquant);
      else
        for (j=0;j<vtkMPEG2WriterStr->block_count;j++)
          MPEG2_iquant_non_intra(vtkMPEG2WriterStr->blocks[k*vtkMPEG2WriterStr->block_count+j],vtkMPEG2WriterStr->blocks[k*vtkMPEG2WriterStr->block_count+j],
            vtkMPEG2WriterStr->inter_q,vtkMPEG2WriterStr->mbinfo[k].mquant);
      }

    MPEG2_itransform(vtkMPEG2WriterStr->predframe,newref,vtkMPEG2WriterStr->mbinfo,vtkMPEG2WriterStr->blocks);
    MPEG2_calcSNR(neworg,newref);
    MPEG2_stats();
    }
  else
    {
    vtkMPEG2WriterStr->pict_struct = FRAME_PICTURE;

    /* do MPEG2_motion_estimation
     *
     * uses source frames (...orgframe) for full pel search
     * and reconstructed frames (...refframe) for half pel search
     */

    MPEG2_motion_estimation(vtkMPEG2WriterStr->oldorgframe[0],vtkMPEG2WriterStr->neworgframe[0],
      vtkMPEG2WriterStr->oldrefframe[0],vtkMPEG2WriterStr->newrefframe[0],
      neworg[0],newref[0],
      sxf,syf,sxb,syb,vtkMPEG2WriterStr->mbinfo,0,0);

    MPEG2_predict(vtkMPEG2WriterStr->oldrefframe,vtkMPEG2WriterStr->newrefframe,vtkMPEG2WriterStr->predframe,0,vtkMPEG2WriterStr->mbinfo);
    MPEG2_dct_type_estimation(vtkMPEG2WriterStr->predframe[0],neworg[0],vtkMPEG2WriterStr->mbinfo);
    MPEG2_transform(vtkMPEG2WriterStr->predframe,neworg,vtkMPEG2WriterStr->mbinfo,vtkMPEG2WriterStr->blocks);

    MPEG2_putpict(neworg[0]);

    for (k=0; k<vtkMPEG2WriterStr->mb_height*vtkMPEG2WriterStr->mb_width; k++)
      {
      if (vtkMPEG2WriterStr->mbinfo[k].mb_type & MB_INTRA)
        for (j=0; j<vtkMPEG2WriterStr->block_count; j++)
          MPEG2_iquant_intra(vtkMPEG2WriterStr->blocks[k*vtkMPEG2WriterStr->block_count+j],vtkMPEG2WriterStr->blocks[k*vtkMPEG2WriterStr->block_count+j],
            vtkMPEG2WriterStr->dc_prec,vtkMPEG2WriterStr->intra_q,vtkMPEG2WriterStr->mbinfo[k].mquant);
      else
        for (j=0;j<vtkMPEG2WriterStr->block_count;j++)
          MPEG2_iquant_non_intra(vtkMPEG2WriterStr->blocks[k*vtkMPEG2WriterStr->block_count+j],vtkMPEG2WriterStr->blocks[k*vtkMPEG2WriterStr->block_count+j],
            vtkMPEG2WriterStr->inter_q,vtkMPEG2WriterStr->mbinfo[k].mquant);
      }

    MPEG2_itransform(vtkMPEG2WriterStr->predframe,newref,vtkMPEG2WriterStr->mbinfo,vtkMPEG2WriterStr->blocks);
    MPEG2_calcSNR(neworg,newref);
    MPEG2_stats();
    }

  sprintf(name,vtkMPEG2WriterStr->tplref,realframe+vtkMPEG2WriterStr->frame0);
  MPEG2_writeframe(name,newref);
  return realframe;
}
