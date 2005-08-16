/* ratectl.c, bitrate control routines (linear quantization only currently) */

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
static void calc_actj _ANSI_ARGS_((unsigned char *frame));
static double var_sblk _ANSI_ARGS_((unsigned char *p, int lx));

/* rate control variables */

void MPEG2_rc_init_seq()
{
  /* reaction parameter (constant) */
  if (vtkMPEG2WriterStr->reaction==0)  vtkMPEG2WriterStr->reaction= (int)floor(2.0*vtkMPEG2WriterStr->bit_rate/vtkMPEG2WriterStr->frame_rate + 0.5);

  /* average activity */
  if (vtkMPEG2WriterStr->avg_act==0.0)  vtkMPEG2WriterStr->avg_act = 400.0;

  /* remaining # of bits in GOP */
  vtkMPEG2WriterStr->R_val = 0;

  /* global complexity measure */
  if (vtkMPEG2WriterStr->Xi==0) vtkMPEG2WriterStr->Xi = (int)floor(160.0*vtkMPEG2WriterStr->bit_rate/115.0 + 0.5);
  if (vtkMPEG2WriterStr->Xp==0) vtkMPEG2WriterStr->Xp = (int)floor( 60.0*vtkMPEG2WriterStr->bit_rate/115.0 + 0.5);
  if (vtkMPEG2WriterStr->Xb==0) vtkMPEG2WriterStr->Xb = (int)floor( 42.0*vtkMPEG2WriterStr->bit_rate/115.0 + 0.5);

  /* virtual buffer fullness */
  if (vtkMPEG2WriterStr->d0i==0) vtkMPEG2WriterStr->d0i = (int)floor(10.0*vtkMPEG2WriterStr->reaction/31.0 + 0.5);
  if (vtkMPEG2WriterStr->d0p==0) vtkMPEG2WriterStr->d0p = (int)floor(10.0*vtkMPEG2WriterStr->reaction/31.0 + 0.5);
  if (vtkMPEG2WriterStr->d0b==0) vtkMPEG2WriterStr->d0b = (int)floor(1.4*10.0*vtkMPEG2WriterStr->reaction/31.0 + 0.5);
/*
  if (vtkMPEG2WriterStr->d0i==0) vtkMPEG2WriterStr->d0i = (int)floor(10.0*r/(vtkMPEG2WriterStr->qscale_tab[0] ? 56.0 : 31.0) + 0.5);
  if (vtkMPEG2WriterStr->d0p==0) vtkMPEG2WriterStr->d0p = (int)floor(10.0*r/(vtkMPEG2WriterStr->qscale_tab[1] ? 56.0 : 31.0) + 0.5);
  if (vtkMPEG2WriterStr->d0b==0) vtkMPEG2WriterStr->d0b = (int)floor(1.4*10.0*r/(vtkMPEG2WriterStr->qscale_tab[2] ? 56.0 : 31.0) + 0.5);
*/

  if ( vtkMPEG2WriterStr->statfile )
    {
    fprintf(vtkMPEG2WriterStr->statfile,"\nrate control: sequence initialization\n");
    fprintf(vtkMPEG2WriterStr->statfile,
      " initial global complexity measures (I,P,B): Xi=%d, Xp=%d, Xb=%d\n",
      vtkMPEG2WriterStr->Xi, vtkMPEG2WriterStr->Xp, vtkMPEG2WriterStr->Xb);
    fprintf(vtkMPEG2WriterStr->statfile," reaction parameter: r=%d\n", vtkMPEG2WriterStr->reaction);
    fprintf(vtkMPEG2WriterStr->statfile,
      " initial virtual buffer fullness (I,P,B): d0i=%d, d0p=%d, d0b=%d\n",
      vtkMPEG2WriterStr->d0i, vtkMPEG2WriterStr->d0p, vtkMPEG2WriterStr->d0b);
    fprintf(vtkMPEG2WriterStr->statfile," initial average activity: avg_act=%.1f\n", vtkMPEG2WriterStr->avg_act);
    }
}

void MPEG2_rc_init_GOP(np,nb)
int np,nb;
{
  vtkMPEG2WriterStr->R_val += (int) floor((1 + np + nb) * vtkMPEG2WriterStr->bit_rate / vtkMPEG2WriterStr->frame_rate + 0.5);
  vtkMPEG2WriterStr->Np = vtkMPEG2WriterStr->fieldpic ? 2*np+1 : np;
  vtkMPEG2WriterStr->Nb = vtkMPEG2WriterStr->fieldpic ? 2*nb : nb;

  if ( vtkMPEG2WriterStr->statfile )
    {
    fprintf(vtkMPEG2WriterStr->statfile,"\nrate control: new group of pictures (GOP)\n");
    fprintf(vtkMPEG2WriterStr->statfile," target number of bits for GOP: R=%d\n",vtkMPEG2WriterStr->R_val);
    fprintf(vtkMPEG2WriterStr->statfile," number of P pictures in GOP: Np=%d\n",vtkMPEG2WriterStr->Np);
    fprintf(vtkMPEG2WriterStr->statfile," number of B pictures in GOP: Nb=%d\n",vtkMPEG2WriterStr->Nb);
    }
}

/* Note: we need to substitute K for the 1.4 and 1.0 constants -- this can
   be modified to fit image content */

/* Step 1: compute target bits for current picture being coded */
void MPEG2_rc_init_pict(frame)
unsigned char *frame;
{
  double Tmin;

  switch (vtkMPEG2WriterStr->pict_type)
  {
  case I_TYPE:
    vtkMPEG2WriterStr->T_val = (int) floor(vtkMPEG2WriterStr->R_val/(1.0+vtkMPEG2WriterStr->Np*vtkMPEG2WriterStr->Xp/(vtkMPEG2WriterStr->Xi*1.0)+vtkMPEG2WriterStr->Nb*vtkMPEG2WriterStr->Xb/(vtkMPEG2WriterStr->Xi*1.4)) + 0.5);
    vtkMPEG2WriterStr->d_val = vtkMPEG2WriterStr->d0i;
    break;
  case P_TYPE:
    vtkMPEG2WriterStr->T_val = (int) floor(vtkMPEG2WriterStr->R_val/(vtkMPEG2WriterStr->Np+vtkMPEG2WriterStr->Nb*1.0*vtkMPEG2WriterStr->Xb/(1.4*vtkMPEG2WriterStr->Xp)) + 0.5);
    vtkMPEG2WriterStr->d_val = vtkMPEG2WriterStr->d0p;
    break;
  case B_TYPE:
    vtkMPEG2WriterStr->T_val = (int) floor(vtkMPEG2WriterStr->R_val/(vtkMPEG2WriterStr->Nb+vtkMPEG2WriterStr->Np*1.4*vtkMPEG2WriterStr->Xp/(1.0*vtkMPEG2WriterStr->Xb)) + 0.5);
    vtkMPEG2WriterStr->d_val = vtkMPEG2WriterStr->d0b;
    break;
  }

  Tmin = (int) floor(vtkMPEG2WriterStr->bit_rate/(8.0*vtkMPEG2WriterStr->frame_rate) + 0.5);

  if (vtkMPEG2WriterStr->T_val<Tmin)
    vtkMPEG2WriterStr->T_val = (int)Tmin;

  vtkMPEG2WriterStr->S_val = MPEG2_bitcount();
  vtkMPEG2WriterStr->Q_val = 0;

  calc_actj(frame);
  vtkMPEG2WriterStr->actsum = 0.0;

  if ( vtkMPEG2WriterStr->statfile )
    {
    fprintf(vtkMPEG2WriterStr->statfile,"\nrate control: start of picture\n");
    fprintf(vtkMPEG2WriterStr->statfile," target number of bits: T=%d\n",vtkMPEG2WriterStr->T_val);
    }
}

static void calc_actj(frame)
unsigned char *frame;
{
  int i,j,k;
  unsigned char *p;
  double actj,var;

  k = 0;

  for (j=0; j<vtkMPEG2WriterStr->height2; j+=16)
    for (i=0; i<vtkMPEG2WriterStr->width; i+=16)
    {
      p = frame + ((vtkMPEG2WriterStr->pict_struct==BOTTOM_FIELD)?vtkMPEG2WriterStr->width:0) + i + vtkMPEG2WriterStr->width2*j;

      /* take minimum spatial activity measure of luminance blocks */

      actj = var_sblk(p,vtkMPEG2WriterStr->width2);
      var = var_sblk(p+8,vtkMPEG2WriterStr->width2);
      if (var<actj) actj = var;
      var = var_sblk(p+8*vtkMPEG2WriterStr->width2,vtkMPEG2WriterStr->width2);
      if (var<actj) actj = var;
      var = var_sblk(p+8*vtkMPEG2WriterStr->width2+8,vtkMPEG2WriterStr->width2);
      if (var<actj) actj = var;

      if (!vtkMPEG2WriterStr->fieldpic && !vtkMPEG2WriterStr->prog_seq)
      {
        /* field */
        var = var_sblk(p,vtkMPEG2WriterStr->width<<1);
        if (var<actj) actj = var;
        var = var_sblk(p+8,vtkMPEG2WriterStr->width<<1);
        if (var<actj) actj = var;
        var = var_sblk(p+vtkMPEG2WriterStr->width,vtkMPEG2WriterStr->width<<1);
        if (var<actj) actj = var;
        var = var_sblk(p+vtkMPEG2WriterStr->width+8,vtkMPEG2WriterStr->width<<1);
        if (var<actj) actj = var;
      }

      actj+= 1.0;

      vtkMPEG2WriterStr->mbinfo[k++].act = actj;
    }
}

void MPEG2_rc_update_pict()
{
  double X;

  vtkMPEG2WriterStr->S_val = MPEG2_bitcount() - vtkMPEG2WriterStr->S_val; /* total # of bits in picture */
  vtkMPEG2WriterStr->R_val-= vtkMPEG2WriterStr->S_val; /* remaining # of bits in GOP */
  X = (int) floor(vtkMPEG2WriterStr->S_val*((0.5*(double)vtkMPEG2WriterStr->Q_val)/(vtkMPEG2WriterStr->mb_width*vtkMPEG2WriterStr->mb_height2)) + 0.5);
  vtkMPEG2WriterStr->d_val+= vtkMPEG2WriterStr->S_val - vtkMPEG2WriterStr->T_val;
  vtkMPEG2WriterStr->avg_act = vtkMPEG2WriterStr->actsum/(vtkMPEG2WriterStr->mb_width*vtkMPEG2WriterStr->mb_height2);

  switch (vtkMPEG2WriterStr->pict_type)
  {
  case I_TYPE:
    vtkMPEG2WriterStr->Xi = (int)X;
    vtkMPEG2WriterStr->d0i = vtkMPEG2WriterStr->d_val;
    break;
  case P_TYPE:
    vtkMPEG2WriterStr->Xp = (int)X;
    vtkMPEG2WriterStr->d0p = vtkMPEG2WriterStr->d_val;
    vtkMPEG2WriterStr->Np--;
    break;
  case B_TYPE:
    vtkMPEG2WriterStr->Xb = (int)X;
    vtkMPEG2WriterStr->d0b = vtkMPEG2WriterStr->d_val;
    vtkMPEG2WriterStr->Nb--;
    break;
  }

  if ( vtkMPEG2WriterStr->statfile )
    {
    fprintf(vtkMPEG2WriterStr->statfile,"\nrate control: end of picture\n");
    fprintf(vtkMPEG2WriterStr->statfile," actual number of bits: S=%d\n",vtkMPEG2WriterStr->S_val);
    fprintf(vtkMPEG2WriterStr->statfile," average quantization parameter Q=%.1f\n",
      (double)vtkMPEG2WriterStr->Q_val/(vtkMPEG2WriterStr->mb_width*vtkMPEG2WriterStr->mb_height2));
    fprintf(vtkMPEG2WriterStr->statfile," remaining number of bits in GOP: R=%d\n",vtkMPEG2WriterStr->R_val);
    fprintf(vtkMPEG2WriterStr->statfile,
      " global complexity measures (I,P,B): Xi=%d, Xp=%d, Xb=%d\n",
      vtkMPEG2WriterStr->Xi, vtkMPEG2WriterStr->Xp, vtkMPEG2WriterStr->Xb);
    fprintf(vtkMPEG2WriterStr->statfile,
      " virtual buffer fullness (I,P,B): d0i=%d, d0p=%d, d0b=%d\n",
      vtkMPEG2WriterStr->d0i, vtkMPEG2WriterStr->d0p, vtkMPEG2WriterStr->d0b);
    fprintf(vtkMPEG2WriterStr->statfile," remaining number of P pictures in GOP: Np=%d\n",vtkMPEG2WriterStr->Np);
    fprintf(vtkMPEG2WriterStr->statfile," remaining number of B pictures in GOP: Nb=%d\n",vtkMPEG2WriterStr->Nb);
    fprintf(vtkMPEG2WriterStr->statfile," average activity: avg_act=%.1f\n", vtkMPEG2WriterStr->avg_act);
    }
}

/* compute initial quantization stepsize (at the beginning of picture) */
int MPEG2_rc_start_mb()
{
  int mquant;

  if (vtkMPEG2WriterStr->q_scale_type)
  {
    mquant = (int) floor(2.0*vtkMPEG2WriterStr->d_val*31.0/vtkMPEG2WriterStr->reaction + 0.5);

    /* clip mquant to legal (linear) range */
    if (mquant<1)
      mquant = 1;
    if (mquant>112)
      mquant = 112;

    /* map to legal quantization level */
    mquant = MPEG2_non_linear_mquant_table[MPEG2_map_non_linear_mquant[mquant]];
  }
  else
  {
    mquant = (int) floor(vtkMPEG2WriterStr->d_val*31.0/vtkMPEG2WriterStr->reaction + 0.5);
    mquant <<= 1;

    /* clip mquant to legal (linear) range */
    if (mquant<2)
      mquant = 2;
    if (mquant>62)
      mquant = 62;

    vtkMPEG2WriterStr->prev_mquant = mquant;
  }

/*
  fprintf(vtkMPEG2WriterStr->statfile,"MPEG2_rc_start_mb:\n");
  fprintf(vtkMPEG2WriterStr->statfile,"mquant=%d\n",mquant);
*/

  return mquant;
}

/* Step 2: measure virtual buffer - estimated buffer discrepancy */
int MPEG2_rc_calc_mquant(j)
int j;
{
  int mquant;
  double dj, Qj, actj, N_actj;

  /* measure virtual buffer discrepancy from uniform distribution model */
  dj = vtkMPEG2WriterStr->d_val + (MPEG2_bitcount()-vtkMPEG2WriterStr->S_val) - j*(vtkMPEG2WriterStr->T_val/(vtkMPEG2WriterStr->mb_width*vtkMPEG2WriterStr->mb_height2));

  /* scale against dynamic range of mquant and the bits/picture count */
  Qj = dj*31.0/vtkMPEG2WriterStr->reaction;
/*Qj = dj*(vtkMPEG2WriterStr->q_scale_type ? 56.0 : 31.0)/r;  */

  actj = vtkMPEG2WriterStr->mbinfo[j].act;
  vtkMPEG2WriterStr->actsum+= actj;

  /* compute normalized activity */
  N_actj = (2.0*actj+vtkMPEG2WriterStr->avg_act)/(actj+2.0*vtkMPEG2WriterStr->avg_act);

  if (vtkMPEG2WriterStr->q_scale_type)
  {
    /* modulate mquant with combined buffer and local activity measures */
    mquant = (int) floor(2.0*Qj*N_actj + 0.5);

    /* clip mquant to legal (linear) range */
    if (mquant<1)
      mquant = 1;
    if (mquant>112)
      mquant = 112;

    /* map to legal quantization level */
    mquant = MPEG2_non_linear_mquant_table[MPEG2_map_non_linear_mquant[mquant]];
  }
  else
  {
    /* modulate mquant with combined buffer and local activity measures */
    mquant = (int) floor(Qj*N_actj + 0.5);
    mquant <<= 1;

    /* clip mquant to legal (linear) range */
    if (mquant<2)
      mquant = 2;
    if (mquant>62)
      mquant = 62;

    /* ignore small changes in mquant */
    if (mquant>=8 && (mquant-vtkMPEG2WriterStr->prev_mquant)>=-4 && (mquant-vtkMPEG2WriterStr->prev_mquant)<=4)
      mquant = vtkMPEG2WriterStr->prev_mquant;

    vtkMPEG2WriterStr->prev_mquant = mquant;
  }

  vtkMPEG2WriterStr->Q_val+= mquant; /* for calculation of average mquant */

/*
  fprintf(vtkMPEG2WriterStr->statfile,"MPEG2_rc_calc_mquant(%d): ",j);
  fprintf(vtkMPEG2WriterStr->statfile,"MPEG2_bitcount=%d, dj=%f, Qj=%f, actj=%f, N_actj=%f, mquant=%d\n",
    MPEG2_bitcount(),dj,Qj,actj,N_actj,mquant);
*/

  return mquant;
}

/* compute variance of 8x8 block */
static double var_sblk(p,lx)
unsigned char *p;
int lx;
{
  int i, j;
  unsigned int v, s, s2;

  s = s2 = 0;

  for (j=0; j<8; j++)
  {
    for (i=0; i<8; i++)
    {
      v = *p++;
      s+= v;
      s2+= v*v;
    }
    p+= lx - 8;
  }

  return s2/64.0 - (s/64.0)*(s/64.0);
}

/* VBV calculations
 *
 * generates warnings if underflow or overflow occurs
 */

/* MPEG2_vbv_end_of_picture
 *
 * - has to be called directly after writing picture_data()
 * - needed for accurate VBV buffer overflow calculation
 * - assumes there is no byte stuffing prior to the next start code
 */

static int bitcnt_EOP;

void MPEG2_vbv_end_of_picture()
{
  bitcnt_EOP = MPEG2_bitcount();
  bitcnt_EOP = (bitcnt_EOP + 7) & ~7; /* account for bit stuffing */
}

/* MPEG2_calc_vbv_delay
 *
 * has to be called directly after writing the picture start code, the
 * reference point for vbv_delay
 */

void MPEG2_calc_vbv_delay()
{
  double picture_delay;
  static double next_ip_delay; /* due to frame reordering delay */
  static double decoding_time;

  /* number of 1/90000 s ticks until next picture is to be decoded */
  if (vtkMPEG2WriterStr->pict_type == B_TYPE)
  {
    if (vtkMPEG2WriterStr->prog_seq)
    {
      if (!vtkMPEG2WriterStr->repeatfirst)
        picture_delay = 90000.0/vtkMPEG2WriterStr->frame_rate; /* 1 frame */
      else
      {
        if (!vtkMPEG2WriterStr->topfirst)
          picture_delay = 90000.0*2.0/vtkMPEG2WriterStr->frame_rate; /* 2 frames */
        else
          picture_delay = 90000.0*3.0/vtkMPEG2WriterStr->frame_rate; /* 3 frames */
      }
    }
    else
    {
      /* interlaced */
      if (vtkMPEG2WriterStr->fieldpic)
        picture_delay = 90000.0/(2.0*vtkMPEG2WriterStr->frame_rate); /* 1 field */
      else
      {
        if (!vtkMPEG2WriterStr->repeatfirst)
          picture_delay = 90000.0*2.0/(2.0*vtkMPEG2WriterStr->frame_rate); /* 2 flds */
        else
          picture_delay = 90000.0*3.0/(2.0*vtkMPEG2WriterStr->frame_rate); /* 3 flds */
      }
    }
  }
  else
  {
    /* I or P picture */
    if (vtkMPEG2WriterStr->fieldpic)
    {
      if(vtkMPEG2WriterStr->topfirst==(vtkMPEG2WriterStr->pict_struct==TOP_FIELD))
      {
        /* first field */
        picture_delay = 90000.0/(2.0*vtkMPEG2WriterStr->frame_rate);
      }
      else
      {
        /* second field */
        /* take frame reordering delay into account */
        picture_delay = next_ip_delay - 90000.0/(2.0*vtkMPEG2WriterStr->frame_rate);
      }
    }
    else
    {
      /* frame picture */
      /* take frame reordering delay into account*/
      picture_delay = next_ip_delay;
    }

    if (!vtkMPEG2WriterStr->fieldpic || vtkMPEG2WriterStr->topfirst!=(vtkMPEG2WriterStr->pict_struct==TOP_FIELD))
    {
      /* frame picture or second field */
      if (vtkMPEG2WriterStr->prog_seq)
      {
        if (!vtkMPEG2WriterStr->repeatfirst)
          next_ip_delay = 90000.0/vtkMPEG2WriterStr->frame_rate;
        else
        {
          if (!vtkMPEG2WriterStr->topfirst)
            next_ip_delay = 90000.0*2.0/vtkMPEG2WriterStr->frame_rate;
          else
            next_ip_delay = 90000.0*3.0/vtkMPEG2WriterStr->frame_rate;
        }
      }
      else
      {
        if (vtkMPEG2WriterStr->fieldpic)
          next_ip_delay = 90000.0/(2.0*vtkMPEG2WriterStr->frame_rate);
        else
        {
          if (!vtkMPEG2WriterStr->repeatfirst)
            next_ip_delay = 90000.0*2.0/(2.0*vtkMPEG2WriterStr->frame_rate);
          else
            next_ip_delay = 90000.0*3.0/(2.0*vtkMPEG2WriterStr->frame_rate);
        }
      }
    }
  }

  if (decoding_time==0.0)
  {
    /* first call of MPEG2_calc_vbv_delay */
    /* we start with a 7/8 filled VBV buffer (12.5% back-off) */
    picture_delay = ((vtkMPEG2WriterStr->vbv_buffer_size*16384*7)/8)*90000.0/vtkMPEG2WriterStr->bit_rate;
    if (vtkMPEG2WriterStr->fieldpic)
      next_ip_delay = (int)(90000.0/vtkMPEG2WriterStr->frame_rate+0.5);
  }

  /* VBV checks */

  /* check for underflow (previous picture) */
  if (!vtkMPEG2WriterStr->low_delay && (decoding_time < bitcnt_EOP*90000.0/vtkMPEG2WriterStr->bit_rate))
  {
    /* picture not completely in buffer at intended decoding time */
    if (!vtkMPEG2WriterStr->quiet)
      fprintf(stderr,"vbv_delay underflow! (decoding_time=%.1f, t_EOP=%.1f\n)",
        decoding_time, bitcnt_EOP*90000.0/vtkMPEG2WriterStr->bit_rate);
  }

  /* when to decode current frame */
  decoding_time += picture_delay;

  /* warning: MPEG2_bitcount() may overflow (e.g. after 9 min. at 8 Mbit/s */
  vtkMPEG2WriterStr->vbv_delay = (int)(decoding_time - MPEG2_bitcount()*90000.0/vtkMPEG2WriterStr->bit_rate);

  /* check for overflow (current picture) */
  if ((decoding_time - bitcnt_EOP*90000.0/vtkMPEG2WriterStr->bit_rate)
      > (vtkMPEG2WriterStr->vbv_buffer_size*16384)*90000.0/vtkMPEG2WriterStr->bit_rate)
  {
    if (!vtkMPEG2WriterStr->quiet)
      fprintf(stderr,"vtkMPEG2WriterStr->vbv_delay overflow!\n");
  }

  if( vtkMPEG2WriterStr->statfile )
    {
    fprintf(vtkMPEG2WriterStr->statfile,
      "\nvbv_delay=%d (MPEG2_bitcount=%d, decoding_time=%.2f, bitcnt_EOP=%d)\n",
      vtkMPEG2WriterStr->vbv_delay,MPEG2_bitcount(),decoding_time,bitcnt_EOP);
    }

  if (vtkMPEG2WriterStr->vbv_delay<0)
  {
    if (!vtkMPEG2WriterStr->quiet)
      fprintf(stderr,"vbv_delay underflow: %d\n",vtkMPEG2WriterStr->vbv_delay);
    vtkMPEG2WriterStr->vbv_delay = 0;
  }

  if (vtkMPEG2WriterStr->vbv_delay>65535)
  {
    if (!vtkMPEG2WriterStr->quiet)
      fprintf(stderr,"vbv_delay overflow: %d\n",vtkMPEG2WriterStr->vbv_delay);
    vtkMPEG2WriterStr->vbv_delay = 65535;
  }
}
