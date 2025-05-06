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
#ifndef viskores_worklet_zfp_decode1_h
#define viskores_worklet_zfp_decode1_h

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
VISKORES_EXEC inline void ScatterPartial1(const Scalar* q,
                                          PortalType& scalars,
                                          viskores::Id offset,
                                          viskores::Int32 nx)
{
  viskores::Id x;
  for (x = 0; x < nx; x++, offset++, q++)
  {
    scalars.Set(offset, *q);
  }
}

template <typename Scalar, typename PortalType>
VISKORES_EXEC inline void Scatter1(const Scalar* q, PortalType& scalars, viskores::Id offset)
{
  for (viskores::Id x = 0; x < 4; x++, ++offset)
  {
    scalars.Set(offset, *q++);
  } // x
}

struct Decode1 : public viskores::worklet::WorkletMapField
{
protected:
  viskores::Id Dims;        // field dims
  viskores::Id PaddedDims;  // dims padded to a multiple of zfp block size
  viskores::Id ZFPDims;     // zfp block dims
  viskores::UInt32 MaxBits; // bits per zfp block
public:
  Decode1(const viskores::Id dims, const viskores::Id paddedDims, const viskores::UInt32 maxbits)
    : Dims(dims)
    , PaddedDims(paddedDims)
    , MaxBits(maxbits)
  {
    ZFPDims = PaddedDims / 4;
  }
  using ControlSignature = void(FieldIn, WholeArrayOut, WholeArrayIn bitstream);

  template <typename InputScalarPortal, typename BitstreamPortal>
  VISKORES_EXEC void operator()(const viskores::Id blockIdx,
                                InputScalarPortal& scalars,
                                BitstreamPortal& stream) const
  {
    using Scalar = typename InputScalarPortal::ValueType;
    constexpr viskores::Int32 BlockSize = 4;
    Scalar fblock[BlockSize];
    // clear
    for (viskores::Int32 i = 0; i < BlockSize; ++i)
    {
      fblock[i] = static_cast<Scalar>(0);
    }


    zfp::zfp_decode<BlockSize>(
      fblock, viskores::Int32(MaxBits), static_cast<viskores::UInt32>(blockIdx), stream);


    viskores::Id zfpBlock;
    zfpBlock = blockIdx % ZFPDims;
    viskores::Id logicalStart = zfpBlock * viskores::Id(4);

    bool partial = false;
    if (logicalStart + 4 > Dims)
      partial = true;
    if (partial)
    {
      const viskores::Int32 nx =
        logicalStart + 4 > Dims ? viskores::Int32(Dims - logicalStart) : viskores::Int32(4);
      ScatterPartial1(fblock, scalars, logicalStart, nx);
    }
    else
    {
      Scatter1(fblock, scalars, logicalStart);
    }
  }
};
}
}
} // namespace viskores::worklet::zfp
#endif
