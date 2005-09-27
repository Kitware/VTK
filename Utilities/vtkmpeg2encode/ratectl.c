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

#include "mpeg2enc_config.h"
#include "mpeg2enc_global.h"

/* private prototypes */
static void calc_actj _ANSI_ARGS_((unsigned char *frame,struct MPEG2_structure *mpeg2_struct));
static double var_sblk _ANSI_ARGS_((unsigned char *p, int lx));

/* rate control variables */

VTK_MPEG2ENC_EXPORT void MPEG2_rc_init_seq(mpeg2_struct)
  struct MPEG2_structure *mpeg2_struct;
{
  /* reaction parameter (constant) */
  if (mpeg2_struct->reaction==0)  mpeg2_struct->reaction= (int)floor(2.0*mpeg2_struct->bit_rate/mpeg2_struct->frame_rate + 0.5);

  /* average activity */
  if (mpeg2_struct->avg_act==0.0)  mpeg2_struct->avg_act = 400.0;

  /* remaining # of bits in GOP */
  mpeg2_struct->R_val = 0;

  /* global complexity measure */
  if (mpeg2_struct->Xi==0) mpeg2_struct->Xi = (int)floor(160.0*mpeg2_struct->bit_rate/115.0 + 0.5);
  if (mpeg2_struct->Xp==0) mpeg2_struct->Xp = (int)floor( 60.0*mpeg2_struct->bit_rate/115.0 + 0.5);
  if (mpeg2_struct->Xb==0) mpeg2_struct->Xb = (int)floor( 42.0*mpeg2_struct->bit_rate/115.0 + 0.5);

  /* virtual buffer fullness */
  if (mpeg2_struct->d0i==0) mpeg2_struct->d0i = (int)floor(10.0*mpeg2_struct->reaction/31.0 + 0.5);
  if (mpeg2_struct->d0p==0) mpeg2_struct->d0p = (int)floor(10.0*mpeg2_struct->reaction/31.0 + 0.5);
  if (mpeg2_struct->d0b==0) mpeg2_struct->d0b = (int)floor(1.4*10.0*mpeg2_struct->reaction/31.0 + 0.5);
/*
  if (mpeg2_struct->d0i==0) mpeg2_struct->d0i = (int)floor(10.0*r/(mpeg2_struct->qscale_tab[0] ? 56.0 : 31.0) + 0.5);
  if (mpeg2_struct->d0p==0) mpeg2_struct->d0p = (int)floor(10.0*r/(mpeg2_struct->qscale_tab[1] ? 56.0 : 31.0) + 0.5);
  if (mpeg2_struct->d0b==0) mpeg2_struct->d0b = (int)floor(1.4*10.0*r/(mpeg2_struct->qscale_tab[2] ? 56.0 : 31.0) + 0.5);
*/

  if ( mpeg2_struct->statfile )
    {
    fprintf(mpeg2_struct->statfile,"\nrate control: sequence initialization\n");
    fprintf(mpeg2_struct->statfile,
      " initial global complexity measures (I,P,B): Xi=%d, Xp=%d, Xb=%d\n",
      mpeg2_struct->Xi, mpeg2_struct->Xp, mpeg2_struct->Xb);
    fprintf(mpeg2_struct->statfile," reaction parameter: r=%d\n", mpeg2_struct->reaction);
    fprintf(mpeg2_struct->statfile,
      " initial virtual buffer fullness (I,P,B): d0i=%d, d0p=%d, d0b=%d\n",
      mpeg2_struct->d0i, mpeg2_struct->d0p, mpeg2_struct->d0b);
    fprintf(mpeg2_struct->statfile," initial average activity: avg_act=%.1f\n", mpeg2_struct->avg_act);
    }
}

void MPEG2_rc_init_GOP(np,nb,mpeg2_struct)
int np,nb;
struct MPEG2_structure *mpeg2_struct;
{
  mpeg2_struct->R_val += (int) floor((1 + np + nb) * mpeg2_struct->bit_rate / mpeg2_struct->frame_rate + 0.5);
  mpeg2_struct->Np = mpeg2_struct->fieldpic ? 2*np+1 : np;
  mpeg2_struct->Nb = mpeg2_struct->fieldpic ? 2*nb : nb;

  if ( mpeg2_struct->statfile )
    {
    fprintf(mpeg2_struct->statfile,"\nrate control: new group of pictures (GOP)\n");
    fprintf(mpeg2_struct->statfile," target number of bits for GOP: R=%d\n",mpeg2_struct->R_val);
    fprintf(mpeg2_struct->statfile," number of P pictures in GOP: Np=%d\n",mpeg2_struct->Np);
    fprintf(mpeg2_struct->statfile," number of B pictures in GOP: Nb=%d\n",mpeg2_struct->Nb);
    }
}

/* Note: we need to substitute K for the 1.4 and 1.0 constants -- this can
   be modified to fit image content */

/* Step 1: compute target bits for current picture being coded */
void MPEG2_rc_init_pict(frame,mpeg2_struct)
unsigned char *frame;
struct MPEG2_structure *mpeg2_struct;
{
  double Tmin;

  switch (mpeg2_struct->pict_type)
  {
  case I_TYPE:
    mpeg2_struct->T_val = (int) floor(mpeg2_struct->R_val/(1.0+mpeg2_struct->Np*mpeg2_struct->Xp/(mpeg2_struct->Xi*1.0)+mpeg2_struct->Nb*mpeg2_struct->Xb/(mpeg2_struct->Xi*1.4)) + 0.5);
    mpeg2_struct->d_val = mpeg2_struct->d0i;
    break;
  case P_TYPE:
    mpeg2_struct->T_val = (int) floor(mpeg2_struct->R_val/(mpeg2_struct->Np+mpeg2_struct->Nb*1.0*mpeg2_struct->Xb/(1.4*mpeg2_struct->Xp)) + 0.5);
    mpeg2_struct->d_val = mpeg2_struct->d0p;
    break;
  case B_TYPE:
    mpeg2_struct->T_val = (int) floor(mpeg2_struct->R_val/(mpeg2_struct->Nb+mpeg2_struct->Np*1.4*mpeg2_struct->Xp/(1.0*mpeg2_struct->Xb)) + 0.5);
    mpeg2_struct->d_val = mpeg2_struct->d0b;
    break;
  }

  Tmin = (int) floor(mpeg2_struct->bit_rate/(8.0*mpeg2_struct->frame_rate) + 0.5);

  if (mpeg2_struct->T_val<Tmin)
    mpeg2_struct->T_val = (int)Tmin;

  mpeg2_struct->S_val = MPEG2_bitcount();
  mpeg2_struct->Q_val = 0;

  calc_actj(frame,mpeg2_struct);
  mpeg2_struct->actsum = 0.0;

  if ( mpeg2_struct->statfile )
    {
    fprintf(mpeg2_struct->statfile,"\nrate control: start of picture\n");
    fprintf(mpeg2_struct->statfile," target number of bits: T=%d\n",mpeg2_struct->T_val);
    }
}

static void calc_actj(frame,mpeg2_struct)
unsigned char *frame;
struct MPEG2_structure *mpeg2_struct;
{
  int i,j,k;
  unsigned char *p;
  double actj,var;

  k = 0;

  for (j=0; j<mpeg2_struct->height2; j+=16)
    for (i=0; i<mpeg2_struct->width; i+=16)
    {
      p = frame + ((mpeg2_struct->pict_struct==BOTTOM_FIELD)?mpeg2_struct->width:0) + i + mpeg2_struct->width2*j;

      /* take minimum spatial activity measure of luminance blocks */

      actj = var_sblk(p,mpeg2_struct->width2);
      var = var_sblk(p+8,mpeg2_struct->width2);
      if (var<actj) actj = var;
      var = var_sblk(p+8*mpeg2_struct->width2,mpeg2_struct->width2);
      if (var<actj) actj = var;
      var = var_sblk(p+8*mpeg2_struct->width2+8,mpeg2_struct->width2);
      if (var<actj) actj = var;

      if (!mpeg2_struct->fieldpic && !mpeg2_struct->prog_seq)
      {
        /* field */
        var = var_sblk(p,mpeg2_struct->width<<1);
        if (var<actj) actj = var;
        var = var_sblk(p+8,mpeg2_struct->width<<1);
        if (var<actj) actj = var;
        var = var_sblk(p+mpeg2_struct->width,mpeg2_struct->width<<1);
        if (var<actj) actj = var;
        var = var_sblk(p+mpeg2_struct->width+8,mpeg2_struct->width<<1);
        if (var<actj) actj = var;
      }

      actj+= 1.0;

      mpeg2_struct->mbinfo[k++].act = actj;
    }
}

void MPEG2_rc_update_pict(mpeg2_struct)
  struct MPEG2_structure *mpeg2_struct;
{
  double X;

  mpeg2_struct->S_val = MPEG2_bitcount() - mpeg2_struct->S_val; /* total # of bits in picture */
  mpeg2_struct->R_val-= mpeg2_struct->S_val; /* remaining # of bits in GOP */
  X = (int) floor(mpeg2_struct->S_val*((0.5*(double)mpeg2_struct->Q_val)/(mpeg2_struct->mb_width*mpeg2_struct->mb_height2)) + 0.5);
  mpeg2_struct->d_val+= mpeg2_struct->S_val - mpeg2_struct->T_val;
  mpeg2_struct->avg_act = mpeg2_struct->actsum/(mpeg2_struct->mb_width*mpeg2_struct->mb_height2);

  switch (mpeg2_struct->pict_type)
  {
  case I_TYPE:
    mpeg2_struct->Xi = (int)X;
    mpeg2_struct->d0i = mpeg2_struct->d_val;
    break;
  case P_TYPE:
    mpeg2_struct->Xp = (int)X;
    mpeg2_struct->d0p = mpeg2_struct->d_val;
    mpeg2_struct->Np--;
    break;
  case B_TYPE:
    mpeg2_struct->Xb = (int)X;
    mpeg2_struct->d0b = mpeg2_struct->d_val;
    mpeg2_struct->Nb--;
    break;
  }

  if ( mpeg2_struct->statfile )
    {
    fprintf(mpeg2_struct->statfile,"\nrate control: end of picture\n");
    fprintf(mpeg2_struct->statfile," actual number of bits: S=%d\n",mpeg2_struct->S_val);
    fprintf(mpeg2_struct->statfile," average quantization parameter Q=%.1f\n",
      (double)mpeg2_struct->Q_val/(mpeg2_struct->mb_width*mpeg2_struct->mb_height2));
    fprintf(mpeg2_struct->statfile," remaining number of bits in GOP: R=%d\n",mpeg2_struct->R_val);
    fprintf(mpeg2_struct->statfile,
      " global complexity measures (I,P,B): Xi=%d, Xp=%d, Xb=%d\n",
      mpeg2_struct->Xi, mpeg2_struct->Xp, mpeg2_struct->Xb);
    fprintf(mpeg2_struct->statfile,
      " virtual buffer fullness (I,P,B): d0i=%d, d0p=%d, d0b=%d\n",
      mpeg2_struct->d0i, mpeg2_struct->d0p, mpeg2_struct->d0b);
    fprintf(mpeg2_struct->statfile," remaining number of P pictures in GOP: Np=%d\n",mpeg2_struct->Np);
    fprintf(mpeg2_struct->statfile," remaining number of B pictures in GOP: Nb=%d\n",mpeg2_struct->Nb);
    fprintf(mpeg2_struct->statfile," average activity: avg_act=%.1f\n", mpeg2_struct->avg_act);
    }
}

/* compute initial quantization stepsize (at the beginning of picture) */
int MPEG2_rc_start_mb(mpeg2_struct)
  struct MPEG2_structure *mpeg2_struct;
{
  int mquant;

  if (mpeg2_struct->q_scale_type)
  {
    mquant = (int) floor(2.0*mpeg2_struct->d_val*31.0/mpeg2_struct->reaction + 0.5);

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
    mquant = (int) floor(mpeg2_struct->d_val*31.0/mpeg2_struct->reaction + 0.5);
    mquant <<= 1;

    /* clip mquant to legal (linear) range */
    if (mquant<2)
      mquant = 2;
    if (mquant>62)
      mquant = 62;

    mpeg2_struct->prev_mquant = mquant;
  }

/*
  fprintf(mpeg2_struct->statfile,"MPEG2_rc_start_mb:\n");
  fprintf(mpeg2_struct->statfile,"mquant=%d\n",mquant);
*/

  return mquant;
}

/* Step 2: measure virtual buffer - estimated buffer discrepancy */
int MPEG2_rc_calc_mquant(j, mpeg2_struct)
int j;
struct MPEG2_structure *mpeg2_struct;
{
  int mquant;
  double dj, Qj, actj, N_actj;

  /* measure virtual buffer discrepancy from uniform distribution model */
  dj = mpeg2_struct->d_val + (MPEG2_bitcount()-mpeg2_struct->S_val) - j*(mpeg2_struct->T_val/(mpeg2_struct->mb_width*mpeg2_struct->mb_height2));

  /* scale against dynamic range of mquant and the bits/picture count */
  Qj = dj*31.0/mpeg2_struct->reaction;
/*Qj = dj*(mpeg2_struct->q_scale_type ? 56.0 : 31.0)/r;  */

  actj = mpeg2_struct->mbinfo[j].act;
  mpeg2_struct->actsum+= actj;

  /* compute normalized activity */
  N_actj = (2.0*actj+mpeg2_struct->avg_act)/(actj+2.0*mpeg2_struct->avg_act);

  if (mpeg2_struct->q_scale_type)
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
    if (mquant>=8 && (mquant-mpeg2_struct->prev_mquant)>=-4 && (mquant-mpeg2_struct->prev_mquant)<=4)
      mquant = mpeg2_struct->prev_mquant;

    mpeg2_struct->prev_mquant = mquant;
  }

  mpeg2_struct->Q_val+= mquant; /* for calculation of average mquant */

/*
  fprintf(mpeg2_struct->statfile,"MPEG2_rc_calc_mquant(%d): ",j);
  fprintf(mpeg2_struct->statfile,"MPEG2_bitcount=%d, dj=%f, Qj=%f, actj=%f, N_actj=%f, mquant=%d\n",
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

void MPEG2_calc_vbv_delay(mpeg2_struct)
  struct MPEG2_structure *mpeg2_struct;
{
  double picture_delay;
  static double next_ip_delay; /* due to frame reordering delay */
  static double decoding_time;

  /* number of 1/90000 s ticks until next picture is to be decoded */
  if (mpeg2_struct->pict_type == B_TYPE)
  {
    if (mpeg2_struct->prog_seq)
    {
      if (!mpeg2_struct->repeatfirst)
        picture_delay = 90000.0/mpeg2_struct->frame_rate; /* 1 frame */
      else
      {
        if (!mpeg2_struct->topfirst)
          picture_delay = 90000.0*2.0/mpeg2_struct->frame_rate; /* 2 frames */
        else
          picture_delay = 90000.0*3.0/mpeg2_struct->frame_rate; /* 3 frames */
      }
    }
    else
    {
      /* interlaced */
      if (mpeg2_struct->fieldpic)
        picture_delay = 90000.0/(2.0*mpeg2_struct->frame_rate); /* 1 field */
      else
      {
        if (!mpeg2_struct->repeatfirst)
          picture_delay = 90000.0*2.0/(2.0*mpeg2_struct->frame_rate); /* 2 flds */
        else
          picture_delay = 90000.0*3.0/(2.0*mpeg2_struct->frame_rate); /* 3 flds */
      }
    }
  }
  else
  {
    /* I or P picture */
    if (mpeg2_struct->fieldpic)
    {
      if(mpeg2_struct->topfirst==(mpeg2_struct->pict_struct==TOP_FIELD))
      {
        /* first field */
        picture_delay = 90000.0/(2.0*mpeg2_struct->frame_rate);
      }
      else
      {
        /* second field */
        /* take frame reordering delay into account */
        picture_delay = next_ip_delay - 90000.0/(2.0*mpeg2_struct->frame_rate);
      }
    }
    else
    {
      /* frame picture */
      /* take frame reordering delay into account*/
      picture_delay = next_ip_delay;
    }

    if (!mpeg2_struct->fieldpic || mpeg2_struct->topfirst!=(mpeg2_struct->pict_struct==TOP_FIELD))
    {
      /* frame picture or second field */
      if (mpeg2_struct->prog_seq)
      {
        if (!mpeg2_struct->repeatfirst)
          next_ip_delay = 90000.0/mpeg2_struct->frame_rate;
        else
        {
          if (!mpeg2_struct->topfirst)
            next_ip_delay = 90000.0*2.0/mpeg2_struct->frame_rate;
          else
            next_ip_delay = 90000.0*3.0/mpeg2_struct->frame_rate;
        }
      }
      else
      {
        if (mpeg2_struct->fieldpic)
          next_ip_delay = 90000.0/(2.0*mpeg2_struct->frame_rate);
        else
        {
          if (!mpeg2_struct->repeatfirst)
            next_ip_delay = 90000.0*2.0/(2.0*mpeg2_struct->frame_rate);
          else
            next_ip_delay = 90000.0*3.0/(2.0*mpeg2_struct->frame_rate);
        }
      }
    }
  }

  if (decoding_time==0.0)
  {
    /* first call of MPEG2_calc_vbv_delay */
    /* we start with a 7/8 filled VBV buffer (12.5% back-off) */
    picture_delay = ((mpeg2_struct->vbv_buffer_size*16384*7)/8)*90000.0/mpeg2_struct->bit_rate;
    if (mpeg2_struct->fieldpic)
      next_ip_delay = (int)(90000.0/mpeg2_struct->frame_rate+0.5);
  }

  /* VBV checks */

  /* check for underflow (previous picture) */
  if (!mpeg2_struct->low_delay && (decoding_time < bitcnt_EOP*90000.0/mpeg2_struct->bit_rate))
  {
    /* picture not completely in buffer at intended decoding time */
    if (!mpeg2_struct->quiet)
      fprintf(stderr,"vbv_delay underflow! (decoding_time=%.1f, t_EOP=%.1f\n)",
        decoding_time, bitcnt_EOP*90000.0/mpeg2_struct->bit_rate);
  }

  /* when to decode current frame */
  decoding_time += picture_delay;

  /* warning: MPEG2_bitcount() may overflow (e.g. after 9 min. at 8 Mbit/s */
  mpeg2_struct->vbv_delay = (int)(decoding_time - MPEG2_bitcount()*90000.0/mpeg2_struct->bit_rate);

  /* check for overflow (current picture) */
  if ((decoding_time - bitcnt_EOP*90000.0/mpeg2_struct->bit_rate)
      > (mpeg2_struct->vbv_buffer_size*16384)*90000.0/mpeg2_struct->bit_rate)
  {
    if (!mpeg2_struct->quiet)
      fprintf(stderr,"mpeg2_struct->vbv_delay overflow!\n");
  }

  if( mpeg2_struct->statfile )
    {
    fprintf(mpeg2_struct->statfile,
      "\nvbv_delay=%d (MPEG2_bitcount=%d, decoding_time=%.2f, bitcnt_EOP=%d)\n",
      mpeg2_struct->vbv_delay,MPEG2_bitcount(),decoding_time,bitcnt_EOP);
    }

  if (mpeg2_struct->vbv_delay<0)
  {
    if (!mpeg2_struct->quiet)
      fprintf(stderr,"vbv_delay underflow: %d\n",mpeg2_struct->vbv_delay);
    mpeg2_struct->vbv_delay = 0;
  }

  if (mpeg2_struct->vbv_delay>65535)
  {
    if (!mpeg2_struct->quiet)
      fprintf(stderr,"vbv_delay overflow: %d\n",mpeg2_struct->vbv_delay);
    mpeg2_struct->vbv_delay = 65535;
  }
}
