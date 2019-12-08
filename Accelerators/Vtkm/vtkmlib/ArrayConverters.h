//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================

#ifndef vtkmlib_ArrayConverters_h
#define vtkmlib_ArrayConverters_h

#include "vtkAcceleratorsVTKmModule.h" //required for correct implementation
#include "vtkmConfig.h"                //required for general vtkm setup

#include "vtkAOSDataArrayTemplate.h"
#include "vtkSOADataArrayTemplate.h"

#include <vtkm/cont/ArrayHandleSOA.h>
#include <vtkm/cont/Field.h>

#include <type_traits> // for std::underlying_type

class vtkDataArray;
class vtkDataSet;
class vtkPoints;

namespace vtkm
{
namespace cont
{
class DataSet;
class CoordinateSystem;
}
}

namespace tovtkm
{

template <typename DataArrayType, vtkm::IdComponent NumComponents>
struct DataArrayToArrayHandle;

template <typename T, vtkm::IdComponent NumComponents>
struct DataArrayToArrayHandle<vtkAOSDataArrayTemplate<T>, NumComponents>
{
  using ValueType =
    typename std::conditional<NumComponents == 1, T, vtkm::Vec<T, NumComponents> >::type;
  using StorageType = vtkm::cont::internal::Storage<ValueType, vtkm::cont::StorageTagBasic>;
  using ArrayHandleType = vtkm::cont::ArrayHandle<ValueType, vtkm::cont::StorageTagBasic>;

  static ArrayHandleType Wrap(vtkAOSDataArrayTemplate<T>* input)
  {
    return vtkm::cont::make_ArrayHandle(
      reinterpret_cast<ValueType*>(input->GetPointer(0)), input->GetNumberOfTuples());
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
    vtkm::cont::internal::Storage<ValueType, vtkm::cont::StorageTagSOA> storage;
    for (vtkm::IdComponent i = 0; i < NumComponents; ++i)
    {
      storage.SetArray(i,
        vtkm::cont::make_ArrayHandle<T>(
          reinterpret_cast<T*>(input->GetComponentArrayPointer(i)), numValues));
    }

    return vtkm::cont::ArrayHandleSOA<ValueType>(storage);
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
      input->GetComponentArrayPointer(0), input->GetNumberOfTuples());
  }
};

enum class FieldsFlag
{
  None = 0x0,
  Points = 0x1,
  Cells = 0x2,

  PointsAndCells = Points | Cells
};

VTKACCELERATORSVTKM_EXPORT
void ProcessFields(vtkDataSet* input, vtkm::cont::DataSet& dataset, tovtkm::FieldsFlag fields);

// determine the type and call the proper Convert routine
VTKACCELERATORSVTKM_EXPORT
vtkm::cont::Field Convert(vtkDataArray* input, int association);
}

namespace fromvtkm
{

VTKACCELERATORSVTKM_EXPORT
vtkDataArray* Convert(const vtkm::cont::Field& input);

VTKACCELERATORSVTKM_EXPORT
vtkPoints* Convert(const vtkm::cont::CoordinateSystem& input);

VTKACCELERATORSVTKM_EXPORT
bool ConvertArrays(const vtkm::cont::DataSet& input, vtkDataSet* output);
}

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

#endif // vtkmlib_ArrayConverters_h
