#ifdef ZFP_WITH_CUDA

#include "../cuda_zfp/cuZFP.h"

static void
_t2(decompress_cuda, Scalar, 1)(zfp_stream* stream, zfp_field* field)
{
  if(zfp_stream_compression_mode(stream) == zfp_mode_fixed_rate)
  {
    cuda_decompress(stream, field);   
  }
}

/* compress 1d strided array */
static void
_t2(decompress_strided_cuda, Scalar, 1)(zfp_stream* stream, zfp_field* field)
{
  if(zfp_stream_compression_mode(stream) == zfp_mode_fixed_rate)
  {
    cuda_decompress(stream, field);   
  }
}

/* compress 2d strided array */
static void
_t2(decompress_strided_cuda, Scalar, 2)(zfp_stream* stream, zfp_field* field)
{
  if(zfp_stream_compression_mode(stream) == zfp_mode_fixed_rate)
  {
    cuda_decompress(stream, field);   
  }
}

/* compress 3d strided array */
static void
_t2(decompress_strided_cuda, Scalar, 3)(zfp_stream* stream, zfp_field* field)
{
  if(zfp_stream_compression_mode(stream) == zfp_mode_fixed_rate)
  {
    cuda_decompress(stream, field);   
  }
}

#endif
