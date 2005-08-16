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
#include "config.h"
#include "global.h"

/* private prototypes */
static int frametotc _ANSI_ARGS_((int frame));

/* generate sequence header (6.2.2.1, 6.3.3)
 *
 * matrix download not implemented
 */
void MPEG2_putseqhdr()
{
  int i;

  MPEG2_alignbits();
  MPEG2_putbits(SEQ_START_CODE,32); /* sequence_header_code */
  MPEG2_putbits(vtkMPEG2WriterStr->horizontal_size,12); /* horizontal_size_value */
  MPEG2_putbits(vtkMPEG2WriterStr->vertical_size,12); /* vertical_size_value */
  MPEG2_putbits(vtkMPEG2WriterStr->aspectratio,4); /* aspect_ratio_information */
  MPEG2_putbits(vtkMPEG2WriterStr->frame_rate_code,4); /* frame_rate_code */
  MPEG2_putbits((int)ceil(vtkMPEG2WriterStr->bit_rate/400.0),18); /* bit_rate_value */
  MPEG2_putbits(1,1); /* marker_bit */
  MPEG2_putbits(vtkMPEG2WriterStr->vbv_buffer_size,10); /* vbv_buffer_size_value */
  MPEG2_putbits(vtkMPEG2WriterStr->constrparms,1); /* constrained_parameters_flag */

  MPEG2_putbits(vtkMPEG2WriterStr->load_iquant,1); /* load_intra_quantizer_matrix */
  if (vtkMPEG2WriterStr->load_iquant)
    for (i=0; i<64; i++)  /* matrices are always downloaded in zig-zag order */
      MPEG2_putbits(vtkMPEG2WriterStr->intra_q[MPEG2_zig_zag_scan[i]],8); /* intra_quantizer_matrix */

  MPEG2_putbits(vtkMPEG2WriterStr->load_niquant,1); /* load_non_intra_quantizer_matrix */
  if (vtkMPEG2WriterStr->load_niquant)
    for (i=0; i<64; i++)
      MPEG2_putbits(vtkMPEG2WriterStr->inter_q[MPEG2_zig_zag_scan[i]],8); /* non_intra_quantizer_matrix */
}

/* generate sequence extension (6.2.2.3, 6.3.5) header (MPEG-2 only) */
void MPEG2_putseqext()
{
  MPEG2_alignbits();
  MPEG2_putbits(EXT_START_CODE,32); /* extension_start_code */
  MPEG2_putbits(SEQ_ID,4); /* extension_start_code_identifier */
  MPEG2_putbits((vtkMPEG2WriterStr->profile<<4)|vtkMPEG2WriterStr->level,8); /* profile_and_level_indication */
  MPEG2_putbits(vtkMPEG2WriterStr->prog_seq,1); /* progressive sequence */
  MPEG2_putbits(vtkMPEG2WriterStr->chroma_format,2); /* chroma_format */
  MPEG2_putbits(vtkMPEG2WriterStr->horizontal_size>>12,2); /* horizontal_size_extension */
  MPEG2_putbits(vtkMPEG2WriterStr->vertical_size>>12,2); /* vertical_size_extension */
  MPEG2_putbits(((int)ceil(vtkMPEG2WriterStr->bit_rate/400.0))>>18,12); /* bit_rate_extension */
  MPEG2_putbits(1,1); /* marker_bit */
  MPEG2_putbits(vtkMPEG2WriterStr->vbv_buffer_size>>10,8); /* vbv_buffer_size_extension */
  MPEG2_putbits(0,1); /* low_delay  -- currently not implemented */
  MPEG2_putbits(0,2); /* frame_rate_extension_n */
  MPEG2_putbits(0,5); /* frame_rate_extension_d */
}

/* generate sequence display extension (6.2.2.4, 6.3.6)
 *
 * content not yet user setable
 */
void MPEG2_putseqdispext()
{
  MPEG2_alignbits();
  MPEG2_putbits(EXT_START_CODE,32); /* extension_start_code */
  MPEG2_putbits(DISP_ID,4); /* extension_start_code_identifier */
  MPEG2_putbits(vtkMPEG2WriterStr->video_format,3); /* video_format */
  MPEG2_putbits(1,1); /* colour_description */
  MPEG2_putbits(vtkMPEG2WriterStr->color_primaries,8); /* colour_primaries */
  MPEG2_putbits(vtkMPEG2WriterStr->transfer_characteristics,8); /* transfer_characteristics */
  MPEG2_putbits(vtkMPEG2WriterStr->matrix_coefficients,8); /* matrix_coefficients */
  MPEG2_putbits(vtkMPEG2WriterStr->display_horizontal_size,14); /* display_horizontal_size */
  MPEG2_putbits(1,1); /* marker_bit */
  MPEG2_putbits(vtkMPEG2WriterStr->display_vertical_size,14); /* display_vertical_size */
}

/* output a zero terminated string as user data (6.2.2.2.2, 6.3.4.1)
 *
 * string must not emulate start codes
 */
void MPEG2_putuserdata(userdata)
char *userdata;
{
  MPEG2_alignbits();
  MPEG2_putbits(USER_START_CODE,32); /* user_data_start_code */
  while (*userdata)
    MPEG2_putbits(*userdata++,8);
}

/* generate group of pictures header (6.2.2.6, 6.3.9)
 *
 * uses tc0 (timecode of first frame) and frame0 (number of first frame)
 */
void MPEG2_putgophdr(frame,closed_gop)
int frame,closed_gop;
{
  int tc;

  MPEG2_alignbits();
  MPEG2_putbits(GOP_START_CODE,32); /* group_start_code */
  tc = frametotc(vtkMPEG2WriterStr->tc0+frame);
  MPEG2_putbits(tc,25); /* time_code */
  MPEG2_putbits(closed_gop,1); /* closed_gop */
  MPEG2_putbits(0,1); /* broken_link */
}

/* convert frame number to time_code
 *
 * drop_frame not implemented
 */
static int frametotc(frame)
int frame;
{
  int fps, pict, sec, minute, hour, tc;

  fps = (int)(vtkMPEG2WriterStr->frame_rate+0.5);
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
void MPEG2_putpicthdr()
{
  MPEG2_alignbits();
  MPEG2_putbits(PICTURE_START_CODE,32); /* picture_start_code */
  MPEG2_calc_vbv_delay();
  MPEG2_putbits(vtkMPEG2WriterStr->temp_ref,10); /* temporal_reference */
  MPEG2_putbits(vtkMPEG2WriterStr->pict_type,3); /* picture_coding_type */
  MPEG2_putbits(vtkMPEG2WriterStr->vbv_delay,16); /* vbv_delay */

  if (vtkMPEG2WriterStr->pict_type==P_TYPE || vtkMPEG2WriterStr->pict_type==B_TYPE)
  {
    MPEG2_putbits(0,1); /* full_pel_forward_vector */
    if (vtkMPEG2WriterStr->mpeg1)
      MPEG2_putbits(vtkMPEG2WriterStr->forw_hor_f_code,3);
    else
      MPEG2_putbits(7,3); /* forward_f_code */
  }

  if (vtkMPEG2WriterStr->pict_type==B_TYPE)
  {
    MPEG2_putbits(0,1); /* full_pel_backward_vector */
    if (vtkMPEG2WriterStr->mpeg1)
      MPEG2_putbits(vtkMPEG2WriterStr->back_hor_f_code,3);
    else
      MPEG2_putbits(7,3); /* backward_f_code */
  }

  MPEG2_putbits(0,1); /* extra_bit_picture */
}

/* generate picture coding extension (6.2.3.1, 6.3.11)
 *
 * composite display information (v_axis etc.) not implemented
 */
void MPEG2_putpictcodext()
{
  MPEG2_alignbits();
  MPEG2_putbits(EXT_START_CODE,32); /* extension_start_code */
  MPEG2_putbits(CODING_ID,4); /* extension_start_code_identifier */
  MPEG2_putbits(vtkMPEG2WriterStr->forw_hor_f_code,4); /* forward_horizontal_f_code */
  MPEG2_putbits(vtkMPEG2WriterStr->forw_vert_f_code,4); /* forward_vertical_f_code */
  MPEG2_putbits(vtkMPEG2WriterStr->back_hor_f_code,4); /* backward_horizontal_f_code */
  MPEG2_putbits(vtkMPEG2WriterStr->back_vert_f_code,4); /* backward_vertical_f_code */
  MPEG2_putbits(vtkMPEG2WriterStr->dc_prec,2); /* intra_dc_precision */
  MPEG2_putbits(vtkMPEG2WriterStr->pict_struct,2); /* picture_structure */
  MPEG2_putbits((vtkMPEG2WriterStr->pict_struct==FRAME_PICTURE)?vtkMPEG2WriterStr->topfirst:0,1); /* top_field_first */
  MPEG2_putbits(vtkMPEG2WriterStr->frame_pred_dct,1); /* frame_pred_frame_dct */
  MPEG2_putbits(0,1); /* concealment_motion_vectors  -- currently not implemented */
  MPEG2_putbits(vtkMPEG2WriterStr->q_scale_type,1); /* vtkMPEG2WriterStr->q_scale_type */
  MPEG2_putbits(vtkMPEG2WriterStr->intravlc,1); /* intra_vlc_format */
  MPEG2_putbits(vtkMPEG2WriterStr->altscan,1); /* alternate_scan */
  MPEG2_putbits(vtkMPEG2WriterStr->repeatfirst,1); /* repeat_first_field */
  MPEG2_putbits(vtkMPEG2WriterStr->prog_frame,1); /* chroma_420_type */
  MPEG2_putbits(vtkMPEG2WriterStr->prog_frame,1); /* progressive_frame */
  MPEG2_putbits(0,1); /* composite_display_flag */
}

/* generate sequence_end_code (6.2.2) */
void MPEG2_putseqend()
{
  MPEG2_alignbits();
  MPEG2_putbits(SEQ_END_CODE,32);
}
