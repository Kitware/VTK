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
#ifndef viskores_worklet_zfp_encode_h
#define viskores_worklet_zfp_encode_h

#include <viskores/Types.h>
#include <viskores/filter/zfp/worklet/zfp/ZFPBlockWriter.h>
#include <viskores/filter/zfp/worklet/zfp/ZFPCodec.h>
#include <viskores/filter/zfp/worklet/zfp/ZFPTypeInfo.h>
#include <viskores/internal/ExportMacros.h>

namespace viskores
{
namespace worklet
{
namespace zfp
{

template <typename Scalar>
VISKORES_EXEC void PadBlock(Scalar* p, viskores::UInt32 n, viskores::UInt32 s)
{
  switch (n)
  {
    case 0:
      p[0 * s] = 0;
    /* FALLTHROUGH */
    case 1:
      p[1 * s] = p[0 * s];
    /* FALLTHROUGH */
    case 2:
      p[2 * s] = p[1 * s];
    /* FALLTHROUGH */
    case 3:
      p[3 * s] = p[0 * s];
    /* FALLTHROUGH */
    default:
      break;
  }
}

template <viskores::Int32 N, typename FloatType>
inline VISKORES_EXEC viskores::Int32 MaxExponent(const FloatType* vals)
{
  FloatType maxVal = 0;
  for (viskores::Int32 i = 0; i < N; ++i)
  {
    maxVal = viskores::Max(maxVal, viskores::Abs(vals[i]));
  }

  if (maxVal > 0)
  {
    viskores::Int32 exponent;
    viskores::Frexp(maxVal, &exponent);
    /* clamp exponent in case x is denormal */
    return viskores::Max(exponent, 1 - get_ebias<FloatType>());
  }
  return -get_ebias<FloatType>();
}

// maximum number of bit planes to encode
inline VISKORES_EXEC viskores::Int32 precision(viskores::Int32 maxexp,
                                               viskores::Int32 maxprec,
                                               viskores::Int32 minexp)
{
  return viskores::Min(maxprec, viskores::Max(0, maxexp - minexp + 8));
}

template <typename Scalar>
inline VISKORES_EXEC Scalar quantize(Scalar x, viskores::Int32 e)
{
  return viskores::Ldexp(x, (CHAR_BIT * (viskores::Int32)sizeof(Scalar) - 2) - e);
}

template <typename Int, typename Scalar, viskores::Int32 BlockSize>
inline VISKORES_EXEC void fwd_cast(Int* iblock, const Scalar* fblock, viskores::Int32 emax)
{
  Scalar s = quantize<Scalar>(1, emax);
  for (viskores::Int32 i = 0; i < BlockSize; ++i)
  {
    iblock[i] = static_cast<Int>(s * fblock[i]);
  }
}

template <typename Int, viskores::Int32 S>
inline VISKORES_EXEC void fwd_lift(Int* p)
{
  Int x, y, z, w;
  x = *p;
  p += S;
  y = *p;
  p += S;
  z = *p;
  p += S;
  w = *p;
  p += S;

  /*
  ** non-orthogonal transform
  **        ( 4  4  4  4) (x)
  ** 1/16 * ( 5  1 -1 -5) (y)
  **        (-4  4  4 -4) (z)
  **        (-2  6 -6  2) (w)
  */
  x += w;
  x >>= 1;
  w -= x;
  z += y;
  z >>= 1;
  y -= z;
  x += z;
  x >>= 1;
  z -= x;
  w += y;
  w >>= 1;
  y -= w;
  w += y >> 1;
  y -= w >> 1;

  p -= S;
  *p = w;
  p -= S;
  *p = z;
  p -= S;
  *p = y;
  p -= S;
  *p = x;
}

template <typename Int, typename UInt>
inline VISKORES_EXEC UInt int2uint(const Int x);

template <>
inline VISKORES_EXEC viskores::UInt64 int2uint<viskores::Int64, viskores::UInt64>(
  const viskores::Int64 x)
{
  return (static_cast<viskores::UInt64>(x) + (viskores::UInt64)0xaaaaaaaaaaaaaaaaull) ^
    (viskores::UInt64)0xaaaaaaaaaaaaaaaaull;
}

template <>
inline VISKORES_EXEC viskores::UInt32 int2uint<viskores::Int32, viskores::UInt32>(
  const viskores::Int32 x)
{
  return (static_cast<viskores::UInt32>(x) + (viskores::UInt32)0xaaaaaaaau) ^
    (viskores::UInt32)0xaaaaaaaau;
}



template <typename UInt, typename Int, viskores::Int32 BlockSize>
inline VISKORES_EXEC void fwd_order(UInt* ublock, const Int* iblock)
{
  const zfp::ZFPCodec<BlockSize> codec;
  for (viskores::Int32 i = 0; i < BlockSize; ++i)
  {
    viskores::UInt8 idx = codec.CodecLookup(i);
    ublock[i] = int2uint<Int, UInt>(iblock[idx]);
  }
}

template <typename Int, viskores::Int32 BlockSize>
inline VISKORES_EXEC void fwd_xform(Int* p);

template <>
inline VISKORES_EXEC void fwd_xform<viskores::Int64, 64>(viskores::Int64* p)
{
  viskores::UInt32 x, y, z;
  /* transform along x */
  for (z = 0; z < 4; z++)
    for (y = 0; y < 4; y++)
      fwd_lift<viskores::Int64, 1>(p + 4 * y + 16 * z);
  /* transform along y */
  for (x = 0; x < 4; x++)
    for (z = 0; z < 4; z++)
      fwd_lift<viskores::Int64, 4>(p + 16 * z + 1 * x);
  /* transform along z */
  for (y = 0; y < 4; y++)
    for (x = 0; x < 4; x++)
      fwd_lift<viskores::Int64, 16>(p + 1 * x + 4 * y);
}

template <>
inline VISKORES_EXEC void fwd_xform<viskores::Int32, 64>(viskores::Int32* p)
{
  viskores::UInt32 x, y, z;
  /* transform along x */
  for (z = 0; z < 4; z++)
    for (y = 0; y < 4; y++)
      fwd_lift<viskores::Int32, 1>(p + 4 * y + 16 * z);
  /* transform along y */
  for (x = 0; x < 4; x++)
    for (z = 0; z < 4; z++)
      fwd_lift<viskores::Int32, 4>(p + 16 * z + 1 * x);
  /* transform along z */
  for (y = 0; y < 4; y++)
    for (x = 0; x < 4; x++)
      fwd_lift<viskores::Int32, 16>(p + 1 * x + 4 * y);
}

template <>
inline VISKORES_EXEC void fwd_xform<viskores::Int64, 16>(viskores::Int64* p)
{
  viskores::UInt32 x, y;
  /* transform along x */
  for (y = 0; y < 4; y++)
    fwd_lift<viskores::Int64, 1>(p + 4 * y);
  /* transform along y */
  for (x = 0; x < 4; x++)
    fwd_lift<viskores::Int64, 4>(p + 1 * x);
}

template <>
inline VISKORES_EXEC void fwd_xform<viskores::Int32, 16>(viskores::Int32* p)
{
  viskores::UInt32 x, y;
  /* transform along x */
  for (y = 0; y < 4; y++)
    fwd_lift<viskores::Int32, 1>(p + 4 * y);
  /* transform along y */
  for (x = 0; x < 4; x++)
    fwd_lift<viskores::Int32, 4>(p + 1 * x);
}

template <>
inline VISKORES_EXEC void fwd_xform<viskores::Int64, 4>(viskores::Int64* p)
{
  /* transform along x */
  fwd_lift<viskores::Int64, 1>(p);
}

template <>
inline VISKORES_EXEC void fwd_xform<viskores::Int32, 4>(viskores::Int32* p)
{
  /* transform along x */
  fwd_lift<viskores::Int32, 1>(p);
}

template <viskores::Int32 BlockSize, typename PortalType, typename Int>
VISKORES_EXEC void encode_block(BlockWriter<BlockSize, PortalType>& stream,
                                viskores::Int32 maxbits,
                                viskores::Int32 maxprec,
                                Int* iblock)
{
  using UInt = typename zfp_traits<Int>::UInt;

  fwd_xform<Int, BlockSize>(iblock);

  UInt ublock[BlockSize];
  fwd_order<UInt, Int, BlockSize>(ublock, iblock);

  viskores::UInt32 intprec = CHAR_BIT * (viskores::UInt32)sizeof(UInt);
  viskores::UInt32 kmin =
    intprec > (viskores::UInt32)maxprec ? intprec - static_cast<viskores::UInt32>(maxprec) : 0;
  viskores::UInt32 bits = static_cast<viskores::UInt32>(maxbits);
  viskores::UInt32 i, m;
  viskores::UInt32 n = 0;
  viskores::UInt64 x;
  /* encode one bit plane at a time from MSB to LSB */
  for (viskores::UInt32 k = intprec; bits && k-- > kmin;)
  {
    /* step 1: extract bit plane #k to x */
    x = 0;
    for (i = 0; i < BlockSize; i++)
    {
      x += (viskores::UInt64)((ublock[i] >> k) & 1u) << i;
    }
    /* step 2: encode first n bits of bit plane */
    m = viskores::Min(n, bits);
    bits -= m;
    x = stream.write_bits(x, m);
    /* step 3: unary run-length encode remainder of bit plane */
    for (; n < BlockSize && bits && (bits--, stream.write_bit(!!x)); x >>= 1, n++)
    {
      for (; n < BlockSize - 1 && bits && (bits--, !stream.write_bit(x & 1u)); x >>= 1, n++)
      {
      }
    }
  }
}


template <viskores::Int32 BlockSize, typename Scalar, typename PortalType>
inline VISKORES_EXEC void zfp_encodef(Scalar* fblock,
                                      viskores::Int32 maxbits,
                                      viskores::UInt32 blockIdx,
                                      PortalType& stream)
{
  using Int = typename zfp::zfp_traits<Scalar>::Int;
  zfp::BlockWriter<BlockSize, PortalType> blockWriter(stream, maxbits, viskores::Id(blockIdx));
  viskores::Int32 emax = zfp::MaxExponent<BlockSize, Scalar>(fblock);
  //  std::cout<<"EMAX "<<emax<<"\n";
  viskores::Int32 maxprec =
    zfp::precision(emax, zfp::get_precision<Scalar>(), zfp::get_min_exp<Scalar>());
  viskores::UInt32 e = viskores::UInt32(maxprec ? emax + zfp::get_ebias<Scalar>() : 0);
  /* encode block only if biased exponent is nonzero */
  if (e)
  {

    const viskores::UInt32 ebits = viskores::UInt32(zfp::get_ebits<Scalar>()) + 1;
    blockWriter.write_bits(2 * e + 1, ebits);

    Int iblock[BlockSize];
    zfp::fwd_cast<Int, Scalar, BlockSize>(iblock, fblock, emax);

    encode_block<BlockSize>(blockWriter, maxbits - viskores::Int32(ebits), maxprec, iblock);
  }
}

// helpers so we can do partial template instantiation since
// the portal type could be on any backend
template <viskores::Int32 BlockSize, typename Scalar, typename PortalType>
struct ZFPBlockEncoder
{
};

template <viskores::Int32 BlockSize, typename PortalType>
struct ZFPBlockEncoder<BlockSize, viskores::Float32, PortalType>
{
  VISKORES_EXEC void encode(viskores::Float32* fblock,
                            viskores::Int32 maxbits,
                            viskores::UInt32 blockIdx,
                            PortalType& stream)
  {
    zfp_encodef<BlockSize>(fblock, maxbits, blockIdx, stream);
  }
};

template <viskores::Int32 BlockSize, typename PortalType>
struct ZFPBlockEncoder<BlockSize, viskores::Float64, PortalType>
{
  VISKORES_EXEC void encode(viskores::Float64* fblock,
                            viskores::Int32 maxbits,
                            viskores::UInt32 blockIdx,
                            PortalType& stream)
  {
    zfp_encodef<BlockSize>(fblock, maxbits, blockIdx, stream);
  }
};

template <viskores::Int32 BlockSize, typename PortalType>
struct ZFPBlockEncoder<BlockSize, viskores::Int32, PortalType>
{
  VISKORES_EXEC void encode(viskores::Int32* fblock,
                            viskores::Int32 maxbits,
                            viskores::UInt32 blockIdx,
                            PortalType& stream)
  {
    using Int = typename zfp::zfp_traits<viskores::Int32>::Int;
    zfp::BlockWriter<BlockSize, PortalType> blockWriter(stream, maxbits, viskores::Id(blockIdx));
    encode_block<BlockSize>(blockWriter, maxbits, get_precision<viskores::Int32>(), (Int*)fblock);
  }
};

template <viskores::Int32 BlockSize, typename PortalType>
struct ZFPBlockEncoder<BlockSize, viskores::Int64, PortalType>
{
  VISKORES_EXEC void encode(viskores::Int64* fblock,
                            viskores::Int32 maxbits,
                            viskores::UInt32 blockIdx,
                            PortalType& stream)
  {
    using Int = typename zfp::zfp_traits<viskores::Int64>::Int;
    zfp::BlockWriter<BlockSize, PortalType> blockWriter(stream, maxbits, viskores::Id(blockIdx));
    encode_block<BlockSize>(blockWriter, maxbits, get_precision<viskores::Int64>(), (Int*)fblock);
  }
};
}
}
} // namespace viskores::worklet::zfp
#endif
