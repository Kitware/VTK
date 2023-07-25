// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGHex.h"

#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkStringToken.h"
#include "vtkTypeFloat32Array.h"
#include "vtkTypeInt32Array.h"

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkDGHex);

// TODO: Use IMPLEMENTABLE/IMPLEMENTS and vtkObjectFactory or equivalent.
static bool registerType = vtkCellMetadata::RegisterType<vtkDGHex>();

const std::array<std::array<double, 3>, 8> vtkDGHex::Parameters{ { { -1., -1., -1. },
  { +1., -1., -1. }, { +1., +1., -1. }, { -1., +1., -1. }, { -1., -1., +1. }, { +1., -1., +1. },
  { +1., +1., +1. }, { -1., +1., +1. } } };

const std::array<int, vtkDGHex::Dimension + 1> vtkDGHex::SideOffsets{ { 0, 6, 18, 26 } };

const std::array<vtkDGCell::Shape, vtkDGHex::Dimension + 1> vtkDGHex::SideShapes{
  { Shape::Quadrilateral, Shape::Edge, Shape::Vertex, Shape::Hexahedron }
};

const std::array<std::vector<vtkIdType>, 26> vtkDGHex::Sides{ {
  { 0, 4, 7, 3 }, // face 0
  { 1, 2, 6, 5 }, // face 1
  { 0, 1, 5, 4 }, // face 2
  { 3, 7, 6, 2 }, // face 3
  { 0, 3, 2, 1 }, // face 4
  { 4, 5, 6, 7 }, // face 5
  { 0, 1 },       // edge 0
  { 1, 2 },       // edge 1
  { 3, 2 },       // edge 2
  { 0, 3 },       // edge 3
  { 4, 5 },       // edge 4
  { 5, 6 },       // edge 5
  { 7, 6 },       // edge 6
  { 4, 7 },       // edge 7
  { 0, 4 },       // edge 8
  { 1, 5 },       // edge 9
  { 3, 7 },       // edge 10
  { 2, 6 },       // edge 11
  { 0 },          // vertex 0
  { 1 },          // vertex 1
  { 2 },          // vertex 2
  { 3 },          // vertex 3
  { 4 },          // vertex 4
  { 5 },          // vertex 5
  { 6 },          // vertex 6
  { 7 }           // vertex 7
} };

vtkDGHex::vtkDGHex() = default;
vtkDGHex::~vtkDGHex() = default;

void vtkDGHex::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkIdType vtkDGHex::GetNumberOfCells()
{
  auto* dsa = this->CellGrid->GetAttributes("DGHex"_token);
  return dsa ? dsa->GetNumberOfTuples() : 0;
}

const std::array<double, 3>& vtkDGHex::GetCornerParameter(int corner) const
{
  if (corner < 0 || corner >= 8)
  {
    static std::array<double, 3> dummy{ 0., 0., 0. }; // Maybe NaN would be better?
    return dummy;
  }
  return this->Parameters[corner];
}

int vtkDGHex::GetNumberOfSideTypes() const
{
  return static_cast<int>(vtkDGHex::SideOffsets.size() - 1);
}

std::pair<int, int> vtkDGHex::GetSideRangeForType(int sideType) const
{
  if (sideType < 0)
  {
    return std::make_pair(SideOffsets[0], SideOffsets[vtkDGHex::Dimension]);
  }
  if (sideType > vtkDGHex::Dimension)
  {
    return std::make_pair(-1, -1);
  }
  return std::make_pair(SideOffsets[sideType], SideOffsets[sideType + 1]);
}

int vtkDGHex::GetNumberOfSidesOfDimension(int dimension) const
{
  if (dimension < 0 || dimension >= this->Dimension)
  {
    return 0;
  }
  return this->SideOffsets[Dimension - dimension] - this->SideOffsets[Dimension - dimension - 1];
}

const std::vector<vtkIdType>& vtkDGHex::GetSideConnectivity(int side) const
{
  if (side < 0 || side >= 26)
  {
    static std::vector<vtkIdType> dummy;
    return dummy;
  }
  return this->Sides[side];
}

vtkTypeFloat32Array* vtkDGHex::GetReferencePoints() const
{
  static vtkNew<vtkTypeFloat32Array> refPts;
  if (refPts->GetNumberOfTuples() == 0)
  {
    this->FillReferencePoints(refPts);
    refPts->SetName("HexReferencePoints");
  }
  return refPts;
}

vtkTypeInt32Array* vtkDGHex::GetSideConnectivity() const
{
  static vtkNew<vtkTypeInt32Array> sideConn;
  if (sideConn->GetNumberOfTuples() == 0)
  {
    this->FillSideConnectivity(sideConn);
    sideConn->SetName("HexSideConn");
  }
  return sideConn;
}

vtkDGHex::Shape vtkDGHex::GetSideShape(int side) const
{
  if (side < 0)
  {
    return Hexahedron;
  }
  else if (side < 6)
  {
    return Quadrilateral;
  }
  else if (side < 18)
  {
    return Edge;
  }
  return Vertex;
}

vtkTypeInt32Array* vtkDGHex::GetSideOffsetsAndShapes() const
{
  static vtkNew<vtkTypeInt32Array> sideOffsetsAndShapes;
  if (sideOffsetsAndShapes->GetNumberOfTuples() == 0)
  {
    this->FillSideOffsetsAndShapes(sideOffsetsAndShapes);
    sideOffsetsAndShapes->SetName("HexOffsetsAndShapes");
  }
  return sideOffsetsAndShapes;
}

VTK_ABI_NAMESPACE_END
