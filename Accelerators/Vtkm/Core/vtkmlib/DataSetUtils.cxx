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
#include "DataSetUtils.h"

#include <algorithm>
#include <numeric>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
std::vector<vtkm::Id> GetFieldsIndicesWithoutCoords(const vtkm::cont::DataSet& input)
{
  std::vector<vtkm::Id> allFields(input.GetNumberOfFields());
  std::vector<vtkm::Id> coords(input.GetNumberOfCoordinateSystems());

  std::iota(allFields.begin(), allFields.end(), 0);
  std::iota(coords.begin(), coords.end(), 0);
  std::transform(coords.begin(), coords.end(), coords.begin(),
    [&input](auto idx) { return input.GetFieldIndex(input.GetCoordinateSystemName(idx)); });

  std::vector<vtkm::Id> fields;
  std::set_difference(
    allFields.begin(), allFields.end(), coords.begin(), coords.end(), std::back_inserter(fields));

  return fields;
}
VTK_ABI_NAMESPACE_END
