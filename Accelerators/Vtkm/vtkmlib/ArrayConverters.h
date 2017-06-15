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

#include <vtkm/cont/Field.h>

#include <type_traits> // for std::underlying_type

class vtkDataArray;
class vtkDataSet;
class vtkPoints;

namespace vtkm {
namespace cont {
class DataSet;
class CoordinateSystem;
}
}

namespace tovtkm {

enum class FieldsFlag
{
  None   = 0x0,
  Points = 0x1,
  Cells  = 0x2,

  PointsAndCells = Points | Cells
};

VTKACCELERATORSVTKM_EXPORT
void ProcessFields(vtkDataSet *input, vtkm::cont::DataSet &dataset,
                   tovtkm::FieldsFlag fields);

// determine the type and call the proper Convert routine
VTKACCELERATORSVTKM_EXPORT
vtkm::cont::Field Convert(vtkDataArray* input, int association);
}

namespace fromvtkm {

VTKACCELERATORSVTKM_EXPORT
vtkDataArray* Convert(const vtkm::cont::Field& input);

VTKACCELERATORSVTKM_EXPORT
vtkPoints* Convert(const vtkm::cont::CoordinateSystem& input);

VTKACCELERATORSVTKM_EXPORT
bool ConvertArrays(const vtkm::cont::DataSet& input, vtkDataSet* output);
}

inline tovtkm::FieldsFlag operator&(const tovtkm::FieldsFlag &a,
                                    const tovtkm::FieldsFlag &b)
{
  using T = std::underlying_type<tovtkm::FieldsFlag>::type;
  return static_cast<tovtkm::FieldsFlag>(static_cast<T>(a) & static_cast<T>(b));
}

inline tovtkm::FieldsFlag operator|(const tovtkm::FieldsFlag &a,
                                    const tovtkm::FieldsFlag &b)
{
  using T = std::underlying_type<tovtkm::FieldsFlag>::type;
  return static_cast<tovtkm::FieldsFlag>(static_cast<T>(a) | static_cast<T>(b));
}

#endif // vtkmlib_ArrayConverters_h
