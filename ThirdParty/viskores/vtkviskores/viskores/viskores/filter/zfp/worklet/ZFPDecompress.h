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
#ifndef viskores_worklet_zfp_decompressor_h
#define viskores_worklet_zfp_decompressor_h

#include <viskores/Math.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/AtomicArray.h>
#include <viskores/cont/Timer.h>
#include <viskores/filter/zfp/worklet/zfp/ZFPDecode3.h>
#include <viskores/filter/zfp/worklet/zfp/ZFPTools.h>
#include <viskores/worklet/DispatcherMapField.h>

using ZFPWord = viskores::UInt64;

#include <stdio.h>

namespace viskores
{
namespace worklet
{
class ZFPDecompressor
{
public:
  template <typename Scalar, typename StorageIn, typename StorageOut>
  void Decompress(const viskores::cont::ArrayHandle<viskores::Int64, StorageIn>& encodedData,
                  viskores::cont::ArrayHandle<Scalar, StorageOut>& output,
                  const viskores::Float64 requestedRate,
                  viskores::Id3 dims)
  {
    //DataDumpb(data, "uncompressed");
    zfp::ZFPStream stream;
    const viskores::Int32 topoDims = 3;
    ;
    stream.SetRate(requestedRate, topoDims, viskores::Float64());


    // Check to see if we need to increase the block sizes
    // in the case where dim[x] is not a multiple of 4

    viskores::Id3 paddedDims = dims;
    // ensure that we have block sizes
    // that are a multiple of 4
    if (paddedDims[0] % 4 != 0)
      paddedDims[0] += 4 - dims[0] % 4;
    if (paddedDims[1] % 4 != 0)
      paddedDims[1] += 4 - dims[1] % 4;
    if (paddedDims[2] % 4 != 0)
      paddedDims[2] += 4 - dims[2] % 4;
    const viskores::Id four = 4;
    viskores::Id totalBlocks =
      (paddedDims[0] / four) * (paddedDims[1] / (four) * (paddedDims[2] / four));


    zfp::detail::CalcMem3d(paddedDims, stream.minbits);

    // hopefully this inits/allocates the mem only on the device
    output.Allocate(dims[0] * dims[1] * dims[2]);

    // launch 1 thread per zfp block
    viskores::cont::ArrayHandleCounting<viskores::Id> blockCounter(0, 1, totalBlocks);

    //    using Timer = viskores::cont::Timer<viskores::cont::DeviceAdapterTagSerial>;
    //    Timer timer;
    viskores::worklet::DispatcherMapField<zfp::Decode3> decompressDispatcher(
      zfp::Decode3(dims, paddedDims, stream.maxbits));
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
#endif //  viskores_worklet_zfp_compressor_h
