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

#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayPortalToIterators.h>
#include <viskores/cont/DeviceAdapter.h>
#include <viskores/cont/Field.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/TryExecute.h>

#include <viskores/rendering/viskores_rendering_export.h>

#include <viskores/rendering/raytracing/ChannelBuffer.h>
#include <viskores/rendering/raytracing/ChannelBufferOperations.h>
#include <viskores/rendering/raytracing/RayTracingTypeDefs.h>
#include <viskores/rendering/raytracing/Worklets.h>

#include <viskores/worklet/DispatcherMapField.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

class BufferAddition : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  BufferAddition() {}
  using ControlSignature = void(FieldIn, FieldInOut);
  using ExecutionSignature = void(_1, _2);

  template <typename ValueType>
  VISKORES_EXEC void operator()(const ValueType& value1, ValueType& value2) const
  {
    value2 += value1;
  }
}; //class BufferAddition

class BufferMultiply : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  BufferMultiply() {}
  using ControlSignature = void(FieldIn, FieldInOut);
  using ExecutionSignature = void(_1, _2);

  template <typename ValueType>
  VISKORES_EXEC void operator()(const ValueType& value1, ValueType& value2) const
  {
    value2 *= value1;
  }
}; //class BufferMultiply

template <typename Precision>
ChannelBuffer<Precision>::ChannelBuffer()
{
  this->NumChannels = 4;
  this->Size = 0;
  this->Name = "default";
}

template <typename Precision>
ChannelBuffer<Precision>::ChannelBuffer(const viskores::Int32 numChannels, const viskores::Id size)
{
  if (size < 0)
    throw viskores::cont::ErrorBadValue("ChannelBuffer: Size must be greater that -1");
  if (numChannels < 0)
    throw viskores::cont::ErrorBadValue("ChannelBuffer: NumChannels must be greater that -1");

  this->NumChannels = numChannels;
  this->Size = size;

  Buffer.Allocate(this->Size * this->NumChannels);
}

template <typename Precision>
viskores::Int32 ChannelBuffer<Precision>::GetNumChannels() const
{
  return this->NumChannels;
}

template <typename Precision>
viskores::Id ChannelBuffer<Precision>::GetSize() const
{
  return this->Size;
}

template <typename Precision>
viskores::Id ChannelBuffer<Precision>::GetBufferLength() const
{
  return this->Size * static_cast<viskores::Id>(this->NumChannels);
}

template <typename Precision>
void ChannelBuffer<Precision>::SetName(const std::string name)
{
  this->Name = name;
}

template <typename Precision>
std::string ChannelBuffer<Precision>::GetName() const
{
  return this->Name;
}


template <typename Precision>
void ChannelBuffer<Precision>::AddBuffer(const ChannelBuffer<Precision>& other)
{
  if (this->NumChannels != other.GetNumChannels())
    throw viskores::cont::ErrorBadValue("ChannelBuffer add: number of channels must be equal");
  if (this->Size != other.GetSize())
    throw viskores::cont::ErrorBadValue("ChannelBuffer add: size must be equal");

  viskores::worklet::DispatcherMapField<BufferAddition>().Invoke(other.Buffer, this->Buffer);
}

template <typename Precision>
void ChannelBuffer<Precision>::MultiplyBuffer(const ChannelBuffer<Precision>& other)
{
  if (this->NumChannels != other.GetNumChannels())
    throw viskores::cont::ErrorBadValue("ChannelBuffer add: number of channels must be equal");
  if (this->Size != other.GetSize())
    throw viskores::cont::ErrorBadValue("ChannelBuffer add: size must be equal");

  viskores::worklet::DispatcherMapField<BufferMultiply>().Invoke(other.Buffer, this->Buffer);
}

template <typename Precision>
void ChannelBuffer<Precision>::Resize(const viskores::Id newSize)
{
  if (newSize < 0)
    throw viskores::cont::ErrorBadValue("ChannelBuffer resize: Size must be greater than -1");
  this->Size = newSize;
  this->Buffer.Allocate(this->Size * static_cast<viskores::Id>(NumChannels));
}

class ExtractChannel : public viskores::worklet::WorkletMapField
{
protected:
  viskores::Id NumChannels; // the nnumber of channels in the buffer
  viskores::Id ChannelNum;  // the channel to extract

public:
  VISKORES_CONT
  ExtractChannel(const viskores::Int32 numChannels, const viskores::Int32 channel)
    : NumChannels(numChannels)
    , ChannelNum(channel)
  {
  }
  using ControlSignature = void(FieldOut, WholeArrayIn, FieldIn);
  using ExecutionSignature = void(_1, _2, _3);
  template <typename T, typename BufferPortalType>
  VISKORES_EXEC void operator()(T& outValue,
                                const BufferPortalType& inBuffer,
                                const viskores::Id& index) const
  {
    viskores::Id valueIndex = index * NumChannels + ChannelNum;
    BOUNDS_CHECK(inBuffer, valueIndex);
    outValue = inBuffer.Get(valueIndex);
  }
}; //class Extract Channel

template <typename Precision>
ChannelBuffer<Precision> ChannelBuffer<Precision>::GetChannel(const viskores::Int32 channel)
{
  if (channel < 0 || channel >= this->NumChannels)
    throw viskores::cont::ErrorBadValue("ChannelBuffer: invalid channel to extract");
  ChannelBuffer<Precision> output(1, this->Size);
  output.SetName(this->Name);
  if (this->Size == 0)
  {
    return output;
  }

  viskores::cont::Invoker invoke;
  invoke(ExtractChannel(this->GetNumChannels(), channel),
         output.Buffer,
         this->Buffer,
         viskores::cont::ArrayHandleIndex(this->GetSize()));

  return output;
}

//static
class Expand : public viskores::worklet::WorkletMapField
{
protected:
  viskores::Int32 NumChannels;

public:
  VISKORES_CONT
  Expand(const viskores::Int32 numChannels)
    : NumChannels(numChannels)
  {
  }
  using ControlSignature = void(FieldIn, WholeArrayIn, WholeArrayOut);
  using ExecutionSignature = void(_1, _2, _3, WorkIndex);
  template <typename T, typename IndexPortalType, typename BufferPortalType>
  VISKORES_EXEC void operator()(const T& inValue,
                                const IndexPortalType& sparseIndexes,
                                BufferPortalType& outBuffer,
                                const viskores::Id& index) const
  {
    viskores::Id sparse = index / NumChannels;
    BOUNDS_CHECK(sparseIndexes, sparse);
    viskores::Id sparseIndex = sparseIndexes.Get(sparse) * NumChannels;
    viskores::Id outIndex = sparseIndex + index % NumChannels;
    BOUNDS_CHECK(outBuffer, outIndex);
    outBuffer.Set(outIndex, inValue);
  }
}; //class Expand

template <typename Precision>
struct ExpandFunctorSignature
{
  viskores::cont::ArrayHandle<Precision> Input;
  viskores::cont::ArrayHandle<viskores::Id> SparseIndexes;
  ChannelBuffer<Precision>* Output;
  viskores::cont::ArrayHandle<Precision> Signature;
  viskores::Id OutputLength;
  viskores::Int32 NumChannels;


  ExpandFunctorSignature(viskores::cont::ArrayHandle<Precision> input,
                         viskores::cont::ArrayHandle<viskores::Id> sparseIndexes,
                         ChannelBuffer<Precision>* outChannelBuffer,
                         viskores::Id outputLength,
                         viskores::Int32 numChannels,
                         viskores::cont::ArrayHandle<Precision> signature)
    : Input(input)
    , SparseIndexes(sparseIndexes)
    , Output(outChannelBuffer)
    , Signature(signature)
    , OutputLength(outputLength)
    , NumChannels(numChannels)
  {
  }

  template <typename Device>
  bool operator()(Device device)
  {

    viskores::Id totalSize = this->OutputLength * static_cast<viskores::Id>(this->NumChannels);
    {
      viskores::cont::Token token;
      this->Output->Buffer.PrepareForOutput(totalSize, device, token);
    }
    ChannelBufferOperations::InitChannels(*this->Output, this->Signature, device);

    viskores::worklet::DispatcherMapField<Expand> dispatcher((Expand(this->NumChannels)));
    dispatcher.SetDevice(Device());
    dispatcher.Invoke(this->Input, this->SparseIndexes, this->Output->Buffer);

    return true;
  }
};

template <typename Precision>
struct ExpandFunctor
{
  viskores::cont::ArrayHandle<Precision> Input;
  viskores::cont::ArrayHandle<viskores::Id> SparseIndexes;
  ChannelBuffer<Precision>* Output;
  viskores::Id OutputLength;
  viskores::Int32 NumChannels;
  Precision InitVal;


  ExpandFunctor(viskores::cont::ArrayHandle<Precision> input,
                viskores::cont::ArrayHandle<viskores::Id> sparseIndexes,
                ChannelBuffer<Precision>* outChannelBuffer,
                viskores::Id outputLength,
                viskores::Int32 numChannels,
                Precision initVal)
    : Input(input)
    , SparseIndexes(sparseIndexes)
    , Output(outChannelBuffer)
    , OutputLength(outputLength)
    , NumChannels(numChannels)
    , InitVal(initVal)
  {
  }

  template <typename Device>
  bool operator()(Device device)
  {

    viskores::Id totalSize = this->OutputLength * static_cast<viskores::Id>(this->NumChannels);
    {
      viskores::cont::Token token;
      this->Output->Buffer.PrepareForOutput(totalSize, device, token);
    }
    ChannelBufferOperations::InitConst(*this->Output, this->InitVal, device);

    viskores::worklet::DispatcherMapField<Expand> dispatcher((Expand(this->NumChannels)));
    dispatcher.SetDevice(Device());
    dispatcher.Invoke(this->Input, this->SparseIndexes, this->Output->Buffer);

    return true;
  }
};

template <typename Precision>
class NormalizeBuffer : public viskores::worklet::WorkletMapField
{
protected:
  Precision MinScalar;
  Precision InvDeltaScalar;
  bool Invert;

public:
  VISKORES_CONT
  NormalizeBuffer(const Precision minScalar, const Precision maxScalar, bool invert)
    : MinScalar(minScalar)
    , Invert(invert)
  {
    if (maxScalar - minScalar == 0.)
    {
      InvDeltaScalar = MinScalar;
    }
    else
    {
      InvDeltaScalar = 1.f / (maxScalar - minScalar);
    }
  }
  using ControlSignature = void(FieldInOut);
  using ExecutionSignature = void(_1);

  VISKORES_EXEC
  void operator()(Precision& value) const
  {
    value = (value - MinScalar) * InvDeltaScalar;
    if (Invert)
      value = 1.f - value;
  }
}; //class normalize buffer


template <typename Precision>
struct NormalizeFunctor
{
  viskores::cont::ArrayHandle<Precision> Input;
  bool Invert;

  NormalizeFunctor(viskores::cont::ArrayHandle<Precision> input, bool invert)
    : Input(input)
    , Invert(invert)
  {
  }

  template <typename Device>
  bool operator()(Device viskoresNotUsed(device))
  {
    auto asField = viskores::cont::make_FieldPoint("name meaningless", this->Input);
    viskores::Range range;
    asField.GetRange(&range);
    Precision minScalar = static_cast<Precision>(range.Min);
    Precision maxScalar = static_cast<Precision>(range.Max);
    viskores::worklet::DispatcherMapField<NormalizeBuffer<Precision>> dispatcher(
      NormalizeBuffer<Precision>(minScalar, maxScalar, Invert));
    dispatcher.SetDevice(Device());
    dispatcher.Invoke(Input);

    return true;
  }
};


template <typename Precision>
ChannelBuffer<Precision> ChannelBuffer<Precision>::ExpandBuffer(
  viskores::cont::ArrayHandle<viskores::Id> sparseIndexes,
  const viskores::Id outputSize,
  viskores::cont::ArrayHandle<Precision> signature)
{
  VISKORES_ASSERT(this->NumChannels == signature.ReadPortal().GetNumberOfValues());
  ChannelBuffer<Precision> output(this->NumChannels, outputSize);

  output.SetName(this->Name);

  ExpandFunctorSignature<Precision> functor(
    this->Buffer, sparseIndexes, &output, outputSize, this->NumChannels, signature);

  viskores::cont::TryExecute(functor);
  return output;
}

template <typename Precision>
ChannelBuffer<Precision> ChannelBuffer<Precision>::ExpandBuffer(
  viskores::cont::ArrayHandle<viskores::Id> sparseIndexes,
  const viskores::Id outputSize,
  Precision initValue)
{
  ChannelBuffer<Precision> output(this->NumChannels, outputSize);

  output.SetName(this->Name);

  ExpandFunctor<Precision> functor(
    this->Buffer, sparseIndexes, &output, outputSize, this->NumChannels, initValue);

  viskores::cont::TryExecute(functor);
  return output;
}

template <typename Precision>
void ChannelBuffer<Precision>::Normalize(bool invert)
{

  auto asField = viskores::cont::make_FieldPoint("name meaningless", this->Buffer);
  viskores::Range range;
  asField.GetRange(&range);
  Precision minScalar = static_cast<Precision>(range.Min);
  Precision maxScalar = static_cast<Precision>(range.Max);
  viskores::worklet::DispatcherMapField<NormalizeBuffer<Precision>> dispatcher(
    NormalizeBuffer<Precision>(minScalar, maxScalar, invert));
  dispatcher.Invoke(this->Buffer);
}

template <typename Precision>
struct ResizeChannelFunctor
{
  ChannelBuffer<Precision>* Self;
  viskores::Int32 NumChannels;

  ResizeChannelFunctor(ChannelBuffer<Precision>* self, viskores::Int32 numChannels)
    : Self(self)
    , NumChannels(numChannels)
  {
  }

  template <typename Device>
  bool operator()(Device device)
  {
    Self->SetNumChannels(NumChannels, device);
    return true;
  }
};

template <typename Precision>
void ChannelBuffer<Precision>::InitConst(const Precision value)
{
  viskores::cont::ArrayHandleConstant<Precision> valueHandle(value, this->GetBufferLength());
  viskores::cont::Algorithm::Copy(valueHandle, this->Buffer);
}

template <typename Precision>
struct InitChannelFunctor
{
  ChannelBuffer<Precision>* Self;
  const viskores::cont::ArrayHandle<Precision>& Signature;
  InitChannelFunctor(ChannelBuffer<Precision>* self,
                     const viskores::cont::ArrayHandle<Precision>& signature)
    : Self(self)
    , Signature(signature)
  {
  }

  template <typename Device>
  bool operator()(Device device)
  {
    ChannelBufferOperations::InitChannels(*Self, Signature, device);
    return true;
  }
};

template <typename Precision>
void ChannelBuffer<Precision>::InitChannels(const viskores::cont::ArrayHandle<Precision>& signature)
{
  InitChannelFunctor<Precision> functor(this, signature);
  viskores::cont::TryExecute(functor);
}

template <typename Precision>
void ChannelBuffer<Precision>::SetNumChannels(const viskores::Int32 numChannels)
{
  ResizeChannelFunctor<Precision> functor(this, numChannels);
  viskores::cont::TryExecute(functor);
}

template <typename Precision>
ChannelBuffer<Precision> ChannelBuffer<Precision>::Copy()
{
  ChannelBuffer res(NumChannels, Size);
  viskores::cont::Algorithm::Copy(this->Buffer, res.Buffer);
  return res;
}

// Instantiate supported types
template class ChannelBuffer<viskores::Float32>;
template class ChannelBuffer<viskores::Float64>;
}
}
} // namespace viskores::rendering::raytracing
