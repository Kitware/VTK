/* global.h, global variables, function prototypes                          */

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
#ifdef __cplusplus
extern "C" {
#endif

#include "mpeg2enc.h"

/* choose between declaration (GLOBAL undefined)
 * and definition (GLOBAL defined)
 * GLOBAL is defined in exactly one file (mpeg2enc.c)
 */

#ifndef GLOBAL
#define EXTERN extern
#else
#define EXTERN
#endif

/* prototypes of global functions */

/* conform.c */
void MPEG2_range_checks _ANSI_ARGS_((void));
void MPEG2_profile_and_level_checks _ANSI_ARGS_(());

/* fdctref.c */
void MPEG2_init_fdct _ANSI_ARGS_((void));
void MPEG2_fdct _ANSI_ARGS_((short *block));

/* idct.c */
void MPEG2_idct _ANSI_ARGS_((short *block));
void MPEG2_init_idct _ANSI_ARGS_((void));

/* motion.c */
void MPEG2_motion_estimation _ANSI_ARGS_((unsigned char *oldorg, unsigned char *neworg,
  unsigned char *oldref, unsigned char *newref, unsigned char *cur,
  unsigned char *curref, int sxf, int syf, int sxb, int syb,
  struct mbinfo *mbi, int secondfield, int ipflag));

/* mpeg2enc.c */
void MPEG2_error _ANSI_ARGS_((const char *text));

/* predict.c */
void MPEG2_predict _ANSI_ARGS_((unsigned char *reff[], unsigned char *refb[],
  unsigned char *cur[3], int secondfield, struct mbinfo *mbi));

/* putbits.c */
void MPEG2_initbits _ANSI_ARGS_((void));
void MPEG2_putbits _ANSI_ARGS_((int val, int n));
void MPEG2_alignbits _ANSI_ARGS_((void));
int MPEG2_bitcount _ANSI_ARGS_((void));

/* puthdr.c */
void MPEG2_putseqhdr _ANSI_ARGS_((void));
void MPEG2_putseqext _ANSI_ARGS_((void));
void MPEG2_putseqdispext _ANSI_ARGS_((void));
void MPEG2_putuserdata _ANSI_ARGS_((char *userdata));
void MPEG2_putgophdr _ANSI_ARGS_((int frame, int closed_gop));
void MPEG2_putpicthdr _ANSI_ARGS_((void));
void MPEG2_putpictcodext _ANSI_ARGS_((void));
void MPEG2_putseqend _ANSI_ARGS_((void));

/* putmpg.c */
void MPEG2_putintrablk _ANSI_ARGS_((short *blk, int cc));
void MPEG2_putnonintrablk _ANSI_ARGS_((short *blk));
void MPEG2_putmv _ANSI_ARGS_((int dmv, int f_code));

/* putpic.c */
void MPEG2_putpict _ANSI_ARGS_((unsigned char *frame));

/* putseq.c */
int MPEG2_putseq_one _ANSI_ARGS_((int cframe, int max));

/* putvlc.c */
void MPEG2_putDClum _ANSI_ARGS_((int val));
void MPEG2_putDCchrom _ANSI_ARGS_((int val));
void MPEG2_putACfirst _ANSI_ARGS_((int run, int val));
void MPEG2_putAC _ANSI_ARGS_((int run, int signed_level, int vlcformat));
void MPEG2_putaddrinc _ANSI_ARGS_((int addrinc));
void MPEG2_putmbtype _ANSI_ARGS_((int pict_type, int mb_type));
void MPEG2_putmotioncode _ANSI_ARGS_((int motion_code));
void MPEG2_putdmv _ANSI_ARGS_((int dmv));
void MPEG2_putcbp _ANSI_ARGS_((int cbp));

/* quantize.c */
int MPEG2_quant_intra _ANSI_ARGS_((short *src, short *dst, int dc_prec,
  unsigned char *quant_mat, int mquant));
int MPEG2_quant_non_intra _ANSI_ARGS_((short *src, short *dst,
  unsigned char *quant_mat, int mquant));
void MPEG2_iquant_intra _ANSI_ARGS_((short *src, short *dst, int dc_prec,
  unsigned char *quant_mat, int mquant));
void MPEG2_iquant_non_intra _ANSI_ARGS_((short *src, short *dst,
  unsigned char *quant_mat, int mquant));

/* ratectl.c */
void MPEG2_rc_init_seq _ANSI_ARGS_((void));
void MPEG2_rc_init_GOP _ANSI_ARGS_((int np, int nb));
void MPEG2_rc_init_pict _ANSI_ARGS_((unsigned char *frame));
void MPEG2_rc_update_pict _ANSI_ARGS_((void));
int MPEG2_rc_start_mb _ANSI_ARGS_((void));
int MPEG2_rc_calc_mquant _ANSI_ARGS_((int j));
void MPEG2_vbv_end_of_picture _ANSI_ARGS_((void));
void MPEG2_calc_vbv_delay _ANSI_ARGS_((void));

/* readpic.c */
void MPEG2_readframe _ANSI_ARGS_((char *fname, unsigned char *frame[]));
unsigned char* vtkMPEG2WriterInternalGetImagePtr _ANSI_ARGS_((const char* fname));

/* stats.c */
void MPEG2_calcSNR _ANSI_ARGS_((unsigned char *org[3], unsigned char *rec[3]));
void MPEG2_stats _ANSI_ARGS_((void));

/* transfrm.c */
void MPEG2_transform _ANSI_ARGS_((unsigned char *pred[], unsigned char *cur[],
  struct mbinfo *mbi, short blocks[][64]));
void MPEG2_itransform _ANSI_ARGS_((unsigned char *pred[], unsigned char *cur[],
  struct mbinfo *mbi, short blocks[][64]));
void MPEG2_dct_type_estimation _ANSI_ARGS_((unsigned char *pred, unsigned char *cur,
  struct mbinfo *mbi));

/* writepic.c */
void MPEG2_writeframe _ANSI_ARGS_((char *fname, unsigned char *frame[]));


/* global variables */

EXTERN char MPEG2_version[]
#ifdef GLOBAL
  ="mpeg2encode V1.2, 96/07/19"
#endif
;

EXTERN char MPEG2_author[]
#ifdef GLOBAL
  ="(C) 1996, MPEG Software Simulation Group"
#endif
;

/* zig-zag scan */
EXTERN unsigned char MPEG2_zig_zag_scan[64]
#ifdef GLOBAL
=
{
  0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,
  12,19,26,33,40,48,41,34,27,20,13,6,7,14,21,28,
  35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,
  58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63
}
#endif
;

/* alternate scan */
EXTERN unsigned char MPEG2_alternate_scan[64]
#ifdef GLOBAL
=
{
  0,8,16,24,1,9,2,10,17,25,32,40,48,56,57,49,
  41,33,26,18,3,11,4,12,19,27,34,42,50,58,35,43,
  51,59,20,28,5,13,6,14,21,29,36,44,52,60,37,45,
  53,61,22,30,7,15,23,31,38,46,54,62,39,47,55,63
}
#endif
;

/* default intra quantization matrix */
EXTERN unsigned char MPEG2_default_intra_quantizer_matrix[64]
#ifdef GLOBAL
=
{
   8, 16, 19, 22, 26, 27, 29, 34,
  16, 16, 22, 24, 27, 29, 34, 37,
  19, 22, 26, 27, 29, 34, 34, 38,
  22, 22, 26, 27, 29, 34, 37, 40,
  22, 26, 27, 29, 32, 35, 40, 48,
  26, 27, 29, 32, 35, 40, 48, 58,
  26, 27, 29, 34, 38, 46, 56, 69,
  27, 29, 35, 38, 46, 56, 69, 83
}
#endif
;

/* non-linear quantization coefficient table */
EXTERN unsigned char MPEG2_non_linear_mquant_table[32]
#ifdef GLOBAL
=
{
   0, 1, 2, 3, 4, 5, 6, 7,
   8,10,12,14,16,18,20,22,
  24,28,32,36,40,44,48,52,
  56,64,72,80,88,96,104,112
}
#endif
;

/* non-linear mquant table for mapping from scale to code
 * since reconstruction levels are not bijective with the index map,
 * it is up to the designer to determine most of the quantization levels
 */

EXTERN unsigned char MPEG2_map_non_linear_mquant[113] 
#ifdef GLOBAL
=
{
0,1,2,3,4,5,6,7,8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15,16,16,
16,17,17,17,18,18,18,18,19,19,19,19,20,20,20,20,21,21,21,21,22,22,
22,22,23,23,23,23,24,24,24,24,24,24,24,25,25,25,25,25,25,25,26,26,
26,26,26,26,26,26,27,27,27,27,27,27,27,27,28,28,28,28,28,28,28,29,
29,29,29,29,29,29,29,29,29,30,30,30,30,30,30,30,31,31,31,31,31
}
#endif
;

/* picture data arrays */

struct vtkMPEG2Structure 
{
  /* reconstructed frames */
  unsigned char *newrefframe[3], *oldrefframe[3], *auxframe[3];
  /* original frames */
  unsigned char *neworgframe[3], *oldorgframe[3], *auxorgframe[3];
  /* prediction of current frame */
  unsigned char *predframe[3];
  /* 8*8 block data */
  short (*blocks)[64];
  /* intra / non_intra quantization matrices */
  unsigned char intra_q[64], inter_q[64];
  unsigned char chrom_intra_q[64],chrom_inter_q[64];
  /* prediction values for DCT coefficient (0,0) */
  int dc_dct_pred[3];
  /* macroblock side information array */
  struct mbinfo *mbinfo;
  /* motion estimation parameters */
  struct motion_data *motion_data;
  /* clipping (=saturation) table */
  unsigned char *clp;

  /* name strings */
  char id_string[256], tplorg[256], tplref[256];
  char iqname[256], niqname[256];
  char statname[256];
  char errortext[256];

  FILE *outfile, *statfile; /* file descriptors */
  int inputtype; /* format of input frames */

  int quiet; /* suppress warnings */


  /* coding model parameters */

  int N_val; /* number of frames in Group of Pictures */
  int M_val; /* distance between I/P frames */
  int P_val; /* intra slice refresh interval */
  int nframes; /* total number of frames to encode */
  int frame0, tc0; /* number and timecode of first frame */
  int mpeg1; /* ISO/IEC IS 11172-2 sequence */
  int fieldpic; /* use field pictures */

  /* sequence specific data (sequence header) */

  int horizontal_size, vertical_size; /* frame size (pels) */
  int width, height; /* encoded frame size (pels) multiples of 16 or 32 */
  int chrom_width,chrom_height,block_count;
  int mb_width, mb_height; /* frame size (macroblocks) */
  int width2, height2, mb_height2, chrom_width2; /* picture size */
  int aspectratio; /* aspect ratio information (pel or display) */
  int frame_rate_code; /* coded value of frame rate */
  double frame_rate; /* frames per second */
  double bit_rate; /* bits per second */
  int vbv_buffer_size; /* size of VBV buffer (* 16 kbit) */
  int constrparms; /* constrained parameters flag (MPEG-1 only) */
  int load_iquant, load_niquant; /* use non-default quant. matrices */
  int load_ciquant,load_cniquant;


  /* sequence specific data (sequence extension) */

  int profile, level; /* syntax / parameter constraints */
  int prog_seq; /* progressive sequence */
  int chroma_format;
  int low_delay; /* no B pictures, skipped pictures */


  /* sequence specific data (sequence display extension) */

  int video_format; /* component, PAL, NTSC, SECAM or MAC */
  int color_primaries; /* source primary chromaticity coordinates */
  int transfer_characteristics; /* opto-electronic transfer char. (gamma) */
  int matrix_coefficients; /* Eg,Eb,Er / Y,Cb,Cr matrix coefficients */
  int display_horizontal_size, display_vertical_size; /* display size */


  /* picture specific data (picture header) */

  int temp_ref; /* temporal reference */
  int pict_type; /* picture coding type (I, P or B) */
  int vbv_delay; /* video buffering verifier delay (1/90000 seconds) */


  /* picture specific data (picture coding extension) */

  int forw_hor_f_code, forw_vert_f_code;
  int back_hor_f_code, back_vert_f_code; /* motion vector ranges */
  int dc_prec; /* DC coefficient precision for intra coded blocks */
  int pict_struct; /* picture structure (frame, top / bottom field) */
  int topfirst; /* display top field first */
  /* use only frame prediction and frame DCT (I,P,B,current) */
  int frame_pred_dct_tab[3], frame_pred_dct;
  int conceal_tab[3]; /* use concealment motion vectors (I,P,B) */
  int qscale_tab[3], q_scale_type; /* linear/non-linear quantizaton table */
  int intravlc_tab[3], intravlc; /* intra vlc format (I,P,B,current) */
  int altscan_tab[3], altscan; /* alternate scan (I,P,B,current) */
  int repeatfirst; /* repeat first field after second field */
  int prog_frame; /* progressive frame */

  int Xi, Xp, Xb, reaction, d0i, d0p, d0b;
  double avg_act;
  int R_val, T_val, d_val;
  double actsum;
  int Np, Nb, S_val, Q_val;
  int prev_mquant;
};

EXTERN struct vtkMPEG2Structure *vtkMPEG2WriterStr;

#ifdef __cplusplus
}
#endif

