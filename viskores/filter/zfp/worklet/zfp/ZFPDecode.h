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
#ifndef viskores_worklet_zfp_decode_h
#define viskores_worklet_zfp_decode_h

#include <viskores/Types.h>
#include <viskores/filter/zfp/worklet/zfp/ZFPBlockReader.h>
#include <viskores/filter/zfp/worklet/zfp/ZFPCodec.h>
#include <viskores/filter/zfp/worklet/zfp/ZFPTypeInfo.h>
#include <viskores/internal/ExportMacros.h>

namespace viskores
{
namespace worklet
{
namespace zfp
{


template <typename Int, typename Scalar>
inline VISKORES_EXEC Scalar dequantize(const Int& x, const int& e);

template <>
inline VISKORES_EXEC viskores::Float64 dequantize<viskores::Int64, viskores::Float64>(
  const viskores::Int64& x,
  const viskores::Int32& e)
{
  return viskores::Ldexp((viskores::Float64)x,
                         e - (CHAR_BIT * scalar_sizeof<viskores::Float64>() - 2));
}

template <>
inline VISKORES_EXEC viskores::Float32 dequantize<viskores::Int32, viskores::Float32>(
  const viskores::Int32& x,
  const viskores::Int32& e)
{
  return viskores::Ldexp((viskores::Float32)x,
                         e - (CHAR_BIT * scalar_sizeof<viskores::Float32>() - 2));
}

template <>
inline VISKORES_EXEC viskores::Int32 dequantize<viskores::Int32, viskores::Int32>(
  const viskores::Int32&,
  const viskores::Int32&)
{
  return 1;
}

template <>
inline VISKORES_EXEC viskores::Int64 dequantize<viskores::Int64, viskores::Int64>(
  const viskores::Int64&,
  const viskores::Int32&)
{
  return 1;
}

template <class Int, viskores::UInt32 s>
VISKORES_EXEC static void inv_lift(Int* p)
{
  Int x, y, z, w;
  x = *p;
  p += s;
  y = *p;
  p += s;
  z = *p;
  p += s;
  w = *p;

  /*
  ** non-orthogonal transform
  **       ( 4  6 -4 -1) (x)
  ** 1/4 * ( 4  2  4  5) (y)
  **       ( 4 -2  4 -5) (z)
  **       ( 4 -6 -4  1) (w)
  */
  y += w >> 1;
  w -= y >> 1;
  y += w;
  w <<= 1;
  w -= y;
  z += x;
  x <<= 1;
  x -= z;
  y += z;
  z <<= 1;
  z -= y;
  w += x;
  x <<= 1;
  x -= w;

  *p = w;
  p -= s;
  *p = z;
  p -= s;
  *p = y;
  p -= s;
  *p = x;
}

template <viskores::Int64 BlockSize>
struct inv_transform;

template <>
struct inv_transform<64>
{
  template <typename Int>
  VISKORES_EXEC void inv_xform(Int* p)
  {
    unsigned int x, y, z;
    /* transform along z */
    for (y = 0; y < 4; y++)
      for (x = 0; x < 4; x++)
        inv_lift<Int, 16>(p + 1 * x + 4 * y);
    /* transform along y */
    for (x = 0; x < 4; x++)
      for (z = 0; z < 4; z++)
        inv_lift<Int, 4>(p + 16 * z + 1 * x);
    /* transform along x */
    for (z = 0; z < 4; z++)
      for (y = 0; y < 4; y++)
        inv_lift<Int, 1>(p + 4 * y + 16 * z);
  }
};

template <>
struct inv_transform<16>
{
  template <typename Int>
  VISKORES_EXEC void inv_xform(Int* p)
  {

    for (int x = 0; x < 4; ++x)
    {
      inv_lift<Int, 4>(p + 1 * x);
    }
    for (int y = 0; y < 4; ++y)
    {
      inv_lift<Int, 1>(p + 4 * y);
    }
  }
};

template <>
struct inv_transform<4>
{
  template <typename Int>
  VISKORES_EXEC void inv_xform(Int* p)
  {
    inv_lift<Int, 1>(p);
  }
};



inline VISKORES_EXEC viskores::Int64 uint2int(viskores::UInt64 x)
{
  return static_cast<viskores::Int64>((x ^ 0xaaaaaaaaaaaaaaaaull) - 0xaaaaaaaaaaaaaaaaull);
}


inline VISKORES_EXEC viskores::Int32 uint2int(viskores::UInt32 x)
{
  return static_cast<viskores::Int32>((x ^ 0xaaaaaaaau) - 0xaaaaaaaau);
}

// Note: I die a little inside everytime I write this sort of template
template <viskores::Int32 BlockSize,
          typename PortalType,
          template <int Size, typename Portal>
          class ReaderType,
          typename UInt>
VISKORES_EXEC void decode_ints(ReaderType<BlockSize, PortalType>& reader,
                               viskores::Int32& maxbits,
                               UInt* data,
                               const viskores::Int32 intprec)
{
  for (viskores::Int32 i = 0; i < BlockSize; ++i)
  {
    data[i] = 0;
  }

  viskores::UInt64 x;
  const viskores::UInt32 kmin = 0;
  viskores::Int32 bits = maxbits;
  for (viskores::UInt32 k = static_cast<viskores::UInt32>(intprec), n = 0; bits && k-- > kmin;)
  {
    // read bit plane
    viskores::UInt32 m = viskores::Min(n, viskores::UInt32(bits));
    bits -= m;
    x = reader.read_bits(static_cast<viskores::Int32>(m));
    for (; n < BlockSize && bits && (bits--, reader.read_bit()); x += (Word)1 << n++)
      for (; n < (BlockSize - 1) && bits && (bits--, !reader.read_bit()); n++)
        ;

    // deposit bit plane
    for (int i = 0; x; i++, x >>= 1)
    {
      data[i] += (UInt)(x & 1u) << k;
    }
  }
}

template <viskores::Int32 BlockSize, typename Scalar, typename PortalType>
VISKORES_EXEC void zfp_decode(Scalar* fblock,
                              viskores::Int32 maxbits,
                              viskores::UInt32 blockIdx,
                              PortalType stream)
{
  zfp::BlockReader<BlockSize, PortalType> reader(stream, maxbits, viskores::Int32(blockIdx));
  using Int = typename zfp::zfp_traits<Scalar>::Int;
  using UInt = typename zfp::zfp_traits<Scalar>::UInt;

  viskores::UInt32 cont = 1;

  if (!zfp::is_int<Scalar>())
  {
    cont = reader.read_bit();
  }

  if (cont)
  {
    viskores::UInt32 ebits = static_cast<viskores::UInt32>(zfp::get_ebits<Scalar>()) + 1;

    viskores::UInt32 emax;
    if (!zfp::is_int<Scalar>())
    {
      emax = viskores::UInt32(reader.read_bits(static_cast<viskores::Int32>(ebits) - 1));
      emax -= static_cast<viskores::UInt32>(zfp::get_ebias<Scalar>());
    }
    else
    {
      // no exponent bits
      ebits = 0;
    }

    maxbits -= ebits;
    UInt ublock[BlockSize];
    decode_ints<BlockSize>(reader, maxbits, ublock, zfp::get_precision<Scalar>());

    Int iblock[BlockSize];
    const zfp::ZFPCodec<BlockSize> codec;
    for (viskores::Int32 i = 0; i < BlockSize; ++i)
    {
      viskores::UInt8 idx = codec.CodecLookup(i);
      iblock[idx] = uint2int(ublock[i]);
    }

    inv_transform<BlockSize> trans;
    trans.inv_xform(iblock);

    Scalar inv_w = dequantize<Int, Scalar>(1, static_cast<viskores::Int32>(emax));

    for (viskores::Int32 i = 0; i < BlockSize; ++i)
    {
      fblock[i] = inv_w * (Scalar)iblock[i];
    }
  }
}
}
}
} // namespace viskores::worklet::zfp
#endif
