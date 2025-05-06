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
#ifndef viskores_cont_ArrayHandleCartesianProduct_h
#define viskores_cont_ArrayHandleCartesianProduct_h

#include <viskores/Assert.h>
#include <viskores/Range.h>
#include <viskores/VecTraits.h>

#include <viskores/cont/ArrayExtractComponent.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ErrorBadAllocation.h>
#include <viskores/cont/Token.h>

#include <array>

namespace viskores
{
namespace internal
{

/// \brief An array portal that acts as a 3D cartesian product of 3 arrays.
///
template <typename ValueType_,
          typename PortalTypeFirst_,
          typename PortalTypeSecond_,
          typename PortalTypeThird_>
class VISKORES_ALWAYS_EXPORT ArrayPortalCartesianProduct
{
public:
  using ValueType = ValueType_;
  using IteratorType = ValueType_;
  using PortalTypeFirst = PortalTypeFirst_;
  using PortalTypeSecond = PortalTypeSecond_;
  using PortalTypeThird = PortalTypeThird_;

  using set_supported_p1 = viskores::internal::PortalSupportsSets<PortalTypeFirst>;
  using set_supported_p2 = viskores::internal::PortalSupportsSets<PortalTypeSecond>;
  using set_supported_p3 = viskores::internal::PortalSupportsSets<PortalTypeThird>;

  using Writable = std::integral_constant<bool,
                                          set_supported_p1::value && set_supported_p2::value &&
                                            set_supported_p3::value>;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ArrayPortalCartesianProduct()
    : PortalFirst()
    , PortalSecond()
    , PortalThird()
  {
  } //needs to be host and device so that cuda can create lvalue of these

  VISKORES_CONT
  ArrayPortalCartesianProduct(const PortalTypeFirst& portalfirst,
                              const PortalTypeSecond& portalsecond,
                              const PortalTypeThird& portalthird)
    : PortalFirst(portalfirst)
    , PortalSecond(portalsecond)
    , PortalThird(portalthird)
  {
  }

  /// Copy constructor for any other ArrayPortalCartesianProduct with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///

  template <class OtherV, class OtherP1, class OtherP2, class OtherP3>
  VISKORES_CONT ArrayPortalCartesianProduct(
    const ArrayPortalCartesianProduct<OtherV, OtherP1, OtherP2, OtherP3>& src)
    : PortalFirst(src.GetPortalFirst())
    , PortalSecond(src.GetPortalSecond())
    , PortalThird(src.GetPortalThird())
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const
  {
    return this->PortalFirst.GetNumberOfValues() * this->PortalSecond.GetNumberOfValues() *
      this->PortalThird.GetNumberOfValues();
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ValueType Get(viskores::Id index) const
  {
    VISKORES_ASSERT(index >= 0);
    VISKORES_ASSERT(index < this->GetNumberOfValues());

    viskores::Id dim1 = this->PortalFirst.GetNumberOfValues();
    viskores::Id dim2 = this->PortalSecond.GetNumberOfValues();
    viskores::Id dim12 = dim1 * dim2;
    viskores::Id idx12 = index % dim12;
    viskores::Id i1 = idx12 % dim1;
    viskores::Id i2 = idx12 / dim1;
    viskores::Id i3 = index / dim12;

    return viskores::make_Vec(
      this->PortalFirst.Get(i1), this->PortalSecond.Get(i2), this->PortalThird.Get(i3));
  }


  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VISKORES_EXEC_CONT void Set(viskores::Id index, const ValueType& value) const
  {
    VISKORES_ASSERT(index >= 0);
    VISKORES_ASSERT(index < this->GetNumberOfValues());

    viskores::Id dim1 = this->PortalFirst.GetNumberOfValues();
    viskores::Id dim2 = this->PortalSecond.GetNumberOfValues();
    viskores::Id dim12 = dim1 * dim2;
    viskores::Id idx12 = index % dim12;

    viskores::Id i1 = idx12 % dim1;
    viskores::Id i2 = idx12 / dim1;
    viskores::Id i3 = index / dim12;

    this->PortalFirst.Set(i1, value[0]);
    this->PortalSecond.Set(i2, value[1]);
    this->PortalThird.Set(i3, value[2]);
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  const PortalTypeFirst& GetFirstPortal() const { return this->PortalFirst; }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  const PortalTypeSecond& GetSecondPortal() const { return this->PortalSecond; }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  const PortalTypeThird& GetThirdPortal() const { return this->PortalThird; }

private:
  PortalTypeFirst PortalFirst;
  PortalTypeSecond PortalSecond;
  PortalTypeThird PortalThird;
};
}
} // namespace viskores::internal

namespace viskores
{
namespace cont
{

template <typename StorageTag1, typename StorageTag2, typename StorageTag3>
struct VISKORES_ALWAYS_EXPORT StorageTagCartesianProduct
{
};

namespace internal
{

/// This helper struct defines the value type for a zip container containing
/// the given two array handles.
///
template <typename AH1, typename AH2, typename AH3>
struct ArrayHandleCartesianProductTraits
{
  VISKORES_IS_ARRAY_HANDLE(AH1);
  VISKORES_IS_ARRAY_HANDLE(AH2);
  VISKORES_IS_ARRAY_HANDLE(AH3);

  using ComponentType = typename AH1::ValueType;
  VISKORES_STATIC_ASSERT_MSG(
    (std::is_same<ComponentType, typename AH2::ValueType>::value),
    "All arrays for ArrayHandleCartesianProduct must have the same value type. "
    "Use ArrayHandleCast as necessary to make types match.");
  VISKORES_STATIC_ASSERT_MSG(
    (std::is_same<ComponentType, typename AH3::ValueType>::value),
    "All arrays for ArrayHandleCartesianProduct must have the same value type. "
    "Use ArrayHandleCast as necessary to make types match.");

  /// The ValueType (a pair containing the value types of the two arrays).
  ///
  using ValueType = viskores::Vec<ComponentType, 3>;

  /// The appropriately templated tag.
  ///
  using Tag = viskores::cont::StorageTagCartesianProduct<typename AH1::StorageTag,
                                                         typename AH2::StorageTag,
                                                         typename AH3::StorageTag>;

  /// The superclass for ArrayHandleCartesianProduct.
  ///
  using Superclass = viskores::cont::ArrayHandle<ValueType, Tag>;
};

template <typename T, typename ST1, typename ST2, typename ST3>
class Storage<viskores::Vec<T, 3>, viskores::cont::StorageTagCartesianProduct<ST1, ST2, ST3>>
{
  struct Info
  {
    std::array<std::size_t, 4> BufferOffset;
  };

  using Storage1 = viskores::cont::internal::Storage<T, ST1>;
  using Storage2 = viskores::cont::internal::Storage<T, ST2>;
  using Storage3 = viskores::cont::internal::Storage<T, ST3>;

  using Array1 = viskores::cont::ArrayHandle<T, ST1>;
  using Array2 = viskores::cont::ArrayHandle<T, ST2>;
  using Array3 = viskores::cont::ArrayHandle<T, ST3>;

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> GetBuffers(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    std::size_t subArray)
  {
    Info info = buffers[0].GetMetaData<Info>();
    return std::vector<viskores::cont::internal::Buffer>(
      buffers.begin() + info.BufferOffset[subArray - 1],
      buffers.begin() + info.BufferOffset[subArray]);
  }

public:
  VISKORES_STORAGE_NO_RESIZE;

  using ReadPortalType =
    viskores::internal::ArrayPortalCartesianProduct<viskores::Vec<T, 3>,
                                                    typename Storage1::ReadPortalType,
                                                    typename Storage2::ReadPortalType,
                                                    typename Storage3::ReadPortalType>;
  using WritePortalType =
    viskores::internal::ArrayPortalCartesianProduct<viskores::Vec<T, 3>,
                                                    typename Storage1::WritePortalType,
                                                    typename Storage2::WritePortalType,
                                                    typename Storage3::WritePortalType>;

  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlat(
    const std::vector<viskores::cont::internal::Buffer>&)
  {
    return viskores::VecFlat<T>::NUM_COMPONENTS * 3;
  }

  VISKORES_CONT static viskores::Id GetNumberOfValues(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return (Storage1::GetNumberOfValues(GetBuffers(buffers, 1)) *
            Storage2::GetNumberOfValues(GetBuffers(buffers, 2)) *
            Storage3::GetNumberOfValues(GetBuffers(buffers, 3)));
  }

  VISKORES_CONT static void Fill(const std::vector<viskores::cont::internal::Buffer>& buffers,
                                 const viskores::Vec<T, 3>& fillValue,
                                 viskores::Id startIndex,
                                 viskores::Id endIndex,
                                 viskores::cont::Token& token)
  {
    if ((startIndex != 0) || (endIndex != GetNumberOfValues(buffers)))
    {
      throw viskores::cont::ErrorBadValue(
        "Fill for ArrayHandleCartesianProduct can only be used to fill entire array.");
    }
    auto subBuffers = GetBuffers(buffers, 1);
    Storage1::Fill(subBuffers, fillValue[0], 0, Storage1::GetNumberOfValues(subBuffers), token);
    subBuffers = GetBuffers(buffers, 2);
    Storage2::Fill(subBuffers, fillValue[1], 0, Storage2::GetNumberOfValues(subBuffers), token);
    subBuffers = GetBuffers(buffers, 3);
    Storage3::Fill(subBuffers, fillValue[2], 0, Storage3::GetNumberOfValues(subBuffers), token);
  }

  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return ReadPortalType(Storage1::CreateReadPortal(GetBuffers(buffers, 1), device, token),
                          Storage2::CreateReadPortal(GetBuffers(buffers, 2), device, token),
                          Storage3::CreateReadPortal(GetBuffers(buffers, 3), device, token));
  }

  VISKORES_CONT static WritePortalType CreateWritePortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return WritePortalType(Storage1::CreateWritePortal(GetBuffers(buffers, 1), device, token),
                           Storage2::CreateWritePortal(GetBuffers(buffers, 2), device, token),
                           Storage3::CreateWritePortal(GetBuffers(buffers, 3), device, token));
  }

  VISKORES_CONT static Array1 GetArrayHandle1(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return Array1(GetBuffers(buffers, 1));
  }
  VISKORES_CONT static Array2 GetArrayHandle2(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return Array2(GetBuffers(buffers, 2));
  }
  VISKORES_CONT static Array3 GetArrayHandle3(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return Array3(GetBuffers(buffers, 3));
  }

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers(
    const Array1& array1 = Array1{},
    const Array2& array2 = Array2{},
    const Array3& array3 = Array3{})
  {
    const std::vector<viskores::cont::internal::Buffer>& buffers1 = array1.GetBuffers();
    const std::vector<viskores::cont::internal::Buffer>& buffers2 = array2.GetBuffers();
    const std::vector<viskores::cont::internal::Buffer>& buffers3 = array3.GetBuffers();

    Info info;
    info.BufferOffset[0] = 1;
    info.BufferOffset[1] = info.BufferOffset[0] + buffers1.size();
    info.BufferOffset[2] = info.BufferOffset[1] + buffers2.size();
    info.BufferOffset[3] = info.BufferOffset[2] + buffers3.size();

    return viskores::cont::internal::CreateBuffers(info, buffers1, buffers2, buffers3);
  }
};
} // namespace internal

/// ArrayHandleCartesianProduct is a specialization of ArrayHandle. It takes two delegate
/// array handle and makes a new handle that access the corresponding entries
/// in these arrays as a pair.
///
template <typename FirstHandleType, typename SecondHandleType, typename ThirdHandleType>
class ArrayHandleCartesianProduct
  : public internal::ArrayHandleCartesianProductTraits<FirstHandleType,
                                                       SecondHandleType,
                                                       ThirdHandleType>::Superclass
{
  // If the following line gives a compile error, then the FirstHandleType
  // template argument is not a valid ArrayHandle type.
  VISKORES_IS_ARRAY_HANDLE(FirstHandleType);
  VISKORES_IS_ARRAY_HANDLE(SecondHandleType);
  VISKORES_IS_ARRAY_HANDLE(ThirdHandleType);

public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleCartesianProduct,
    (ArrayHandleCartesianProduct<FirstHandleType, SecondHandleType, ThirdHandleType>),
    (typename internal::ArrayHandleCartesianProductTraits<FirstHandleType,
                                                          SecondHandleType,
                                                          ThirdHandleType>::Superclass));

  /// Construct an `ArrayHandleCartesianProduct` given arrays for the coordinates in
  /// the x, y, and z diretions.
  VISKORES_CONT
  ArrayHandleCartesianProduct(const FirstHandleType& firstArray,
                              const SecondHandleType& secondArray,
                              const ThirdHandleType& thirdArray)
    : Superclass(StorageType::CreateBuffers(firstArray, secondArray, thirdArray))
  {
  }

  /// Implemented so that it is defined exclusively in the control environment.
  /// If there is a separate device for the execution environment (for example,
  /// with CUDA), then the automatically generated destructor could be
  /// created for all devices, and it would not be valid for all devices.
  ///
  ~ArrayHandleCartesianProduct() {}

  /// Get the array for the coordinates in the x direction.
  VISKORES_CONT FirstHandleType GetFirstArray() const
  {
    return StorageType::GetArrayHandle1(this->GetBuffers());
  }
  /// Get the array for the coordinates in the y direction.
  VISKORES_CONT SecondHandleType GetSecondArray() const
  {
    return StorageType::GetArrayHandle2(this->GetBuffers());
  }
  /// Get the array for the coordinates in the z direction.
  VISKORES_CONT ThirdHandleType GetThirdArray() const
  {
    return StorageType::GetArrayHandle3(this->GetBuffers());
  }
};

/// A convenience function for creating an ArrayHandleCartesianProduct. It takes the two
/// arrays to be zipped together.
///
template <typename FirstHandleType, typename SecondHandleType, typename ThirdHandleType>
VISKORES_CONT
  viskores::cont::ArrayHandleCartesianProduct<FirstHandleType, SecondHandleType, ThirdHandleType>
  make_ArrayHandleCartesianProduct(const FirstHandleType& first,
                                   const SecondHandleType& second,
                                   const ThirdHandleType& third)
{
  return ArrayHandleCartesianProduct<FirstHandleType, SecondHandleType, ThirdHandleType>(
    first, second, third);
}

//--------------------------------------------------------------------------------
// Specialization of ArrayExtractComponent
namespace internal
{

// Superclass will inherit the ArrayExtractComponentImplInefficient property if any
// of the sub-storage are inefficient (thus making everything inefficient).
template <typename... STs>
struct ArrayExtractComponentImpl<viskores::cont::StorageTagCartesianProduct<STs...>>
  : viskores::cont::internal::ArrayExtractComponentImplInherit<STs...>
{
  template <typename T>
  viskores::cont::ArrayHandleStride<T> AdjustStrideForComponent(
    const viskores::cont::ArrayHandleStride<T>& componentArray,
    const viskores::Id3& dims,
    viskores::IdComponent component,
    viskores::Id totalNumValues) const
  {
    VISKORES_ASSERT(componentArray.GetModulo() == 0);
    VISKORES_ASSERT(componentArray.GetDivisor() == 1);

    viskores::Id modulo = 0;
    if (component < 2)
    {
      modulo = dims[component];
    }

    viskores::Id divisor = 1;
    for (viskores::IdComponent c = 0; c < component; ++c)
    {
      divisor *= dims[c];
    }

    return viskores::cont::ArrayHandleStride<T>(componentArray.GetBasicArray(),
                                                totalNumValues,
                                                componentArray.GetStride(),
                                                componentArray.GetOffset(),
                                                modulo,
                                                divisor);
  }

  template <typename T, typename ST, typename CartesianArrayType>
  viskores::cont::ArrayHandleStride<typename viskores::VecTraits<T>::BaseComponentType>
  GetStrideForComponentArray(const viskores::cont::ArrayHandle<T, ST>& componentArray,
                             const CartesianArrayType& cartesianArray,
                             viskores::IdComponent subIndex,
                             viskores::IdComponent productIndex,
                             viskores::CopyFlag allowCopy) const
  {
    viskores::cont::ArrayHandleStride<typename viskores::VecTraits<T>::BaseComponentType>
      strideArray = ArrayExtractComponentImpl<ST>{}(componentArray, subIndex, allowCopy);
    if ((strideArray.GetModulo() != 0) || (strideArray.GetDivisor() != 1))
    {
      // If the sub array has its own modulo and/or divisor, that will likely interfere
      // with this math. Give up and fall back to simple copy.
      constexpr viskores::IdComponent NUM_SUB_COMPONENTS = viskores::VecFlat<T>::NUM_COMPONENTS;
      return viskores::cont::internal::ArrayExtractComponentFallback(
        cartesianArray, (productIndex * NUM_SUB_COMPONENTS) + subIndex, allowCopy);
    }

    viskores::Id3 dims = { cartesianArray.GetFirstArray().GetNumberOfValues(),
                           cartesianArray.GetSecondArray().GetNumberOfValues(),
                           cartesianArray.GetThirdArray().GetNumberOfValues() };

    return this->AdjustStrideForComponent(
      strideArray, dims, productIndex, cartesianArray.GetNumberOfValues());
  }

  template <typename T>
  viskores::cont::ArrayHandleStride<typename viskores::VecTraits<T>::BaseComponentType> operator()(
    const viskores::cont::ArrayHandle<viskores::Vec<T, 3>,
                                      viskores::cont::StorageTagCartesianProduct<STs...>>& src,
    viskores::IdComponent componentIndex,
    viskores::CopyFlag allowCopy) const
  {
    viskores::cont::ArrayHandleCartesianProduct<viskores::cont::ArrayHandle<T, STs>...> array(src);
    constexpr viskores::IdComponent NUM_SUB_COMPONENTS = viskores::VecFlat<T>::NUM_COMPONENTS;
    viskores::IdComponent subIndex = componentIndex % NUM_SUB_COMPONENTS;
    viskores::IdComponent productIndex = componentIndex / NUM_SUB_COMPONENTS;

    switch (productIndex)
    {
      case 0:
        return this->GetStrideForComponentArray(
          array.GetFirstArray(), array, subIndex, productIndex, allowCopy);
      case 1:
        return this->GetStrideForComponentArray(
          array.GetSecondArray(), array, subIndex, productIndex, allowCopy);
      case 2:
        return this->GetStrideForComponentArray(
          array.GetThirdArray(), array, subIndex, productIndex, allowCopy);
      default:
        throw viskores::cont::ErrorBadValue("Invalid component index to ArrayExtractComponent.");
    }
  }
};

template <typename T, typename S>
viskores::cont::ArrayHandle<viskores::Range> ArrayRangeComputeGeneric(
  const viskores::cont::ArrayHandle<T, S>& input,
  const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
  bool computeFiniteRange,
  viskores::cont::DeviceAdapterId device);

template <typename S>
struct ArrayRangeComputeImpl;

template <typename ST1, typename ST2, typename ST3>
struct VISKORES_CONT_EXPORT
  ArrayRangeComputeImpl<viskores::cont::StorageTagCartesianProduct<ST1, ST2, ST3>>
{
  template <typename T>
  VISKORES_CONT viskores::cont::ArrayHandle<viskores::Range> operator()(
    const viskores::cont::ArrayHandle<viskores::Vec<T, 3>,
                                      viskores::cont::StorageTagCartesianProduct<ST1, ST2, ST3>>&
      input_,
    const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
    bool computeFiniteRange,
    viskores::cont::DeviceAdapterId device) const
  {
    if (maskArray.GetNumberOfValues() != 0)
    {
      return viskores::cont::internal::ArrayRangeComputeGeneric(
        input_, maskArray, computeFiniteRange, device);
    }

    const auto& input = static_cast<
      const viskores::cont::ArrayHandleCartesianProduct<viskores::cont::ArrayHandle<T, ST1>,
                                                        viskores::cont::ArrayHandle<T, ST2>,
                                                        viskores::cont::ArrayHandle<T, ST3>>&>(
      input_);

    viskores::cont::ArrayHandle<viskores::Range> ranges[3];
    ranges[0] = viskores::cont::internal::ArrayRangeComputeImpl<ST1>{}(
      input.GetFirstArray(), maskArray, computeFiniteRange, device);
    ranges[1] = viskores::cont::internal::ArrayRangeComputeImpl<ST2>{}(
      input.GetSecondArray(), maskArray, computeFiniteRange, device);
    ranges[2] = viskores::cont::internal::ArrayRangeComputeImpl<ST3>{}(
      input.GetThirdArray(), maskArray, computeFiniteRange, device);

    auto numComponents =
      ranges[0].GetNumberOfValues() + ranges[1].GetNumberOfValues() + ranges[2].GetNumberOfValues();
    viskores::cont::ArrayHandle<viskores::Range> result;
    result.Allocate(numComponents);
    auto resultPortal = result.WritePortal();
    for (viskores::Id i = 0, index = 0; i < 3; ++i)
    {
      auto rangePortal = ranges[i].ReadPortal();
      for (viskores::Id j = 0; j < rangePortal.GetNumberOfValues(); ++j, ++index)
      {
        resultPortal.Set(index, rangePortal.Get(j));
      }
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

template <typename AH1, typename AH2, typename AH3>
struct SerializableTypeString<viskores::cont::ArrayHandleCartesianProduct<AH1, AH2, AH3>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "AH_CartesianProduct<" + SerializableTypeString<AH1>::Get() + "," +
      SerializableTypeString<AH2>::Get() + "," + SerializableTypeString<AH3>::Get() + ">";
    return name;
  }
};

template <typename T, typename ST1, typename ST2, typename ST3>
struct SerializableTypeString<
  viskores::cont::ArrayHandle<viskores::Vec<T, 3>,
                              viskores::cont::StorageTagCartesianProduct<ST1, ST2, ST3>>>
  : SerializableTypeString<
      viskores::cont::ArrayHandleCartesianProduct<viskores::cont::ArrayHandle<T, ST1>,
                                                  viskores::cont::ArrayHandle<T, ST2>,
                                                  viskores::cont::ArrayHandle<T, ST3>>>
{
};
}
} // viskores::cont

namespace mangled_diy_namespace
{

template <typename AH1, typename AH2, typename AH3>
struct Serialization<viskores::cont::ArrayHandleCartesianProduct<AH1, AH2, AH3>>
{
private:
  using Type = typename viskores::cont::ArrayHandleCartesianProduct<AH1, AH2, AH3>;
  using BaseType = viskores::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    Type array = obj;
    viskoresdiy::save(bb, array.GetFirstArray());
    viskoresdiy::save(bb, array.GetSecondArray());
    viskoresdiy::save(bb, array.GetThirdArray());
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH1 array1;
    AH2 array2;
    AH3 array3;

    viskoresdiy::load(bb, array1);
    viskoresdiy::load(bb, array2);
    viskoresdiy::load(bb, array3);

    obj = viskores::cont::make_ArrayHandleCartesianProduct(array1, array2, array3);
  }
};

template <typename T, typename ST1, typename ST2, typename ST3>
struct Serialization<
  viskores::cont::ArrayHandle<viskores::Vec<T, 3>,
                              viskores::cont::StorageTagCartesianProduct<ST1, ST2, ST3>>>
  : Serialization<viskores::cont::ArrayHandleCartesianProduct<viskores::cont::ArrayHandle<T, ST1>,
                                                              viskores::cont::ArrayHandle<T, ST2>,
                                                              viskores::cont::ArrayHandle<T, ST3>>>
{
};
} // diy
/// @endcond SERIALIZATION

#endif //viskores_cont_ArrayHandleCartesianProduct_h
