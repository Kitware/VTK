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
#ifndef viskores_worklet_zfp_functions_h
#define viskores_worklet_zfp_functions_h

#include <viskores/Math.h>
#include <viskores/filter/zfp/worklet/zfp/ZFPBlockWriter.h>
#include <viskores/filter/zfp/worklet/zfp/ZFPCodec.h>
#include <viskores/filter/zfp/worklet/zfp/ZFPTypeInfo.h>

namespace viskores
{
namespace worklet
{
namespace zfp
{

template <typename T>
void PrintBits(T bits)
{
  const int bit_size = sizeof(T) * 8;
  for (int i = bit_size - 1; i >= 0; --i)
  {
    T one = 1;
    T mask = one << i;
    int val = (bits & mask) >> T(i);
    printf("%d", val);
  }
  printf("\n");
}

template <typename T>
inline viskores::UInt32 MinBits(const viskores::UInt32 bits)
{
  return bits;
}

template <>
inline viskores::UInt32 MinBits<viskores::Float32>(const viskores::UInt32 bits)
{
  return viskores::Max(bits, 1 + 8u);
}

template <>
inline viskores::UInt32 MinBits<viskores::Float64>(const viskores::UInt32 bits)
{
  return viskores::Max(bits, 1 + 11u);
}




} // namespace zfp
} // namespace worklet
} // namespace viskores
#endif //  viskores_worklet_zfp_type_info_h
