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
#ifndef viskores_worklet_zfp_encode1_h
#define viskores_worklet_zfp_encode1_h

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
VISKORES_EXEC inline void GatherPartial1(Scalar* q,
                                         const PortalType& scalars,
                                         viskores::Id offset,
                                         int nx,
                                         int sx)
{
  viskores::Id x;
  for (x = 0; x < nx; x++, offset += sx)
    q[x] = scalars.Get(offset);
  PadBlock(q, viskores::UInt32(nx), 1);
}

template <typename Scalar, typename PortalType>
VISKORES_EXEC inline void Gather1(Scalar* fblock,
                                  const PortalType& scalars,
                                  viskores::Id offset,
                                  int sx)
{
  viskores::Id counter = 0;

  for (viskores::Id x = 0; x < 4; x++, offset += sx)
  {
    fblock[counter] = scalars.Get(offset);
    counter++;
  }
}

struct Encode1 : public viskores::worklet::WorkletMapField
{
protected:
  viskores::Id Dims;        // field dims
  viskores::Id PaddedDims;  // dims padded to a multiple of zfp block size
  viskores::Id ZFPDims;     // zfp block dims
  viskores::UInt32 MaxBits; // bits per zfp block

public:
  Encode1(const viskores::Id dims, const viskores::Id paddedDims, const viskores::UInt32 maxbits)
    : Dims(dims)
    , PaddedDims(paddedDims)
    , MaxBits(maxbits)
  {
    ZFPDims = PaddedDims / 4;
  }
  using ControlSignature = void(FieldIn, WholeArrayIn, AtomicArrayInOut bitstream);

  template <class InputScalarPortal, typename BitstreamPortal>
  VISKORES_EXEC void operator()(const viskores::Id blockIdx,
                                const InputScalarPortal& scalars,
                                BitstreamPortal& stream) const
  {
    using Scalar = typename InputScalarPortal::ValueType;

    viskores::Id zfpBlock;
    zfpBlock = blockIdx % ZFPDims;
    viskores::Id logicalStart = zfpBlock * viskores::Id(4);

    constexpr viskores::Int32 BlockSize = 4;
    Scalar fblock[BlockSize];

    bool partial = false;
    if (logicalStart + 4 > Dims)
      partial = true;

    if (partial)
    {
      const viskores::Int32 nx =
        logicalStart + 4 > Dims ? viskores::Int32(Dims - logicalStart) : viskores::Int32(4);
      GatherPartial1(fblock, scalars, logicalStart, nx, 1);
    }
    else
    {
      Gather1(fblock, scalars, logicalStart, 1);
    }

    zfp::ZFPBlockEncoder<BlockSize, Scalar, BitstreamPortal> encoder;
    encoder.encode(
      fblock, static_cast<viskores::Int32>(MaxBits), viskores::UInt32(blockIdx), stream);
  }
};
}
}
}
#endif
