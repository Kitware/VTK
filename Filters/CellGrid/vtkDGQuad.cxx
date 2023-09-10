// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGQuad.h"

#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkStringToken.h"
#include "vtkTypeFloat32Array.h"
#include "vtkTypeInt32Array.h"

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkDGQuad);

// TODO: Use IMPLEMENTABLE/IMPLEMENTS and vtkObjectFactory or equivalent.
static bool registerType = vtkCellMetadata::RegisterType<vtkDGQuad>();

const std::array<std::array<double, 3>, 4> vtkDGQuad::Parameters{ {
  { 0., 0., 0. }, // node 0
  { 1., 0., 0. }, // node 1
  { 1., 1., 0. }, // node 2
  { 0., 1., 0. }  // node 3
} };

const std::array<std::vector<vtkIdType>, 9> vtkDGQuad::Sides{ {
  { 0, 1, 2, 3 }, // quadrilateral itself
  { 0, 1 },       // edge 0
  { 1, 2 },       // edge 1
  { 3, 2 },       // edge 2
  { 0, 3 },       // edge 3
  { 0 },          // vertex 0
  { 1 },          // vertex 1
  { 2 },          // vertex 2
  { 3 }           // vertex 3
} };

const std::array<int, vtkDGQuad::Dimension + 2> vtkDGQuad::SideOffsets{ { 0, 1, 5, 9 } };

const std::array<vtkDGCell::Shape, vtkDGQuad::Dimension + 2> vtkDGQuad::SideShapes{
  { Shape::Quadrilateral, Shape::Edge, Shape::Vertex, Shape::None }
};

vtkDGQuad::vtkDGQuad() = default;
vtkDGQuad::~vtkDGQuad() = default;

void vtkDGQuad::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

bool vtkDGQuad::IsInside(const vtkVector3d& rst, double tolerance)
{
  tolerance = std::abs(tolerance);
  double pb = 1 + tolerance;
  double nb = -1 - tolerance;
  return rst[0] >= nb && rst[0] <= pb && rst[1] >= nb && rst[1] <= pb &&
    std::abs(rst[2]) < tolerance;
}

const std::array<double, 3>& vtkDGQuad::GetCornerParameter(int corner) const
{
  if (corner < 0 || corner >= 4)
  {
    static std::array<double, 3> dummy{ 0., 0., 0. }; // Maybe NaN would be better?
    return dummy;
  }
  return this->Parameters[corner];
}

int vtkDGQuad::GetNumberOfSideTypes() const
{
  return static_cast<int>(vtkDGQuad::SideOffsets.size() - 2);
}

std::pair<int, int> vtkDGQuad::GetSideRangeForType(int sideType) const
{
  if (sideType < -1)
  {
    return std::make_pair(SideOffsets[1] - 1, SideOffsets[vtkDGQuad::Dimension + 1] - 1);
  }
  if (sideType > vtkDGQuad::Dimension)
  {
    return std::make_pair(-1, -1);
  }
  return std::make_pair(SideOffsets[sideType + 1] - 1, SideOffsets[sideType + 2] - 1);
}

int vtkDGQuad::GetNumberOfSidesOfDimension(int dimension) const
{
  if (dimension < 0 || dimension >= this->Dimension)
  {
    return 0;
  }
  return this->SideOffsets[Dimension - dimension + 1] - this->SideOffsets[Dimension - dimension];
}

const std::vector<vtkIdType>& vtkDGQuad::GetSideConnectivity(int side) const
{
  if (side < -1 || side >= 8)
  {
    static std::vector<vtkIdType> dummy;
    return dummy;
  }
  return this->Sides[side + 1];
}

vtkTypeFloat32Array* vtkDGQuad::GetReferencePoints() const
{
  static vtkNew<vtkTypeFloat32Array> refPts;
  if (refPts->GetNumberOfTuples() == 0)
  {
    this->FillReferencePoints(refPts);
    refPts->SetName("QuadReferencePoints");
  }
  return refPts;
}

vtkTypeInt32Array* vtkDGQuad::GetSideConnectivity() const
{
  static vtkNew<vtkTypeInt32Array> sideConn;
  if (sideConn->GetNumberOfTuples() == 0)
  {
    this->FillSideConnectivity(sideConn);
    sideConn->SetName("QuadSideConn");
  }
  return sideConn;
}

vtkDGCell::Shape vtkDGQuad::GetSideShape(int side) const
{
  if (side < -1)
  {
    return None;
  }
  for (std::size_t ii = 0; ii < SideOffsets.size() - 1; ++ii)
  {
    if (side + 1 < vtkDGQuad::SideOffsets[ii + 1])
    {
      return vtkDGQuad::SideShapes[ii];
    }
  }
  return None;
}

vtkTypeInt32Array* vtkDGQuad::GetSideOffsetsAndShapes() const
{
  static vtkNew<vtkTypeInt32Array> sideOffsetsAndShapes;
  if (sideOffsetsAndShapes->GetNumberOfTuples() == 0)
  {
    this->FillSideOffsetsAndShapes(sideOffsetsAndShapes);
    sideOffsetsAndShapes->SetName("QuadOffsetsAndShapes");
  }
  return sideOffsetsAndShapes;
}

VTK_ABI_NAMESPACE_END
