// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGTet.h"

#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkStringToken.h"
#include "vtkTypeFloat32Array.h"
#include "vtkTypeInt32Array.h"

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkDGTet);

// TODO: Use IMPLEMENTABLE/IMPLEMENTS and vtkObjectFactory or equivalent.
static bool registerType = vtkCellMetadata::RegisterType<vtkDGTet>();

const std::array<std::array<double, 3>, 4> vtkDGTet::Parameters{ { { 0., 0., 0. }, { +1., 0., 0. },
  { 0., +1., 0. }, { 0., 0., +1. } } };

const std::array<std::vector<vtkIdType>, 14> vtkDGTet::Sides{ {
  { 0, 1, 3 }, // face 0
  { 1, 2, 3 }, // face 1
  { 2, 0, 3 }, // face 2
  { 0, 2, 1 }, // face 3
  { 0, 1 },    // edge 0
  { 1, 2 },    // edge 1
  { 2, 0 },    // edge 2
  { 0, 3 },    // edge 3
  { 1, 3 },    // edge 4
  { 2, 3 },    // edge 5
  { 0 },       // vertex 0
  { 1 },       // vertex 1
  { 2 },       // vertex 2
  { 3 }        // vertex 3
} };

const std::array<int, vtkDGTet::Dimension + 1> vtkDGTet::SideOffsets{ { 0, 4, 10, 14 } };

const std::array<vtkDGCell::Shape, vtkDGTet::Dimension + 1> vtkDGTet::SideShapes{ { Shape::Triangle,
  Shape::Edge, Shape::Vertex, Shape::Tetrahedron } };

vtkDGTet::vtkDGTet() = default;
vtkDGTet::~vtkDGTet() = default;

void vtkDGTet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkIdType vtkDGTet::GetNumberOfCells()
{
  auto* dsa = this->CellGrid->GetAttributes("DGTet"_token);
  return dsa ? dsa->GetNumberOfTuples() : 0;
}

const std::array<double, 3>& vtkDGTet::GetCornerParameter(int corner) const
{
  if (corner < 0 || corner >= 4)
  {
    static std::array<double, 3> dummy{ 0., 0., 0. }; // Maybe NaN would be better?
    return dummy;
  }
  return this->Parameters[corner];
}

int vtkDGTet::GetNumberOfSideTypes() const
{
  return static_cast<int>(vtkDGTet::SideOffsets.size() - 1);
}

std::pair<int, int> vtkDGTet::GetSideRangeForType(int sideType) const
{
  if (sideType < 0)
  {
    return std::make_pair(SideOffsets[0], SideOffsets[vtkDGTet::Dimension]);
  }
  if (sideType > vtkDGTet::Dimension)
  {
    return std::make_pair(-1, -1);
  }
  return std::make_pair(SideOffsets[sideType], SideOffsets[sideType + 1]);
}

int vtkDGTet::GetNumberOfSidesOfDimension(int dimension) const
{
  switch (dimension)
  {
    case 2:
      return 4;
    case 1:
      return 6;
    case 0:
      return 4;
    default:
      break;
  }
  return 0;
}

const std::vector<vtkIdType>& vtkDGTet::GetSideConnectivity(int side) const
{
  if (side < 0 || side >= 14)
  {
    static std::vector<vtkIdType> dummy;
    return dummy;
  }
  return this->Sides[side];
}

vtkDGCell::Shape vtkDGTet::GetSideShape(int side) const
{
  if (side < 0)
  {
    return Shape::Tetrahedron;
  }
  else if (side < 4)
  {
    return Shape::Triangle;
  }
  else if (side < 10)
  {
    return Shape::Edge;
  }
  return Shape::Vertex;
}

vtkTypeFloat32Array* vtkDGTet::GetReferencePoints() const
{
  static vtkNew<vtkTypeFloat32Array> refPts;
  if (refPts->GetNumberOfTuples() == 0)
  {
    this->FillReferencePoints(refPts);
  }
  return refPts;
}

vtkTypeInt32Array* vtkDGTet::GetSideConnectivity() const
{
  static vtkNew<vtkTypeInt32Array> sideConn;
  if (sideConn->GetNumberOfTuples() == 0)
  {
    this->FillSideConnectivity(sideConn);
  }
  return sideConn;
}

vtkTypeInt32Array* vtkDGTet::GetSideOffsetsAndShapes() const
{
  static vtkNew<vtkTypeInt32Array> sideOffsetsAndShapes;
  if (sideOffsetsAndShapes->GetNumberOfTuples() == 0)
  {
    this->FillSideOffsetsAndShapes(sideOffsetsAndShapes);
  }
  return sideOffsetsAndShapes;
}

VTK_ABI_NAMESPACE_END
