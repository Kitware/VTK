//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef fides_xgc_ArrayHandleXGCField_h
#define fides_xgc_ArrayHandleXGCField_h

#include <vtkm/cont/ArrayHandle.h>
#include <fides/xgc/StorageXGC.h>

namespace vtkm
{
namespace cont
{

template <typename T>
class VTKM_ALWAYS_EXPORT ArrayHandleXGCField
  : public vtkm::cont::ArrayHandle<T, internal::StorageTagXGCPlane>
{
  using StorageType = vtkm::cont::internal::Storage<T, internal::StorageTagXGCPlane>;

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleXGCField,
                             (ArrayHandleXGCField<T>),
                             (vtkm::cont::ArrayHandle<T, internal::StorageTagXGCPlane>));

  ArrayHandleXGCField(const StorageType& storage)
    : Superclass(storage)
  {
  }

  ArrayHandleXGCField(StorageType& storage)
    : Superclass(storage)
  {}

  vtkm::Id GetNumberOfValuesPerPlane() const
  {
    return this->GetStorage().GetNumberOfValuesPerPlane();
  }

  vtkm::Id GetNumberOfPlanes() const { return this->GetStorage().GetNumberOfPlanes(); }

  const vtkm::cont::ArrayHandle<T>& GetArray() const { return this->GetStorage().Array; }
};

template <typename T>
vtkm::cont::ArrayHandleXGCField<T> make_ArrayHandleXGCField(
  const vtkm::cont::ArrayHandle<T>& array,
  vtkm::Id numberOfPlanes,
  bool is2dField)
{
  using StorageType = vtkm::cont::internal::Storage<T, internal::StorageTagXGCPlane>;
  auto storage = StorageType(array, numberOfPlanes, is2dField);
  return ArrayHandleXGCField<T>(storage);
}

template <typename T>
vtkm::cont::ArrayHandleXGCField<T> make_ArrayHandleXGCField(
  const T* array,
  vtkm::Id length,
  vtkm::Id numberOfPlanes,
  bool is2dField,
  vtkm::CopyFlag copy = vtkm::CopyFlag::Off)
{
  using StorageType = vtkm::cont::internal::Storage<T, internal::StorageTagXGCPlane>;
  auto storage =
    StorageType(vtkm::cont::make_ArrayHandle(array, length, copy), numberOfPlanes, is2dField);
  return ArrayHandleXGCField<T>(storage);
}

template <typename T>
vtkm::cont::ArrayHandleXGCField<T> make_ArrayHandleXGCField(
  const std::vector<T>& array,
  vtkm::Id numberOfPlanes,
  bool is2dField,
  vtkm::CopyFlag copy)
{
  if (!array.empty())
  {
    return make_ArrayHandleXGCField(
      array.data(), static_cast<vtkm::Id>(array.size()), numberOfPlanes, is2dField, copy);
  }
  else
  {
    // Vector empty. Just return an empty array handle.
    return ArrayHandleXGCField<T>();
  }
}

template <typename T>
vtkm::cont::ArrayHandleXGCField<T> make_ArrayHandleXGCField(
  const std::vector<vtkm::cont::ArrayHandle<T> >& arrays,
  vtkm::Id numberOfPlanes,
  bool is2dField)
{
  using StorageType = vtkm::cont::internal::Storage<T, internal::StorageTagXGCPlane>;
  auto storage = StorageType(arrays, numberOfPlanes, is2dField);
  return ArrayHandleXGCField<T>(storage);
}

template <typename T>
vtkm::cont::ArrayHandleXGCField<T> make_ArrayHandleXGCField(
  vtkm::Id numberOfPlanes,
  vtkm::Id numberOfValuesPerPlane,
  bool is2dField)
{
  using StorageType = vtkm::cont::internal::Storage<T, internal::StorageTagXGCPlane>;
  auto storage = StorageType(numberOfPlanes, numberOfValuesPerPlane, is2dField);
  return ArrayHandleXGCField<T>(storage);
}


}
} // vtkm::cont

#endif
