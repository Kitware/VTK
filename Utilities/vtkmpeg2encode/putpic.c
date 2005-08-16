/* putpic.c, block and motion vector encoding routines                      */

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

/* private prototypes */
static void MPEG2_putmvs _ANSI_ARGS_((int MV[2][2][2], int PMV[2][2][2],
  int mv_field_sel[2][2], int dmvector[2], int s, int motion_type,
  int hor_f_code, int vert_f_code));

/* quantization / variable length encoding of a complete picture */
void MPEG2_putpict(frame)
unsigned char *frame;
{
  int i, j, k, comp, cc;
  int mb_type;
  int PMV[2][2][2];
  int prev_mquant;
  int cbp, MBAinc;

  MPEG2_rc_init_pict(frame); /* set up rate control */

  /* picture header and picture coding extension */
  MPEG2_putpicthdr();

  if (!vtkMPEG2WriterStr->mpeg1)
    MPEG2_putpictcodext();

  prev_mquant = MPEG2_rc_start_mb(); /* initialize quantization parameter */

  k = 0;

  for (j=0; j<vtkMPEG2WriterStr->mb_height2; j++)
  {
    /* macroblock row loop */

    for (i=0; i<vtkMPEG2WriterStr->mb_width; i++)
    {
      /* macroblock loop */
      if (i==0)
      {
        /* slice header (6.2.4) */
        MPEG2_alignbits();

        if (vtkMPEG2WriterStr->mpeg1 || vtkMPEG2WriterStr->vertical_size<=2800)
          MPEG2_putbits(SLICE_MIN_START+j,32); /* slice_start_code */
        else
        {
          MPEG2_putbits(SLICE_MIN_START+(j&127),32); /* slice_start_code */
          MPEG2_putbits(j>>7,3); /* slice_vertical_position_extension */
        }
  
        /* quantiser_scale_code */
        MPEG2_putbits(vtkMPEG2WriterStr->q_scale_type ? MPEG2_map_non_linear_mquant[prev_mquant]
                             : prev_mquant >> 1, 5);
  
        MPEG2_putbits(0,1); /* extra_bit_slice */
  
        /* reset predictors */

        for (cc=0; cc<3; cc++)
          vtkMPEG2WriterStr->dc_dct_pred[cc] = 0;

        PMV[0][0][0]=PMV[0][0][1]=PMV[1][0][0]=PMV[1][0][1]=0;
        PMV[0][1][0]=PMV[0][1][1]=PMV[1][1][0]=PMV[1][1][1]=0;
  
        MBAinc = i + 1; /* first MBAinc denotes absolute position */
      }

      mb_type = vtkMPEG2WriterStr->mbinfo[k].mb_type;

      /* determine mquant (rate control) */
      vtkMPEG2WriterStr->mbinfo[k].mquant = MPEG2_rc_calc_mquant(k);

      /* quantize macroblock */
      if (mb_type & MB_INTRA)
      {
        for (comp=0; comp<vtkMPEG2WriterStr->block_count; comp++)
          MPEG2_quant_intra(vtkMPEG2WriterStr->blocks[k*vtkMPEG2WriterStr->block_count+comp],vtkMPEG2WriterStr->blocks[k*vtkMPEG2WriterStr->block_count+comp],
                      vtkMPEG2WriterStr->dc_prec,vtkMPEG2WriterStr->intra_q,vtkMPEG2WriterStr->mbinfo[k].mquant);
        vtkMPEG2WriterStr->mbinfo[k].cbp = cbp = (1<<vtkMPEG2WriterStr->block_count) - 1;
      }
      else
      {
        cbp = 0;
        for (comp=0;comp<vtkMPEG2WriterStr->block_count;comp++)
          cbp = (cbp<<1) | MPEG2_quant_non_intra(vtkMPEG2WriterStr->blocks[k*vtkMPEG2WriterStr->block_count+comp],
                                           vtkMPEG2WriterStr->blocks[k*vtkMPEG2WriterStr->block_count+comp],
                                           vtkMPEG2WriterStr->inter_q,vtkMPEG2WriterStr->mbinfo[k].mquant);

        vtkMPEG2WriterStr->mbinfo[k].cbp = cbp;

        if (cbp)
          mb_type|= MB_PATTERN;
      }

      /* output mquant if it has changed */
      if (cbp && prev_mquant!=vtkMPEG2WriterStr->mbinfo[k].mquant)
        mb_type|= MB_QUANT;

      /* check if macroblock can be skipped */
      if (i!=0 && i!=vtkMPEG2WriterStr->mb_width-1 && !cbp)
      {
        /* no DCT coefficients and neither first nor last macroblock of slice */

        if (vtkMPEG2WriterStr->pict_type==P_TYPE && !(mb_type&MB_FORWARD))
        {
          /* P picture, no motion vectors -> skip */

          /* reset predictors */

          for (cc=0; cc<3; cc++)
            vtkMPEG2WriterStr->dc_dct_pred[cc] = 0;

          PMV[0][0][0]=PMV[0][0][1]=PMV[1][0][0]=PMV[1][0][1]=0;
          PMV[0][1][0]=PMV[0][1][1]=PMV[1][1][0]=PMV[1][1][1]=0;

          vtkMPEG2WriterStr->mbinfo[k].mb_type = mb_type;
          vtkMPEG2WriterStr->mbinfo[k].skipped = 1;
          MBAinc++;
          k++;
          continue;
        }

        if (vtkMPEG2WriterStr->pict_type==B_TYPE && vtkMPEG2WriterStr->pict_struct==FRAME_PICTURE
            && vtkMPEG2WriterStr->mbinfo[k].motion_type==MC_FRAME
            && ((vtkMPEG2WriterStr->mbinfo[k-1].mb_type^mb_type)&(MB_FORWARD|MB_BACKWARD))==0
            && (!(mb_type&MB_FORWARD) ||
                (PMV[0][0][0]==vtkMPEG2WriterStr->mbinfo[k].MV[0][0][0] &&
                 PMV[0][0][1]==vtkMPEG2WriterStr->mbinfo[k].MV[0][0][1]))
            && (!(mb_type&MB_BACKWARD) ||
                (PMV[0][1][0]==vtkMPEG2WriterStr->mbinfo[k].MV[0][1][0] &&
                 PMV[0][1][1]==vtkMPEG2WriterStr->mbinfo[k].MV[0][1][1])))
        {
          /* conditions for skipping in B frame pictures:
           * - must be frame predicted
           * - must be the same prediction type (forward/backward/interp.)
           *   as previous macroblock
           * - relevant vectors (forward/backward/both) have to be the same
           *   as in previous macroblock
           */

          vtkMPEG2WriterStr->mbinfo[k].mb_type = mb_type;
          vtkMPEG2WriterStr->mbinfo[k].skipped = 1;
          MBAinc++;
          k++;
          continue;
        }

        if (vtkMPEG2WriterStr->pict_type==B_TYPE && vtkMPEG2WriterStr->pict_struct!=FRAME_PICTURE
            && vtkMPEG2WriterStr->mbinfo[k].motion_type==MC_FIELD
            && ((vtkMPEG2WriterStr->mbinfo[k-1].mb_type^mb_type)&(MB_FORWARD|MB_BACKWARD))==0
            && (!(mb_type&MB_FORWARD) ||
                (PMV[0][0][0]==vtkMPEG2WriterStr->mbinfo[k].MV[0][0][0] &&
                 PMV[0][0][1]==vtkMPEG2WriterStr->mbinfo[k].MV[0][0][1] &&
                 vtkMPEG2WriterStr->mbinfo[k].mv_field_sel[0][0]==(vtkMPEG2WriterStr->pict_struct==BOTTOM_FIELD)))
            && (!(mb_type&MB_BACKWARD) ||
                (PMV[0][1][0]==vtkMPEG2WriterStr->mbinfo[k].MV[0][1][0] &&
                 PMV[0][1][1]==vtkMPEG2WriterStr->mbinfo[k].MV[0][1][1] &&
                 vtkMPEG2WriterStr->mbinfo[k].mv_field_sel[0][1]==(vtkMPEG2WriterStr->pict_struct==BOTTOM_FIELD))))
        {
          /* conditions for skipping in B field pictures:
           * - must be field predicted
           * - must be the same prediction type (forward/backward/interp.)
           *   as previous macroblock
           * - relevant vectors (forward/backward/both) have to be the same
           *   as in previous macroblock
           * - relevant motion_vertical_field_selects have to be of same
           *   parity as current field
           */

          vtkMPEG2WriterStr->mbinfo[k].mb_type = mb_type;
          vtkMPEG2WriterStr->mbinfo[k].skipped = 1;
          MBAinc++;
          k++;
          continue;
        }
      }

      /* macroblock cannot be skipped */
      vtkMPEG2WriterStr->mbinfo[k].skipped = 0;

      /* there's no VLC for 'No MC, Not Coded':
       * we have to transmit (0,0) motion vectors
       */
      if (vtkMPEG2WriterStr->pict_type==P_TYPE && !cbp && !(mb_type&MB_FORWARD))
        mb_type|= MB_FORWARD;

      MPEG2_putaddrinc(MBAinc); /* macroblock_address_increment */
      MBAinc = 1;

      MPEG2_putmbtype(vtkMPEG2WriterStr->pict_type,mb_type); /* macroblock type */

      if (mb_type & (MB_FORWARD|MB_BACKWARD) && !vtkMPEG2WriterStr->frame_pred_dct)
        MPEG2_putbits(vtkMPEG2WriterStr->mbinfo[k].motion_type,2);

      if (vtkMPEG2WriterStr->pict_struct==FRAME_PICTURE && cbp && !vtkMPEG2WriterStr->frame_pred_dct)
        MPEG2_putbits(vtkMPEG2WriterStr->mbinfo[k].dct_type,1);

      if (mb_type & MB_QUANT)
      {
        MPEG2_putbits(vtkMPEG2WriterStr->q_scale_type ? MPEG2_map_non_linear_mquant[vtkMPEG2WriterStr->mbinfo[k].mquant]
                             : vtkMPEG2WriterStr->mbinfo[k].mquant>>1,5);
        prev_mquant = vtkMPEG2WriterStr->mbinfo[k].mquant;
      }

      if (mb_type & MB_FORWARD)
      {
        /* forward motion vectors, update predictors */
        MPEG2_putmvs(vtkMPEG2WriterStr->mbinfo[k].MV,PMV,vtkMPEG2WriterStr->mbinfo[k].mv_field_sel,vtkMPEG2WriterStr->mbinfo[k].dmvector,0,
          vtkMPEG2WriterStr->mbinfo[k].motion_type,vtkMPEG2WriterStr->forw_hor_f_code,vtkMPEG2WriterStr->forw_vert_f_code);
      }

      if (mb_type & MB_BACKWARD)
      {
        /* backward motion vectors, update predictors */
        MPEG2_putmvs(vtkMPEG2WriterStr->mbinfo[k].MV,PMV,vtkMPEG2WriterStr->mbinfo[k].mv_field_sel,vtkMPEG2WriterStr->mbinfo[k].dmvector,1,
          vtkMPEG2WriterStr->mbinfo[k].motion_type,vtkMPEG2WriterStr->back_hor_f_code,vtkMPEG2WriterStr->back_vert_f_code);
      }

      if (mb_type & MB_PATTERN)
      {
        MPEG2_putcbp((cbp >> (vtkMPEG2WriterStr->block_count-6)) & 63);
        if (vtkMPEG2WriterStr->chroma_format!=CHROMA420)
          MPEG2_putbits(cbp,vtkMPEG2WriterStr->block_count-6);
      }

      for (comp=0; comp<vtkMPEG2WriterStr->block_count; comp++)
      {
        /* block loop */
        if (cbp & (1<<(vtkMPEG2WriterStr->block_count-1-comp)))
        {
          if (mb_type & MB_INTRA)
          {
            cc = (comp<4) ? 0 : (comp&1)+1;
            MPEG2_putintrablk(vtkMPEG2WriterStr->blocks[k*vtkMPEG2WriterStr->block_count+comp],cc);
          }
          else
            MPEG2_putnonintrablk(vtkMPEG2WriterStr->blocks[k*vtkMPEG2WriterStr->block_count+comp]);
        }
      }

      /* reset predictors */
      if (!(mb_type & MB_INTRA))
        for (cc=0; cc<3; cc++)
          vtkMPEG2WriterStr->dc_dct_pred[cc] = 0;

      if (mb_type & MB_INTRA || (vtkMPEG2WriterStr->pict_type==P_TYPE && !(mb_type & MB_FORWARD)))
      {
        PMV[0][0][0]=PMV[0][0][1]=PMV[1][0][0]=PMV[1][0][1]=0;
        PMV[0][1][0]=PMV[0][1][1]=PMV[1][1][0]=PMV[1][1][1]=0;
      }

      vtkMPEG2WriterStr->mbinfo[k].mb_type = mb_type;
      k++;
    }
  }

  MPEG2_rc_update_pict();
  MPEG2_vbv_end_of_picture();
}


/* output motion vectors (6.2.5.2, 6.3.16.2)
 *
 * this routine also updates the predictions for motion vectors (PMV)
 */
 
static void MPEG2_putmvs(MV,PMV,mv_field_sel,dmvector,s,motion_type,
  hor_f_code,vert_f_code)
int MV[2][2][2],PMV[2][2][2];
int mv_field_sel[2][2];
int dmvector[2];
int s,motion_type,hor_f_code,vert_f_code;
{
  if (vtkMPEG2WriterStr->pict_struct==FRAME_PICTURE)
  {
    if (motion_type==MC_FRAME)
    {
      /* frame prediction */
      MPEG2_putmv(MV[0][s][0]-PMV[0][s][0],hor_f_code);
      MPEG2_putmv(MV[0][s][1]-PMV[0][s][1],vert_f_code);
      PMV[0][s][0]=PMV[1][s][0]=MV[0][s][0];
      PMV[0][s][1]=PMV[1][s][1]=MV[0][s][1];
    }
    else if (motion_type==MC_FIELD)
    {
      /* field prediction */
      MPEG2_putbits(mv_field_sel[0][s],1);
      MPEG2_putmv(MV[0][s][0]-PMV[0][s][0],hor_f_code);
      MPEG2_putmv((MV[0][s][1]>>1)-(PMV[0][s][1]>>1),vert_f_code);
      MPEG2_putbits(mv_field_sel[1][s],1);
      MPEG2_putmv(MV[1][s][0]-PMV[1][s][0],hor_f_code);
      MPEG2_putmv((MV[1][s][1]>>1)-(PMV[1][s][1]>>1),vert_f_code);
      PMV[0][s][0]=MV[0][s][0];
      PMV[0][s][1]=MV[0][s][1];
      PMV[1][s][0]=MV[1][s][0];
      PMV[1][s][1]=MV[1][s][1];
    }
    else
    {
      /* dual prime prediction */
      MPEG2_putmv(MV[0][s][0]-PMV[0][s][0],hor_f_code);
      MPEG2_putdmv(dmvector[0]);
      MPEG2_putmv((MV[0][s][1]>>1)-(PMV[0][s][1]>>1),vert_f_code);
      MPEG2_putdmv(dmvector[1]);
      PMV[0][s][0]=PMV[1][s][0]=MV[0][s][0];
      PMV[0][s][1]=PMV[1][s][1]=MV[0][s][1];
    }
  }
  else
  {
    /* field picture */
    if (motion_type==MC_FIELD)
    {
      /* field prediction */
      MPEG2_putbits(mv_field_sel[0][s],1);
      MPEG2_putmv(MV[0][s][0]-PMV[0][s][0],hor_f_code);
      MPEG2_putmv(MV[0][s][1]-PMV[0][s][1],vert_f_code);
      PMV[0][s][0]=PMV[1][s][0]=MV[0][s][0];
      PMV[0][s][1]=PMV[1][s][1]=MV[0][s][1];
    }
    else if (motion_type==MC_16X8)
    {
      /* 16x8 prediction */
      MPEG2_putbits(mv_field_sel[0][s],1);
      MPEG2_putmv(MV[0][s][0]-PMV[0][s][0],hor_f_code);
      MPEG2_putmv(MV[0][s][1]-PMV[0][s][1],vert_f_code);
      MPEG2_putbits(mv_field_sel[1][s],1);
      MPEG2_putmv(MV[1][s][0]-PMV[1][s][0],hor_f_code);
      MPEG2_putmv(MV[1][s][1]-PMV[1][s][1],vert_f_code);
      PMV[0][s][0]=MV[0][s][0];
      PMV[0][s][1]=MV[0][s][1];
      PMV[1][s][0]=MV[1][s][0];
      PMV[1][s][1]=MV[1][s][1];
    }
    else
    {
      /* dual prime prediction */
      MPEG2_putmv(MV[0][s][0]-PMV[0][s][0],hor_f_code);
      MPEG2_putdmv(dmvector[0]);
      MPEG2_putmv(MV[0][s][1]-PMV[0][s][1],vert_f_code);
      MPEG2_putdmv(dmvector[1]);
      PMV[0][s][0]=PMV[1][s][0]=MV[0][s][0];
      PMV[0][s][1]=PMV[1][s][1]=MV[0][s][1];
    }
  }
}
