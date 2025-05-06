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
#ifndef viskores_worklet_zfp_encode2_h
#define viskores_worklet_zfp_encode2_h

#include <viskores/Types.h>
#include <viskores/internal/ExportMacros.h>

#include <viskores/filter/zfp/worklet/zfp/ZFPBlockWriter.h>
#include <viskores/filter/zfp/worklet/zfp/ZFPEncode.h>
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
VISKORES_EXEC inline void GatherPartial2(Scalar* q,
                                         const PortalType& scalars,
                                         viskores::Id offset,
                                         viskores::Int32 nx,
                                         viskores::Int32 ny,
                                         viskores::Int32 sx,
                                         viskores::Int32 sy)
{
  viskores::Id x, y;
  for (y = 0; y < ny; y++, offset += sy - nx * sx)
  {
    for (x = 0; x < nx; x++, offset += 1)
      q[4 * y + x] = scalars.Get(offset);
    PadBlock(q + 4 * y, viskores::UInt32(nx), 1);
  }
  for (x = 0; x < 4; x++)
    PadBlock(q + x, viskores::UInt32(ny), 4);
}

template <typename Scalar, typename PortalType>
VISKORES_EXEC inline void Gather2(Scalar* fblock,
                                  const PortalType& scalars,
                                  viskores::Id offset,
                                  int sx,
                                  int sy)
{
  viskores::Id counter = 0;

  for (viskores::Id y = 0; y < 4; y++, offset += sy - 4 * sx)
    for (viskores::Id x = 0; x < 4; x++, offset += sx)
    {
      fblock[counter] = scalars.Get(offset);
      counter++;
    }
}

struct Encode2 : public viskores::worklet::WorkletMapField
{
protected:
  viskores::Id2 Dims;       // field dims
  viskores::Id2 PaddedDims; // dims padded to a multiple of zfp block size
  viskores::Id2 ZFPDims;    // zfp block dims
  viskores::UInt32 MaxBits; // bits per zfp block

public:
  Encode2(const viskores::Id2 dims, const viskores::Id2 paddedDims, const viskores::UInt32 maxbits)
    : Dims(dims)
    , PaddedDims(paddedDims)
    , MaxBits(maxbits)
  {
    ZFPDims[0] = PaddedDims[0] / 4;
    ZFPDims[1] = PaddedDims[1] / 4;
  }
  using ControlSignature = void(FieldIn, WholeArrayIn, AtomicArrayInOut bitstream);

  template <class InputScalarPortal, typename BitstreamPortal>
  VISKORES_EXEC void operator()(const viskores::Id blockIdx,
                                const InputScalarPortal& scalars,
                                BitstreamPortal& stream) const
  {
    using Scalar = typename InputScalarPortal::ValueType;

    viskores::Id2 zfpBlock;
    zfpBlock[0] = blockIdx % ZFPDims[0];
    zfpBlock[1] = (blockIdx / ZFPDims[0]) % ZFPDims[1];
    viskores::Id2 logicalStart = zfpBlock * viskores::Id(4);
    viskores::Id offset = logicalStart[1] * Dims[0] + logicalStart[0];

    constexpr viskores::Int32 BlockSize = 16;
    Scalar fblock[BlockSize];

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
      GatherPartial2(fblock, scalars, offset, nx, ny, 1, static_cast<viskores::Int32>(Dims[0]));
    }
    else
    {
      Gather2(fblock, scalars, offset, 1, static_cast<viskores::Int32>(Dims[0]));
    }

    zfp::ZFPBlockEncoder<BlockSize, Scalar, BitstreamPortal> encoder;
    encoder.encode(fblock, viskores::Int32(MaxBits), viskores::UInt32(blockIdx), stream);
  }
};
}
}
}
#endif
