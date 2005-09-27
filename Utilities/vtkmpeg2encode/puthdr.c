/* puthdr.c, generation of headers                                          */

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
static int frametotc _ANSI_ARGS_((int frame,struct MPEG2_structure *mpeg2_struct));

/* generate sequence header (6.2.2.1, 6.3.3)
 *
 * matrix download not implemented
 */
VTK_MPEG2ENC_EXPORT void MPEG2_putseqhdr(mpeg2_struct)
  struct MPEG2_structure *mpeg2_struct;
{
  int i;

  MPEG2_alignbits(mpeg2_struct);
  MPEG2_putbits(SEQ_START_CODE,32,mpeg2_struct); /* sequence_header_code */
  MPEG2_putbits(mpeg2_struct->horizontal_size,12,mpeg2_struct); /* horizontal_size_value */
  MPEG2_putbits(mpeg2_struct->vertical_size,12,mpeg2_struct); /* vertical_size_value */
  MPEG2_putbits(mpeg2_struct->aspectratio,4,mpeg2_struct); /* aspect_ratio_information */
  MPEG2_putbits(mpeg2_struct->frame_rate_code,4,mpeg2_struct); /* frame_rate_code */
  MPEG2_putbits((int)ceil(mpeg2_struct->bit_rate/400.0),18,mpeg2_struct); /* bit_rate_value */
  MPEG2_putbits(1,1,mpeg2_struct); /* marker_bit */
  MPEG2_putbits(mpeg2_struct->vbv_buffer_size,10,mpeg2_struct); /* vbv_buffer_size_value */
  MPEG2_putbits(mpeg2_struct->constrparms,1,mpeg2_struct); /* constrained_parameters_flag */

  MPEG2_putbits(mpeg2_struct->load_iquant,1,mpeg2_struct); /* load_intra_quantizer_matrix */
  if (mpeg2_struct->load_iquant)
    for (i=0; i<64; i++)  /* matrices are always downloaded in zig-zag order */
      MPEG2_putbits(mpeg2_struct->intra_q[MPEG2_zig_zag_scan[i]],8,mpeg2_struct); /* intra_quantizer_matrix */

  MPEG2_putbits(mpeg2_struct->load_niquant,1,mpeg2_struct); /* load_non_intra_quantizer_matrix */
  if (mpeg2_struct->load_niquant)
    for (i=0; i<64; i++)
      MPEG2_putbits(mpeg2_struct->inter_q[MPEG2_zig_zag_scan[i]],8,mpeg2_struct); /* non_intra_quantizer_matrix */
}

/* generate sequence extension (6.2.2.3, 6.3.5) header (MPEG-2 only) */
VTK_MPEG2ENC_EXPORT void MPEG2_putseqext(mpeg2_struct)
  struct MPEG2_structure *mpeg2_struct;
{
  MPEG2_alignbits(mpeg2_struct);
  MPEG2_putbits(EXT_START_CODE,32,mpeg2_struct); /* extension_start_code */
  MPEG2_putbits(SEQ_ID,4,mpeg2_struct); /* extension_start_code_identifier */
  MPEG2_putbits((mpeg2_struct->profile<<4)|mpeg2_struct->level,8,mpeg2_struct); /* profile_and_level_indication */
  MPEG2_putbits(mpeg2_struct->prog_seq,1,mpeg2_struct); /* progressive sequence */
  MPEG2_putbits(mpeg2_struct->chroma_format,2,mpeg2_struct); /* chroma_format */
  MPEG2_putbits(mpeg2_struct->horizontal_size>>12,2,mpeg2_struct); /* horizontal_size_extension */
  MPEG2_putbits(mpeg2_struct->vertical_size>>12,2,mpeg2_struct); /* vertical_size_extension */
  MPEG2_putbits(((int)ceil(mpeg2_struct->bit_rate/400.0))>>18,12,mpeg2_struct); /* bit_rate_extension */
  MPEG2_putbits(1,1,mpeg2_struct); /* marker_bit */
  MPEG2_putbits(mpeg2_struct->vbv_buffer_size>>10,8,mpeg2_struct); /* vbv_buffer_size_extension */
  MPEG2_putbits(0,1,mpeg2_struct); /* low_delay  -- currently not implemented */
  MPEG2_putbits(0,2,mpeg2_struct); /* frame_rate_extension_n */
  MPEG2_putbits(0,5,mpeg2_struct); /* frame_rate_extension_d */
}

/* generate sequence display extension (6.2.2.4, 6.3.6)
 *
 * content not yet user setable
 */
VTK_MPEG2ENC_EXPORT void MPEG2_putseqdispext(mpeg2_struct)
  struct MPEG2_structure *mpeg2_struct;
{
  MPEG2_alignbits(mpeg2_struct);
  MPEG2_putbits(EXT_START_CODE,32,mpeg2_struct); /* extension_start_code */
  MPEG2_putbits(DISP_ID,4,mpeg2_struct); /* extension_start_code_identifier */
  MPEG2_putbits(mpeg2_struct->video_format,3,mpeg2_struct); /* video_format */
  MPEG2_putbits(1,1,mpeg2_struct); /* colour_description */
  MPEG2_putbits(mpeg2_struct->color_primaries,8,mpeg2_struct); /* colour_primaries */
  MPEG2_putbits(mpeg2_struct->transfer_characteristics,8,mpeg2_struct); /* transfer_characteristics */
  MPEG2_putbits(mpeg2_struct->matrix_coefficients,8,mpeg2_struct); /* matrix_coefficients */
  MPEG2_putbits(mpeg2_struct->display_horizontal_size,14,mpeg2_struct); /* display_horizontal_size */
  MPEG2_putbits(1,1,mpeg2_struct); /* marker_bit */
  MPEG2_putbits(mpeg2_struct->display_vertical_size,14,mpeg2_struct); /* display_vertical_size */
}

/* output a zero terminated string as user data (6.2.2.2.2, 6.3.4.1)
 *
 * string must not emulate start codes
 */
VTK_MPEG2ENC_EXPORT void MPEG2_putuserdata(userdata,mpeg2_struct)
char *userdata;
struct MPEG2_structure *mpeg2_struct;
{
  MPEG2_alignbits(mpeg2_struct);
  MPEG2_putbits(USER_START_CODE,32,mpeg2_struct); /* user_data_start_code */
  while (*userdata)
    MPEG2_putbits(*userdata++,8,mpeg2_struct);
}

/* generate group of pictures header (6.2.2.6, 6.3.9)
 *
 * uses tc0 (timecode of first frame) and frame0 (number of first frame)
 */
void MPEG2_putgophdr(frame,closed_gop,mpeg2_struct)
int frame,closed_gop;
struct MPEG2_structure *mpeg2_struct;
{
  int tc;

  MPEG2_alignbits(mpeg2_struct);
  MPEG2_putbits(GOP_START_CODE,32,mpeg2_struct); /* group_start_code */
  tc = frametotc(mpeg2_struct->tc0+frame,mpeg2_struct);
  MPEG2_putbits(tc,25,mpeg2_struct); /* time_code */
  MPEG2_putbits(closed_gop,1,mpeg2_struct); /* closed_gop */
  MPEG2_putbits(0,1,mpeg2_struct); /* broken_link */
}

/* convert frame number to time_code
 *
 * drop_frame not implemented
 */
static int frametotc(frame, mpeg2_struct)
int frame;
struct MPEG2_structure *mpeg2_struct;
{
  int fps, pict, sec, minute, hour, tc;

  fps = (int)(mpeg2_struct->frame_rate+0.5);
  pict = frame%fps;
  frame = (frame-pict)/fps;
  sec = frame%60;
  frame = (frame-sec)/60;
  minute = frame%60;
  frame = (frame-minute)/60;
  hour = frame%24;
  tc = (hour<<19) | (minute<<13) | (1<<12) | (sec<<6) | pict;

  return tc;
}

/* generate picture header (6.2.3, 6.3.10) */
void MPEG2_putpicthdr(mpeg2_struct)
  struct MPEG2_structure *mpeg2_struct;
{
  MPEG2_alignbits(mpeg2_struct);
  MPEG2_putbits(PICTURE_START_CODE,32,mpeg2_struct); /* picture_start_code */
  MPEG2_calc_vbv_delay(mpeg2_struct);
  MPEG2_putbits(mpeg2_struct->temp_ref,10,mpeg2_struct); /* temporal_reference */
  MPEG2_putbits(mpeg2_struct->pict_type,3,mpeg2_struct); /* picture_coding_type */
  MPEG2_putbits(mpeg2_struct->vbv_delay,16,mpeg2_struct); /* vbv_delay */

  if (mpeg2_struct->pict_type==P_TYPE || mpeg2_struct->pict_type==B_TYPE)
  {
    MPEG2_putbits(0,1,mpeg2_struct); /* full_pel_forward_vector */
    if (mpeg2_struct->mpeg1)
      MPEG2_putbits(mpeg2_struct->forw_hor_f_code,3,mpeg2_struct);
    else
      MPEG2_putbits(7,3,mpeg2_struct); /* forward_f_code */
  }

  if (mpeg2_struct->pict_type==B_TYPE)
  {
    MPEG2_putbits(0,1,mpeg2_struct); /* full_pel_backward_vector */
    if (mpeg2_struct->mpeg1)
      MPEG2_putbits(mpeg2_struct->back_hor_f_code,3,mpeg2_struct);
    else
      MPEG2_putbits(7,3,mpeg2_struct); /* backward_f_code */
  }

  MPEG2_putbits(0,1,mpeg2_struct); /* extra_bit_picture */
}

/* generate picture coding extension (6.2.3.1, 6.3.11)
 *
 * composite display information (v_axis etc.) not implemented
 */
void MPEG2_putpictcodext(mpeg2_struct)
  struct MPEG2_structure *mpeg2_struct;
{
  MPEG2_alignbits(mpeg2_struct);
  MPEG2_putbits(EXT_START_CODE,32,mpeg2_struct); /* extension_start_code */
  MPEG2_putbits(CODING_ID,4,mpeg2_struct); /* extension_start_code_identifier */
  MPEG2_putbits(mpeg2_struct->forw_hor_f_code,4,mpeg2_struct); /* forward_horizontal_f_code */
  MPEG2_putbits(mpeg2_struct->forw_vert_f_code,4,mpeg2_struct); /* forward_vertical_f_code */
  MPEG2_putbits(mpeg2_struct->back_hor_f_code,4,mpeg2_struct); /* backward_horizontal_f_code */
  MPEG2_putbits(mpeg2_struct->back_vert_f_code,4,mpeg2_struct); /* backward_vertical_f_code */
  MPEG2_putbits(mpeg2_struct->dc_prec,2,mpeg2_struct); /* intra_dc_precision */
  MPEG2_putbits(mpeg2_struct->pict_struct,2,mpeg2_struct); /* picture_structure */
  MPEG2_putbits((mpeg2_struct->pict_struct==FRAME_PICTURE)?mpeg2_struct->topfirst:0,1,mpeg2_struct); /* top_field_first */
  MPEG2_putbits(mpeg2_struct->frame_pred_dct,1,mpeg2_struct); /* frame_pred_frame_dct */
  MPEG2_putbits(0,1,mpeg2_struct); /* concealment_motion_vectors  -- currently not implemented */
  MPEG2_putbits(mpeg2_struct->q_scale_type,1,mpeg2_struct); /* mpeg2_struct->q_scale_type */
  MPEG2_putbits(mpeg2_struct->intravlc,1,mpeg2_struct); /* intra_vlc_format */
  MPEG2_putbits(mpeg2_struct->altscan,1,mpeg2_struct); /* alternate_scan */
  MPEG2_putbits(mpeg2_struct->repeatfirst,1,mpeg2_struct); /* repeat_first_field */
  MPEG2_putbits(mpeg2_struct->prog_frame,1,mpeg2_struct); /* chroma_420_type */
  MPEG2_putbits(mpeg2_struct->prog_frame,1,mpeg2_struct); /* progressive_frame */
  MPEG2_putbits(0,1,mpeg2_struct); /* composite_display_flag */
}

/* generate sequence_end_code (6.2.2) */
VTK_MPEG2ENC_EXPORT void MPEG2_putseqend(mpeg2_struct)
  struct MPEG2_structure *mpeg2_struct;
{
  MPEG2_alignbits(mpeg2_struct);
  MPEG2_putbits(SEQ_END_CODE,32,mpeg2_struct);
}
