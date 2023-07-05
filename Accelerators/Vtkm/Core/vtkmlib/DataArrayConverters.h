// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#ifndef vtkmlib_DataArrayConverters_h
#define vtkmlib_DataArrayConverters_h

#include "vtkAcceleratorsVTKmCoreModule.h" //required for correct implementation
#include "vtkmConfigCore.h"                //required for general vtkm setup

#include "vtkAOSDataArrayTemplate.h"
#include "vtkSOADataArrayTemplate.h"

#include <vtkm/cont/ArrayHandleSOA.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/UnknownArrayHandle.h>

#include <type_traits> // for std::underlying_type

namespace vtkm
{
namespace cont
{
class CoordinateSystem;
}
}

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkPoints;
VTK_ABI_NAMESPACE_END

namespace tovtkm
{
VTK_ABI_NAMESPACE_BEGIN

/// Temporary name for arrays converted from VTK that do not have a name.
/// Unnamed arrays seem to be supported by VTK, but VTK-m requires all fields to have a name.
///
inline static const char* NoNameVTKFieldName()
{
  static const char* name = "NoNameVTKField";
  return name;
}

template <typename DataArrayType, vtkm::IdComponent NumComponents>
struct DataArrayToArrayHandle;

template <typename T, vtkm::IdComponent NumComponents>
struct DataArrayToArrayHandle<vtkAOSDataArrayTemplate<T>, NumComponents>
{
  using ValueType =
    typename std::conditional<NumComponents == 1, T, vtkm::Vec<T, NumComponents>>::type;
  using StorageType = vtkm::cont::internal::Storage<ValueType, vtkm::cont::StorageTagBasic>;
  using ArrayHandleType = vtkm::cont::ArrayHandle<ValueType, vtkm::cont::StorageTagBasic>;

  static ArrayHandleType Wrap(vtkAOSDataArrayTemplate<T>* input)
  {
    return vtkm::cont::make_ArrayHandle(reinterpret_cast<ValueType*>(input->GetPointer(0)),
      input->GetNumberOfTuples(), vtkm::CopyFlag::Off);
  }
};

template <typename T, vtkm::IdComponent NumComponents>
struct DataArrayToArrayHandle<vtkSOADataArrayTemplate<T>, NumComponents>
{
  using ValueType = vtkm::Vec<T, NumComponents>;
  using StorageType = vtkm::cont::internal::Storage<ValueType, vtkm::cont::StorageTagSOA>;
  using ArrayHandleType = vtkm::cont::ArrayHandle<ValueType, vtkm::cont::StorageTagSOA>;

  static ArrayHandleType Wrap(vtkSOADataArrayTemplate<T>* input)
  {
    vtkm::Id numValues = input->GetNumberOfTuples();
    vtkm::cont::ArrayHandleSOA<ValueType> handle;
    for (vtkm::IdComponent i = 0; i < NumComponents; ++i)
    {
      handle.SetArray(i,
        vtkm::cont::make_ArrayHandle<T>(reinterpret_cast<T*>(input->GetComponentArrayPointer(i)),
          numValues, vtkm::CopyFlag::Off));
    }

    return std::move(handle);
  }
};

template <typename T>
struct DataArrayToArrayHandle<vtkSOADataArrayTemplate<T>, 1>
{
  using StorageType = vtkm::cont::internal::Storage<T, vtkm::cont::StorageTagBasic>;
  using ArrayHandleType = vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>;

  static ArrayHandleType Wrap(vtkSOADataArrayTemplate<T>* input)
  {
    return vtkm::cont::make_ArrayHandle(
      input->GetComponentArrayPointer(0), input->GetNumberOfTuples(), vtkm::CopyFlag::Off);
  }
};

enum class FieldsFlag
{
  None = 0x0,
  Points = 0x1,
  Cells = 0x2,

  PointsAndCells = Points | Cells
};

VTK_ABI_NAMESPACE_END
}

namespace fromvtkm
{
VTK_ABI_NAMESPACE_BEGIN

VTKACCELERATORSVTKMCORE_EXPORT
vtkDataArray* Convert(const vtkm::cont::Field& input);

VTKACCELERATORSVTKMCORE_EXPORT
vtkDataArray* Convert(const vtkm::cont::UnknownArrayHandle& input, const char* name);

VTKACCELERATORSVTKMCORE_EXPORT
vtkPoints* Convert(const vtkm::cont::CoordinateSystem& input);

VTK_ABI_NAMESPACE_END
}

VTK_ABI_NAMESPACE_BEGIN
inline tovtkm::FieldsFlag operator&(const tovtkm::FieldsFlag& a, const tovtkm::FieldsFlag& b)
{
  using T = std::underlying_type<tovtkm::FieldsFlag>::type;
  return static_cast<tovtkm::FieldsFlag>(static_cast<T>(a) & static_cast<T>(b));
}

inline tovtkm::FieldsFlag operator|(const tovtkm::FieldsFlag& a, const tovtkm::FieldsFlag& b)
{
  using T = std::underlying_type<tovtkm::FieldsFlag>::type;
  return static_cast<tovtkm::FieldsFlag>(static_cast<T>(a) | static_cast<T>(b));
}
VTK_ABI_NAMESPACE_END

#endif // vtkmlib_ArrayConverters_h
/* VTK-HeaderTest-Exclude: DataArrayConverters.h */
