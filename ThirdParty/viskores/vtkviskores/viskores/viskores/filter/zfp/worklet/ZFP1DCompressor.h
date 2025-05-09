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
#ifndef viskores_worklet_zfp_1d_compressor_h
#define viskores_worklet_zfp_1d_compressor_h

#include <viskores/Math.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/AtomicArray.h>
#include <viskores/cont/Timer.h>
#include <viskores/worklet/DispatcherMapField.h>

#include <viskores/filter/zfp/worklet/zfp/ZFPEncode1.h>
#include <viskores/filter/zfp/worklet/zfp/ZFPTools.h>

using ZFPWord = viskores::UInt64;

#include <stdio.h>

namespace viskores
{
namespace worklet
{


class ZFP1DCompressor
{
public:
  template <typename Scalar, typename Storage>
  viskores::cont::ArrayHandle<viskores::Int64> Compress(
    const viskores::cont::ArrayHandle<Scalar, Storage>& data,
    const viskores::Float64 requestedRate,
    const viskores::Id dims)
  {
    // DataDump(data, "uncompressed");
    zfp::ZFPStream stream;
    constexpr viskores::Int32 topoDims = 1;
    stream.SetRate(requestedRate, topoDims, viskores::Float64());
    //VISKORES_ASSERT(

    // Check to see if we need to increase the block sizes
    // in the case where dim[x] is not a multiple of 4

    viskores::Id paddedDims = dims;
    // ensure that we have block sizes
    // that are a multiple of 4
    if (paddedDims % 4 != 0)
      paddedDims += 4 - dims % 4;
    constexpr viskores::Id four = 4;
    const viskores::Id totalBlocks = (paddedDims / four);


    size_t outbits = zfp::detail::CalcMem1d(paddedDims, stream.minbits);
    viskores::Id outsize = viskores::Id(outbits / sizeof(ZFPWord));

    viskores::cont::ArrayHandle<viskores::Int64> output;
    // hopefully this inits/allocates the mem only on the device
    viskores::cont::ArrayHandleConstant<viskores::Int64> zero(0, outsize);
    viskores::cont::Algorithm::Copy(zero, output);

    // launch 1 thread per zfp block
    viskores::cont::ArrayHandleCounting<viskores::Id> blockCounter(0, 1, totalBlocks);

    //    using Timer = viskores::cont::Timer<viskores::cont::DeviceAdapterTagSerial>;
    //    Timer timer;
    viskores::worklet::DispatcherMapField<zfp::Encode1> compressDispatcher(
      zfp::Encode1(dims, paddedDims, stream.maxbits));
    compressDispatcher.Invoke(blockCounter, data, output);

    //    viskores::Float64 time = timer.GetElapsedTime();
    //    size_t total_bytes =  data.GetNumberOfValues() * sizeof(viskores::Float64);
    //    viskores::Float64 gB = viskores::Float64(total_bytes) / (1024. * 1024. * 1024.);
    //    viskores::Float64 rate = gB / time;
    //    std::cout<<"Compress time "<<time<<" sec\n";
    //    std::cout<<"Compress rate "<<rate<<" GB / sec\n";
    //    DataDump(output, "compressed");

    return output;
  }
};
} // namespace worklet
} // namespace viskores
#endif //  viskores_worklet_zfp_1d_compressor_h
