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
#ifndef viskores_worklet_zfp_decode2_h
#define viskores_worklet_zfp_decode2_h

#include <viskores/Types.h>
#include <viskores/internal/ExportMacros.h>

#include <viskores/filter/zfp/worklet/zfp/ZFPBlockWriter.h>
#include <viskores/filter/zfp/worklet/zfp/ZFPDecode.h>
#include <viskores/filter/zfp/worklet/zfp/ZFPFunctions.h>
#include <viskores/filter/zfp/worklet/zfp/ZFPStructs.h>
#include <viskores/filter/zfp/worklet/zfp/ZFPTypeInfo.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace zfp
{

template <typename Scalar, typename PortalType>
VISKORES_EXEC inline void ScatterPartial2(const Scalar* q,
                                          PortalType& scalars,
                                          const viskores::Id2 dims,
                                          viskores::Id offset,
                                          viskores::Int32 nx,
                                          viskores::Int32 ny)
{
  viskores::Id x, y;
  for (y = 0; y < ny; y++, offset += dims[0] - nx, q += 4 - nx)
  {
    for (x = 0; x < nx; x++, offset++, q++)
    {
      scalars.Set(offset, *q);
    }
  }
}

template <typename Scalar, typename PortalType>
VISKORES_EXEC inline void Scatter2(const Scalar* q,
                                   PortalType& scalars,
                                   const viskores::Id2 dims,
                                   viskores::Id offset)
{
  for (viskores::Id y = 0; y < 4; y++, offset += dims[0] - 4)
  {
    for (viskores::Id x = 0; x < 4; x++, ++offset)
    {
      scalars.Set(offset, *q++);
    } // x
  }   // y
}

struct Decode2 : public viskores::worklet::WorkletMapField
{
protected:
  viskores::Id2 Dims;       // field dims
  viskores::Id2 PaddedDims; // dims padded to a multiple of zfp block size
  viskores::Id2 ZFPDims;    // zfp block dims
  viskores::UInt32 MaxBits; // bits per zfp block
public:
  Decode2(const viskores::Id2 dims, const viskores::Id2 paddedDims, const viskores::UInt32 maxbits)
    : Dims(dims)
    , PaddedDims(paddedDims)
    , MaxBits(maxbits)
  {
    ZFPDims[0] = PaddedDims[0] / 4;
    ZFPDims[1] = PaddedDims[1] / 4;
  }
  using ControlSignature = void(FieldIn, WholeArrayOut, WholeArrayIn bitstream);

  template <typename InputScalarPortal, typename BitstreamPortal>
  VISKORES_EXEC void operator()(const viskores::Id blockIdx,
                                InputScalarPortal& scalars,
                                BitstreamPortal& stream) const
  {
    using Scalar = typename InputScalarPortal::ValueType;
    constexpr viskores::Int32 BlockSize = 16;
    Scalar fblock[BlockSize];
    // clear
    for (viskores::Int32 i = 0; i < BlockSize; ++i)
    {
      fblock[i] = static_cast<Scalar>(0);
    }


    zfp::zfp_decode<BlockSize>(
      fblock, viskores::Int32(MaxBits), static_cast<viskores::UInt32>(blockIdx), stream);

    viskores::Id2 zfpBlock;
    zfpBlock[0] = blockIdx % ZFPDims[0];
    zfpBlock[1] = (blockIdx / ZFPDims[0]) % ZFPDims[1];
    viskores::Id2 logicalStart = zfpBlock * viskores::Id(4);

    viskores::Id offset = logicalStart[0] + logicalStart[1] * Dims[0];
    bool partial = false;
    if (logicalStart[0] + 4 > Dims[0])
      partial = true;
    if (logicalStart[1] + 4 > Dims[1])
      partial = true;
    if (partial)
    {
      const viskores::Int32 nx = logicalStart[0] + 4 > Dims[0]
        ? viskores::Int32(Dims[0] - logicalStart[0])
        : viskores::Int32(4);
      const viskores::Int32 ny = logicalStart[1] + 4 > Dims[1]
        ? viskores::Int32(Dims[1] - logicalStart[1])
        : viskores::Int32(4);
      ScatterPartial2(fblock, scalars, Dims, offset, nx, ny);
    }
    else
    {
      Scatter2(fblock, scalars, Dims, offset);
    }
  }
};
}
}
} // namespace viskores::worklet::zfp
#endif
