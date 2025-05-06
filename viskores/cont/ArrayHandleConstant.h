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
#ifndef viskores_cont_ArrayHandleConstant_h
#define viskores_cont_ArrayHandleConstant_h

#include <viskores/cont/ArrayExtractComponent.h>
#include <viskores/cont/ArrayHandleImplicit.h>

#include <viskores/cont/internal/ArrayRangeComputeUtils.h>

#include <viskores/Range.h>
#include <viskores/VecFlat.h>
#include <viskores/VectorAnalysis.h>

namespace viskores
{
namespace cont
{

struct VISKORES_ALWAYS_EXPORT StorageTagConstant
{
};

namespace internal
{

template <typename ValueType>
struct VISKORES_ALWAYS_EXPORT ConstantFunctor
{
  VISKORES_EXEC_CONT
  ConstantFunctor(const ValueType& value = ValueType())
    : Value(value)
  {
  }

  VISKORES_EXEC_CONT
  ValueType operator()(viskores::Id viskoresNotUsed(index)) const { return this->Value; }

private:
  ValueType Value;
};

template <typename T>
using StorageTagConstantSuperclass =
  typename viskores::cont::ArrayHandleImplicit<ConstantFunctor<T>>::StorageTag;

template <typename T>
struct Storage<T, viskores::cont::StorageTagConstant> : Storage<T, StorageTagConstantSuperclass<T>>
{
};

} // namespace internal

/// @brief An array handle with a constant value.
///
/// `ArrayHandleConstant` is an implicit array handle with a constant value. A
/// constant array handle is constructed by giving a value and an array length.
/// The resulting array is of the given size with each entry the same value
/// given in the constructor. The array is defined implicitly, so there it
/// takes (almost) no memory.
///
template <typename T>
class ArrayHandleConstant
  : public viskores::cont::ArrayHandle<T, viskores::cont::StorageTagConstant>
{
public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleConstant,
    (ArrayHandleConstant<T>),
    (viskores::cont::ArrayHandle<T, viskores::cont::StorageTagConstant>));

  /// Construct a constant array containing the given value.
  VISKORES_CONT
  ArrayHandleConstant(T value, viskores::Id numberOfValues = 0)
    : Superclass(internal::FunctorToArrayHandleImplicitBuffers(internal::ConstantFunctor<T>(value),
                                                               numberOfValues))
  {
  }

  /// @brief Returns the constant value stored in this array.
  ///
  /// The value set in the constructor of this array is returned even if the number of values is 0.
  ///
  VISKORES_CONT T GetValue() const { return this->ReadPortal().GetFunctor()(0); }
};

/// `make_ArrayHandleConstant` is convenience function to generate an
/// ArrayHandleImplicit.
template <typename T>
viskores::cont::ArrayHandleConstant<T> make_ArrayHandleConstant(T value,
                                                                viskores::Id numberOfValues)
{
  return viskores::cont::ArrayHandleConstant<T>(value, numberOfValues);
}

namespace internal
{

template <>
struct VISKORES_CONT_EXPORT ArrayExtractComponentImpl<viskores::cont::StorageTagConstant>
{
  template <typename T>
  VISKORES_CONT auto operator()(
    const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagConstant>& src,
    viskores::IdComponent componentIndex,
    viskores::CopyFlag allowCopy) const
  {
    if (allowCopy != viskores::CopyFlag::On)
    {
      throw viskores::cont::ErrorBadValue(
        "Cannot extract component of ArrayHandleConstant without copying. "
        "(However, the whole array does not need to be copied.)");
    }

    viskores::cont::ArrayHandleConstant<T> srcArray = src;

    viskores::VecFlat<T> vecValue{ srcArray.GetValue() };

    // Make a basic array with one entry (the constant value).
    auto basicArray = viskores::cont::make_ArrayHandle({ vecValue[componentIndex] });

    // Set up a modulo = 1 so all indices go to this one value.
    return viskores::cont::make_ArrayHandleStride(basicArray, src.GetNumberOfValues(), 1, 0, 1, 1);
  }
};

template <typename S>
struct ArrayRangeComputeImpl;

template <>
struct VISKORES_CONT_EXPORT ArrayRangeComputeImpl<viskores::cont::StorageTagConstant>
{
  template <typename T>
  VISKORES_CONT viskores::cont::ArrayHandle<viskores::Range> operator()(
    const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagConstant>& input,
    const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
    bool computeFiniteRange,
    viskores::cont::DeviceAdapterId devId) const
  {
    bool allMasked = false;
    if (maskArray.GetNumberOfValues() != 0)
    {
      // Find if there is atleast one value that is not masked
      auto ids = GetFirstAndLastUnmaskedIndices(maskArray, devId);
      allMasked = (ids[1] < ids[0]);
    }

    auto value = viskores::make_VecFlat(input.ReadPortal().Get(0));

    viskores::cont::ArrayHandle<viskores::Range> result;
    result.Allocate(value.GetNumberOfComponents());
    auto resultPortal = result.WritePortal();
    for (viskores::IdComponent index = 0; index < value.GetNumberOfComponents(); ++index)
    {
      auto comp = static_cast<viskores::Float64>(value[index]);
      if (allMasked || (computeFiniteRange && !viskores::IsFinite(comp)))
      {
        resultPortal.Set(index, viskores::Range{});
      }
      else
      {
        resultPortal.Set(index, viskores::Range{ comp, comp });
      }
    }
    return result;
  }
};

template <typename S>
struct ArrayRangeComputeMagnitudeImpl;

template <>
struct VISKORES_CONT_EXPORT ArrayRangeComputeMagnitudeImpl<viskores::cont::StorageTagConstant>
{
  template <typename T>
  VISKORES_CONT viskores::Range operator()(
    const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagConstant>& input,
    const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
    bool computeFiniteRange,
    viskores::cont::DeviceAdapterId devId) const
  {
    if (maskArray.GetNumberOfValues() != 0)
    {
      // Find if there is atleast one value that is not masked
      auto ids = GetFirstAndLastUnmaskedIndices(maskArray, devId);
      if (ids[1] < ids[0])
      {
        return viskores::Range{};
      }
    }

    auto value = input.ReadPortal().Get(0);
    viskores::Float64 rangeValue = viskores::Magnitude(viskores::make_VecFlat(value));
    return (computeFiniteRange && !viskores::IsFinite(rangeValue))
      ? viskores::Range{}
      : viskores::Range{ rangeValue, rangeValue };
  }
};

} // namespace internal

}
} // viskores::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace viskores
{
namespace cont
{

template <typename T>
struct SerializableTypeString<viskores::cont::ArrayHandleConstant<T>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "AH_Constant<" + SerializableTypeString<T>::Get() + ">";
    return name;
  }
};

template <typename T>
struct SerializableTypeString<viskores::cont::ArrayHandle<T, viskores::cont::StorageTagConstant>>
  : SerializableTypeString<viskores::cont::ArrayHandleConstant<T>>
{
};
}
} // viskores::cont

namespace mangled_diy_namespace
{

template <typename T>
struct Serialization<viskores::cont::ArrayHandleConstant<T>>
{
private:
  using Type = viskores::cont::ArrayHandleConstant<T>;
  using BaseType = viskores::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    viskoresdiy::save(bb, obj.GetNumberOfValues());
    viskoresdiy::save(bb, obj.ReadPortal().Get(0));
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    viskores::Id count = 0;
    viskoresdiy::load(bb, count);

    T value;
    viskoresdiy::load(bb, value);

    obj = viskores::cont::make_ArrayHandleConstant(value, count);
  }
};

template <typename T>
struct Serialization<viskores::cont::ArrayHandle<T, viskores::cont::StorageTagConstant>>
  : Serialization<viskores::cont::ArrayHandleConstant<T>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //viskores_cont_ArrayHandleConstant_h
