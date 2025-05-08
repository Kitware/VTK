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
#ifndef viskores_cont_ArrayHandleConcatenate_h
#define viskores_cont_ArrayHandleConcatenate_h

#include <viskores/StaticAssert.h>

#include <viskores/cont/ArrayHandle.h>

namespace viskores
{
namespace internal
{

template <typename PortalType1, typename PortalType2>
class VISKORES_ALWAYS_EXPORT ArrayPortalConcatenate
{
  using WritableP1 = viskores::internal::PortalSupportsSets<PortalType1>;
  using WritableP2 = viskores::internal::PortalSupportsSets<PortalType2>;
  using Writable = std::integral_constant<bool, WritableP1::value && WritableP2::value>;

public:
  using ValueType = typename PortalType1::ValueType;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ArrayPortalConcatenate()
    : portal1()
    , portal2()
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ArrayPortalConcatenate(const PortalType1& p1, const PortalType2& p2)
    : portal1(p1)
    , portal2(p2)
  {
  }

  // Copy constructor
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename OtherP1, typename OtherP2>
  VISKORES_EXEC_CONT ArrayPortalConcatenate(const ArrayPortalConcatenate<OtherP1, OtherP2>& src)
    : portal1(src.GetPortal1())
    , portal2(src.GetPortal2())
  {
  }

  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const
  {
    return this->portal1.GetNumberOfValues() + this->portal2.GetNumberOfValues();
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ValueType Get(viskores::Id index) const
  {
    if (index < this->portal1.GetNumberOfValues())
    {
      return this->portal1.Get(index);
    }
    else
    {
      return this->portal2.Get(index - this->portal1.GetNumberOfValues());
    }
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VISKORES_EXEC_CONT void Set(viskores::Id index, const ValueType& value) const
  {
    if (index < this->portal1.GetNumberOfValues())
    {
      this->portal1.Set(index, value);
    }
    else
    {
      this->portal2.Set(index - this->portal1.GetNumberOfValues(), value);
    }
  }

  VISKORES_EXEC_CONT
  const PortalType1& GetPortal1() const { return this->portal1; }

  VISKORES_EXEC_CONT
  const PortalType2& GetPortal2() const { return this->portal2; }

private:
  PortalType1 portal1;
  PortalType2 portal2;
}; // class ArrayPortalConcatenate

}
} // namespace viskores::internal

namespace viskores
{
namespace cont
{

template <typename StorageTag1, typename StorageTag2>
class VISKORES_ALWAYS_EXPORT StorageTagConcatenate
{
};

namespace internal
{

template <typename T, typename ST1, typename ST2>
class Storage<T, StorageTagConcatenate<ST1, ST2>>
{
  using SourceStorage1 = viskores::cont::internal::Storage<T, ST1>;
  using SourceStorage2 = viskores::cont::internal::Storage<T, ST2>;

  using ArrayHandleType1 = viskores::cont::ArrayHandle<T, ST1>;
  using ArrayHandleType2 = viskores::cont::ArrayHandle<T, ST2>;

  struct Info
  {
    std::size_t NumBuffers1;
    std::size_t NumBuffers2;
  };

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> Buffers1(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    Info info = buffers[0].GetMetaData<Info>();
    return std::vector<viskores::cont::internal::Buffer>(buffers.begin() + 1,
                                                         buffers.begin() + 1 + info.NumBuffers1);
  }

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> Buffers2(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    Info info = buffers[0].GetMetaData<Info>();
    return std::vector<viskores::cont::internal::Buffer>(buffers.begin() + 1 + info.NumBuffers1,
                                                         buffers.end());
  }

public:
  VISKORES_STORAGE_NO_RESIZE;

  using ReadPortalType =
    viskores::internal::ArrayPortalConcatenate<typename SourceStorage1::ReadPortalType,
                                               typename SourceStorage2::ReadPortalType>;
  using WritePortalType =
    viskores::internal::ArrayPortalConcatenate<typename SourceStorage1::WritePortalType,
                                               typename SourceStorage2::WritePortalType>;

  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlat(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    viskores::IdComponent components1 =
      SourceStorage1::GetNumberOfComponentsFlat(Buffers1(buffers));
    viskores::IdComponent components2 =
      SourceStorage2::GetNumberOfComponentsFlat(Buffers2(buffers));
    if (components1 == components2)
    {
      return components1;
    }
    else
    {
      // Inconsistent component size.
      return 0;
    }
  }

  VISKORES_CONT static viskores::Id GetNumberOfValues(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return (SourceStorage1::GetNumberOfValues(Buffers1(buffers)) +
            SourceStorage2::GetNumberOfValues(Buffers2(buffers)));
  }

  VISKORES_CONT static void Fill(const std::vector<viskores::cont::internal::Buffer>& buffers,
                                 const T& fillValue,
                                 viskores::Id startIndex,
                                 viskores::Id endIndex,
                                 viskores::cont::Token& token)
  {
    viskores::Id size1 = SourceStorage1::GetNumberOfValues(Buffers1(buffers));
    if ((startIndex < size1) && (endIndex <= size1))
    {
      SourceStorage1::Fill(Buffers1(buffers), fillValue, startIndex, endIndex, token);
    }
    else if (startIndex < size1) // && (endIndex > size1)
    {
      SourceStorage1::Fill(Buffers1(buffers), fillValue, startIndex, size1, token);
      SourceStorage2::Fill(Buffers2(buffers), fillValue, 0, endIndex - size1, token);
    }
    else // startIndex >= size1
    {
      SourceStorage2::Fill(
        Buffers2(buffers), fillValue, startIndex - size1, endIndex - size1, token);
    }
  }

  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return ReadPortalType(SourceStorage1::CreateReadPortal(Buffers1(buffers), device, token),
                          SourceStorage2::CreateReadPortal(Buffers2(buffers), device, token));
  }

  VISKORES_CONT static WritePortalType CreateWritePortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return WritePortalType(SourceStorage1::CreateWritePortal(Buffers1(buffers), device, token),
                           SourceStorage2::CreateWritePortal(Buffers2(buffers), device, token));
  }

  VISKORES_CONT static auto CreateBuffers(const ArrayHandleType1& array1 = ArrayHandleType1{},
                                          const ArrayHandleType2& array2 = ArrayHandleType2{})
    -> decltype(viskores::cont::internal::CreateBuffers())
  {
    Info info;
    info.NumBuffers1 = array1.GetBuffers().size();
    info.NumBuffers2 = array2.GetBuffers().size();
    return viskores::cont::internal::CreateBuffers(info, array1, array2);
  }

  VISKORES_CONT static const ArrayHandleType1 GetArray1(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return ArrayHandleType1(Buffers1(buffers));
  }

  VISKORES_CONT static const ArrayHandleType2 GetArray2(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return ArrayHandleType2(Buffers2(buffers));
  }
}; // class Storage

}
}
} // namespace viskores::cont::internal

namespace viskores
{
namespace cont
{

template <typename ArrayHandleType1, typename ArrayHandleType2>
class ArrayHandleConcatenate
  : public viskores::cont::ArrayHandle<typename ArrayHandleType1::ValueType,
                                       StorageTagConcatenate<typename ArrayHandleType1::StorageTag,
                                                             typename ArrayHandleType2::StorageTag>>
{
public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleConcatenate,
    (ArrayHandleConcatenate<ArrayHandleType1, ArrayHandleType2>),
    (viskores::cont::ArrayHandle<typename ArrayHandleType1::ValueType,
                                 StorageTagConcatenate<typename ArrayHandleType1::StorageTag,
                                                       typename ArrayHandleType2::StorageTag>>));

  VISKORES_CONT
  ArrayHandleConcatenate(const ArrayHandleType1& array1, const ArrayHandleType2& array2)
    : Superclass(StorageType::CreateBuffers(array1, array2))
  {
  }
};

template <typename ArrayHandleType1, typename ArrayHandleType2>
VISKORES_CONT ArrayHandleConcatenate<ArrayHandleType1, ArrayHandleType2>
make_ArrayHandleConcatenate(const ArrayHandleType1& array1, const ArrayHandleType2& array2)
{
  return ArrayHandleConcatenate<ArrayHandleType1, ArrayHandleType2>(array1, array2);
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
struct SerializableTypeString<viskores::cont::ArrayHandleConcatenate<AH1, AH2>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "AH_Concatenate<" + SerializableTypeString<AH1>::Get() + "," +
      SerializableTypeString<AH2>::Get() + ">";
    return name;
  }
};

template <typename T, typename ST1, typename ST2>
struct SerializableTypeString<
  viskores::cont::ArrayHandle<T, viskores::cont::StorageTagConcatenate<ST1, ST2>>>
  : SerializableTypeString<
      viskores::cont::ArrayHandleConcatenate<viskores::cont::ArrayHandle<T, ST1>,
                                             viskores::cont::ArrayHandle<T, ST2>>>
{
};
}
} // viskores::cont

namespace mangled_diy_namespace
{

template <typename AH1, typename AH2>
struct Serialization<viskores::cont::ArrayHandleConcatenate<AH1, AH2>>
{
private:
  using Type = viskores::cont::ArrayHandleConcatenate<AH1, AH2>;
  using BaseType = viskores::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    auto storage = obj.GetStorage();
    viskoresdiy::save(bb, storage.GetArray1());
    viskoresdiy::save(bb, storage.GetArray2());
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH1 array1;
    AH2 array2;

    viskoresdiy::load(bb, array1);
    viskoresdiy::load(bb, array2);

    obj = viskores::cont::make_ArrayHandleConcatenate(array1, array2);
  }
};

template <typename T, typename ST1, typename ST2>
struct Serialization<
  viskores::cont::ArrayHandle<T, viskores::cont::StorageTagConcatenate<ST1, ST2>>>
  : Serialization<viskores::cont::ArrayHandleConcatenate<viskores::cont::ArrayHandle<T, ST1>,
                                                         viskores::cont::ArrayHandle<T, ST2>>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //viskores_cont_ArrayHandleConcatenate_h
