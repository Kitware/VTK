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
#ifndef viskores_worklet_zfp_1d_decompressor_h
#define viskores_worklet_zfp_1d_decompressor_h

#include <viskores/Math.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/AtomicArray.h>
#include <viskores/cont/Timer.h>
#include <viskores/filter/zfp/worklet/zfp/ZFPDecode1.h>
#include <viskores/filter/zfp/worklet/zfp/ZFPTools.h>
#include <viskores/worklet/DispatcherMapField.h>

using ZFPWord = viskores::UInt64;

#include <stdio.h>

namespace viskores
{
namespace worklet
{
namespace detail
{



} // namespace detail


class ZFP1DDecompressor
{
public:
  template <typename Scalar, typename StorageIn, typename StorageOut>
  void Decompress(const viskores::cont::ArrayHandle<viskores::Int64, StorageIn>& encodedData,
                  viskores::cont::ArrayHandle<Scalar, StorageOut>& output,
                  const viskores::Float64 requestedRate,
                  viskores::Id dims)
  {
    //DataDumpb(data, "uncompressed");
    zfp::ZFPStream stream;
    constexpr viskores::Int32 topoDims = 1;
    ;
    stream.SetRate(requestedRate, topoDims, viskores::Float64());


    // Check to see if we need to increase the block sizes
    // in the case where dim[x] is not a multiple of 4

    viskores::Id paddedDims = dims;
    // ensure that we have block sizes
    // that are a multiple of 4
    if (paddedDims % 4 != 0)
      paddedDims += 4 - dims % 4;
    constexpr viskores::Id four = 4;
    viskores::Id totalBlocks = (paddedDims / four);


    zfp::detail::CalcMem1d(paddedDims, stream.minbits);

    // hopefully this inits/allocates the mem only on the device
    output.Allocate(dims);

    // launch 1 thread per zfp block
    viskores::cont::ArrayHandleCounting<viskores::Id> blockCounter(0, 1, totalBlocks);

    //    using Timer = viskores::cont::Timer<viskores::cont::DeviceAdapterTagSerial>;
    //    Timer timer;
    viskores::worklet::DispatcherMapField<zfp::Decode1> decompressDispatcher(
      zfp::Decode1(dims, paddedDims, stream.maxbits));
    decompressDispatcher.Invoke(blockCounter, output, encodedData);

    //    viskores::Float64 time = timer.GetElapsedTime();
    //    size_t total_bytes =  output.GetNumberOfValues() * sizeof(viskores::Float64);
    //    viskores::Float64 gB = viskores::Float64(total_bytes) / (1024. * 1024. * 1024.);
    //    viskores::Float64 rate = gB / time;
    //    std::cout<<"Decompress time "<<time<<" sec\n";
    //    std::cout<<"Decompress rate "<<rate<<" GB / sec\n";
    //    DataDump(output, "decompressed");
  }
};
} // namespace worklet
} // namespace viskores
#endif //  viskores_worklet_zfp_1d_decompressor_h
