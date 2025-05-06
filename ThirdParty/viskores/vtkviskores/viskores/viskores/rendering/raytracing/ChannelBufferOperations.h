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
#ifndef viskores_rendering_raytracing_ChannelBuffer_Operations_h
#define viskores_rendering_raytracing_ChannelBuffer_Operations_h

#include <viskores/Types.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandleCast.h>

#include <viskores/rendering/raytracing/ChannelBuffer.h>
#include <viskores/rendering/raytracing/Worklets.h>

#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{
namespace detail
{

class CompactBuffer : public viskores::worklet::WorkletMapField
{
protected:
  const viskores::Id NumChannels; // the number of channels in the buffer

public:
  VISKORES_CONT
  CompactBuffer(const viskores::Int32 numChannels)
    : NumChannels(numChannels)
  {
  }
  using ControlSignature = void(FieldIn, WholeArrayIn, FieldIn, WholeArrayOut);
  using ExecutionSignature = void(_1, _2, _3, _4, WorkIndex);
  template <typename InBufferPortalType, typename OutBufferPortalType>
  VISKORES_EXEC void operator()(const viskores::UInt8& mask,
                                const InBufferPortalType& inBuffer,
                                const viskores::Id& offset,
                                OutBufferPortalType& outBuffer,
                                const viskores::Id& index) const
  {
    if (mask == 0)
    {
      return;
    }
    viskores::Id inIndex = index * NumChannels;
    viskores::Id outIndex = offset * NumChannels;
    for (viskores::Int32 i = 0; i < NumChannels; ++i)
    {
      BOUNDS_CHECK(inBuffer, inIndex + i);
      BOUNDS_CHECK(outBuffer, outIndex + i);
      outBuffer.Set(outIndex + i, inBuffer.Get(inIndex + i));
    }
  }
}; //class Compact

class InitBuffer : public viskores::worklet::WorkletMapField
{
protected:
  viskores::Int32 NumChannels;

public:
  VISKORES_CONT
  InitBuffer(const viskores::Int32 numChannels)
    : NumChannels(numChannels)
  {
  }
  using ControlSignature = void(FieldOut, WholeArrayIn);
  using ExecutionSignature = void(_1, _2, WorkIndex);
  template <typename ValueType, typename PortalType>
  VISKORES_EXEC void operator()(ValueType& outValue,
                                const PortalType& source,
                                const viskores::Id& index) const
  {
    outValue = source.Get(index % NumChannels);
  }
}; //class InitBuffer


} // namespace detail

class ChannelBufferOperations
{
public:
  template <typename Precision>
  static void Compact(ChannelBuffer<Precision>& buffer,
                      viskores::cont::ArrayHandle<UInt8>& masks,
                      const viskores::Id& newSize)
  {
    viskores::cont::ArrayHandle<viskores::Id> offsets;
    offsets.Allocate(buffer.Size);
    viskores::cont::ArrayHandleCast<viskores::Id, viskores::cont::ArrayHandle<viskores::UInt8>>
      castedMasks(masks);
    viskores::cont::Algorithm::ScanExclusive(castedMasks, offsets);

    viskores::cont::ArrayHandle<Precision> compactedBuffer;
    compactedBuffer.Allocate(newSize * buffer.NumChannels);

    viskores::worklet::DispatcherMapField<detail::CompactBuffer> dispatcher(
      detail::CompactBuffer(buffer.NumChannels));
    dispatcher.Invoke(masks, buffer.Buffer, offsets, compactedBuffer);
    buffer.Buffer = compactedBuffer;
    buffer.Size = newSize;
  }

  template <typename Device, typename Precision>
  static void InitChannels(ChannelBuffer<Precision>& buffer,
                           viskores::cont::ArrayHandle<Precision> sourceSignature,
                           Device)
  {
    if (sourceSignature.GetNumberOfValues() != buffer.NumChannels)
    {
      std::string msg = "ChannelBuffer: number of bins in sourse signature must match NumChannels";
      throw viskores::cont::ErrorBadValue(msg);
    }
    viskores::worklet::DispatcherMapField<detail::InitBuffer> initBufferDispatcher(
      detail::InitBuffer(buffer.NumChannels));
    initBufferDispatcher.SetDevice(Device());
    initBufferDispatcher.Invoke(buffer.Buffer, sourceSignature);
  }

  template <typename Device, typename Precision>
  static void InitConst(ChannelBuffer<Precision>& buffer, const Precision value, Device)
  {
    viskores::cont::ArrayHandleConstant<Precision> valueHandle(value, buffer.GetBufferLength());
    viskores::cont::Algorithm::Copy(Device(), valueHandle, buffer.Buffer);
  }
};
}
}
} // namespace viskores::rendering::raytracing
#endif
