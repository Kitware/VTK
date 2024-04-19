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

FIDES_DEPRECATED_SUPPRESS_BEGIN
vtkm::cont::Field::Association ConvertToVTKmAssociation(fides::Association assoc)
{
  switch (assoc)
  {
    case fides::Association::POINTS:
      return vtkm::cont::Field::Association::Points;
    case fides::Association::CELL_SET:
      return vtkm::cont::Field::Association::Cells;
    case fides::Association::FIELD_DATA:
      return vtkm::cont::Field::Association::WholeDataSet;
    default:
      throw std::runtime_error("unknown association provided");
  }
}

fides::Association ConvertVTKmAssociationToFides(vtkm::cont::Field::Association assoc)
{
  switch (assoc)
  {
    case vtkm::cont::Field::Association::Points:
      return fides::Association::POINTS;
    case vtkm::cont::Field::Association::Cells:
      return fides::Association::CELL_SET;
    default:
      throw std::runtime_error("Can only convert POINTS and CELL_SET to an fides::Association");
  }
}
FIDES_DEPRECATED_SUPPRESS_END


std::string ConvertVTKmCellTypeToFides(vtkm::UInt8 cellShapeType)
{
  std::string cellName;

  if (cellShapeType == vtkm::CELL_SHAPE_VERTEX)
  {
    cellName = "vertex";
  }
  else if (cellShapeType == vtkm::CELL_SHAPE_LINE)
  {
    cellName = "line";
  }
  else if (cellShapeType == vtkm::CELL_SHAPE_TRIANGLE)
  {
    cellName = "triangle";
  }
  else if (cellShapeType == vtkm::CELL_SHAPE_QUAD)
  {
    cellName = "quad";
  }
  else if (cellShapeType == vtkm::CELL_SHAPE_TETRA)
  {
    cellName = "tetrahedron";
  }
  else if (cellShapeType == vtkm::CELL_SHAPE_HEXAHEDRON)
  {
    cellName = "hexahedron";
  }
  else if (cellShapeType == vtkm::CELL_SHAPE_WEDGE)
  {
    cellName = "wedge";
  }
  else if (cellShapeType == vtkm::CELL_SHAPE_PYRAMID)
  {
    cellName = "pyramid";
  }

  return cellName;
}

/// Converts a fides cell name to VTKm cell shape type.
/// Throws a runtime error for unsupported cell types.
vtkm::UInt8 FIDES_EXPORT ConvertFidesCellTypeToVTKm(const std::string& cellShapeName)
{
  vtkm::UInt8 cellType = vtkm::CELL_SHAPE_EMPTY;

  if (cellShapeName == "vertex")
  {
    cellType = vtkm::CELL_SHAPE_VERTEX;
  }
  else if (cellShapeName == "line")
  {
    cellType = vtkm::CELL_SHAPE_LINE;
  }
  else if (cellShapeName == "triangle")
  {
    cellType = vtkm::CELL_SHAPE_TRIANGLE;
  }
  else if (cellShapeName == "quad")
  {
    cellType = vtkm::CELL_SHAPE_QUAD;
  }
  else if (cellShapeName == "tetrahedron")
  {
    cellType = vtkm::CELL_SHAPE_TETRA;
  }
  else if (cellShapeName == "hexahedron")
  {
    cellType = vtkm::CELL_SHAPE_HEXAHEDRON;
  }
  else if (cellShapeName == "wedge")
  {
    cellType = vtkm::CELL_SHAPE_WEDGE;
  }
  else if (cellShapeName == "pyramid")
  {
    cellType = vtkm::CELL_SHAPE_PYRAMID;
  }
  else
  {
    throw std::runtime_error("Unsupported fides cell type: " + cellShapeName);
  }

  return cellType;
}

}
