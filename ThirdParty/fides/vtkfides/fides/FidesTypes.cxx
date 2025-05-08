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
viskores::cont::Field::Association ConvertToViskoresAssociation(fides::Association assoc)
{
  switch (assoc)
  {
    case fides::Association::POINTS:
      return viskores::cont::Field::Association::Points;
    case fides::Association::CELL_SET:
      return viskores::cont::Field::Association::Cells;
    case fides::Association::FIELD_DATA:
      return viskores::cont::Field::Association::WholeDataSet;
    default:
      throw std::runtime_error("unknown association provided");
  }
}

fides::Association ConvertViskoresAssociationToFides(viskores::cont::Field::Association assoc)
{
  switch (assoc)
  {
    case viskores::cont::Field::Association::Points:
      return fides::Association::POINTS;
    case viskores::cont::Field::Association::Cells:
      return fides::Association::CELL_SET;
    default:
      throw std::runtime_error("Can only convert POINTS and CELL_SET to an fides::Association");
  }
}
FIDES_DEPRECATED_SUPPRESS_END


std::string ConvertViskoresCellTypeToFides(viskores::UInt8 cellShapeType)
{
  std::string cellName;

  if (cellShapeType == viskores::CELL_SHAPE_VERTEX)
  {
    cellName = "vertex";
  }
  else if (cellShapeType == viskores::CELL_SHAPE_LINE)
  {
    cellName = "line";
  }
  else if (cellShapeType == viskores::CELL_SHAPE_TRIANGLE)
  {
    cellName = "triangle";
  }
  else if (cellShapeType == viskores::CELL_SHAPE_QUAD)
  {
    cellName = "quad";
  }
  else if (cellShapeType == viskores::CELL_SHAPE_TETRA)
  {
    cellName = "tetrahedron";
  }
  else if (cellShapeType == viskores::CELL_SHAPE_HEXAHEDRON)
  {
    cellName = "hexahedron";
  }
  else if (cellShapeType == viskores::CELL_SHAPE_WEDGE)
  {
    cellName = "wedge";
  }
  else if (cellShapeType == viskores::CELL_SHAPE_PYRAMID)
  {
    cellName = "pyramid";
  }

  return cellName;
}

/// Converts a fides cell name to Viskores cell shape type.
/// Throws a runtime error for unsupported cell types.
viskores::UInt8 FIDES_EXPORT ConvertFidesCellTypeToViskores(const std::string& cellShapeName)
{
  viskores::UInt8 cellType = viskores::CELL_SHAPE_EMPTY;

  if (cellShapeName == "vertex")
  {
    cellType = viskores::CELL_SHAPE_VERTEX;
  }
  else if (cellShapeName == "line")
  {
    cellType = viskores::CELL_SHAPE_LINE;
  }
  else if (cellShapeName == "triangle")
  {
    cellType = viskores::CELL_SHAPE_TRIANGLE;
  }
  else if (cellShapeName == "quad")
  {
    cellType = viskores::CELL_SHAPE_QUAD;
  }
  else if (cellShapeName == "tetrahedron")
  {
    cellType = viskores::CELL_SHAPE_TETRA;
  }
  else if (cellShapeName == "hexahedron")
  {
    cellType = viskores::CELL_SHAPE_HEXAHEDRON;
  }
  else if (cellShapeName == "wedge")
  {
    cellType = viskores::CELL_SHAPE_WEDGE;
  }
  else if (cellShapeName == "pyramid")
  {
    cellType = viskores::CELL_SHAPE_PYRAMID;
  }
  else
  {
    throw std::runtime_error("Unsupported fides cell type: " + cellShapeName);
  }

  return cellType;
}

}
