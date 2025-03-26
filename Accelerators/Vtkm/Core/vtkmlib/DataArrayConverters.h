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

#include "vtkLogger.h"

#include <vtkm/cont/ArrayExtractComponent.h>
#include <vtkm/cont/ArrayHandleBasic.h>
#include <vtkm/cont/ArrayHandleRecombineVec.h>
#include <vtkm/cont/ArrayHandleRuntimeVec.h>
#include <vtkm/cont/ArrayHandleSOA.h>
#include <vtkm/cont/ArrayHandleStride.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/UnknownArrayHandle.h>

#include <type_traits> // for std::underlying_type
#include <utility>     // for std::pair

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

template <typename T>
vtkm::cont::ArrayHandleBasic<T> vtkAOSDataArrayToFlatArrayHandle(vtkAOSDataArrayTemplate<T>* input)
{
  // Register a reference to the input here to make sure the array cannot
  // be deleted before the `ArrayHandle` is done with it. (Note that you
  // will still get problems if the `vtkAOSDataArrayTemplate` gets resized.
  input->Register(nullptr);

  auto deleter = [](void* container) {
    vtkAOSDataArrayTemplate<T>* vtkArray = reinterpret_cast<vtkAOSDataArrayTemplate<T>*>(container);
    vtkArray->UnRegister(nullptr);
  };
  auto reallocator = [](void*& memory, void*& container, vtkm::BufferSizeType oldSize,
                       vtkm::BufferSizeType newSize) {
    vtkAOSDataArrayTemplate<T>* vtkArray = reinterpret_cast<vtkAOSDataArrayTemplate<T>*>(container);
    if ((vtkArray->GetVoidPointer(0) != memory) || (vtkArray->GetNumberOfValues() != oldSize))
    {
      vtkLog(ERROR,
        "Dangerous inconsistency found between pointers for VTK and VTK-m. "
        "Was the VTK array resized outside of VTK-m?");
    }
    vtkArray->SetNumberOfValues(newSize);
    memory = vtkArray->GetVoidPointer(0);
  };

  return vtkm::cont::ArrayHandleBasic<T>(
    input->GetPointer(0), input, input->GetNumberOfValues(), deleter, reallocator);
}

template <typename T>
vtkm::cont::ArrayHandleBasic<T> vtkSOADataArrayToComponentArrayHandle(
  vtkSOADataArrayTemplate<T>* input, int componentIndex)
{
  // Register for each component (as each will have the deleter call to
  // unregister).
  input->Register(nullptr);

  using ContainerPair = std::pair<vtkSOADataArrayTemplate<T>*, int>;
  ContainerPair* componentInput = new ContainerPair(input, componentIndex);

  auto deleter = [](void* container) {
    ContainerPair* containerPair = reinterpret_cast<ContainerPair*>(container);
    containerPair->first->UnRegister(nullptr);
    delete containerPair;
  };
  auto reallocator = [](void*& memory, void*& container, vtkm::BufferSizeType vtkNotUsed(oldSize),
                       vtkm::BufferSizeType newSize) {
    ContainerPair* containerPair = reinterpret_cast<ContainerPair*>(container);
    containerPair->first->SetNumberOfTuples(newSize);
    memory = containerPair->first->GetComponentArrayPointer(containerPair->second);
  };

  return vtkm::cont::ArrayHandleBasic<T>(input->GetComponentArrayPointer(componentIndex),
    componentInput, input->GetNumberOfTuples(), deleter, reallocator);
}

template <typename T>
vtkm::cont::ArrayHandleRuntimeVec<T> vtkDataArrayToArrayHandle(vtkAOSDataArrayTemplate<T>* input)
{
  auto flatArray = vtkAOSDataArrayToFlatArrayHandle(input);
  return vtkm::cont::make_ArrayHandleRuntimeVec(input->GetNumberOfComponents(), flatArray);
}

template <typename T>
vtkm::cont::ArrayHandleRecombineVec<T> vtkDataArrayToArrayHandle(vtkSOADataArrayTemplate<T>* input)
{
  // Wrap each component array in a basic array handle, convert that to a
  // strided array, and then add that as a component to the returned
  // recombined vec.
  vtkm::cont::ArrayHandleRecombineVec<T> output;

  for (int componentIndex = 0; componentIndex < input->GetNumberOfComponents(); ++componentIndex)
  {
    auto componentArray = vtkSOADataArrayToComponentArrayHandle(input, componentIndex);
    output.AppendComponentArray(
      vtkm::cont::ArrayExtractComponent(componentArray, 0, vtkm::CopyFlag::Off));
  }

  return output;
}

template <typename DataArrayType>
vtkm::cont::UnknownArrayHandle vtkDataArrayToUnknownArrayHandle(DataArrayType* input)
{
  return vtkDataArrayToArrayHandle(input);
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

  VTK_DEPRECATED_IN_9_3_0("Use vtkDataArrayToArrayHandle or vtkAOSDataArrayToFlatArrayHandle.")
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

  VTK_DEPRECATED_IN_9_3_0("Use vtkDataArrayToArrayHandle or vtkSOADataArrayToComponentArrayHandle.")
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

  VTK_DEPRECATED_IN_9_3_0("Use vtkDataArrayToArrayHandle or vtkSOADataArrayToComponentArrayHandle.")
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
