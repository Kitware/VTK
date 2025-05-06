//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_worklet_zfp_structs_h
#define viskores_worklet_zfp_structs_h

#define ZFP_MIN_BITS 0    /* minimum number of bits per block */
#define ZFP_MAX_BITS 4171 /* maximum number of bits per block */
#define ZFP_MAX_PREC 64   /* maximum precision supported */
#define ZFP_MIN_EXP -1074 /* minimum floating-point base-2 exponent */

#include <viskores/filter/zfp/worklet/zfp/ZFPFunctions.h>
#include <viskores/filter/zfp/worklet/zfp/ZFPTypeInfo.h>

namespace viskores
{
namespace worklet
{
namespace zfp
{

struct ZFPStream
{
  viskores::UInt32 minbits;
  viskores::UInt32 maxbits;
  viskores::UInt32 maxprec;
  viskores::Int32 minexp;

  template <typename T>
  viskores::Float64 SetRate(const viskores::Float64 rate,
                            const viskores::Int32 dims,
                            T viskoresNotUsed(valueType))
  {
    viskores::UInt32 n = 1u << (2 * dims);
    viskores::UInt32 bits = (unsigned int)floor(n * rate + 0.5);
    bits = zfp::MinBits<T>(bits);
    //if (wra) {
    //  /* for write random access, round up to next multiple of stream word size */
    //  bits += (uint)stream_word_bits - 1;
    //  bits &= ~(stream_word_bits - 1);
    //}
    minbits = bits;
    maxbits = bits;
    maxprec = ZFP_MAX_PREC;
    minexp = ZFP_MIN_EXP;
    return (double)bits / n;
  }
};
}
}
} // namespace viskores::worklet::zfp
#endif
