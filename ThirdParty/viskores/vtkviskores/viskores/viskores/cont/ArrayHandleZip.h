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
#ifndef viskores_cont_ArrayHandleZip_h
#define viskores_cont_ArrayHandleZip_h

#include <viskores/Pair.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/internal/ArrayPortalHelpers.h>

namespace viskores
{
namespace exec
{
namespace internal
{

/// \brief An array portal that zips two portals together into a single value
/// for the execution environment
template <typename PortalTypeFirst, typename PortalTypeSecond>
class ArrayPortalZip
{
  using ReadableP1 = viskores::internal::PortalSupportsGets<PortalTypeFirst>;
  using ReadableP2 = viskores::internal::PortalSupportsGets<PortalTypeSecond>;
  using WritableP1 = viskores::internal::PortalSupportsSets<PortalTypeFirst>;
  using WritableP2 = viskores::internal::PortalSupportsSets<PortalTypeSecond>;

  using Readable = std::integral_constant<bool, ReadableP1::value && ReadableP2::value>;
  using Writable = std::integral_constant<bool, WritableP1::value && WritableP2::value>;

public:
  using T = typename PortalTypeFirst::ValueType;
  using U = typename PortalTypeSecond::ValueType;
  using ValueType = viskores::Pair<T, U>;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ArrayPortalZip()
    : PortalFirst()
    , PortalSecond()
  {
  } //needs to be host and device so that cuda can create lvalue of these

  VISKORES_CONT
  ArrayPortalZip(const PortalTypeFirst& portalfirst, const PortalTypeSecond& portalsecond)
    : PortalFirst(portalfirst)
    , PortalSecond(portalsecond)
  {
  }

  /// Copy constructor for any other ArrayPortalZip with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template <class OtherF, class OtherS>
  VISKORES_CONT ArrayPortalZip(const ArrayPortalZip<OtherF, OtherS>& src)
    : PortalFirst(src.GetFirstPortal())
    , PortalSecond(src.GetSecondPortal())
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const { return this->PortalFirst.GetNumberOfValues(); }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename Readable_ = Readable,
            typename = typename std::enable_if<Readable_::value>::type>
  VISKORES_EXEC_CONT ValueType Get(viskores::Id index) const noexcept
  {
    return viskores::make_Pair(this->PortalFirst.Get(index), this->PortalSecond.Get(index));
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VISKORES_EXEC_CONT void Set(viskores::Id index, const ValueType& value) const noexcept
  {
    this->PortalFirst.Set(index, value.first);
    this->PortalSecond.Set(index, value.second);
  }

  VISKORES_EXEC_CONT
  const PortalTypeFirst& GetFirstPortal() const { return this->PortalFirst; }

  VISKORES_EXEC_CONT
  const PortalTypeSecond& GetSecondPortal() const { return this->PortalSecond; }

private:
  PortalTypeFirst PortalFirst;
  PortalTypeSecond PortalSecond;
};
}
}
} // namespace viskores::exec::internal

namespace viskores
{
namespace cont
{

template <typename ST1, typename ST2>
struct VISKORES_ALWAYS_EXPORT StorageTagZip
{
};

namespace internal
{

/// This helper struct defines the value type for a zip container containing
/// the given two array handles.
///
template <typename FirstHandleType, typename SecondHandleType>
struct ArrayHandleZipTraits
{
  /// The ValueType (a pair containing the value types of the two arrays).
  ///
  using ValueType =
    viskores::Pair<typename FirstHandleType::ValueType, typename SecondHandleType::ValueType>;

  /// The appropriately templated tag.
  ///
  using Tag =
    StorageTagZip<typename FirstHandleType::StorageTag, typename SecondHandleType::StorageTag>;

  /// The superclass for ArrayHandleZip.
  ///
  using Superclass = viskores::cont::ArrayHandle<ValueType, Tag>;
};

template <typename T1, typename T2, typename ST1, typename ST2>
class Storage<viskores::Pair<T1, T2>, viskores::cont::StorageTagZip<ST1, ST2>>
{
  using FirstStorage = Storage<T1, ST1>;
  using SecondStorage = Storage<T2, ST2>;
  using ValueType = viskores::Pair<T1, T2>;

  using FirstArrayType = viskores::cont::ArrayHandle<T1, ST1>;
  using SecondArrayType = viskores::cont::ArrayHandle<T2, ST2>;

  struct Info
  {
    std::size_t SecondBuffersOffset;
  };

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> FirstArrayBuffers(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    const Info& info = buffers[0].GetMetaData<Info>();
    return std::vector<viskores::cont::internal::Buffer>(
      buffers.begin() + 1, buffers.begin() + info.SecondBuffersOffset);
  }
  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> SecondArrayBuffers(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    const Info& info = buffers[0].GetMetaData<Info>();
    return std::vector<viskores::cont::internal::Buffer>(buffers.begin() + info.SecondBuffersOffset,
                                                         buffers.end());
  }

public:
  using ReadPortalType =
    viskores::exec::internal::ArrayPortalZip<typename FirstStorage::ReadPortalType,
                                             typename SecondStorage::ReadPortalType>;
  using WritePortalType =
    viskores::exec::internal::ArrayPortalZip<typename FirstStorage::WritePortalType,
                                             typename SecondStorage::WritePortalType>;

  static std::vector<viskores::cont::internal::Buffer> CreateBuffers(
    const FirstArrayType& firstArray = FirstArrayType{},
    const SecondArrayType& secondArray = SecondArrayType{})
  {
    Info info;
    info.SecondBuffersOffset = 1 + firstArray.GetBuffers().size();
    return viskores::cont::internal::CreateBuffers(info, firstArray, secondArray);
  }

  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlat(
    const std::vector<viskores::cont::internal::Buffer>&)
  {
    return 1;
  }

  VISKORES_CONT static void ResizeBuffers(
    viskores::Id numValues,
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::CopyFlag preserve,
    viskores::cont::Token& token)
  {
    FirstStorage::ResizeBuffers(numValues, FirstArrayBuffers(buffers), preserve, token);
    SecondStorage::ResizeBuffers(numValues, SecondArrayBuffers(buffers), preserve, token);
  }

  VISKORES_CONT static viskores::Id GetNumberOfValues(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    viskores::Id numValues = FirstStorage::GetNumberOfValues(FirstArrayBuffers(buffers));
    VISKORES_ASSERT(numValues == SecondStorage::GetNumberOfValues(SecondArrayBuffers(buffers)));
    return numValues;
  }

  VISKORES_CONT static void Fill(const std::vector<viskores::cont::internal::Buffer>& buffers,
                                 const ValueType& fillValue,
                                 viskores::Id startIndex,
                                 viskores::Id endIndex,
                                 viskores::cont::Token& token)
  {
    FirstStorage::Fill(FirstArrayBuffers(buffers), fillValue.first, startIndex, endIndex, token);
    SecondStorage::Fill(SecondArrayBuffers(buffers), fillValue.second, startIndex, endIndex, token);
  }

  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return ReadPortalType(
      FirstStorage::CreateReadPortal(FirstArrayBuffers(buffers), device, token),
      SecondStorage::CreateReadPortal(SecondArrayBuffers(buffers), device, token));
  }

  VISKORES_CONT static WritePortalType CreateWritePortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return WritePortalType(
      FirstStorage::CreateWritePortal(FirstArrayBuffers(buffers), device, token),
      SecondStorage::CreateWritePortal(SecondArrayBuffers(buffers), device, token));
  }

  static FirstArrayType GetFirstArray(const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return FirstArrayType(FirstArrayBuffers(buffers));
  }
  static SecondArrayType GetSecondArray(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return SecondArrayType(SecondArrayBuffers(buffers));
  }
};
} // namespace internal

/// ArrayHandleZip is a specialization of ArrayHandle. It takes two delegate
/// array handle and makes a new handle that access the corresponding entries
/// in these arrays as a pair.
///
template <typename FirstHandleType, typename SecondHandleType>
class ArrayHandleZip
  : public internal::ArrayHandleZipTraits<FirstHandleType, SecondHandleType>::Superclass
{
  // If the following line gives a compile error, then the FirstHandleType
  // template argument is not a valid ArrayHandle type.
  VISKORES_IS_ARRAY_HANDLE(FirstHandleType);

  // If the following line gives a compile error, then the SecondHandleType
  // template argument is not a valid ArrayHandle type.
  VISKORES_IS_ARRAY_HANDLE(SecondHandleType);

public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleZip,
    (ArrayHandleZip<FirstHandleType, SecondHandleType>),
    (typename internal::ArrayHandleZipTraits<FirstHandleType, SecondHandleType>::Superclass));

  /// Create `ArrayHandleZip` with two arrays.
  VISKORES_CONT
  ArrayHandleZip(const FirstHandleType& firstArray, const SecondHandleType& secondArray)
    : Superclass(StorageType::CreateBuffers(firstArray, secondArray))
  {
  }

  /// Returns the the array for the first part of the zip pair.
  FirstHandleType GetFirstArray() const { return StorageType::GetFirstArray(this->GetBuffers()); }
  /// Returns the the array for the second part of the zip pair.
  SecondHandleType GetSecondArray() const
  {
    return StorageType::GetSecondArray(this->GetBuffers());
  }
};

/// A convenience function for creating an ArrayHandleZip. It takes the two
/// arrays to be zipped together.
///
template <typename FirstHandleType, typename SecondHandleType>
VISKORES_CONT viskores::cont::ArrayHandleZip<FirstHandleType, SecondHandleType> make_ArrayHandleZip(
  const FirstHandleType& first,
  const SecondHandleType& second)
{
  return ArrayHandleZip<FirstHandleType, SecondHandleType>(first, second);
}
}
} // namespace viskores::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace viskores
{
namespace cont
{

template <typename AH1, typename AH2>
struct SerializableTypeString<viskores::cont::ArrayHandleZip<AH1, AH2>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "AH_Zip<" + SerializableTypeString<AH1>::Get() + "," +
      SerializableTypeString<AH2>::Get() + ">";
    return name;
  }
};

template <typename T1, typename T2, typename ST1, typename ST2>
struct SerializableTypeString<
  viskores::cont::ArrayHandle<viskores::Pair<T1, T2>, viskores::cont::StorageTagZip<ST1, ST2>>>
  : SerializableTypeString<viskores::cont::ArrayHandleZip<viskores::cont::ArrayHandle<T1, ST1>,
                                                          viskores::cont::ArrayHandle<T2, ST2>>>
{
};
}
} // namespace viskores::cont

namespace mangled_diy_namespace
{

template <typename AH1, typename AH2>
struct Serialization<viskores::cont::ArrayHandleZip<AH1, AH2>>
{
private:
  using Type = typename viskores::cont::ArrayHandleZip<AH1, AH2>;
  using BaseType = viskores::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    auto storage = obj.GetStorage();
    viskoresdiy::save(bb, storage.GetFirstArray());
    viskoresdiy::save(bb, storage.GetSecondArray());
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH1 a1;
    AH2 a2;

    viskoresdiy::load(bb, a1);
    viskoresdiy::load(bb, a2);

    obj = viskores::cont::make_ArrayHandleZip(a1, a2);
  }
};

template <typename T1, typename T2, typename ST1, typename ST2>
struct Serialization<
  viskores::cont::ArrayHandle<viskores::Pair<T1, T2>, viskores::cont::StorageTagZip<ST1, ST2>>>
  : Serialization<viskores::cont::ArrayHandleZip<viskores::cont::ArrayHandle<T1, ST1>,
                                                 viskores::cont::ArrayHandle<T2, ST2>>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //viskores_cont_ArrayHandleZip_h
