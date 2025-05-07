// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#ifndef vtkmlib_DataArrayConverters_h
#define vtkmlib_DataArrayConverters_h

#include "vtkAcceleratorsVTKmCoreModule.h" //required for correct implementation
#include "vtkmConfigCore.h"                //required for general viskores setup

#include "vtkAOSDataArrayTemplate.h"
#include "vtkSOADataArrayTemplate.h"

#include "vtkLogger.h"

#include <viskores/cont/ArrayExtractComponent.h>
#include <viskores/cont/ArrayHandleBasic.h>
#include <viskores/cont/ArrayHandleRecombineVec.h>
#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/ArrayHandleSOA.h>
#include <viskores/cont/ArrayHandleStride.h>
#include <viskores/cont/Field.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <type_traits> // for std::underlying_type
#include <utility>     // for std::pair

namespace viskores
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
/// Unnamed arrays seem to be supported by VTK, but Viskores requires all fields to have a name.
///
inline static const char* NoNameVTKFieldName()
{
  static const char* name = "NoNameVTKField";
  return name;
}

template <typename T>
viskores::cont::ArrayHandleBasic<T> vtkAOSDataArrayToFlatArrayHandle(
  vtkAOSDataArrayTemplate<T>* input)
{
  // Register a reference to the input here to make sure the array cannot
  // be deleted before the `ArrayHandle` is done with it. (Note that you
  // will still get problems if the `vtkAOSDataArrayTemplate` gets resized.
  input->Register(nullptr);

  auto deleter = [](void* container)
  {
    vtkAOSDataArrayTemplate<T>* vtkArray = reinterpret_cast<vtkAOSDataArrayTemplate<T>*>(container);
    vtkArray->UnRegister(nullptr);
  };
  auto reallocator = [](void*& memory, void*& container, viskores::BufferSizeType oldSize,
                       viskores::BufferSizeType newSize)
  {
    vtkAOSDataArrayTemplate<T>* vtkArray = reinterpret_cast<vtkAOSDataArrayTemplate<T>*>(container);
    if ((vtkArray->GetVoidPointer(0) != memory) || (vtkArray->GetNumberOfValues() != oldSize))
    {
      vtkLog(ERROR,
        "Dangerous inconsistency found between pointers for VTK and Viskores. "
        "Was the VTK array resized outside of Viskores?");
    }
    vtkArray->SetNumberOfValues(newSize);
    memory = vtkArray->GetVoidPointer(0);
  };

  return viskores::cont::ArrayHandleBasic<T>(
    input->GetPointer(0), input, input->GetNumberOfValues(), deleter, reallocator);
}

template <typename T>
viskores::cont::ArrayHandleBasic<T> vtkSOADataArrayToComponentArrayHandle(
  vtkSOADataArrayTemplate<T>* input, int componentIndex)
{
  // Register for each component (as each will have the deleter call to
  // unregister).
  input->Register(nullptr);

  using ContainerPair = std::pair<vtkSOADataArrayTemplate<T>*, int>;
  ContainerPair* componentInput = new ContainerPair(input, componentIndex);

  auto deleter = [](void* container)
  {
    ContainerPair* containerPair = reinterpret_cast<ContainerPair*>(container);
    containerPair->first->UnRegister(nullptr);
    delete containerPair;
  };
  auto reallocator = [](void*& memory, void*& container,
                       viskores::BufferSizeType vtkNotUsed(oldSize),
                       viskores::BufferSizeType newSize)
  {
    ContainerPair* containerPair = reinterpret_cast<ContainerPair*>(container);
    containerPair->first->SetNumberOfTuples(newSize);
    memory = containerPair->first->GetComponentArrayPointer(containerPair->second);
  };

  return viskores::cont::ArrayHandleBasic<T>(input->GetComponentArrayPointer(componentIndex),
    componentInput, input->GetNumberOfTuples(), deleter, reallocator);
}

template <typename T>
viskores::cont::ArrayHandleRuntimeVec<T> vtkDataArrayToArrayHandle(
  vtkAOSDataArrayTemplate<T>* input)
{
  auto flatArray = vtkAOSDataArrayToFlatArrayHandle(input);
  return viskores::cont::make_ArrayHandleRuntimeVec(input->GetNumberOfComponents(), flatArray);
}

template <typename T>
viskores::cont::ArrayHandleRecombineVec<T> vtkDataArrayToArrayHandle(
  vtkSOADataArrayTemplate<T>* input)
{
  // Wrap each component array in a basic array handle, convert that to a
  // strided array, and then add that as a component to the returned
  // recombined vec.
  viskores::cont::ArrayHandleRecombineVec<T> output;

  for (int componentIndex = 0; componentIndex < input->GetNumberOfComponents(); ++componentIndex)
  {
    auto componentArray = vtkSOADataArrayToComponentArrayHandle(input, componentIndex);
    output.AppendComponentArray(
      viskores::cont::ArrayExtractComponent(componentArray, 0, viskores::CopyFlag::Off));
  }

  return output;
}

template <typename DataArrayType>
viskores::cont::UnknownArrayHandle vtkDataArrayToUnknownArrayHandle(DataArrayType* input)
{
  return vtkDataArrayToArrayHandle(input);
}

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
vtkDataArray* Convert(const viskores::cont::Field& input);

VTKACCELERATORSVTKMCORE_EXPORT
vtkDataArray* Convert(const viskores::cont::UnknownArrayHandle& input, const std::string& name);

VTKACCELERATORSVTKMCORE_EXPORT
vtkPoints* Convert(const viskores::cont::CoordinateSystem& input);

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
