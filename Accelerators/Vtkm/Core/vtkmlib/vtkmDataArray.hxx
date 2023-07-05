// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2019 Sandia Corporation.
// SPDX-FileCopyrightText: Copyright 2019 UT-Battelle, LLC.
// SPDX-FileCopyrightText: Copyright 2019 Los Alamos National Security.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-LANL-USGov
#ifndef vtkmDataArray_hxx
#define vtkmDataArray_hxx

#include "vtkObjectFactory.h"

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleDecorator.h>
#include <vtkm/cont/ArrayHandleGroupVecVariable.h>

#include <atomic>
#include <cassert>
#include <mutex>
#include <type_traits>

namespace internal
{
VTK_ABI_NAMESPACE_BEGIN

///----------------------------------------------------------------------------
template <typename VecType,
  bool IsScalarType =
    std::is_same<VecType, typename vtkm::VecTraits<VecType>::ComponentType>::value>
struct VecFlatAccessImpl;

template <typename VecType>
struct VecFlatAccessImpl<VecType, true>
{
  using ValueType = VecType;

  static constexpr vtkm::IdComponent NUM_COMPONENTS = 1;

  VTKM_EXEC_CONT
  static vtkm::IdComponent GetNumberOfComponents(const VecType&) { return NUM_COMPONENTS; }

  VTKM_EXEC_CONT
  static ValueType GetComponent(const VecType& vec, vtkm::IdComponent) { return vec; }

  VTKM_EXEC_CONT
  static void SetComponent(VecType& vec, vtkm::IdComponent, const ValueType& val) { vec = val; }

  VTKM_EXEC_CONT
  static void CopyTo(const VecType& vec, ValueType* dest) { *dest = vec; }

  VTKM_EXEC_CONT
  static void CopyFrom(VecType& vec, const ValueType* src) { vec = *src; }
};

template <typename VecType, typename SizeTag = typename vtkm::VecTraits<VecType>::IsSizeStatic>
struct VecNumStaticComponents;

template <typename V>
struct VecNumStaticComponents<V, vtkm::VecTraitsTagSizeStatic>
{
  static constexpr vtkm::IdComponent Value = vtkm::VecTraits<V>::NUM_COMPONENTS;
};

template <typename V>
struct VecNumStaticComponents<V, vtkm::VecTraitsTagSizeVariable>
{
  static constexpr vtkm::IdComponent Value = 0;
};

/// Note that `NUM_COMPONENTS` will be 0 for vectors with non-static size.
/// Bellow, it is assumed that only the top level Vector will be a non-static-size type.
/// The code will need to be changed in the future if that assumption no longer holds true.
///
template <typename VecType>
struct VecFlatAccessImpl<VecType, false>
{
  using VecTraits = vtkm::VecTraits<VecType>;
  using SubVecFlatAccess = VecFlatAccessImpl<typename VecTraits::ComponentType>;
  using ValueType = typename SubVecFlatAccess::ValueType;

  static constexpr vtkm::IdComponent NUM_COMPONENTS =
    VecNumStaticComponents<VecType>::Value * SubVecFlatAccess::NUM_COMPONENTS;

  VTKM_EXEC_CONT
  static vtkm::IdComponent GetNumberOfComponents(const VecType& vec)
  {
    return VecTraits::GetNumberOfComponents(vec) * SubVecFlatAccess::NUM_COMPONENTS;
  }

  VTKM_EXEC_CONT
  static ValueType GetComponent(const VecType& vec, vtkm::IdComponent flatCompIdx)
  {
    auto compIdxThis = flatCompIdx / SubVecFlatAccess::NUM_COMPONENTS;
    auto compIdxSub = flatCompIdx % SubVecFlatAccess::NUM_COMPONENTS;
    return SubVecFlatAccess::GetComponent(VecTraits::GetComponent(vec, compIdxThis), compIdxSub);
  }

  VTKM_EXEC_CONT
  static void SetComponent(VecType& vec, vtkm::IdComponent flatCompIdx, const ValueType& val)
  {
    auto compIdxThis = flatCompIdx / SubVecFlatAccess::NUM_COMPONENTS;
    auto compIdxSub = flatCompIdx % SubVecFlatAccess::NUM_COMPONENTS;
    SubVecFlatAccess::SetComponent(VecTraits::GetComponent(vec, compIdxThis), compIdxSub, val);
  }

  VTKM_EXEC_CONT
  static void CopyTo(const VecType& vec, ValueType* dest)
  {
    for (vtkm::IdComponent i = 0; i < vtkm::VecTraits<VecType>::GetNumberOfComponents(vec); ++i)
    {
      SubVecFlatAccess::CopyTo(
        VecTraits::GetComponent(vec, i), dest + (i * SubVecFlatAccess::NUM_COMPONENTS));
    }
  }

  VTKM_EXEC_CONT
  static void CopyFrom(VecType& vec, const ValueType* src)
  {
    for (vtkm::IdComponent i = 0; i < vtkm::VecTraits<VecType>::GetNumberOfComponents(vec); ++i)
    {
      SubVecFlatAccess::CopyFrom(
        VecTraits::GetComponent(vec, i), src + (i * SubVecFlatAccess::NUM_COMPONENTS));
    }
  }
};

//-----------------------------------------------------------------------------
/// A single API to access a Scalar, Vector or a Nested-Vector as a Flat-Vector, with depth-first
/// ordering. This is also intended to work for vectors that do not have a compile time static
/// number of components. Note that `NUM_COMPONENTS` will be 0 for such vectors, use the
/// `GetNumberOfComponents` method instead. Also, it is assumed that only the top level Vector
/// can be a non-static-size type.
///
struct VecFlatAccess
{
  template <typename VecType>
  static constexpr vtkm::IdComponent NumberOfComponents =
    VecFlatAccessImpl<VecType>::NUM_COMPONENTS;

  template <typename VecType>
  VTKM_EXEC_CONT static vtkm::IdComponent GetNumberOfComponents(const VecType& vec)
  {
    return VecFlatAccessImpl<VecType>::GetNumberOfComponents(vec);
  }

  template <typename VecType>
  VTKM_EXEC_CONT static auto GetComponent(const VecType& vec, vtkm::IdComponent flatCompIdx)
  {
    return VecFlatAccessImpl<VecType>::GetComponent(vec, flatCompIdx);
  }

  template <typename VecType>
  VTKM_EXEC_CONT static void SetComponent(VecType& vec, vtkm::IdComponent flatCompIdx,
    const typename VecFlatAccessImpl<VecType>::ValueType& val)
  {
    VecFlatAccessImpl<VecType>::SetComponent(vec, flatCompIdx, val);
  }

  template <typename PortalType>
  VTKM_EXEC_CONT static void SetComponent(vtkm::VecFromPortal<PortalType>& vec,
    vtkm::IdComponent flatCompIdx,
    const typename VecFlatAccessImpl<vtkm::VecFromPortal<PortalType>>::ValueType& val)
  {
    using SubVecType = typename vtkm::VecFromPortal<PortalType>::ComponentType;

    auto compIdxThis = flatCompIdx / VecFlatAccessImpl<SubVecType>::NUM_COMPONENTS;
    auto compIdxSub = flatCompIdx % VecFlatAccessImpl<SubVecType>::NUM_COMPONENTS;

    SubVecType subVec = vec[compIdxThis];
    SetComponent(subVec, compIdxSub, val);
    vec[compIdxThis] = subVec;
  }

  template <typename VecType>
  VTKM_EXEC_CONT static void CopyTo(
    const VecType& vec, typename VecFlatAccessImpl<VecType>::ValueType* dest)
  {
    VecFlatAccessImpl<VecType>::CopyTo(vec, dest);
  }

  template <typename VecType>
  VTKM_EXEC_CONT static void CopyFrom(
    VecType& vec, const typename VecFlatAccessImpl<VecType>::ValueType* src)
  {
    VecFlatAccessImpl<VecType>::CopyFrom(vec, src);
  }

  template <typename PortalType>
  VTKM_EXEC_CONT static void CopyFrom(vtkm::VecFromPortal<PortalType>& vec,
    const typename VecFlatAccessImpl<vtkm::VecFromPortal<PortalType>>::ValueType* src)
  {
    using VecType = vtkm::VecFromPortal<PortalType>;
    using SubVecType = typename VecType::ComponentType;

    for (vtkm::IdComponent i = 0; i < vtkm::VecTraits<VecType>::GetNumberOfComponents(vec); ++i)
    {
      SubVecType subVec{};
      CopyFrom(subVec, src + (i * VecFlatAccessImpl<SubVecType>::NUM_COMPONENTS));
      vec[i] = subVec;
    }
  }
};

//-----------------------------------------------------------------------------
template <vtkm::IdComponent NUM_COMPONENTS>
struct MinMaxHelper
{
  using ValueType = vtkm::Vec<vtkm::Float64, NUM_COMPONENTS>;
  using Operator = vtkm::MinAndMax<ValueType>;
  using ResultType = vtkm::Vec<ValueType, 2>;

  VTKM_EXEC_CONT
  static ResultType Identity() { return { { VTK_DOUBLE_MAX }, { VTK_DOUBLE_MIN } }; }
};

/// Decorator classes for `ArrayHandleDecorator` to be used to wrap an array for computing its
/// range using `Reduce` while ignoring ghost and non-finite values. This is achieved by returning
/// the reduce identiy for such values.
///
/// These decorators Wrap an array to be processed, `Src`, and a (possibly empty) `Ghosts` array.
/// They test the values of the `Src` and `Ghosts` arrays and return the identity value when a
/// specific condition is met based on the other parameters.
/// `GhostValueToSkip`: Only used if the ghost array is not empty. Return identity if the
///                     bitwise-and of the value in the `Ghosts` array and this value is not zero.
/// `FinitesOnly`     : Return identiy if the value in the Src array is not a finite value.
///
/// Note that `DecoratorForScalarRanage` only works for static length Vec values.
///
struct DecoratorParameters
{
  vtkm::UInt8 GhostValueToSkip;
  bool FinitesOnly;
};

struct DecoratorForScalarRanage
{
public:
  DecoratorParameters Params;

  template <typename SrcPortal, typename GhostPortal>
  struct Functor
  {
    using InValueType = typename SrcPortal::ValueType;
    static constexpr auto NumComponents = VecFlatAccess::NumberOfComponents<InValueType>;
    using Helper = MinMaxHelper<NumComponents>;

    SrcPortal Src;
    GhostPortal Ghosts;
    DecoratorParameters Params;

    VTKM_EXEC_CONT
    typename Helper::ResultType operator()(vtkm::Id idx) const
    {
      if ((this->Ghosts.GetNumberOfValues() != 0) &&
        (this->Ghosts.Get(idx) & this->Params.GhostValueToSkip))
      {
        return Helper::Identity();
      }

      typename Helper::ResultType outVal;
      const auto& inVal = this->Src.Get(idx);
      for (vtkm::IdComponent i = 0; i < NumComponents; ++i)
      {
        auto val = static_cast<vtkm::Float64>(VecFlatAccess::GetComponent(inVal, i));
        if (this->Params.FinitesOnly && !vtkm::IsFinite(val))
        {
          outVal[0][i] = VTK_DOUBLE_MAX;
          outVal[1][i] = VTK_DOUBLE_MIN;
        }
        else
        {
          outVal[0][i] = outVal[1][i] = val;
        }
      }

      return outVal;
    }
  };

  template <typename SrcPortal, typename GhostPortal>
  Functor<SrcPortal, GhostPortal> CreateFunctor(const SrcPortal& sp, const GhostPortal& gp) const
  {
    return { sp, gp, this->Params };
  }
};

class DecoratorForVectorRanage
{
public:
  DecoratorParameters Params;

  template <typename SrcPortal, typename GhostPortal>
  struct Functor
  {
    using Helper = MinMaxHelper<1>;

    SrcPortal Src;
    GhostPortal Ghosts;
    DecoratorParameters Params;

    VTKM_EXEC_CONT
    typename Helper::ResultType operator()(vtkm::Id idx) const
    {
      if ((this->Ghosts.GetNumberOfValues() != 0) &&
        (this->Ghosts.Get(idx) & this->Params.GhostValueToSkip))
      {
        return Helper::Identity();
      }

      vtkm::Float64 outVal{};
      const auto& inVal = this->Src.Get(idx);
      for (vtkm::IdComponent i = 0; i < VecFlatAccess::GetNumberOfComponents(inVal); ++i)
      {
        auto comp = static_cast<vtkm::Float64>(VecFlatAccess::GetComponent(inVal, i));
        outVal += comp * comp;
        if (this->Params.FinitesOnly && !vtkm::IsFinite(outVal))
        {
          return Helper::Identity();
        }
      }

      return { outVal, outVal };
    }
  };

  template <typename SrcPortal, typename GhostPortal>
  Functor<SrcPortal, GhostPortal> CreateFunctor(const SrcPortal& sp, const GhostPortal& gp) const
  {
    return { sp, gp, this->Params };
  }
};

template <typename ArrayHandleType>
auto TransformForScalarRange(const ArrayHandleType& src,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>& ghost, vtkm::UInt8 ghostValueToSkip, bool finitesOnly)
{
  DecoratorForScalarRanage decorator{ ghostValueToSkip, finitesOnly };
  return vtkm::cont::make_ArrayHandleDecorator(src.GetNumberOfValues(), decorator, src, ghost);
}

template <typename ArrayHandleType>
auto TransformForVectorRange(const ArrayHandleType& src,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>& ghost, vtkm::UInt8 ghostValueToSkip, bool finitesOnly)
{
  DecoratorForVectorRanage decorator{ ghostValueToSkip, finitesOnly };
  return vtkm::cont::make_ArrayHandleDecorator(src.GetNumberOfValues(), decorator, src, ghost);
}

template <typename ArrayHandleType>
void ComputeArrayHandleScalarRange(const ArrayHandleType& values,
  const vtkm::cont::ArrayHandle<vtkm::UInt8> ghosts, vtkm::UInt8 ghostValueToSkip, bool finitesOnly,
  vtkm::Float64* ranges)
{
  static constexpr auto NumComponents =
    VecFlatAccess::NumberOfComponents<typename ArrayHandleType::ValueType>;
  using Helper = MinMaxHelper<NumComponents>;

  auto input = TransformForScalarRange(values, ghosts, ghostValueToSkip, finitesOnly);
  auto result =
    vtkm::cont::Algorithm::Reduce(input, Helper::Identity(), typename Helper::Operator());

  for (vtkm::IdComponent i = 0; i < NumComponents; ++i)
  {
    ranges[2 * i] = result[0][i];
    ranges[(2 * i) + 1] = result[1][i];
  }
}

template <typename ArrayHandleType>
void ComputeArrayHandleVectorRange(const ArrayHandleType& values,
  const vtkm::cont::ArrayHandle<vtkm::UInt8> ghosts, vtkm::UInt8 ghostValueToSkip, bool finitesOnly,
  vtkm::Float64 range[2])
{
  using Helper = MinMaxHelper<1>;

  auto input = TransformForVectorRange(values, ghosts, ghostValueToSkip, finitesOnly);
  auto result =
    vtkm::cont::Algorithm::Reduce(input, Helper::Identity(), typename Helper::Operator());

  range[0] = vtkm::Sqrt(result[0][0]);
  range[1] = vtkm::Sqrt(result[1][0]);
}

//-----------------------------------------------------------------------------

/// vtkmDataArray only supports array handles which store elements with a compile-time
/// constant Vec size. The only exception is a specialization of ArrayHandleGroupVecVariable
/// which is used to store arrays with elements with uncommon number of
/// components (i.e Vecs with number of components other than 2, 3 and 4).
///
template <typename T>
using ArrayHandleRuntimeVec = vtkm::cont::ArrayHandleGroupVecVariable<vtkm::cont::ArrayHandle<T>,
  vtkm::cont::ArrayHandleCounting<vtkm::Id>>;
template <typename T>
using ArrayHandleRuntimeVecBase = typename ArrayHandleRuntimeVec<T>::Superclass;

//-----------------------------------------------------------------------------
template <typename T>
class ArrayHandleHelperInterface
{
public:
  using ValueType = T;

  virtual ~ArrayHandleHelperInterface() = default;

  virtual bool IsReadOnly() const = 0;
  virtual vtkm::IdComponent GetNumberOfComponents() const = 0;
  virtual void GetTuple(vtkm::Id valIdx, ValueType* values) const = 0;
  virtual void SetTuple(vtkm::Id valIdx, const ValueType* values) = 0;
  virtual ValueType GetComponent(vtkm::Id valIdx, vtkm::IdComponent compIdx) const = 0;
  virtual void SetComponent(vtkm::Id valIdx, vtkm::IdComponent compIdx, const ValueType& value) = 0;
  virtual bool Reallocate(vtkm::Id numberOfValues) = 0;
  virtual bool ComputeScalarRange(double* ranges, const unsigned char* ghosts,
    vtkm::UInt8 ghostValueToSkip, bool finitesOnly) = 0;
  virtual bool ComputeVectorRange(double range[2], const unsigned char* ghosts,
    vtkm::UInt8 ghostValueToSkip, bool finitesOnly) = 0;
  virtual vtkm::cont::UnknownArrayHandle GetArrayHandle() const = 0;
};

template <typename ArrayHandleType>
class ArrayHandleHelper
  : public ArrayHandleHelperInterface<
      typename vtkm::VecTraits<typename ArrayHandleType::ValueType>::BaseComponentType>
{
private:
  using VecType = typename ArrayHandleType::ValueType;
  using ValueType = typename vtkm::VecTraits<VecType>::BaseComponentType;
  using WritableTag = typename vtkm::cont::internal::IsWritableArrayHandle<ArrayHandleType>::type;

public:
  explicit ArrayHandleHelper(const ArrayHandleType& arrayHandle)
    : Array(arrayHandle)
    , ReadPortalValid(false)
    , WritePortalValid(false)
  {
  }

  bool IsReadOnly() const override { return !WritableTag::value; }

private:
  template <typename AH>
  static vtkm::IdComponent GetNumberOfComponentsImpl(const AH&)
  {
    return VecFlatAccess::NumberOfComponents<typename AH::ValueType>;
  }

  static vtkm::IdComponent GetNumberOfComponentsImpl(
    const ArrayHandleRuntimeVecBase<typename vtkm::VecTraits<VecType>::ComponentType>& ah)
  {
    using ComponentType = typename vtkm::VecTraits<VecType>::ComponentType;

    const auto& sub = static_cast<const ArrayHandleRuntimeVec<ComponentType>&>(ah);
    return sub.GetOffsetsArray().GetStep() * VecFlatAccess::NumberOfComponents<ComponentType>;
  }

public:
  vtkm::IdComponent GetNumberOfComponents() const override
  {
    return this->GetNumberOfComponentsImpl(this->Array);
  }

  void GetTuple(vtkm::Id valIdx, ValueType* values) const override
  {
    assert(valIdx < this->Array.GetNumberOfValues());

    auto portal = this->GetReadPortal();
    VecFlatAccess::CopyTo(portal.Get(valIdx), values);
  }

private:
  void SetTupleImpl(vtkm::Id valIdx, const ValueType* values, std::true_type)
  {
    assert(valIdx < this->Array.GetNumberOfValues());

    auto portal = this->GetWritePortal();
    // this line is needed for this to work for ArrayHandleGroupVecVariable
    auto toSet = portal.Get(valIdx);
    VecFlatAccess::CopyFrom(toSet, values);
    portal.Set(valIdx, toSet);
  }

  void SetTupleImpl(vtkm::Id, const ValueType*, std::false_type) const {}

public:
  void SetTuple(vtkm::Id valIdx, const ValueType* values) override
  {
    this->SetTupleImpl(valIdx, values, WritableTag{});
  }

  ValueType GetComponent(vtkm::Id valIdx, vtkm::IdComponent compIdx) const override
  {
    assert(compIdx < this->GetNumberOfComponents());
    assert(valIdx < this->Array.GetNumberOfValues());

    auto portal = this->GetReadPortal();
    return VecFlatAccess::GetComponent(portal.Get(valIdx), compIdx);
  }

private:
  void SetComponentImpl(
    vtkm::Id valIdx, vtkm::IdComponent compIdx, const ValueType& value, std::true_type)
  {
    assert(compIdx < this->GetNumberOfComponents());
    assert(valIdx < this->Array.GetNumberOfValues());

    auto portal = this->GetWritePortal();
    auto toSet = portal.Get(valIdx);
    VecFlatAccess::SetComponent(toSet, compIdx, value);
    portal.Set(valIdx, toSet);
  }

  void SetComponentImpl(vtkm::Id, vtkm::IdComponent, const ValueType&, std::false_type) const {}

public:
  void SetComponent(vtkm::Id valIdx, vtkm::IdComponent compIdx, const ValueType& value) override
  {
    this->SetComponentImpl(valIdx, compIdx, value, WritableTag{});
  }

  bool Reallocate(vtkm::Id numberOfValues) override
  try
  {
    this->Array.Allocate(numberOfValues, vtkm::CopyFlag::On);
    this->ReadPortalValid = false;
    this->WritePortalValid = false;
    return true;
  }
  catch (const vtkm::cont::Error& e)
  {
    vtkGenericWarningMacro(<< "Warning: ReallocateTuples: " << e.GetMessage());
    return false;
  }

private:
  template <typename AH>
  static void ComputeScalarRangeImpl(const AH& array,
    const vtkm::cont::ArrayHandle<vtkm::UInt8>& ghostsArrayHandle, vtkm::UInt8 ghostValueToSkip,
    bool finitesOnly, double* ranges)
  {
    ComputeArrayHandleScalarRange(array, ghostsArrayHandle, ghostValueToSkip, finitesOnly, ranges);
  }

  static void ComputeScalarRangeImpl(
    const ArrayHandleRuntimeVecBase<typename vtkm::VecTraits<VecType>::ComponentType>& array,
    const vtkm::cont::ArrayHandle<vtkm::UInt8>& ghostsArrayHandle, vtkm::UInt8 ghostValueToSkip,
    bool finitesOnly, double* ranges)
  {
    using ComponentType = typename vtkm::VecTraits<VecType>::ComponentType;

    const auto& sub = static_cast<const ArrayHandleRuntimeVec<ComponentType>&>(array);
    auto components = sub.GetComponentsArray();
    auto offsets = sub.GetOffsetsArray();

    for (vtkm::Id i = 0; i < offsets.GetStep(); ++i)
    {
      vtkm::cont::ArrayHandleStride<ComponentType> compArray(
        components, sub.GetNumberOfValues(), offsets.GetStep(), i);

      ComputeArrayHandleScalarRange(
        static_cast<typename vtkm::cont::ArrayHandleStride<ComponentType>::Superclass>(compArray),
        ghostsArrayHandle, ghostValueToSkip, finitesOnly,
        ranges + (i * 2 * VecFlatAccess::NumberOfComponents<ComponentType>));
    }
  }

public:
  bool ComputeScalarRange(double* ranges, const unsigned char* ghosts, vtkm::UInt8 ghostValueToSkip,
    bool finitesOnly) override
  {
    if (this->Array.GetNumberOfValues() == 0)
    {
      for (int i = 0; i < this->GetNumberOfComponents(); ++i)
      {
        ranges[2 * i] = VTK_DOUBLE_MAX;
        ranges[(2 * i) + 1] = VTK_DOUBLE_MIN;
      }
      return false;
    }

    vtkm::cont::ArrayHandle<vtkm::UInt8> ghostsArrayHandle;
    if (ghosts)
    {
      ghostsArrayHandle =
        vtkm::cont::make_ArrayHandle(ghosts, this->Array.GetNumberOfValues(), vtkm::CopyFlag::Off);
    }

    this->ComputeScalarRangeImpl(
      this->Array, ghostsArrayHandle, ghostValueToSkip, finitesOnly, ranges);

    // Invalidate the write portal, so that a new write portal is created for further writes which
    // will appropriately signal the need to update the device side buffers
    this->WritePortalValid = false;

    return true;
  }

  bool ComputeVectorRange(double range[2], const unsigned char* ghosts,
    vtkm::UInt8 ghostValueToSkip, bool finitesOnly) override
  {
    // Faster path for 1 component arrays
    if (this->GetNumberOfComponents() == 1)
    {
      return this->ComputeScalarRange(range, ghosts, ghostValueToSkip, finitesOnly);
    }

    if (this->Array.GetNumberOfValues() == 0)
    {
      range[0] = VTK_DOUBLE_MAX;
      range[1] = VTK_DOUBLE_MIN;
      return false;
    }

    vtkm::cont::ArrayHandle<vtkm::UInt8> ghostsArrayHandle;
    if (ghosts)
    {
      ghostsArrayHandle =
        vtkm::cont::make_ArrayHandle(ghosts, this->Array.GetNumberOfValues(), vtkm::CopyFlag::Off);
    }

    ComputeArrayHandleVectorRange(
      this->Array, ghostsArrayHandle, ghostValueToSkip, finitesOnly, range);

    // Invalidate the write portal, so that a new write portal is created for further writes which
    // will appropriately signal the need to update the device side buffers
    this->WritePortalValid = false;

    return true;
  }

  vtkm::cont::UnknownArrayHandle GetArrayHandle() const override
  {
    this->ReadPortalValid = false;
    this->WritePortalValid = false;
    return this->Array;
  }

protected:
  ArrayHandleType Array;

  typename ArrayHandleType::ReadPortalType GetReadPortal() const
  {
    if (!this->ReadPortalValid)
    {
      std::lock_guard<std::mutex> lock(this->PortalCreationLock);
      if (!this->ReadPortalValid)
      {
        this->ReadPortal = this->Array.ReadPortal();
        this->ReadPortalValid = true;
      }
    }
    return this->ReadPortal;
  }

  typename ArrayHandleType::WritePortalType GetWritePortal() const
  {
    if (!this->WritePortalValid)
    {
      std::lock_guard<std::mutex> lock(this->PortalCreationLock);
      if (!this->WritePortalValid)
      {
        this->WritePortal = this->Array.WritePortal();
        this->WritePortalValid = true;
      }
    }
    return this->WritePortal;
  }

private:
  mutable std::mutex PortalCreationLock;

  mutable std::atomic_bool ReadPortalValid;
  mutable typename ArrayHandleType::ReadPortalType ReadPortal;

  mutable std::atomic_bool WritePortalValid;
  mutable typename ArrayHandleType::WritePortalType WritePortal;
};

template <typename T, typename S>
inline auto NewArrayHandleHelper(const vtkm::cont::ArrayHandle<T, S>& ah)
{
  return new ArrayHandleHelper<vtkm::cont::ArrayHandle<T, S>>(ah);
}

VTK_ABI_NAMESPACE_END
} // internal

VTK_ABI_NAMESPACE_BEGIN
//=============================================================================
template <typename T>
vtkmDataArray<T>::vtkmDataArray() = default;

template <typename T>
vtkmDataArray<T>::~vtkmDataArray() = default;

template <typename T>
vtkmDataArray<T>* vtkmDataArray<T>::New()
{
  VTK_STANDARD_NEW_BODY(vtkmDataArray<T>);
}

template <typename T>
template <typename V, typename S>
void vtkmDataArray<T>::SetVtkmArrayHandle(const vtkm::cont::ArrayHandle<V, S>& ah)
{
  static_assert(std::is_same<T, typename vtkm::VecTraits<V>::BaseComponentType>::value,
    "Base Component types of the arrays don't match");
  static_assert(
    std::is_same<typename vtkm::VecTraits<V>::IsSizeStatic, vtkm::VecTraitsTagSizeStatic>::value ||
      std::is_same<vtkm::cont::ArrayHandle<V, S>,
        internal::ArrayHandleRuntimeVecBase<typename vtkm::VecTraits<V>::ComponentType>>::value,
    "Unsupported ArrayHandle type");

  this->Helper.reset(internal::NewArrayHandleHelper(ah));

  this->SetNumberOfComponents(this->Helper->GetNumberOfComponents());
  this->Size = ah.GetNumberOfValues() * this->GetNumberOfComponents();
  this->MaxId = this->Size - 1;
}

template <typename T>
vtkm::cont::UnknownArrayHandle vtkmDataArray<T>::GetVtkmUnknownArrayHandle() const
{
  if (this->Helper)
  {
    return this->Helper->GetArrayHandle();
  }
  return {};
}

//-----------------------------------------------------------------------------
template <typename T>
auto vtkmDataArray<T>::GetValue(vtkIdType valueIdx) const -> ValueType
{
  assert(this->Helper);
  auto idx = valueIdx / this->GetNumberOfComponents();
  auto comp = valueIdx % this->GetNumberOfComponents();
  return this->Helper->GetComponent(idx, comp);
}

template <typename T>
void vtkmDataArray<T>::SetValue(vtkIdType valueIdx, ValueType value)
{
  assert(this->Helper);
  if (this->Helper->IsReadOnly())
  {
    vtkErrorMacro(<< "Underlying ArrayHandle (" << this->Helper->GetArrayHandle().GetArrayTypeName()
                  << ") does not support writes through vtkmDataArray");
    return;
  }

  auto idx = valueIdx / this->GetNumberOfComponents();
  auto comp = valueIdx % this->GetNumberOfComponents();
  this->Helper->SetComponent(idx, comp, value);
}

template <typename T>
void vtkmDataArray<T>::GetTypedTuple(vtkIdType tupleIdx, ValueType* tuple) const
{
  assert(this->Helper);
  this->Helper->GetTuple(tupleIdx, tuple);
}

template <typename T>
void vtkmDataArray<T>::SetTypedTuple(vtkIdType tupleIdx, const ValueType* tuple)
{
  assert(this->Helper);
  if (this->Helper->IsReadOnly())
  {
    vtkErrorMacro(<< "Underlying ArrayHandle (" << this->Helper->GetArrayHandle().GetArrayTypeName()
                  << ") is read-only");
    return;
  }

  this->Helper->SetTuple(tupleIdx, tuple);
}

template <typename T>
auto vtkmDataArray<T>::GetTypedComponent(vtkIdType tupleIdx, int compIdx) const -> ValueType
{
  assert(this->Helper);
  return this->Helper->GetComponent(tupleIdx, compIdx);
}

template <typename T>
void vtkmDataArray<T>::SetTypedComponent(vtkIdType tupleIdx, int compIdx, ValueType value)
{
  assert(this->Helper);
  if (this->Helper->IsReadOnly())
  {
    vtkErrorMacro(<< "Underlying ArrayHandle (" << this->Helper->GetArrayHandle().GetArrayTypeName()
                  << ") is read-only");
    return;
  }

  this->Helper->SetComponent(tupleIdx, compIdx, value);
}

//-----------------------------------------------------------------------------
template <typename T>
bool vtkmDataArray<T>::ComputeScalarRange(
  double* ranges, const unsigned char* ghosts, unsigned char ghostsToSkip)
{
  if (this->Helper)
  {
    return this->Helper->ComputeScalarRange(ranges, ghosts, ghostsToSkip, false);
  }
  return false;
}

template <typename T>
bool vtkmDataArray<T>::ComputeVectorRange(
  double range[2], const unsigned char* ghosts, unsigned char ghostsToSkip)
{
  if (this->Helper)
  {
    return this->Helper->ComputeVectorRange(range, ghosts, ghostsToSkip, false);
  }
  return false;
}

template <typename T>
bool vtkmDataArray<T>::ComputeFiniteScalarRange(
  double* ranges, const unsigned char* ghosts, unsigned char ghostsToSkip)
{
  if (this->Helper)
  {
    return this->Helper->ComputeScalarRange(ranges, ghosts, ghostsToSkip, true);
  }
  return false;
}

template <typename T>
bool vtkmDataArray<T>::ComputeFiniteVectorRange(
  double range[2], const unsigned char* ghosts, unsigned char ghostsToSkip)
{
  if (this->Helper)
  {
    return this->Helper->ComputeVectorRange(range, ghosts, ghostsToSkip, true);
  }
  return false;
}

//-----------------------------------------------------------------------------
template <typename T>
bool vtkmDataArray<T>::AllocateTuples(vtkIdType numberOfTuples)
{
  switch (this->NumberOfComponents)
  {
    case 1:
    {
      vtkm::cont::ArrayHandle<T> ah;
      ah.Allocate(numberOfTuples);
      this->Helper.reset(internal::NewArrayHandleHelper(ah));
      break;
    }
    case 2:
    {
      vtkm::cont::ArrayHandle<vtkm::Vec<T, 2>> ah;
      ah.Allocate(numberOfTuples);
      this->Helper.reset(internal::NewArrayHandleHelper(ah));
      break;
    }
    case 3:
    {
      vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> ah;
      ah.Allocate(numberOfTuples);
      this->Helper.reset(internal::NewArrayHandleHelper(ah));
      break;
    }
    case 4:
    {
      vtkm::cont::ArrayHandle<vtkm::Vec<T, 4>> ah;
      ah.Allocate(numberOfTuples);
      this->Helper.reset(internal::NewArrayHandleHelper(ah));
      break;
    }
    default:
    {
      vtkm::cont::ArrayHandle<T> ah;
      ah.Allocate(numberOfTuples * this->NumberOfComponents);
      vtkm::cont::ArrayHandleCounting<vtkm::Id> offsets(
        0, this->NumberOfComponents, numberOfTuples + 1);
      auto aosArray = vtkm::cont::make_ArrayHandleGroupVecVariable(ah, offsets);
      this->Helper.reset(internal::NewArrayHandleHelper(aosArray));
      break;
    }
  }

  // Size and MaxId are updated by the caller
  return true;
}

template <typename T>
bool vtkmDataArray<T>::ReallocateTuples(vtkIdType numberOfTuples)
{
  if (this->Helper)
  {
    // Size and MaxId are updated by the caller
    return this->Helper->Reallocate(numberOfTuples);
  }
  return false;
}

VTK_ABI_NAMESPACE_END
#endif // vtkmDataArray_hxx
