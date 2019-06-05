/*
** Copyright (c) 2014-2019, Lawrence Livermore National Security, LLC.
** Produced at the Lawrence Livermore National Laboratory.
** Authors: Peter Lindstrom, Markus Salasoo, Matt Larsen, Stephen Herbein.
** LLNL-CODE-663824.
** All rights reserved.
**
** This file is part of the zfp library.
** For details, see http://computation.llnl.gov/casc/zfp/.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** 1. Redistributions of source code must retain the above copyright notice,
** this list of conditions and the disclaimer below.
**
** 2. Redistributions in binary form must reproduce the above copyright notice,
** this list of conditions and the disclaimer (as noted below) in the
** documentation and/or other materials provided with the distribution.
**
** 3. Neither the name of the LLNS/LLNL nor the names of its contributors may
** be used to endorse or promote products derived from this software without
** specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL LAWRENCE LIVERMORE NATIONAL SECURITY,
** LLC, THE U.S. DEPARTMENT OF ENERGY OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
** INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
**
** Additional BSD Notice
**
** 1. This notice is required to be provided under our contract with the U.S.
** Department of Energy (DOE).  This work was produced at Lawrence Livermore
** National Laboratory under Contract No. DE-AC52-07NA27344 with the DOE.

** 2. Neither the United States Government nor Lawrence Livermore National
** Security, LLC nor any of their employees, makes any warranty, express or
** implied, or assumes any liability or responsibility for the accuracy,
** completeness, or usefulness of any information, apparatus, product, or
** process disclosed, or represents that its use would not infringe
** privately-owned rights.
**
** 3. Also, reference herein to any specific commercial products, process, or
** services by trade name, trademark, manufacturer or otherwise does not
** necessarily constitute or imply its endorsement, recommendation, or
** favoring by the United States Government or Lawrence Livermore National
** Security, LLC.  The views and opinions of authors expressed herein do not
** necessarily state or reflect those of the United States Government or
** Lawrence Livermore National Security, LLC, and shall not be used for
** advertising or product endorsement purposes.
*/

#ifndef ZFP_H
#define ZFP_H

#include "vtk_zfp_mangle.h"

#include "zfp/types.h"
#include "zfp/system.h"
#include "bitstream.h"

/* macros ------------------------------------------------------------------ */

/* stringification */
#define _zfp_str_(x) # x
#define _zfp_str(x) _zfp_str_(x)

/* library version information */
#define ZFP_VERSION_MAJOR 0 /* library major version number */
#define ZFP_VERSION_MINOR 5 /* library minor version number */
#define ZFP_VERSION_PATCH 5 /* library patch version number */
#define ZFP_VERSION_RELEASE ZFP_VERSION_PATCH

/* codec version number (see also zfp_codec_version) */
#define ZFP_CODEC 5

/* library version number (see also zfp_library_version) */
#define ZFP_VERSION \
  ((ZFP_VERSION_MAJOR << 8) + \
   (ZFP_VERSION_MINOR << 4) + \
   (ZFP_VERSION_PATCH << 0))

/* library version string (see also zfp_version_string) */
#define ZFP_VERSION_STRING \
  _zfp_str(ZFP_VERSION_MAJOR) "." \
  _zfp_str(ZFP_VERSION_MINOR) "." \
  _zfp_str(ZFP_VERSION_PATCH)

/* default compression parameters */
#define ZFP_MIN_BITS     1 /* minimum number of bits per block */
#define ZFP_MAX_BITS 16657 /* maximum number of bits per block */
#define ZFP_MAX_PREC    64 /* maximum precision supported */
#define ZFP_MIN_EXP  -1074 /* minimum floating-point base-2 exponent */

/* header masks (enable via bitwise or; reader must use same mask) */
#define ZFP_HEADER_MAGIC  0x1u /* embed 64-bit magic */
#define ZFP_HEADER_META   0x2u /* embed 52-bit field metadata */
#define ZFP_HEADER_MODE   0x4u /* embed 12- or 64-bit compression mode */
#define ZFP_HEADER_FULL   0x7u /* embed all of the above */

/* field metadata indeterminate state and error code */
#define ZFP_META_NULL (UINT64C(-1))

/* number of bits per header entry */
#define ZFP_MAGIC_BITS       32 /* number of magic word bits */
#define ZFP_META_BITS        52 /* number of field metadata bits */
#define ZFP_MODE_SHORT_BITS  12 /* number of mode bits in short format */
#define ZFP_MODE_LONG_BITS   64 /* number of mode bits in long format */
#define ZFP_HEADER_MAX_BITS 148 /* max number of header bits */
#define ZFP_MODE_SHORT_MAX  ((1u << ZFP_MODE_SHORT_BITS) - 2)

/* types ------------------------------------------------------------------- */

/* execution policy */
typedef enum {
  zfp_exec_serial = 0, /* serial execution (default) */
  zfp_exec_omp    = 1, /* OpenMP multi-threaded execution */
  zfp_exec_cuda   = 2  /* CUDA parallel execution */
} zfp_exec_policy;

/* OpenMP execution parameters */
typedef struct {
  uint threads;    /* number of requested threads */
  uint chunk_size; /* number of blocks per chunk (1D only) */
} zfp_exec_params_omp;

/* execution parameters */
typedef union {
  zfp_exec_params_omp omp; /* OpenMP parameters */
} zfp_exec_params;

typedef struct {
  zfp_exec_policy policy; /* execution policy (serial, omp, ...) */
  zfp_exec_params params; /* execution parameters */
} zfp_execution;

/* compressed stream; use accessors to get/set members */
typedef struct {
  uint minbits;       /* minimum number of bits to store per block */
  uint maxbits;       /* maximum number of bits to store per block */
  uint maxprec;       /* maximum number of bit planes to store */
  int minexp;         /* minimum floating point bit plane number to store */
  bitstream* stream;  /* compressed bit stream */
  zfp_execution exec; /* execution policy and parameters */
} zfp_stream;

/* compression mode */
typedef enum {
  zfp_mode_null            = 0, /* an invalid configuration of the 4 params */
  zfp_mode_expert          = 1, /* expert mode (4 params set manually) */
  zfp_mode_fixed_rate      = 2, /* fixed rate mode */
  zfp_mode_fixed_precision = 3, /* fixed precision mode */
  zfp_mode_fixed_accuracy  = 4, /* fixed accuracy mode */
  zfp_mode_reversible      = 5  /* reversible (lossless) mode */
} zfp_mode;

/* scalar type */
typedef enum {
  zfp_type_none   = 0, /* unspecified type */
  zfp_type_int32  = 1, /* 32-bit signed integer */
  zfp_type_int64  = 2, /* 64-bit signed integer */
  zfp_type_float  = 3, /* single precision floating point */
  zfp_type_double = 4  /* double precision floating point */
} zfp_type;

/* uncompressed array; use accessors to get/set members */
typedef struct {
  zfp_type type;       /* scalar type (e.g. int32, double) */
  uint nx, ny, nz, nw; /* sizes (zero for unused dimensions) */
  int sx, sy, sz, sw;  /* strides (zero for contiguous array a[nw][nz][ny][nx]) */
  void* data;          /* pointer to array data */
} zfp_field;

#ifdef __cplusplus
extern "C" {
#endif

/* public data ------------------------------------------------------------- */

extern_ const uint zfp_codec_version;         /* codec version ZFP_CODEC */
extern_ const uint zfp_library_version;       /* library version ZFP_VERSION */
extern_ const char* const zfp_version_string; /* verbose version string */

/* high-level API: utility functions --------------------------------------- */

size_t          /* byte size of scalar type */
zfp_type_size(
  zfp_type type /* scalar type */
);

/* high-level API: compressed stream construction/destruction -------------- */

/* open compressed stream and associate with bit stream */
zfp_stream*         /* allocated compressed stream */
zfp_stream_open(
  bitstream* stream /* bit stream to read from and write to (may be NULL) */
);

/* close and deallocate compressed stream (does not affect bit stream) */
void
zfp_stream_close(
  zfp_stream* stream /* compressed stream */
);

/* high-level API: compressed stream inspectors ---------------------------- */

/* bit stream associated with compressed stream */
bitstream*                 /* bit stream associated with compressed stream */
zfp_stream_bit_stream(
  const zfp_stream* stream /* compressed stream */
);

/* returns enum of compression mode */
zfp_mode                   /* enum for compression mode */
zfp_stream_compression_mode(
  const zfp_stream* stream /* compressed stream */
);

/* get all compression parameters in a compact representation */
uint64                     /* 12- or 64-bit encoding of parameters */
zfp_stream_mode(
  const zfp_stream* stream /* compressed stream */
);

/* get all compression parameters (pointers may be NULL) */
void
zfp_stream_params(
  const zfp_stream* stream, /* compressed stream */
  uint* minbits,            /* minimum number of bits per 4^d block */
  uint* maxbits,            /* maximum number of bits per 4^d block */
  uint* maxprec,            /* maximum precision (# bit planes coded) */
  int* minexp               /* minimum base-2 exponent; error <= 2^minexp */
);

/* byte size of sequentially compressed stream (call after compression) */
size_t                     /* actual number of bytes of compressed storage */
zfp_stream_compressed_size(
  const zfp_stream* stream /* compressed stream */
);

/* conservative estimate of compressed size in bytes */
size_t                      /* maximum number of bytes of compressed storage */
zfp_stream_maximum_size(
  const zfp_stream* stream, /* compressed stream */
  const zfp_field* field    /* array to compress */
);

/* high-level API: initialization of compressed stream parameters ---------- */

/* rewind bit stream to beginning for compression or decompression */
void
zfp_stream_rewind(
  zfp_stream* stream /* compressed bit stream */
);

/* associate bit stream with compressed stream */
void
zfp_stream_set_bit_stream(
  zfp_stream* stream, /* compressed stream */
  bitstream* bs       /* bit stream to read from and write to */
);

/* enable reversible (lossless) compression */
void
zfp_stream_set_reversible(
  zfp_stream* stream /* compressed stream */
);

/* set size in compressed bits/scalar (fixed-rate mode) */
double                /* actual rate in compressed bits/scalar */
zfp_stream_set_rate(
  zfp_stream* stream, /* compressed stream */
  double rate,        /* desired rate in compressed bits/scalar */
  zfp_type type,      /* scalar type to compress */
  uint dims,          /* array dimensionality (1, 2, 3, or 4) */
  int wra             /* nonzero if write random access is needed */
);

/* set precision in uncompressed bits/scalar (fixed-precision mode) */
uint                  /* actual precision */
zfp_stream_set_precision(
  zfp_stream* stream, /* compressed stream */
  uint precision      /* desired precision in uncompressed bits/scalar */
);

/* set accuracy as absolute error tolerance (fixed-accuracy mode) */
double                /* actual error tolerance */
zfp_stream_set_accuracy(
  zfp_stream* stream, /* compressed stream */
  double tolerance    /* desired error tolerance */
);

/* set parameters from compact encoding; leaves stream intact on failure */
zfp_mode              /* compression mode or zfp_mode_null upon failure */
zfp_stream_set_mode(
  zfp_stream* stream, /* compressed stream */
  uint64 mode         /* 12- or 64-bit encoding of parameters */
);

/* set all parameters (expert mode); leaves stream intact on failure */
int                   /* nonzero upon success */
zfp_stream_set_params(
  zfp_stream* stream, /* compressed stream */
  uint minbits,       /* minimum number of bits per 4^d block */
  uint maxbits,       /* maximum number of bits per 4^d block */
  uint maxprec,       /* maximum precision (# bit planes coded) */
  int minexp          /* minimum base-2 exponent; error <= 2^minexp */
);

/* high-level API: execution policy ---------------------------------------- */

/* current execution policy */
zfp_exec_policy
zfp_stream_execution(
  const zfp_stream* stream /* compressed stream */
);

/* number of OpenMP threads to use */
uint                       /* number of threads (0 for default) */
zfp_stream_omp_threads(
  const zfp_stream* stream /* compressed stream */
);

/* number of blocks per OpenMP chunk (1D only) */
uint                       /* number of blocks per chunk (0 for default) */
zfp_stream_omp_chunk_size(
  const zfp_stream* stream /* compressed stream */
);

/* set execution policy */
int                      /* nonzero upon success */
zfp_stream_set_execution(
  zfp_stream* stream,    /* compressed stream */
  zfp_exec_policy policy /* execution policy */
);

/* set OpenMP execution policy and number of threads */
int                   /* nonzero upon success */
zfp_stream_set_omp_threads(
  zfp_stream* stream, /* compressed stream */
  uint threads        /* number of OpenMP threads to use (0 for default) */
);

/* set OpenMP execution policy and number of blocks per chunk (1D only) */
int                   /* nonzero upon success */
zfp_stream_set_omp_chunk_size(
  zfp_stream* stream, /* compressed stream */
  uint chunk_size     /* number of blocks per chunk (0 for default) */
);

/* high-level API: uncompressed array construction/destruction ------------- */

/* allocate field struct */
zfp_field* /* pointer to default initialized field */
zfp_field_alloc();

/* allocate metadata for 1D field f[nx] */
zfp_field*       /* allocated field metadata */
zfp_field_1d(
  void* pointer, /* pointer to uncompressed scalars (may be NULL) */
  zfp_type type, /* scalar type */
  uint nx        /* number of scalars */
);

/* allocate metadata for 2D field f[ny][nx] */
zfp_field*       /* allocated field metadata */
zfp_field_2d(
  void* pointer, /* pointer to uncompressed scalars (may be NULL) */
  zfp_type type, /* scalar type */
  uint nx,       /* number of scalars in x dimension */
  uint ny        /* number of scalars in y dimension */
);

/* allocate metadata for 3D field f[nz][ny][nx] */
zfp_field*       /* allocated field metadata */
zfp_field_3d(
  void* pointer, /* pointer to uncompressed scalars (may be NULL) */
  zfp_type type, /* scalar type */
  uint nx,       /* number of scalars in x dimension */
  uint ny,       /* number of scalars in y dimension */
  uint nz        /* number of scalars in z dimension */
);

/* allocate metadata for 4D field f[nw][nz][ny][nx] */
zfp_field*       /* allocated field metadata */
zfp_field_4d(
  void* pointer, /* pointer to uncompressed scalars (may be NULL) */
  zfp_type type, /* scalar type */
  uint nx,       /* number of scalars in x dimension */
  uint ny,       /* number of scalars in y dimension */
  uint nz,       /* number of scalars in z dimension */
  uint nw        /* number of scalars in w dimension */
);

/* deallocate field metadata */
void
zfp_field_free(
  zfp_field* field /* field metadata */
);

/* high-level API: uncompressed array inspectors --------------------------- */

/* pointer to first scalar in field */
void*                    /* array pointer */
zfp_field_pointer(
  const zfp_field* field /* field metadata */
);

/* field scalar type */
zfp_type                 /* scalar type */
zfp_field_type(
  const zfp_field* field /* field metadata */
);

/* precision of field scalar type */
uint                     /* scalar type precision in number of bits */
zfp_field_precision(
  const zfp_field* field /* field metadata */
);

/* field dimensionality (1, 2, or 3) */
uint                     /* number of dimensions */
zfp_field_dimensionality(
  const zfp_field* field /* field metadata */
);

/* field size in number of scalars */
size_t                    /* total number of scalars */
zfp_field_size(
  const zfp_field* field, /* field metadata */
  uint* size              /* number of scalars per dimension (may be NULL) */
);

/* field strides per dimension */
int                       /* zero if array is contiguous */
zfp_field_stride(
  const zfp_field* field, /* field metadata */
  int* stride             /* stride in scalars per dimension (may be NULL) */
);

/* field scalar type and dimensions */
uint64                   /* compact 52-bit encoding of metadata */
zfp_field_metadata(
  const zfp_field* field /* field metadata */
);

/* high-level API: uncompressed array specification ------------------------ */

/* set pointer to first scalar in field */
void
zfp_field_set_pointer(
  zfp_field* field, /* field metadata */
  void* pointer     /* pointer to first scalar */
);

/* set field scalar type */
zfp_type            /* actual scalar type */
zfp_field_set_type(
  zfp_field* field, /* field metadata */
  zfp_type type     /* desired scalar type */
);

/* set 1D field size */
void
zfp_field_set_size_1d(
  zfp_field* field, /* field metadata */
  uint nx           /* number of scalars */
);

/* set 2D field size */
void
zfp_field_set_size_2d(
  zfp_field* field, /* field metadata */
  uint nx,          /* number of scalars in x dimension */
  uint ny           /* number of scalars in y dimension */
);

/* set 3D field size */
void
zfp_field_set_size_3d(
  zfp_field* field, /* field metadata */
  uint nx,          /* number of scalars in x dimension */
  uint ny,          /* number of scalars in y dimension */
  uint nz           /* number of scalars in z dimension */
);

/* set 4D field size */
void
zfp_field_set_size_4d(
  zfp_field* field, /* field metadata */
  uint nx,          /* number of scalars in x dimension */
  uint ny,          /* number of scalars in y dimension */
  uint nz,          /* number of scalars in z dimension */
  uint nw           /* number of scalars in w dimension */
);

/* set 1D field stride in number of scalars */
void
zfp_field_set_stride_1d(
  zfp_field* field, /* field metadata */
  int sx            /* stride in number of scalars: &f[1] - &f[0] */
);

/* set 2D field strides in number of scalars */
void
zfp_field_set_stride_2d(
  zfp_field* field, /* field metadata */
  int sx,           /* stride in x dimension: &f[0][1] - &f[0][0] */
  int sy            /* stride in y dimension: &f[1][0] - &f[0][0] */
);

/* set 3D field strides in number of scalars */
void
zfp_field_set_stride_3d(
  zfp_field* field, /* field metadata */
  int sx,           /* stride in x dimension: &f[0][0][1] - &f[0][0][0] */
  int sy,           /* stride in y dimension: &f[0][1][0] - &f[0][0][0] */
  int sz            /* stride in z dimension: &f[1][0][0] - &f[0][0][0] */
);

/* set 4D field strides in number of scalars */
void
zfp_field_set_stride_4d(
  zfp_field* field, /* field metadata */
  int sx,           /* stride in x dimension: &f[0][0][0][1] - &f[0][0][0][0] */
  int sy,           /* stride in y dimension: &f[0][0][1][0] - &f[0][0][0][0] */
  int sz,           /* stride in z dimension: &f[0][1][0][0] - &f[0][0][0][0] */
  int sw            /* stride in w dimension: &f[1][0][0][0] - &f[0][0][0][0] */
);

/* set field scalar type and dimensions */
int                 /* nonzero upon success */
zfp_field_set_metadata(
  zfp_field* field, /* field metadata */
  uint64 meta       /* compact 52-bit encoding of metadata */
);

/* high-level API: compression and decompression --------------------------- */

/* compress entire field (nonzero return value upon success) */
size_t                   /* cumulative number of bytes of compressed storage */
zfp_compress(
  zfp_stream* stream,    /* compressed stream */
  const zfp_field* field /* field metadata */
);

/* decompress entire field (nonzero return value upon success) */
size_t                /* cumulative number of bytes of compressed storage */
zfp_decompress(
  zfp_stream* stream, /* compressed stream */
  zfp_field* field    /* field metadata */
);

/* write compression parameters and field metadata (optional) */
size_t                    /* number of bits written or zero upon failure */
zfp_write_header(
  zfp_stream* stream,     /* compressed stream */
  const zfp_field* field, /* field metadata */
  uint mask               /* information to write */
);

/* read compression parameters and field metadata when previously written */
size_t                /* number of bits read or zero upon failure */
zfp_read_header(
  zfp_stream* stream, /* compressed stream */
  zfp_field* field,   /* field metadata */
  uint mask           /* information to read */
);

/* low-level API: stream manipulation -------------------------------------- */

/* flush bit stream--must be called after last encode call or between seeks */
size_t
zfp_stream_flush(
  zfp_stream* stream /* compressed bit stream */
);

/* align bit stream on next word boundary (decoding analogy to flush) */
size_t
zfp_stream_align(
  zfp_stream* stream /* compressed bit stream */
);

/* low-level API: encoder -------------------------------------------------- */

/*
The functions below all compress either a complete contiguous d-dimensional
block of 4^d scalars or a complete or partial block assembled from a strided
array.  In the latter case, p points to the first scalar; (nx, ny, nz) specify
the size of the block, with 1 <= nx, ny, nz <= 4; and (sx, sy, sz) specify the
strides, i.e. the number of scalars to advance to get to the next scalar along
each dimension.  The functions return the number of bits of compressed storage
needed for the compressed block.
*/

/* encode 1D contiguous block of 4 values */
uint zfp_encode_block_int32_1(zfp_stream* stream, const int32* block);
uint zfp_encode_block_int64_1(zfp_stream* stream, const int64* block);
uint zfp_encode_block_float_1(zfp_stream* stream, const float* block);
uint zfp_encode_block_double_1(zfp_stream* stream, const double* block);

/* encode 1D complete or partial block from strided array */
uint zfp_encode_block_strided_int32_1(zfp_stream* stream, const int32* p, int sx);
uint zfp_encode_block_strided_int64_1(zfp_stream* stream, const int64* p, int sx);
uint zfp_encode_block_strided_float_1(zfp_stream* stream, const float* p, int sx);
uint zfp_encode_block_strided_double_1(zfp_stream* stream, const double* p, int sx);
uint zfp_encode_partial_block_strided_int32_1(zfp_stream* stream, const int32* p, uint nx, int sx);
uint zfp_encode_partial_block_strided_int64_1(zfp_stream* stream, const int64* p, uint nx, int sx);
uint zfp_encode_partial_block_strided_float_1(zfp_stream* stream, const float* p, uint nx, int sx);
uint zfp_encode_partial_block_strided_double_1(zfp_stream* stream, const double* p, uint nx, int sx);

/* encode 2D contiguous block of 4x4 values */
uint zfp_encode_block_int32_2(zfp_stream* stream, const int32* block);
uint zfp_encode_block_int64_2(zfp_stream* stream, const int64* block);
uint zfp_encode_block_float_2(zfp_stream* stream, const float* block);
uint zfp_encode_block_double_2(zfp_stream* stream, const double* block);

/* encode 2D complete or partial block from strided array */
uint zfp_encode_partial_block_strided_int32_2(zfp_stream* stream, const int32* p, uint nx, uint ny, int sx, int sy);
uint zfp_encode_partial_block_strided_int64_2(zfp_stream* stream, const int64* p, uint nx, uint ny, int sx, int sy);
uint zfp_encode_partial_block_strided_float_2(zfp_stream* stream, const float* p, uint nx, uint ny, int sx, int sy);
uint zfp_encode_partial_block_strided_double_2(zfp_stream* stream, const double* p, uint nx, uint ny, int sx, int sy);
uint zfp_encode_block_strided_int32_2(zfp_stream* stream, const int32* p, int sx, int sy);
uint zfp_encode_block_strided_int64_2(zfp_stream* stream, const int64* p, int sx, int sy);
uint zfp_encode_block_strided_float_2(zfp_stream* stream, const float* p, int sx, int sy);
uint zfp_encode_block_strided_double_2(zfp_stream* stream, const double* p, int sx, int sy);

/* encode 3D contiguous block of 4x4x4 values */
uint zfp_encode_block_int32_3(zfp_stream* stream, const int32* block);
uint zfp_encode_block_int64_3(zfp_stream* stream, const int64* block);
uint zfp_encode_block_float_3(zfp_stream* stream, const float* block);
uint zfp_encode_block_double_3(zfp_stream* stream, const double* block);

/* encode 3D complete or partial block from strided array */
uint zfp_encode_block_strided_int32_3(zfp_stream* stream, const int32* p, int sx, int sy, int sz);
uint zfp_encode_block_strided_int64_3(zfp_stream* stream, const int64* p, int sx, int sy, int sz);
uint zfp_encode_block_strided_float_3(zfp_stream* stream, const float* p, int sx, int sy, int sz);
uint zfp_encode_block_strided_double_3(zfp_stream* stream, const double* p, int sx, int sy, int sz);
uint zfp_encode_partial_block_strided_int32_3(zfp_stream* stream, const int32* p, uint nx, uint ny, uint nz, int sx, int sy, int sz);
uint zfp_encode_partial_block_strided_int64_3(zfp_stream* stream, const int64* p, uint nx, uint ny, uint nz, int sx, int sy, int sz);
uint zfp_encode_partial_block_strided_float_3(zfp_stream* stream, const float* p, uint nx, uint ny, uint nz, int sx, int sy, int sz);
uint zfp_encode_partial_block_strided_double_3(zfp_stream* stream, const double* p, uint nx, uint ny, uint nz, int sx, int sy, int sz);

/* encode 4D contiguous block of 4x4x4x4 values */
uint zfp_encode_block_int32_4(zfp_stream* stream, const int32* block);
uint zfp_encode_block_int64_4(zfp_stream* stream, const int64* block);
uint zfp_encode_block_float_4(zfp_stream* stream, const float* block);
uint zfp_encode_block_double_4(zfp_stream* stream, const double* block);

/* encode 4D complete or partial block from strided array */
uint zfp_encode_block_strided_int32_4(zfp_stream* stream, const int32* p, int sx, int sy, int sz, int sw);
uint zfp_encode_block_strided_int64_4(zfp_stream* stream, const int64* p, int sx, int sy, int sz, int sw);
uint zfp_encode_block_strided_float_4(zfp_stream* stream, const float* p, int sx, int sy, int sz, int sw);
uint zfp_encode_block_strided_double_4(zfp_stream* stream, const double* p, int sx, int sy, int sz, int sw);
uint zfp_encode_partial_block_strided_int32_4(zfp_stream* stream, const int32* p, uint nx, uint ny, uint nz, uint nw, int sx, int sy, int sz, int sw);
uint zfp_encode_partial_block_strided_int64_4(zfp_stream* stream, const int64* p, uint nx, uint ny, uint nz, uint nw, int sx, int sy, int sz, int sw);
uint zfp_encode_partial_block_strided_float_4(zfp_stream* stream, const float* p, uint nx, uint ny, uint nz, uint nw, int sx, int sy, int sz, int sw);
uint zfp_encode_partial_block_strided_double_4(zfp_stream* stream, const double* p, uint nx, uint ny, uint nz, uint nw, int sx, int sy, int sz, int sw);

/* low-level API: decoder -------------------------------------------------- */

/*
Each function below decompresses a single block and returns the number of bits
of compressed storage consumed.  See corresponding encoder functions above for
further details.
*/

/* decode 1D contiguous block of 4 values */
uint zfp_decode_block_int32_1(zfp_stream* stream, int32* block);
uint zfp_decode_block_int64_1(zfp_stream* stream, int64* block);
uint zfp_decode_block_float_1(zfp_stream* stream, float* block);
uint zfp_decode_block_double_1(zfp_stream* stream, double* block);

/* decode 1D complete or partial block from strided array */
uint zfp_decode_block_strided_int32_1(zfp_stream* stream, int32* p, int sx);
uint zfp_decode_block_strided_int64_1(zfp_stream* stream, int64* p, int sx);
uint zfp_decode_block_strided_float_1(zfp_stream* stream, float* p, int sx);
uint zfp_decode_block_strided_double_1(zfp_stream* stream, double* p, int sx);
uint zfp_decode_partial_block_strided_int32_1(zfp_stream* stream, int32* p, uint nx, int sx);
uint zfp_decode_partial_block_strided_int64_1(zfp_stream* stream, int64* p, uint nx, int sx);
uint zfp_decode_partial_block_strided_float_1(zfp_stream* stream, float* p, uint nx, int sx);
uint zfp_decode_partial_block_strided_double_1(zfp_stream* stream, double* p, uint nx, int sx);

/* decode 2D contiguous block of 4x4 values */
uint zfp_decode_block_int32_2(zfp_stream* stream, int32* block);
uint zfp_decode_block_int64_2(zfp_stream* stream, int64* block);
uint zfp_decode_block_float_2(zfp_stream* stream, float* block);
uint zfp_decode_block_double_2(zfp_stream* stream, double* block);

/* decode 2D complete or partial block from strided array */
uint zfp_decode_block_strided_int32_2(zfp_stream* stream, int32* p, int sx, int sy);
uint zfp_decode_block_strided_int64_2(zfp_stream* stream, int64* p, int sx, int sy);
uint zfp_decode_block_strided_float_2(zfp_stream* stream, float* p, int sx, int sy);
uint zfp_decode_block_strided_double_2(zfp_stream* stream, double* p, int sx, int sy);
uint zfp_decode_partial_block_strided_int32_2(zfp_stream* stream, int32* p, uint nx, uint ny, int sx, int sy);
uint zfp_decode_partial_block_strided_int64_2(zfp_stream* stream, int64* p, uint nx, uint ny, int sx, int sy);
uint zfp_decode_partial_block_strided_float_2(zfp_stream* stream, float* p, uint nx, uint ny, int sx, int sy);
uint zfp_decode_partial_block_strided_double_2(zfp_stream* stream, double* p, uint nx, uint ny, int sx, int sy);

/* decode 3D contiguous block of 4x4x4 values */
uint zfp_decode_block_int32_3(zfp_stream* stream, int32* block);
uint zfp_decode_block_int64_3(zfp_stream* stream, int64* block);
uint zfp_decode_block_float_3(zfp_stream* stream, float* block);
uint zfp_decode_block_double_3(zfp_stream* stream, double* block);

/* decode 3D complete or partial block from strided array */
uint zfp_decode_block_strided_int32_3(zfp_stream* stream, int32* p, int sx, int sy, int sz);
uint zfp_decode_block_strided_int64_3(zfp_stream* stream, int64* p, int sx, int sy, int sz);
uint zfp_decode_block_strided_float_3(zfp_stream* stream, float* p, int sx, int sy, int sz);
uint zfp_decode_block_strided_double_3(zfp_stream* stream, double* p, int sx, int sy, int sz);
uint zfp_decode_partial_block_strided_int32_3(zfp_stream* stream, int32* p, uint nx, uint ny, uint nz, int sx, int sy, int sz);
uint zfp_decode_partial_block_strided_int64_3(zfp_stream* stream, int64* p, uint nx, uint ny, uint nz, int sx, int sy, int sz);
uint zfp_decode_partial_block_strided_float_3(zfp_stream* stream, float* p, uint nx, uint ny, uint nz, int sx, int sy, int sz);
uint zfp_decode_partial_block_strided_double_3(zfp_stream* stream, double* p, uint nx, uint ny, uint nz, int sx, int sy, int sz);

/* decode 4D contiguous block of 4x4x4x4 values */
uint zfp_decode_block_int32_4(zfp_stream* stream, int32* block);
uint zfp_decode_block_int64_4(zfp_stream* stream, int64* block);
uint zfp_decode_block_float_4(zfp_stream* stream, float* block);
uint zfp_decode_block_double_4(zfp_stream* stream, double* block);

/* decode 4D complete or partial block from strided array */
uint zfp_decode_block_strided_int32_4(zfp_stream* stream, int32* p, int sx, int sy, int sz, int sw);
uint zfp_decode_block_strided_int64_4(zfp_stream* stream, int64* p, int sx, int sy, int sz, int sw);
uint zfp_decode_block_strided_float_4(zfp_stream* stream, float* p, int sx, int sy, int sz, int sw);
uint zfp_decode_block_strided_double_4(zfp_stream* stream, double* p, int sx, int sy, int sz, int sw);
uint zfp_decode_partial_block_strided_int32_4(zfp_stream* stream, int32* p, uint nx, uint ny, uint nz, uint nw, int sx, int sy, int sz, int sw);
uint zfp_decode_partial_block_strided_int64_4(zfp_stream* stream, int64* p, uint nx, uint ny, uint nz, uint nw, int sx, int sy, int sz, int sw);
uint zfp_decode_partial_block_strided_float_4(zfp_stream* stream, float* p, uint nx, uint ny, uint nz, uint nw, int sx, int sy, int sz, int sw);
uint zfp_decode_partial_block_strided_double_4(zfp_stream* stream, double* p, uint nx, uint ny, uint nz, uint nw, int sx, int sy, int sz, int sw);

/* low-level API: utility functions ---------------------------------------- */

/* convert dims-dimensional contiguous block to 32-bit integer type */
void zfp_promote_int8_to_int32(int32* oblock, const int8* iblock, uint dims);
void zfp_promote_uint8_to_int32(int32* oblock, const uint8* iblock, uint dims);
void zfp_promote_int16_to_int32(int32* oblock, const int16* iblock, uint dims);
void zfp_promote_uint16_to_int32(int32* oblock, const uint16* iblock, uint dims);

/* convert dims-dimensional contiguous block from 32-bit integer type */
void zfp_demote_int32_to_int8(int8* oblock, const int32* iblock, uint dims);
void zfp_demote_int32_to_uint8(uint8* oblock, const int32* iblock, uint dims);
void zfp_demote_int32_to_int16(int16* oblock, const int32* iblock, uint dims);
void zfp_demote_int32_to_uint16(uint16* oblock, const int32* iblock, uint dims);

#ifdef __cplusplus
}
#endif

#endif
