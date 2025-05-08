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
#ifndef viskores_io_internal_Endian_h
#define viskores_io_internal_Endian_h

#include <viskores/Types.h>

#include <algorithm>
#include <vector>

namespace viskores
{
namespace io
{
namespace internal
{

inline bool IsLittleEndian()
{
  static constexpr viskores::Int16 i16 = 0x1;
  const viskores::Int8* i8p = reinterpret_cast<const viskores::Int8*>(&i16);
  return (*i8p == 1);
}

template <typename T>
inline void FlipEndianness(std::vector<T>& buffer)
{
  viskores::UInt8* bytes = reinterpret_cast<viskores::UInt8*>(&buffer[0]);
  const std::size_t tsize = sizeof(T);
  const std::size_t bsize = buffer.size();
  for (std::size_t i = 0; i < bsize; i++, bytes += tsize)
  {
    std::reverse(bytes, bytes + tsize);
  }
}

template <typename T, viskores::IdComponent N>
inline void FlipEndianness(std::vector<viskores::Vec<T, N>>& buffer)
{
  viskores::UInt8* bytes = reinterpret_cast<viskores::UInt8*>(&buffer[0]);
  const std::size_t tsize = sizeof(T);
  const std::size_t bsize = buffer.size();
  for (std::size_t i = 0; i < bsize; i++)
  {
    for (viskores::IdComponent j = 0; j < N; j++, bytes += tsize)
    {
      std::reverse(bytes, bytes + tsize);
    }
  }
}
}
}
} // viskores::io::internal

#endif //viskores_io_internal_Endian_h
