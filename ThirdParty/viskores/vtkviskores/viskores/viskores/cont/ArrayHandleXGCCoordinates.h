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
#ifndef viskores_cont_ArrayHandleXGCCoordinates_h
#define viskores_cont_ArrayHandleXGCCoordinates_h

#include <viskores/Math.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ErrorBadType.h>

#include <viskores/internal/IndicesExtrude.h>


namespace viskores
{
namespace internal
{

template <typename PortalType>
struct VISKORES_ALWAYS_EXPORT ArrayPortalXGCCoordinates
{
  using ValueType = viskores::Vec<typename PortalType::ValueType, 3>;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ArrayPortalXGCCoordinates()
    : Portal()
    , NumberOfPointsPerPlane(0)
    , NumberOfPlanes(0)
    , NumberOfPlanesOwned(0)
    , PlaneStartId(0)
    , UseCylindrical(false){};

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ArrayPortalXGCCoordinates(const PortalType& p,
                            viskores::Id numOfPlanes,
                            viskores::Id numOfPlanesOwned,
                            viskores::Id planeStartId,
                            bool cylindrical = false)
    : Portal(p)
    , NumberOfPlanes(numOfPlanes)
    , NumberOfPlanesOwned(numOfPlanesOwned)
    , PlaneStartId(planeStartId)
    , UseCylindrical(cylindrical)
  {
    this->NumberOfPointsPerPlane = this->Portal.GetNumberOfValues() / 2;
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const
  {
    return (this->NumberOfPointsPerPlane * static_cast<viskores::Id>(NumberOfPlanesOwned));
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ValueType Get(viskores::Id index) const
  {
    const viskores::Id realIdx = ((index * 2) % this->Portal.GetNumberOfValues()) / 2;
    const viskores::Id whichPlane =
      (index * 2) / this->Portal.GetNumberOfValues() + this->PlaneStartId;
    return this->Get(viskores::Id2(realIdx, whichPlane));
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ValueType Get(viskores::Id2 index) const
  {
    using CompType = typename ValueType::ComponentType;

    const viskores::Id realIdx = (index[0] * 2);
    const viskores::Id whichPlane = index[1];
    const auto phi = static_cast<CompType>(whichPlane * (viskores::TwoPi() / this->NumberOfPlanes));

    auto r = this->Portal.Get(realIdx);
    auto z = this->Portal.Get(realIdx + 1);
    if (this->UseCylindrical)
    {
      return ValueType(r, phi, z);
    }
    else
    {
      return ValueType(r * viskores::Cos(phi), r * viskores::Sin(phi), z);
    }
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  viskores::Vec<ValueType, 6> GetWedge(const exec::IndicesExtrude& index) const
  {
    using CompType = typename ValueType::ComponentType;

    viskores::Vec<ValueType, 6> result;
    for (int j = 0; j < 2; ++j)
    {
      const auto phi =
        static_cast<CompType>(index.Planes[j] * (viskores::TwoPi() / this->NumberOfPlanes));
      for (int i = 0; i < 3; ++i)
      {
        const viskores::Id realIdx = index.PointIds[j][i] * 2;
        auto r = this->Portal.Get(realIdx);
        auto z = this->Portal.Get(realIdx + 1);
        result[3 * j + i] = this->UseCylindrical
          ? ValueType(r, phi, z)
          : ValueType(r * viskores::Cos(phi), r * viskores::Sin(phi), z);
      }
    }

    return result;
  }

private:
  PortalType Portal;
  viskores::Id NumberOfPointsPerPlane;
  viskores::Id NumberOfPlanes;
  viskores::Id NumberOfPlanesOwned;
  viskores::Id PlaneStartId;
  bool UseCylindrical;
};

}
} // namespace viskores::internal

namespace viskores
{
namespace cont
{
struct VISKORES_ALWAYS_EXPORT StorageTagXGCCoordinates
{
};

namespace internal
{

struct XGCCoordinatesMetaData
{
  viskores::Id NumberOfPlanes = 0;
  viskores::Id NumberOfPlanesOwned = 0;
  viskores::Id PlaneStartId = -1;
  bool UseCylindrical = false;

  XGCCoordinatesMetaData() = default;

  XGCCoordinatesMetaData(viskores::Id numberOfPlanes,
                         viskores::Id numberOfPlanesOwned,
                         viskores::Id planeStartId,
                         bool useCylindrical)
    : NumberOfPlanes(numberOfPlanes)
    , NumberOfPlanesOwned(numberOfPlanesOwned)
    , PlaneStartId(planeStartId)
    , UseCylindrical(useCylindrical)
  {
  }
};

namespace detail
{

template <typename T>
class XGCCoordinatesStorageImpl
{
  using SourceStorage = Storage<T, StorageTagBasic>; // only allow input AH to use StorageTagBasic
  using MetaData = XGCCoordinatesMetaData;

  static MetaData& GetMetaData(const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return buffers[0].GetMetaData<MetaData>();
  }

  // Used to skip the metadata buffer and return only actual data buffers
  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> SourceBuffers(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return std::vector<viskores::cont::internal::Buffer>(buffers.begin() + 1, buffers.end());
  }

public:
  using ReadPortalType =
    viskores::internal::ArrayPortalXGCCoordinates<typename SourceStorage::ReadPortalType>;

  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlat(
    const std::vector<viskores::cont::internal::Buffer>&)
  {
    return 3;
  }

  VISKORES_CONT static viskores::Id GetNumberOfValues(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return GetNumberOfValuesPerPlane(buffers) * GetNumberOfPlanesOwned(buffers);
  }

  VISKORES_CONT static viskores::Id GetNumberOfValuesPerPlane(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return SourceStorage::GetNumberOfValues(SourceBuffers(buffers)) / 2;
  }

  VISKORES_CONT static viskores::Id GetNumberOfPlanes(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return GetMetaData(buffers).NumberOfPlanes;
  }

  VISKORES_CONT static viskores::Id GetNumberOfPlanesOwned(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return GetMetaData(buffers).NumberOfPlanesOwned;
  }

  VISKORES_CONT static viskores::Id GetPlaneStartId(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return GetMetaData(buffers).PlaneStartId;
  }

  VISKORES_CONT static bool GetUseCylindrical(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return GetMetaData(buffers).UseCylindrical;
  }

  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return ReadPortalType(SourceStorage::CreateReadPortal(SourceBuffers(buffers), device, token),
                          GetNumberOfPlanes(buffers),
                          GetNumberOfPlanesOwned(buffers),
                          GetPlaneStartId(buffers),
                          GetUseCylindrical(buffers));
  }

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers(
    const viskores::cont::ArrayHandle<T>& array,
    viskores::Id numberOfPlanes,
    viskores::Id numberOfPlanesOwned,
    viskores::Id planeStartId,
    bool useCylindrical)
  {
    return viskores::cont::internal::CreateBuffers(
      MetaData(numberOfPlanes, numberOfPlanesOwned, planeStartId, useCylindrical), array);
  }

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers()
  {
    return CreateBuffers(viskores::cont::ArrayHandle<T>{}, 0, 0, 0, false);
  }

  VISKORES_CONT static viskores::cont::ArrayHandle<T> GetArrayHandle(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return viskores::cont::ArrayHandle<T>(SourceBuffers(buffers));
  }
};

} // namespace detail

template <>
class Storage<viskores::Vec3f_32, viskores::cont::StorageTagXGCCoordinates>
  : public detail::XGCCoordinatesStorageImpl<viskores::Float32>
{
public:
  VISKORES_STORAGE_NO_RESIZE;
  VISKORES_STORAGE_NO_WRITE_PORTAL;
};

template <>
class Storage<viskores::Vec3f_64, viskores::cont::StorageTagXGCCoordinates>
  : public detail::XGCCoordinatesStorageImpl<viskores::Float64>
{
public:
  VISKORES_STORAGE_NO_RESIZE;
  VISKORES_STORAGE_NO_WRITE_PORTAL;
};

} // namespace internal

template <typename T>
class VISKORES_ALWAYS_EXPORT ArrayHandleXGCCoordinates
  : public viskores::cont::ArrayHandle<viskores::Vec<T, 3>,
                                       viskores::cont::StorageTagXGCCoordinates>
{
  using AHandleType = viskores::cont::ArrayHandle<viskores::Vec<T, 3>>;
  using OriginalType = viskores::cont::ArrayHandle<T>;

public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleXGCCoordinates,
    (ArrayHandleXGCCoordinates<T>),
    (viskores::cont::ArrayHandle<viskores::Vec<T, 3>, viskores::cont::StorageTagXGCCoordinates>));

  VISKORES_CONT
  ArrayHandleXGCCoordinates(const OriginalType& array,
                            viskores::Id numberOfPlanes,
                            viskores::Id numberOfPlanesOwned,
                            viskores::Id planeStartId,
                            bool cylindrical)
    : Superclass(StorageType::CreateBuffers(array,
                                            numberOfPlanes,
                                            numberOfPlanesOwned,
                                            planeStartId,
                                            cylindrical))
  {
  }

  ~ArrayHandleXGCCoordinates() {}

  VISKORES_CONT viskores::Id GetNumberOfPlanes() const
  {
    return StorageType::GetNumberOfPlanes(this->GetBuffers());
  }

  VISKORES_CONT viskores::Id GetNumberOfPlanesOwned() const
  {
    return StorageType::GetNumberOfPlanesOwned(this->GetBuffers());
  }

  VISKORES_CONT viskores::Id GetPlaneStartId() const
  {
    return StorageType::GetPlaneStartId(this->GetBuffers());
  }

  VISKORES_CONT bool GetUseCylindrical() const
  {
    return StorageType::GetUseCylindrical(this->GetBuffers());
  }

  VISKORES_CONT viskores::Id GetNumberOfPointsPerPlane() const
  {
    return StorageType::GetNumberOfValuesPerPlane(this->GetBuffers());
  }

  VISKORES_CONT OriginalType GetArray() const
  {
    return StorageType::GetArrayHandle(this->GetBuffers());
  }
};

template <typename T>
viskores::cont::ArrayHandleXGCCoordinates<T> make_ArrayHandleXGCCoordinates(
  const viskores::cont::ArrayHandle<T>& arrHandle,
  viskores::Id numberOfPlanesOwned,
  bool cylindrical,
  viskores::Id numberOfPlanes = -1,
  viskores::Id planeStartId = 0)
{
  if (numberOfPlanes == -1)
  {
    numberOfPlanes = numberOfPlanesOwned;
  }
  return ArrayHandleXGCCoordinates<T>(
    arrHandle, numberOfPlanes, numberOfPlanesOwned, planeStartId, cylindrical);
}

template <typename T>
viskores::cont::ArrayHandleXGCCoordinates<T> make_ArrayHandleXGCCoordinates(
  const T* array,
  viskores::Id length,
  viskores::Id numberOfPlanesOwned,
  bool cylindrical,
  viskores::Id numberOfPlanes = -1,
  viskores::Id planeStartId = 0,
  viskores::CopyFlag copy = viskores::CopyFlag::Off)
{
  if (numberOfPlanes == -1)
  {
    numberOfPlanes = numberOfPlanesOwned;
  }
  return ArrayHandleXGCCoordinates<T>(viskores::cont::make_ArrayHandle(array, length, copy),
                                      numberOfPlanes,
                                      numberOfPlanesOwned,
                                      planeStartId,
                                      cylindrical);
}

// if all planes belong to a single partition, then numberOfPlanes and planeStartId not needed
template <typename T>
viskores::cont::ArrayHandleXGCCoordinates<T> make_ArrayHandleXGCCoordinates(
  const std::vector<T>& array,
  viskores::Id numberOfPlanesOwned,
  bool cylindrical,
  viskores::Id numberOfPlanes = -1,
  viskores::Id planeStartId = 0,
  viskores::CopyFlag copy = viskores::CopyFlag::Off)
{
  if (!array.empty())
  {
    if (numberOfPlanes == -1)
    {
      numberOfPlanes = numberOfPlanesOwned;
    }
    return make_ArrayHandleXGCCoordinates<T>(&array.front(),
                                             static_cast<viskores::Id>(array.size()),
                                             numberOfPlanesOwned,
                                             cylindrical,
                                             numberOfPlanes,
                                             planeStartId,
                                             copy);
  }
  else
  {
    // Vector empty. Just return an empty array handle.
    return ArrayHandleXGCCoordinates<T>();
  }
}

}
} // end namespace viskores::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace viskores
{
namespace cont
{

template <typename T>
struct SerializableTypeString<viskores::cont::ArrayHandleXGCCoordinates<T>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "AH_XGCCoordinates<" + SerializableTypeString<T>::Get() + ">";
    return name;
  }
};

template <typename T>
struct SerializableTypeString<
  viskores::cont::ArrayHandle<viskores::Vec<T, 3>, viskores::cont::StorageTagXGCCoordinates>>
  : SerializableTypeString<viskores::cont::ArrayHandleXGCCoordinates<T>>
{
};
}
} // viskores::cont

namespace mangled_diy_namespace
{

template <typename T>
struct Serialization<viskores::cont::ArrayHandleXGCCoordinates<T>>
{
private:
  using Type = viskores::cont::ArrayHandleXGCCoordinates<T>;
  using BaseType = viskores::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    Type ah = obj;
    viskoresdiy::save(bb, ah.GetNumberOfPlanes());
    viskoresdiy::save(bb, ah.GetNumberOfPlanesOwned());
    viskoresdiy::save(bb, ah.GetPlaneStartId());
    viskoresdiy::save(bb, ah.GetUseCylindrical());
    viskoresdiy::save(bb, ah.GetArray());
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, BaseType& ah)
  {
    viskores::Id numberOfPlanes;
    viskores::Id numberOfPlanesOwned;
    viskores::Id planeStartId;
    bool isCylindrical;
    viskores::cont::ArrayHandle<T> array;

    viskoresdiy::load(bb, numberOfPlanes);
    viskoresdiy::load(bb, numberOfPlanesOwned);
    viskoresdiy::load(bb, planeStartId);
    viskoresdiy::load(bb, isCylindrical);
    viskoresdiy::load(bb, array);

    ah = viskores::cont::make_ArrayHandleXGCCoordinates(
      array, numberOfPlanes, numberOfPlanesOwned, planeStartId, isCylindrical);
  }
};

template <typename T>
struct Serialization<
  viskores::cont::ArrayHandle<viskores::Vec<T, 3>, viskores::cont::StorageTagXGCCoordinates>>
  : Serialization<viskores::cont::ArrayHandleXGCCoordinates<T>>
{
};
} // diy
/// @endcond SERIALIZATION
#endif
