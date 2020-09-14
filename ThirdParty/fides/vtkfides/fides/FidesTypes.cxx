//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/FidesTypes.h>


namespace fides
{

vtkm::cont::Field::Association ConvertToVTKmAssociation(fides::Association assoc)
{
  switch (assoc)
  {
    case fides::Association::POINTS:
      return vtkm::cont::Field::Association::POINTS;
    case fides::Association::CELL_SET:
      return vtkm::cont::Field::Association::CELL_SET;
    case fides::Association::FIELD_DATA:
      throw std::runtime_error("FIELD_DATA association is not valid in VTK-m");
    default:
      throw std::runtime_error("unknown association provided");
  }
}

fides::Association ConvertVTKmAssociationToFides(vtkm::cont::Field::Association assoc)
{
  switch (assoc)
  {
    case vtkm::cont::Field::Association::POINTS:
      return fides::Association::POINTS;
    case vtkm::cont::Field::Association::CELL_SET:
      return fides::Association::CELL_SET;
    default:
      throw std::runtime_error("Can only convert POINTS and CELL_SET to an fides::Association");
  }
}

}
