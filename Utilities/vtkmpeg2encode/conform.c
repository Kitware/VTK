/* conform.c, conformance checks                                            */

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
#include <stdlib.h>

#include "config.h"
#include "global.h"

/* check for (level independent) parameter limits */
void MPEG2_range_checks()
{
  int i;

  /* range and value checks */

  if (vtkMPEG2WriterStr->horizontal_size<1 || vtkMPEG2WriterStr->horizontal_size>16383)
    MPEG2_error("horizontal_size must be between 1 and 16383");
  if (vtkMPEG2WriterStr->mpeg1 && vtkMPEG2WriterStr->horizontal_size>4095)
    MPEG2_error("horizontal_size must be less than 4096 (MPEG-1)");
  if ((vtkMPEG2WriterStr->horizontal_size&4095)==0)
    MPEG2_error("horizontal_size must not be a multiple of 4096");
  if (vtkMPEG2WriterStr->chroma_format!=CHROMA444 && vtkMPEG2WriterStr->horizontal_size%2 != 0)
    MPEG2_error("horizontal_size must be a even (4:2:0 / 4:2:2)");

  if (vtkMPEG2WriterStr->vertical_size<1 || vtkMPEG2WriterStr->vertical_size>16383)
    MPEG2_error("vertical_size must be between 1 and 16383");
  if (vtkMPEG2WriterStr->mpeg1 && vtkMPEG2WriterStr->vertical_size>4095)
    MPEG2_error("vertical size must be less than 4096 (MPEG-1)");
  if ((vtkMPEG2WriterStr->vertical_size&4095)==0)
    MPEG2_error("vertical_size must not be a multiple of 4096");
  if (vtkMPEG2WriterStr->chroma_format==CHROMA420 && vtkMPEG2WriterStr->vertical_size%2 != 0)
    MPEG2_error("vertical_size must be a even (4:2:0)");
  if(vtkMPEG2WriterStr->fieldpic)
  {
    if (vtkMPEG2WriterStr->vertical_size%2 != 0)
      MPEG2_error("vertical_size must be a even (field pictures)");
    if (vtkMPEG2WriterStr->chroma_format==CHROMA420 && vtkMPEG2WriterStr->vertical_size%4 != 0)
      MPEG2_error("vertical_size must be a multiple of 4 (4:2:0 field pictures)");
  }

  if (vtkMPEG2WriterStr->mpeg1)
  {
    if (vtkMPEG2WriterStr->aspectratio<1 || vtkMPEG2WriterStr->aspectratio>14)
      MPEG2_error("pel_aspect_ratio must be between 1 and 14 (MPEG-1)");
  }
  else
  {
    if (vtkMPEG2WriterStr->aspectratio<1 || vtkMPEG2WriterStr->aspectratio>4)
      MPEG2_error("aspect_ratio_information must be 1, 2, 3 or 4");
  }

  if (vtkMPEG2WriterStr->frame_rate_code<1 || vtkMPEG2WriterStr->frame_rate_code>8)
    MPEG2_error("frame_rate code must be between 1 and 8");

  if (vtkMPEG2WriterStr->bit_rate<=0.0)
    MPEG2_error("bit_rate must be positive");
  if (vtkMPEG2WriterStr->bit_rate > ((1<<30)-1)*400.0)
    MPEG2_error("bit_rate must be less than 429 Gbit/s");
  if (vtkMPEG2WriterStr->mpeg1 && vtkMPEG2WriterStr->bit_rate > ((1<<18)-1)*400.0)
    MPEG2_error("bit_rate must be less than 104 Mbit/s (MPEG-1)");

  if (vtkMPEG2WriterStr->vbv_buffer_size<1 || vtkMPEG2WriterStr->vbv_buffer_size>0x3ffff)
    MPEG2_error("vbv_buffer_size must be in range 1..(2^18-1)");
  if (vtkMPEG2WriterStr->mpeg1 && vtkMPEG2WriterStr->vbv_buffer_size>=1024)
    MPEG2_error("vbv_buffer_size must be less than 1024 (MPEG-1)");

  if (vtkMPEG2WriterStr->chroma_format<CHROMA420 || vtkMPEG2WriterStr->chroma_format>CHROMA444)
    MPEG2_error("chroma_format must be in range 1...3");

  if (vtkMPEG2WriterStr->video_format<0 || vtkMPEG2WriterStr->video_format>4)
    MPEG2_error("video_format must be in range 0...4");

  if (vtkMPEG2WriterStr->color_primaries<1 || vtkMPEG2WriterStr->color_primaries>7 || vtkMPEG2WriterStr->color_primaries==3)
    MPEG2_error("color_primaries must be in range 1...2 or 4...7");

  if (vtkMPEG2WriterStr->transfer_characteristics<1 || vtkMPEG2WriterStr->transfer_characteristics>7
      || vtkMPEG2WriterStr->transfer_characteristics==3)
    MPEG2_error("transfer_characteristics must be in range 1...2 or 4...7");

  if (vtkMPEG2WriterStr->matrix_coefficients<1 || vtkMPEG2WriterStr->matrix_coefficients>7 || vtkMPEG2WriterStr->matrix_coefficients==3)
    MPEG2_error("matrix_coefficients must be in range 1...2 or 4...7");

  if (vtkMPEG2WriterStr->display_horizontal_size<0 || vtkMPEG2WriterStr->display_horizontal_size>16383)
    MPEG2_error("display_horizontal_size must be in range 0...16383");
  if (vtkMPEG2WriterStr->display_vertical_size<0 || vtkMPEG2WriterStr->display_vertical_size>16383)
    MPEG2_error("display_vertical_size must be in range 0...16383");

  if (vtkMPEG2WriterStr->dc_prec<0 || vtkMPEG2WriterStr->dc_prec>3)
    MPEG2_error("intra_dc_precision must be in range 0...3");

  for (i=0; i<vtkMPEG2WriterStr->M_val; i++)
  {
    if (vtkMPEG2WriterStr->motion_data[i].forw_hor_f_code<1 || vtkMPEG2WriterStr->motion_data[i].forw_hor_f_code>9)
      MPEG2_error("f_code must be between 1 and 9");
    if (vtkMPEG2WriterStr->motion_data[i].forw_vert_f_code<1 || vtkMPEG2WriterStr->motion_data[i].forw_vert_f_code>9)
      MPEG2_error("f_code must be between 1 and 9");
    if (vtkMPEG2WriterStr->mpeg1 && vtkMPEG2WriterStr->motion_data[i].forw_hor_f_code>7)
      MPEG2_error("f_code must be le less than 8");
    if (vtkMPEG2WriterStr->mpeg1 && vtkMPEG2WriterStr->motion_data[i].forw_vert_f_code>7)
      MPEG2_error("f_code must be le less than 8");
    if (vtkMPEG2WriterStr->motion_data[i].sxf<=0)
      MPEG2_error("search window must be positive"); /* doesn't belong here */
    if (vtkMPEG2WriterStr->motion_data[i].syf<=0)
      MPEG2_error("search window must be positive");
    if (i!=0)
    {
      if (vtkMPEG2WriterStr->motion_data[i].back_hor_f_code<1 || vtkMPEG2WriterStr->motion_data[i].back_hor_f_code>9)
        MPEG2_error("f_code must be between 1 and 9");
      if (vtkMPEG2WriterStr->motion_data[i].back_vert_f_code<1 || vtkMPEG2WriterStr->motion_data[i].back_vert_f_code>9)
        MPEG2_error("f_code must be between 1 and 9");
      if (vtkMPEG2WriterStr->mpeg1 && vtkMPEG2WriterStr->motion_data[i].back_hor_f_code>7)
        MPEG2_error("f_code must be le less than 8");
      if (vtkMPEG2WriterStr->mpeg1 && vtkMPEG2WriterStr->motion_data[i].back_vert_f_code>7)
        MPEG2_error("f_code must be le less than 8");
      if (vtkMPEG2WriterStr->motion_data[i].sxb<=0)
        MPEG2_error("search window must be positive");
      if (vtkMPEG2WriterStr->motion_data[i].syb<=0)
        MPEG2_error("search window must be positive");
    }
  }
}

/* identifies valid profile / level combinations */
static char profile_level_defined[5][4] =
{
/* HL   H-14 ML   LL  */
  {1,   1,   1,   0},  /* HP   */
  {0,   1,   0,   0},  /* Spat */
  {0,   0,   1,   1},  /* SNR  */
  {1,   1,   1,   1},  /* MP   */
  {0,   0,   1,   0}   /* SP   */
};

static struct level_limits {
  int hor_f_code;
  int vert_f_code;
  int hor_size;
  int vert_size;
  int sample_rate;
  int bit_rate; /* Mbit/s */
  int vbv_buffer_size; /* 16384 bit steps */
} maxval_tab[4] =
{
  {9, 5, 1920, 1152, 62668800, 80, 597}, /* HL */
  {9, 5, 1440, 1152, 47001600, 60, 448}, /* H-14 */
  {8, 5,  720,  576, 10368000, 15, 112}, /* ML */
  {7, 4,  352,  288,  3041280,  4,  29}  /* LL */
};

#define SP   5
#define MP   4
#define SNR  3
#define SPAT 2
#define HP   1

#define LL  10
#define ML   8
#define H14  6
#define HL   4

void MPEG2_profile_and_level_checks()
{
  int i;
  struct level_limits *maxval;

  if (vtkMPEG2WriterStr->profile<0 || vtkMPEG2WriterStr->profile>15)
    MPEG2_error("profile must be between 0 and 15");

  if (vtkMPEG2WriterStr->level<0 || vtkMPEG2WriterStr->level>15)
    MPEG2_error("level must be between 0 and 15");

  if (vtkMPEG2WriterStr->profile>=8)
  {
    if (!vtkMPEG2WriterStr->quiet)
      fprintf(stderr,"Warning: profile uses a reserved value, conformance checks skipped\n");
    return;
  }

  if (vtkMPEG2WriterStr->profile<HP || vtkMPEG2WriterStr->profile>SP)
    MPEG2_error("undefined Profile");

  if (vtkMPEG2WriterStr->profile==SNR || vtkMPEG2WriterStr->profile==SPAT)
    MPEG2_error("This encoder currently generates no scalable bitstreams");

  if (vtkMPEG2WriterStr->level<HL || vtkMPEG2WriterStr->level>LL || vtkMPEG2WriterStr->level&1)
    MPEG2_error("undefined Level");

  maxval = &maxval_tab[(vtkMPEG2WriterStr->level-4) >> 1];

  /* check profile@level combination */
  if(!profile_level_defined[vtkMPEG2WriterStr->profile-1][(vtkMPEG2WriterStr->level-4) >> 1])
    MPEG2_error("undefined profile@level combination");
  

  /* profile (syntax) constraints */

  if (vtkMPEG2WriterStr->profile==SP && vtkMPEG2WriterStr->M_val!=1)
    MPEG2_error("Simple Profile does not allow B pictures");

  if (vtkMPEG2WriterStr->profile!=HP && vtkMPEG2WriterStr->chroma_format!=CHROMA420)
    MPEG2_error("chroma format must be 4:2:0 in specified Profile");

  if (vtkMPEG2WriterStr->profile==HP && vtkMPEG2WriterStr->chroma_format==CHROMA444)
    MPEG2_error("chroma format must be 4:2:0 or 4:2:2 in High Profile");

  if (vtkMPEG2WriterStr->profile>=MP) /* SP, MP: constrained repeat_first_field */
  {
    if (vtkMPEG2WriterStr->frame_rate_code<=2 && vtkMPEG2WriterStr->repeatfirst)
      MPEG2_error("repeat_first_first must be zero");
    if (vtkMPEG2WriterStr->frame_rate_code<=6 && vtkMPEG2WriterStr->prog_seq && vtkMPEG2WriterStr->repeatfirst)
      MPEG2_error("repeat_first_first must be zero");
  }

  if (vtkMPEG2WriterStr->profile!=HP && vtkMPEG2WriterStr->dc_prec==3)
    MPEG2_error("11 bit DC precision only allowed in High Profile");


  /* level (parameter value) constraints */

  /* Table 8-8 */
  if (vtkMPEG2WriterStr->frame_rate_code>5 && vtkMPEG2WriterStr->level>=ML)
    MPEG2_error("Picture rate greater than permitted in specified Level");

  for (i=0; i<vtkMPEG2WriterStr->M_val; i++)
  {
    if (vtkMPEG2WriterStr->motion_data[i].forw_hor_f_code > maxval->hor_f_code)
      MPEG2_error("forward horizontal f_code greater than permitted in specified Level");

    if (vtkMPEG2WriterStr->motion_data[i].forw_vert_f_code > maxval->vert_f_code)
      MPEG2_error("forward vertical f_code greater than permitted in specified Level");

    if (i!=0)
    {
      if (vtkMPEG2WriterStr->motion_data[i].back_hor_f_code > maxval->hor_f_code)
        MPEG2_error("backward horizontal f_code greater than permitted in specified Level");
  
      if (vtkMPEG2WriterStr->motion_data[i].back_vert_f_code > maxval->vert_f_code)
        MPEG2_error("backward vertical f_code greater than permitted in specified Level");
    }
  }

  /* Table 8-10 */
  if (vtkMPEG2WriterStr->horizontal_size > maxval->hor_size)
    MPEG2_error("Horizontal size is greater than permitted in specified Level");

  if (vtkMPEG2WriterStr->vertical_size > maxval->vert_size)
    MPEG2_error("Horizontal size is greater than permitted in specified Level");

  /* Table 8-11 */
  if (vtkMPEG2WriterStr->horizontal_size*vtkMPEG2WriterStr->vertical_size*vtkMPEG2WriterStr->frame_rate > maxval->sample_rate)
    MPEG2_error("Sample rate is greater than permitted in specified Level");

  /* Table 8-12 */
  if (vtkMPEG2WriterStr->bit_rate> 1.0e6 * maxval->bit_rate)
    MPEG2_error("Bit rate is greater than permitted in specified Level");

  /* Table 8-13 */
  if (vtkMPEG2WriterStr->vbv_buffer_size > maxval->vbv_buffer_size)
    MPEG2_error("vbv_buffer_size exceeds High Level limit");
}
