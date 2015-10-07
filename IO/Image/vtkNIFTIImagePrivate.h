#ifndef vtkNIFTIImagePrivate_h
#define vtkNIFTIImagePrivate_h

/*****************************************************************************
      ** This file defines the "NIFTI-1" header format.               **
      ** It is derived from 2 meetings at the NIH (31 Mar 2003 and    **
      ** 02 Sep 2003) of the Data Format Working Group (DFWG),        **
      ** chartered by the NIfTI (Neuroimaging Informatics Technology  **
      ** Initiative) at the National Institutes of Health (NIH).      **
      **--------------------------------------------------------------**
      ** Neither the National Institutes of Health (NIH), the DFWG,   **
      ** nor any of the members or employees of these institutions    **
      ** imply any warranty of usefulness of this material for any    **
      ** purpose, and do not assume any liability for damages,        **
      ** incidental or otherwise, caused by any use of this document. **
      ** If these conditions are not acceptable, do not use this!     **
      **--------------------------------------------------------------**
      ** Author:   Robert W Cox (NIMH, Bethesda)                      **
      ** Advisors: John Ashburner (FIL, London),                      **
      **           Stephen Smith (FMRIB, Oxford),                     **
      **           Mark Jenkinson (FMRIB, Oxford)                     **
******************************************************************************/

/*=================*/
#ifdef  __cplusplus
extern "C" {
#endif
/*=================*/

/*! \struct nifti_1_header
    \brief Data structure defining the fields in the nifti1 header.
           This binary header should be found at the beginning of a valid
           NIFTI-1 header file.
 */
                        /*************************/  /************/
struct nifti_1_header { /* NIFTI-1 usage         */  /*  offset  */
                        /*************************/  /************/

  int   sizeof_hdr;    /*!< MUST be 348           */  /*   0 */
  char  data_type[10]; /*!< ++UNUSED++            */  /*   4 */
  char  db_name[18];   /*!< ++UNUSED++            */  /*  14 */
  int   extents;       /*!< ++UNUSED++            */  /*  32 */
  short session_error; /*!< ++UNUSED++            */  /*  36 */
  char  regular;       /*!< ++UNUSED++            */  /*  38 */
  char  dim_info;      /*!< MRI slice ordering.   */  /*  39 */
  short dim[8];        /*!< Data array dimensions.*/  /*  40 */
  float intent_p1;     /*!< 1st intent parameter. */  /*  56 */
  float intent_p2;     /*!< 2nd intent parameter. */  /*  60 */
  float intent_p3;     /*!< 3rd intent parameter. */  /*  64 */
  short intent_code;   /*!< NIFTI_INTENT_* code.  */  /*  68 */
  short datatype;      /*!< Defines data type!    */  /*  70 */
  short bitpix;        /*!< Number bits/voxel.    */  /*  72 */
  short slice_start;   /*!< First slice index.    */  /*  74 */
  float pixdim[8];     /*!< Grid spacings.        */  /*  76 */
  float vox_offset;    /*!< Offset into .nii file */  /* 108 */
  float scl_slope;     /*!< Data scaling: slope.  */  /* 112 */
  float scl_inter;     /*!< Data scaling: offset. */  /* 116 */
  short slice_end;     /*!< Last slice index.     */  /* 120 */
  char  slice_code;    /*!< Slice timing order.   */  /* 122 */
  char  xyzt_units;    /*!< Units of pixdim[1..4] */  /* 123 */
  float cal_max;       /*!< Max display intensity */  /* 124 */
  float cal_min;       /*!< Min display intensity */  /* 128 */
  float slice_duration;/*!< Time for 1 slice.     */  /* 132 */
  float toffset;       /*!< Time axis shift.      */  /* 136 */
  int   glmax;         /*!< ++UNUSED++            */  /* 140 */
  int   glmin;         /*!< ++UNUSED++            */  /* 144 */
  char  descrip[80];   /*!< any text you like.    */  /* 148 */
  char  aux_file[24];  /*!< auxiliary filename.   */  /* 228 */
  short qform_code;    /*!< NIFTI_XFORM_* code.   */  /* 252 */
  short sform_code;    /*!< NIFTI_XFORM_* code.   */  /* 254 */
  float quatern_b;     /*!< Quaternion b param.   */  /* 256 */
  float quatern_c;     /*!< Quaternion c param.   */  /* 260 */
  float quatern_d;     /*!< Quaternion d param.   */  /* 264 */
  float qoffset_x;     /*!< Quaternion x shift.   */  /* 268 */
  float qoffset_y;     /*!< Quaternion y shift.   */  /* 272 */
  float qoffset_z;     /*!< Quaternion z shift.   */  /* 276 */
  float srow_x[4];     /*!< 1st row affine transform.   */  /* 280 */
  float srow_y[4];     /*!< 2nd row affine transform.   */  /* 296 */
  float srow_z[4];     /*!< 3rd row affine transform.   */  /* 312 */
  char intent_name[16];/*!< 'name' or meaning of data.  */  /* 328 */
  char magic[4];       /*!< MUST be "ni1\0" or "n+1\0". */  /* 344 */

};                    /**** 348 bytes total ****/

typedef struct nifti_1_header nifti_1_header;

/*---------------------------------------------------------------------------*/
/* TYPE OF DATA (acceptable values for datatype field):
   ---------------------------------------------------
   Values of datatype smaller than 256 are ANALYZE 7.5 compatible.
   Larger values are NIFTI-1 additions.  These are all multiples of 256, so
   that no bits below position 8 are set in datatype.  But there is no need
   to use only powers-of-2, as the original ANALYZE 7.5 datatype codes do.

   The additional codes are intended to include a complete list of basic
   scalar types, including signed and unsigned integers from 8 to 64 bits,
   floats from 32 to 128 bits, and complex (float pairs) from 64 to 256 bits.

   Note that most programs will support only a few of these datatypes!
   A NIFTI-1 program should fail gracefully (e.g., print a warning message)
   when it encounters a dataset with a type it doesn't like.
-----------------------------------------------------------------------------*/

/*! \defgroup NIFTI1_DATATYPE_ALIASES
    \brief aliases for the nifti1 datatype codes
    @{
 */
                                       /*! unsigned char. */
#define NIFTI_TYPE_UINT8           2
                                       /*! signed short. */
#define NIFTI_TYPE_INT16           4
                                       /*! signed int. */
#define NIFTI_TYPE_INT32           8
                                       /*! 32 bit float. */
#define NIFTI_TYPE_FLOAT32        16
                                       /*! 64 bit complex = 2 32 bit floats. */
#define NIFTI_TYPE_COMPLEX64      32
                                       /*! 64 bit float = double. */
#define NIFTI_TYPE_FLOAT64        64
                                       /*! 3 8 bit bytes. */
#define NIFTI_TYPE_RGB24         128
                                       /*! signed char. */
#define NIFTI_TYPE_INT8          256
                                       /*! unsigned short. */
#define NIFTI_TYPE_UINT16        512
                                       /*! unsigned int. */
#define NIFTI_TYPE_UINT32        768
                                       /*! signed long long. */
#define NIFTI_TYPE_INT64        1024
                                       /*! unsigned long long. */
#define NIFTI_TYPE_UINT64       1280
                                       /*! 128 bit float = long double. */
#define NIFTI_TYPE_FLOAT128     1536
                                       /*! 128 bit complex = 2 64 bit floats. */
#define NIFTI_TYPE_COMPLEX128   1792
                                       /*! 256 bit complex = 2 128 bit floats */
#define NIFTI_TYPE_COMPLEX256   2048
                                       /*! 4 8 bit bytes. */
#define NIFTI_TYPE_RGBA32       2304
/* @} */


/*---------------------------------------------------------------------------*/
/* MISCELLANEOUS C MACROS
-----------------------------------------------------------------------------*/

/*.................*/
/*! Given a nifti_1_header struct, check if it has a good magic number.
    Returns NIFTI version number (1..9) if magic is good, 0 if it is not. */

#define NIFTI_VERSION(h)                               \
 ( ( (h).magic[0]=='n' && (h).magic[3]=='\0'    &&     \
     ( (h).magic[1]=='i' || (h).magic[1]=='+' ) &&     \
     ( (h).magic[2]>='1' && (h).magic[2]<='9' )   )    \
 ? (h).magic[2]-'0' : 0 )

/*.................*/
/*! Check if a nifti_1_header struct says if the data is stored in the
    same file or in a separate file.  Returns 1 if the data is in the same
    file as the header, 0 if it is not.                                   */

#define NIFTI_ONEFILE(h) ( (h).magic[1] == '+' )

/*.................*/
/*! Check if a nifti_1_header struct needs to be byte swapped.
    Returns 1 if it needs to be swapped, 0 if it does not.     */

#define NIFTI_NEEDS_SWAP(h) ( (h).dim[0] < 0 || (h).dim[0] > 7 )


/*=================*/
#ifdef  __cplusplus
}
#endif
/*=================*/

/*---------------------------------------------------------------------------*/
/* Changes to the header from NIFTI-1 to NIFTI-2 are intended to allow for
   larger and more accurate fields.  The changes are as follows:

      - short dim[8]         -> int64_t dim[8]
      - float intent_p1,2,3  -> double intent_p1,2,3    (3 fields)
      - float pixdim[8]      -> double pixdim[8]
      - float vox_offset     -> int64_t vox_offset
      - float scl_slope      -> double scl_slope
      - float scl_inter      -> double scl_inter
      - float cal_max        -> double cal_max
      - float cal_min        -> double cal_min
      - float slice_duration -> double slice_duration
      - float toffset        -> double toffset
      - short slice_start    -> int64_t slice_start
      - short slice_end      -> int64_t slice_end
      - char slice_code      -> int32_t slice_code
      - char xyzt_units      -> int32_t xyzt_units
      - short intent_code    -> int32_t intent_code
      - short qform_code     -> int32_t qform_code
      - short sform_code     -> int32_t sform_code
      - float quatern_b,c,d  -> double quatern_b,c,d    (3 fields)
      - float srow_x,y,z[4]  -> double srow_x,y,z[4]    (3 fields)
      - char magic[4]        -> char magic[8]
      - char unused_str[15]  -> padding added at the end of the header

      - previously unused fields have been removed:
           data_type, db_name, extents, session_error, regular, glmax, glmin

      - the field ordering has been changed
-----------------------------------------------------------------------------*/

/*=================*/
#ifdef  __cplusplus
extern "C" {
#endif
/*=================*/

/*! \struct nifti_2_header
    \brief Data structure defining the fields in the nifti2 header.
           This binary header should be found at the beginning of a valid
           NIFTI-2 header file.
 */

                        /*************************/  /************/
struct nifti_2_header { /* NIFTI-2 usage         */  /*  offset  */
                        /*************************/  /************/
  int   sizeof_hdr;     /*!< MUST be 540           */   /*   0 */
  char  magic[8];       /*!< MUST be valid signature. */ /*   4 */
  short datatype;       /*!< Defines data type!    */   /*  12 */
  short bitpix;         /*!< Number bits/voxel.    */   /*  14 */
  long long dim[8];     /*!< Data array dimensions.*/   /*  16 */
  double intent_p1;     /*!< 1st intent parameter. */   /*  80 */
  double intent_p2;     /*!< 2nd intent parameter. */   /*  88 */
  double intent_p3;     /*!< 3rd intent parameter. */   /*  96 */
  double pixdim[8];     /*!< Grid spacings.        */   /* 104 */
  long long vox_offset; /*!< Offset into .nii file */   /* 168 */
  double scl_slope;     /*!< Data scaling: slope.  */   /* 176 */
  double scl_inter;     /*!< Data scaling: offset. */   /* 184 */
  double cal_max;       /*!< Max display intensity */   /* 192 */
  double cal_min;       /*!< Min display intensity */   /* 200 */
  double slice_duration;/*!< Time for 1 slice.     */   /* 208 */
  double toffset;       /*!< Time axis shift.      */   /* 216 */
  long long slice_start;/*!< First slice index.    */   /* 224 */
  long long slice_end;  /*!< Last slice index.     */   /* 232 */
  char descrip[80];     /*!< any text you like.    */   /* 240 */
  char aux_file[24];    /*!< auxiliary filename.   */   /* 320 */
  int qform_code;       /*!< NIFTI_XFORM_* code.   */   /* 344 */
  int sform_code;       /*!< NIFTI_XFORM_* code.   */   /* 348 */
  double quatern_b;     /*!< Quaternion b param.   */   /* 352 */
  double quatern_c;     /*!< Quaternion c param.   */   /* 360 */
  double quatern_d;     /*!< Quaternion d param.   */   /* 368 */
  double qoffset_x;     /*!< Quaternion x shift.   */   /* 376 */
  double qoffset_y;     /*!< Quaternion y shift.   */   /* 384 */
  double qoffset_z;     /*!< Quaternion z shift.   */   /* 392 */
  double srow_x[4];     /*!< 1st row affine transform. */ /* 400 */
  double srow_y[4];     /*!< 2nd row affine transform. */ /* 432 */
  double srow_z[4];     /*!< 3rd row affine transform. */ /* 464 */
  int slice_code;       /*!< Slice timing order.   */   /* 496 */
  int xyzt_units;       /*!< Units of pixdim[1..4] */   /* 500 */
  int intent_code;      /*!< NIFTI_INTENT_* code.  */   /* 504 */
  char intent_name[16]; /*!< 'name' or meaning of data. */  /* 508 */
  char dim_info;        /*!< MRI slice ordering.   */   /* 524 */
  char unused_str[15];  /*!< unused, filled with \0 */  /* 525 */
};                    /**** 540 bytes total ****/

typedef struct nifti_2_header nifti_2_header;

/*=================*/
#ifdef  __cplusplus
}
#endif
/*=================*/

#endif /* vtkNIFTIImagePrivate_h */
// VTK-HeaderTest-Exclude: vtkNIFTIImagePrivate.h
