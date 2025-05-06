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
#ifndef viskores_rendering_raytracing_Worklets_h
#define viskores_rendering_raytracing_Worklets_h
#include <viskores/worklet/WorkletMapField.h>
namespace viskores
{
namespace rendering
{
namespace raytracing
{
//
// Utility memory set functor
//
template <class T>
class MemSet : public viskores::worklet::WorkletMapField
{
  T Value;

public:
  VISKORES_CONT
  MemSet(T value)
    : Value(value)
  {
  }
  using ControlSignature = void(FieldOut);
  using ExecutionSignature = void(_1);
  VISKORES_EXEC
  void operator()(T& outValue) const { outValue = Value; }
}; //class MemSet

template <typename FloatType>
class CopyAndOffset : public viskores::worklet::WorkletMapField
{
  FloatType Offset;

public:
  VISKORES_CONT
  CopyAndOffset(const FloatType offset = 0.00001)
    : Offset(offset)
  {
  }
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  VISKORES_EXEC inline void operator()(const FloatType& inValue, FloatType& outValue) const
  {
    outValue = inValue + Offset;
  }
}; //class Copy and iffset

template <typename FloatType>
class CopyAndOffsetMask : public viskores::worklet::WorkletMapField
{
  FloatType Offset;
  viskores::UInt8 MaskValue;

public:
  VISKORES_CONT
  CopyAndOffsetMask(const FloatType offset = 0.00001, const viskores::UInt8 mask = 1)
    : Offset(offset)
    , MaskValue(mask)
  {
  }
  using ControlSignature = void(FieldIn, FieldInOut, FieldIn);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename MaskType>
  VISKORES_EXEC inline void operator()(const FloatType& inValue,
                                       FloatType& outValue,
                                       const MaskType& mask) const
  {
    if (mask == MaskValue)
      outValue = inValue + Offset;
  }
}; //class Copy and iffset

template <class T>
class Mask : public viskores::worklet::WorkletMapField
{
  T Value;

public:
  VISKORES_CONT
  Mask(T value)
    : Value(value)
  {
  }
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename O>
  VISKORES_EXEC void operator()(const T& inValue, O& outValue) const
  {
    if (inValue == Value)
      outValue = static_cast<O>(1);
    else
      outValue = static_cast<O>(0);
  }
}; //class mask

template <class T, int N>
class ManyMask : public viskores::worklet::WorkletMapField
{
  viskores::Vec<T, N> Values;

public:
  VISKORES_CONT
  ManyMask(viskores::Vec<T, N> values)
    : Values(values)
  {
  }
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename O>
  VISKORES_EXEC void operator()(const T& inValue, O& outValue) const
  {
    bool doMask = false;
    for (viskores::Int32 i = 0; i < N; ++i)
    {
      if (inValue == Values[i])
        doMask = true;
    }
    if (doMask)
      outValue = static_cast<O>(1);
    else
      outValue = static_cast<O>(0);
  }
}; //class double mask

struct MaxValue
{
  template <typename T>
  VISKORES_EXEC_CONT T operator()(const T& a, const T& b) const
  {
    return (a > b) ? a : b;
  }

}; //struct MaxValue

struct MinValue
{
  template <typename T>
  VISKORES_EXEC_CONT T operator()(const T& a, const T& b) const
  {
    return (a < b) ? a : b;
  }

}; //struct MinValue
}
}
} //namespace viskores::rendering::raytracing
#endif //viskores_rendering_raytracing_Worklets_h
