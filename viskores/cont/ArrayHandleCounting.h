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
#ifndef viskores_cont_ArrayHandleCounting_h
#define viskores_cont_ArrayHandleCounting_h

#include <viskores/cont/ArrayHandleImplicit.h>

#include <viskores/cont/internal/ArrayRangeComputeUtils.h>

#include <viskores/Range.h>
#include <viskores/TypeTraits.h>
#include <viskores/VecFlat.h>
#include <viskores/VecTraits.h>

namespace viskores
{
namespace cont
{

struct VISKORES_ALWAYS_EXPORT StorageTagCounting
{
};

namespace internal
{

/// \brief An implicit array portal that returns a counting value.
template <class CountingValueType>
class VISKORES_ALWAYS_EXPORT ArrayPortalCounting
{
  using ComponentType = typename viskores::VecTraits<CountingValueType>::ComponentType;

public:
  using ValueType = CountingValueType;

  VISKORES_EXEC_CONT
  ArrayPortalCounting()
    : Start(0)
    , Step(1)
    , NumberOfValues(0)
  {
  }

  /// @brief Create an implicit counting array.
  ///
  /// @param start The starting value in the first value of the array.
  /// @param step The increment between sequential values in the array.
  /// @param numValues The size of the array.
  VISKORES_EXEC_CONT
  ArrayPortalCounting(ValueType start, ValueType step, viskores::Id numValues)
    : Start(start)
    , Step(step)
    , NumberOfValues(numValues)
  {
  }

  /// Returns the starting value.
  VISKORES_EXEC_CONT
  ValueType GetStart() const { return this->Start; }

  /// Returns the step value.
  VISKORES_EXEC_CONT
  ValueType GetStep() const { return this->Step; }

  /// Returns the number of values in the array.
  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const { return this->NumberOfValues; }

  /// Returns the value for the given index.
  VISKORES_EXEC_CONT
  ValueType Get(viskores::Id index) const
  {
    return ValueType(this->Start + this->Step * ValueType(static_cast<ComponentType>(index)));
  }

private:
  ValueType Start;
  ValueType Step;
  viskores::Id NumberOfValues;
};

namespace detail
{

template <typename T>
struct CanCountImpl
{
  using VTraits = viskores::VecTraits<T>;
  using BaseType = typename VTraits::BaseComponentType;
  using TTraits = viskores::TypeTraits<BaseType>;
  static constexpr bool IsNumeric =
    !std::is_same<typename TTraits::NumericTag, viskores::TypeTraitsUnknownTag>::value;
  static constexpr bool IsBool = std::is_same<BaseType, bool>::value;

  static constexpr bool value = IsNumeric && !IsBool;
};

} // namespace detail

// Not all types can be counted.
template <typename T>
struct CanCount
{
  static constexpr bool value = detail::CanCountImpl<T>::value;
};

template <typename T>
using StorageTagCountingSuperclass =
  viskores::cont::StorageTagImplicit<internal::ArrayPortalCounting<T>>;

template <typename T>
struct Storage<
  T,
  typename std::enable_if<CanCount<T>::value, viskores::cont::StorageTagCounting>::type>
  : Storage<T, StorageTagCountingSuperclass<T>>
{
};

} // namespace internal

/// ArrayHandleCounting is a specialization of ArrayHandle. By default it
/// contains a increment value, that is increment for each step between zero
/// and the passed in length
template <typename CountingValueType>
class ArrayHandleCounting
  : public viskores::cont::ArrayHandle<CountingValueType, viskores::cont::StorageTagCounting>
{
public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleCounting,
    (ArrayHandleCounting<CountingValueType>),
    (viskores::cont::ArrayHandle<CountingValueType, StorageTagCounting>));

  VISKORES_CONT
  ArrayHandleCounting(CountingValueType start, CountingValueType step, viskores::Id length)
    : Superclass(internal::PortalToArrayHandleImplicitBuffers(
        internal::ArrayPortalCounting<CountingValueType>(start, step, length)))
  {
  }

  VISKORES_CONT CountingValueType GetStart() const { return this->ReadPortal().GetStart(); }

  VISKORES_CONT CountingValueType GetStep() const { return this->ReadPortal().GetStep(); }
};

/// A convenience function for creating an ArrayHandleCounting. It takes the
/// value to start counting from and and the number of times to increment.
template <typename CountingValueType>
VISKORES_CONT viskores::cont::ArrayHandleCounting<CountingValueType>
make_ArrayHandleCounting(CountingValueType start, CountingValueType step, viskores::Id length)
{
  return viskores::cont::ArrayHandleCounting<CountingValueType>(start, step, length);
}

namespace internal
{

template <typename S>
struct ArrayRangeComputeImpl;

template <>
struct VISKORES_CONT_EXPORT ArrayRangeComputeImpl<viskores::cont::StorageTagCounting>
{
  template <typename T>
  VISKORES_CONT viskores::cont::ArrayHandle<viskores::Range> operator()(
    const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagCounting>& input,
    const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
    bool viskoresNotUsed(computeFiniteRange), // assume array produces only finite values
    viskores::cont::DeviceAdapterId device) const
  {
    using Traits = viskores::VecTraits<viskores::VecFlat<T>>;
    viskores::cont::ArrayHandle<viskores::Range> result;
    result.Allocate(Traits::NUM_COMPONENTS);

    if (input.GetNumberOfValues() <= 0)
    {
      result.Fill(viskores::Range{});
      return result;
    }

    viskores::Id2 firstAndLast{ 0, input.GetNumberOfValues() - 1 };
    if (maskArray.GetNumberOfValues() > 0)
    {
      firstAndLast = GetFirstAndLastUnmaskedIndices(maskArray, device);
    }

    if (firstAndLast[1] < firstAndLast[0])
    {
      result.Fill(viskores::Range{});
      return result;
    }

    auto portal = result.WritePortal();
    // assume the values to be finite
    auto first = make_VecFlat(input.ReadPortal().Get(firstAndLast[0]));
    auto last = make_VecFlat(input.ReadPortal().Get(firstAndLast[1]));
    for (viskores::IdComponent cIndex = 0; cIndex < Traits::NUM_COMPONENTS; ++cIndex)
    {
      auto firstComponent = Traits::GetComponent(first, cIndex);
      auto lastComponent = Traits::GetComponent(last, cIndex);
      portal.Set(cIndex,
                 viskores::Range(viskores::Min(firstComponent, lastComponent),
                                 viskores::Max(firstComponent, lastComponent)));
    }

    return result;
  }
};

} // namespace internal

}
} // namespace viskores::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace viskores
{
namespace cont
{

template <typename T>
struct SerializableTypeString<viskores::cont::ArrayHandleCounting<T>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "AH_Counting<" + SerializableTypeString<T>::Get() + ">";
    return name;
  }
};

template <typename T>
struct SerializableTypeString<viskores::cont::ArrayHandle<T, viskores::cont::StorageTagCounting>>
  : SerializableTypeString<viskores::cont::ArrayHandleCounting<T>>
{
};
}
} // viskores::cont

namespace mangled_diy_namespace
{

template <typename T>
struct Serialization<viskores::cont::ArrayHandleCounting<T>>
{
private:
  using Type = viskores::cont::ArrayHandleCounting<T>;
  using BaseType = viskores::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    auto portal = obj.ReadPortal();
    viskoresdiy::save(bb, portal.GetStart());
    viskoresdiy::save(bb, portal.GetStep());
    viskoresdiy::save(bb, portal.GetNumberOfValues());
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    T start{}, step{};
    viskores::Id count = 0;

    viskoresdiy::load(bb, start);
    viskoresdiy::load(bb, step);
    viskoresdiy::load(bb, count);

    obj = viskores::cont::make_ArrayHandleCounting(start, step, count);
  }
};

template <typename T>
struct Serialization<viskores::cont::ArrayHandle<T, viskores::cont::StorageTagCounting>>
  : Serialization<viskores::cont::ArrayHandleCounting<T>>
{
};
} // diy
/// @endcond SERIALIZATION

#endif //viskores_cont_ArrayHandleCounting_h
