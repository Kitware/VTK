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
#ifndef viskores_worklet_zfp_encode3_h
#define viskores_worklet_zfp_encode3_h

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
VISKORES_EXEC inline void GatherPartial3(Scalar* q,
                                         const PortalType& scalars,
                                         const viskores::Id3 dims,
                                         viskores::Id offset,
                                         viskores::Int32 nx,
                                         viskores::Int32 ny,
                                         viskores::Int32 nz)
{
  viskores::Id x, y, z;

  for (z = 0; z < nz; z++, offset += dims[0] * dims[1] - ny * dims[0])
  {
    for (y = 0; y < ny; y++, offset += dims[0] - nx)
    {
      for (x = 0; x < nx; x++, offset += 1)
      {
        q[16 * z + 4 * y + x] = scalars.Get(offset);
      }
      PadBlock(q + 16 * z + 4 * y, static_cast<viskores::UInt32>(nx), 1);
    }

    for (x = 0; x < 4; x++)
    {
      PadBlock(q + 16 * z + x, viskores::UInt32(ny), 4);
    }
  }

  for (y = 0; y < 4; y++)
  {
    for (x = 0; x < 4; x++)
    {
      PadBlock(q + 4 * y + x, viskores::UInt32(nz), 16);
    }
  }
}

template <typename Scalar, typename PortalType>
VISKORES_EXEC inline void Gather3(Scalar* fblock,
                                  const PortalType& scalars,
                                  const viskores::Id3 dims,
                                  viskores::Id offset)
{
  // TODO: gather partial
  viskores::Id counter = 0;
  for (viskores::Id z = 0; z < 4; z++, offset += dims[0] * dims[1] - 4 * dims[0])
  {
    for (viskores::Id y = 0; y < 4; y++, offset += dims[0] - 4)
    {
      for (viskores::Id x = 0; x < 4; x++, ++offset)
      {
        fblock[counter] = scalars.Get(offset);
        counter++;
      } // x
    }   // y
  }     // z
}

struct Encode3 : public viskores::worklet::WorkletMapField
{
protected:
  viskores::Id3 Dims;       // field dims
  viskores::Id3 PaddedDims; // dims padded to a multiple of zfp block size
  viskores::Id3 ZFPDims;    // zfp block dims
  viskores::UInt32 MaxBits; // bits per zfp block
public:
  Encode3(const viskores::Id3 dims, const viskores::Id3 paddedDims, const viskores::UInt32 maxbits)
    : Dims(dims)
    , PaddedDims(paddedDims)
    , MaxBits(maxbits)
  {
    ZFPDims[0] = PaddedDims[0] / 4;
    ZFPDims[1] = PaddedDims[1] / 4;
    ZFPDims[2] = PaddedDims[2] / 4;
  }
  using ControlSignature = void(FieldIn, WholeArrayIn, AtomicArrayInOut bitstream);

  template <typename InputScalarPortal, typename BitstreamPortal>
  VISKORES_EXEC void operator()(const viskores::Id blockIdx,
                                const InputScalarPortal& scalars,
                                BitstreamPortal& stream) const
  {
    using Scalar = typename InputScalarPortal::ValueType;
    constexpr viskores::Int32 BlockSize = 64;
    Scalar fblock[BlockSize];

    viskores::Id3 zfpBlock;
    zfpBlock[0] = blockIdx % ZFPDims[0];
    zfpBlock[1] = (blockIdx / ZFPDims[0]) % ZFPDims[1];
    zfpBlock[2] = blockIdx / (ZFPDims[0] * ZFPDims[1]);
    viskores::Id3 logicalStart = zfpBlock * viskores::Id(4);

    // get the offset into the field
    //viskores::Id offset = (zfpBlock[2]*4*ZFPDims[1] + zfpBlock[1] * 4)*ZFPDims[0] * 4 + zfpBlock[0] * 4;
    viskores::Id offset = (logicalStart[2] * Dims[1] + logicalStart[1]) * Dims[0] + logicalStart[0];

    bool partial = false;
    if (logicalStart[0] + 4 > Dims[0])
      partial = true;
    if (logicalStart[1] + 4 > Dims[1])
      partial = true;
    if (logicalStart[2] + 4 > Dims[2])
      partial = true;
    if (partial)
    {
      const viskores::Int32 nx = logicalStart[0] + 4 > Dims[0]
        ? viskores::Int32(Dims[0] - logicalStart[0])
        : viskores::Int32(4);
      const viskores::Int32 ny = logicalStart[1] + 4 > Dims[1]
        ? viskores::Int32(Dims[1] - logicalStart[1])
        : viskores::Int32(4);
      const viskores::Int32 nz = logicalStart[2] + 4 > Dims[2]
        ? viskores::Int32(Dims[2] - logicalStart[2])
        : viskores::Int32(4);

      GatherPartial3(fblock, scalars, Dims, offset, nx, ny, nz);
    }
    else
    {
      Gather3(fblock, scalars, Dims, offset);
    }

    zfp::ZFPBlockEncoder<BlockSize, Scalar, BitstreamPortal> encoder;

    encoder.encode(fblock, viskores::Int32(MaxBits), viskores::UInt32(blockIdx), stream);
  }
};
}
}
} // namespace viskores::worklet::zfp
#endif
