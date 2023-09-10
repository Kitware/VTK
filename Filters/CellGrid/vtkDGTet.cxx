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

const std::array<std::array<double, 3>, 4> vtkDGTet::Parameters{ {
  { 0., 0., 0. }, // node 0
  { 1., 0., 0. }, // node 1
  { 0., 1., 0. }, // node 2
  { 0., 0., 1. }  // node 3
} };

const std::array<int, vtkDGTet::Dimension + 2> vtkDGTet::SideOffsets{ { 0, 1, 5, 11, 15 } };

const std::array<vtkDGCell::Shape, vtkDGTet::Dimension + 2> vtkDGTet::SideShapes{
  { Shape::Tetrahedron, Shape::Triangle, Shape::Edge, Shape::Vertex, Shape::None }
};

const std::array<std::vector<vtkIdType>, 15> vtkDGTet::Sides{ {
  { 0, 1, 2, 3 }, // tetrahedron itself
  { 0, 1, 3 },    // face 0
  { 1, 2, 3 },    // face 1
  { 2, 0, 3 },    // face 2
  { 0, 2, 1 },    // face 3
  { 0, 1 },       // edge 0
  { 1, 2 },       // edge 1
  { 2, 0 },       // edge 2
  { 0, 3 },       // edge 3
  { 1, 3 },       // edge 4
  { 2, 3 },       // edge 5
  { 0 },          // vertex 0
  { 1 },          // vertex 1
  { 2 },          // vertex 2
  { 3 }           // vertex 3
} };

vtkDGTet::vtkDGTet() = default;
vtkDGTet::~vtkDGTet() = default;

void vtkDGTet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

bool vtkDGTet::IsInside(const vtkVector3d& rst, double tolerance)
{
  tolerance = std::abs(tolerance);
  double u = 1.0 - rst[0] - rst[1] - rst[2];
  double pb = 1.0 + tolerance;
  double nb = -tolerance;
  return rst[0] >= nb && rst[0] <= pb && rst[1] >= nb && rst[1] <= pb && rst[2] >= nb &&
    rst[2] <= pb && u >= nb && u <= pb;
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
  return static_cast<int>(vtkDGTet::SideOffsets.size() - 2);
}

std::pair<int, int> vtkDGTet::GetSideRangeForType(int sideType) const
{
  if (sideType < -1)
  {
    return std::make_pair(SideOffsets[1] - 1, SideOffsets[vtkDGTet::Dimension + 1] - 1);
  }
  if (sideType > vtkDGTet::Dimension)
  {
    return std::make_pair(-1, -1);
  }
  return std::make_pair(SideOffsets[sideType + 1] - 1, SideOffsets[sideType + 2] - 1);
}

int vtkDGTet::GetNumberOfSidesOfDimension(int dimension) const
{
  if (dimension < 0 || dimension >= this->Dimension)
  {
    return 0;
  }
  return this->SideOffsets[Dimension - dimension + 1] - this->SideOffsets[Dimension - dimension];
}

const std::vector<vtkIdType>& vtkDGTet::GetSideConnectivity(int side) const
{
  if (side < -1 || side >= 14)
  {
    static std::vector<vtkIdType> dummy;
    return dummy;
  }
  return this->Sides[side + 1];
}

vtkTypeFloat32Array* vtkDGTet::GetReferencePoints() const
{
  static vtkNew<vtkTypeFloat32Array> refPts;
  if (refPts->GetNumberOfTuples() == 0)
  {
    this->FillReferencePoints(refPts);
    refPts->SetName("TetReferencePoints");
  }
  return refPts;
}

vtkTypeInt32Array* vtkDGTet::GetSideConnectivity() const
{
  static vtkNew<vtkTypeInt32Array> sideConn;
  if (sideConn->GetNumberOfTuples() == 0)
  {
    this->FillSideConnectivity(sideConn);
    sideConn->SetName("TetSideConn");
  }
  return sideConn;
}

vtkDGCell::Shape vtkDGTet::GetSideShape(int side) const
{
  if (side < -1)
  {
    return None;
  }
  for (std::size_t ii = 0; ii < SideOffsets.size() - 1; ++ii)
  {
    if (side + 1 < vtkDGTet::SideOffsets[ii + 1])
    {
      return vtkDGTet::SideShapes[ii];
    }
  }
  return None;
}

vtkTypeInt32Array* vtkDGTet::GetSideOffsetsAndShapes() const
{
  static vtkNew<vtkTypeInt32Array> sideOffsetsAndShapes;
  if (sideOffsetsAndShapes->GetNumberOfTuples() == 0)
  {
    this->FillSideOffsetsAndShapes(sideOffsetsAndShapes);
    sideOffsetsAndShapes->SetName("TetOffsetsAndShapes");
  }
  return sideOffsetsAndShapes;
}

VTK_ABI_NAMESPACE_END
