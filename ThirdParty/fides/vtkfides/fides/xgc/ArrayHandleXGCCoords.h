//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef fides_xgc_ArrayHandleXGCCoords_h
#define fides_xgc_ArrayHandleXGCCoords_h

#include <fides/xgc/StorageXGC.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CoordinateSystem.h>

namespace vtkm
{
namespace cont
{

template <typename T>
class VTKM_ALWAYS_EXPORT ArrayHandleXGCCoords
  : public vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, internal::StorageTagXGC>
{

  using StorageType = vtkm::cont::internal::Storage<vtkm::Vec<T, 3>, internal::StorageTagXGC>;

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleXGCCoords,
    (ArrayHandleXGCCoords<T>),
    (vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, internal::StorageTagXGC>));

  ArrayHandleXGCCoords(const StorageType& storage)
    : Superclass(storage)
  {
  }

  vtkm::Id GetNumberOfPointsPerPlane() const { return (this->GetStorage().GetLength() / 2); }
  vtkm::Id GetNumberOfPlanes() const { return this->GetStorage().GetNumberOfPlanes(); }
  bool GetUseCylindrical() const { return this->GetStorage().GetUseCylindrical(); }
  const vtkm::cont::ArrayHandle<T>& GetArray() const { return this->GetStorage().Array; }
};

template <typename T>
vtkm::cont::ArrayHandleXGCCoords<T> make_ArrayHandleXGCCoords(
  const vtkm::cont::ArrayHandle<T> arrHandle,
  vtkm::Id numberOfPlanes,
  vtkm::Id numberOfPlanesOwned,
  vtkm::Id planeStartId,
  bool cylindrical)
{
  using StorageType = vtkm::cont::internal::Storage<vtkm::Vec<T, 3>, internal::StorageTagXGC>;
  auto storage = StorageType(arrHandle, numberOfPlanes, numberOfPlanesOwned, planeStartId, cylindrical);
  return ArrayHandleXGCCoords<T>(storage);
}

template <typename T>
vtkm::cont::ArrayHandleXGCCoords<T> make_ArrayHandleXGCCoords(
  const T* array,
  vtkm::Id length,
  vtkm::Id numberOfPlanes,
  vtkm::Id numberOfPlanesOwned,
  vtkm::Id planeStartId,
  bool cylindrical,
  vtkm::CopyFlag copy = vtkm::CopyFlag::Off)
{
  using StorageType = vtkm::cont::internal::Storage<vtkm::Vec<T, 3>, internal::StorageTagXGC>;
  if (copy == vtkm::CopyFlag::Off)
  {
    return ArrayHandleXGCCoords<T>(StorageType(array, length, numberOfPlanes,
      numberOfPlanesOwned, planeStartId, cylindrical));
  }
  else
  {
    auto storage = StorageType(
      vtkm::cont::make_ArrayHandle(array, length, vtkm::CopyFlag::On), numberOfPlanes,
      numberOfPlanesOwned, planeStartId, cylindrical);
    return ArrayHandleXGCCoords<T>(storage);
  }
}

template <typename T>
vtkm::cont::ArrayHandleXGCCoords<T> make_ArrayHandleXGCCoords(
  const std::vector<T>& array,
  vtkm::Id numberOfPlanes,
  vtkm::Id numberOfPlanesOwned,
  vtkm::Id planeStartId,
  bool cylindrical,
  vtkm::CopyFlag copy = vtkm::CopyFlag::Off)
{
  if (!array.empty())
  {
    return make_ArrayHandleXGCCoords(
      &array.front(), static_cast<vtkm::Id>(array.size()), numberOfPlanes,
      numberOfPlanesOwned, planeStartId, cylindrical, copy);
  }
  else
  {
    // Vector empty. Just return an empty array handle.
    return ArrayHandleXGCCoords<T>();
  }
}
}
} // end namespace vtkm::cont

#endif
