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
#ifndef viskores_worklet_zfp_codec_h
#define viskores_worklet_zfp_codec_h

#define index3(i, j, k) ((i) + 4 * ((j) + 4 * (k)))
#define index2(i, j) ((i) + 4 * (j))

#include <viskores/Types.h>
#include <viskores/internal/ExportMacros.h>

namespace viskores
{
namespace worklet
{
namespace zfp
{

template <viskores::Int32 BlockSize>
struct ZFPCodec;

template <>
struct ZFPCodec<4>
{
  VISKORES_EXEC_CONT ZFPCodec() {}
  VISKORES_EXEC viskores::UInt8 CodecLookup(viskores::Int32 x) const
  {
    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::UInt8 perm_1[4] = { 0, 1, 2, 3 };
    return perm_1[x];
  }
};

template <>
struct ZFPCodec<16>
{
  VISKORES_EXEC_CONT ZFPCodec() {}
  VISKORES_EXEC viskores::UInt8 CodecLookup(viskores::Int32 x) const
  {
    /* order coefficients (i, j) by i + j, then i^2 + j^2 */
    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::UInt8 perm_2[16] = {
      index2(0, 0), /*  0 : 0 */

      index2(1, 0), /*  1 : 1 */
      index2(0, 1), /*  2 : 1 */

      index2(1, 1), /*  3 : 2 */

      index2(2, 0), /*  4 : 2 */
      index2(0, 2), /*  5 : 2 */

      index2(2, 1), /*  6 : 3 */
      index2(1, 2), /*  7 : 3 */

      index2(3, 0), /*  8 : 3 */
      index2(0, 3), /*  9 : 3 */

      index2(2, 2), /* 10 : 4 */

      index2(3, 1), /* 11 : 4 */
      index2(1, 3), /* 12 : 4 */

      index2(3, 2), /* 13 : 5 */
      index2(2, 3), /* 14 : 5 */

      index2(3, 3), /* 15 : 6 */
    };
    return perm_2[x];
  }
};

template <>
struct ZFPCodec<64>
{
  VISKORES_EXEC_CONT ZFPCodec() {}
  VISKORES_EXEC viskores::UInt8 CodecLookup(viskores::Int32 x) const
  {
    /* order coefficients (i, j, k) by i + j + k, then i^2 + j^2 + k^2 */
    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::UInt8 perm_3[64] = {
      index3(0, 0, 0), /*  0 : 0 */

      index3(1, 0, 0), /*  1 : 1 */
      index3(0, 1, 0), /*  2 : 1 */
      index3(0, 0, 1), /*  3 : 1 */

      index3(0, 1, 1), /*  4 : 2 */
      index3(1, 0, 1), /*  5 : 2 */
      index3(1, 1, 0), /*  6 : 2 */

      index3(2, 0, 0), /*  7 : 2 */
      index3(0, 2, 0), /*  8 : 2 */
      index3(0, 0, 2), /*  9 : 2 */

      index3(1, 1, 1), /* 10 : 3 */

      index3(2, 1, 0), /* 11 : 3 */
      index3(2, 0, 1), /* 12 : 3 */
      index3(0, 2, 1), /* 13 : 3 */
      index3(1, 2, 0), /* 14 : 3 */
      index3(1, 0, 2), /* 15 : 3 */
      index3(0, 1, 2), /* 16 : 3 */

      index3(3, 0, 0), /* 17 : 3 */
      index3(0, 3, 0), /* 18 : 3 */
      index3(0, 0, 3), /* 19 : 3 */

      index3(2, 1, 1), /* 20 : 4 */
      index3(1, 2, 1), /* 21 : 4 */
      index3(1, 1, 2), /* 22 : 4 */

      index3(0, 2, 2), /* 23 : 4 */
      index3(2, 0, 2), /* 24 : 4 */
      index3(2, 2, 0), /* 25 : 4 */

      index3(3, 1, 0), /* 26 : 4 */
      index3(3, 0, 1), /* 27 : 4 */
      index3(0, 3, 1), /* 28 : 4 */
      index3(1, 3, 0), /* 29 : 4 */
      index3(1, 0, 3), /* 30 : 4 */
      index3(0, 1, 3), /* 31 : 4 */

      index3(1, 2, 2), /* 32 : 5 */
      index3(2, 1, 2), /* 33 : 5 */
      index3(2, 2, 1), /* 34 : 5 */

      index3(3, 1, 1), /* 35 : 5 */
      index3(1, 3, 1), /* 36 : 5 */
      index3(1, 1, 3), /* 37 : 5 */

      index3(3, 2, 0), /* 38 : 5 */
      index3(3, 0, 2), /* 39 : 5 */
      index3(0, 3, 2), /* 40 : 5 */
      index3(2, 3, 0), /* 41 : 5 */
      index3(2, 0, 3), /* 42 : 5 */
      index3(0, 2, 3), /* 43 : 5 */

      index3(2, 2, 2), /* 44 : 6 */

      index3(3, 2, 1), /* 45 : 6 */
      index3(3, 1, 2), /* 46 : 6 */
      index3(1, 3, 2), /* 47 : 6 */
      index3(2, 3, 1), /* 48 : 6 */
      index3(2, 1, 3), /* 49 : 6 */
      index3(1, 2, 3), /* 50 : 6 */

      index3(0, 3, 3), /* 51 : 6 */
      index3(3, 0, 3), /* 52 : 6 */
      index3(3, 3, 0), /* 53 : 6 */

      index3(3, 2, 2), /* 54 : 7 */
      index3(2, 3, 2), /* 55 : 7 */
      index3(2, 2, 3), /* 56 : 7 */

      index3(1, 3, 3), /* 57 : 7 */
      index3(3, 1, 3), /* 58 : 7 */
      index3(3, 3, 1), /* 59 : 7 */

      index3(2, 3, 3), /* 60 : 8 */
      index3(3, 2, 3), /* 61 : 8 */
      index3(3, 3, 2), /* 62 : 8 */

      index3(3, 3, 3), /* 63 : 9 */
    };
    return perm_3[x];
  }
};

#undef index3
#undef index2
}
}
} // namespace viskores::worklet::zfp
#endif
