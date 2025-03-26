// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGEdge.h"

#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkStringToken.h"
#include "vtkTypeFloat32Array.h"
#include "vtkTypeInt32Array.h"

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkDGEdge);

// TODO: Use IMPLEMENTABLE/IMPLEMENTS and vtkObjectFactory or equivalent.
static bool registerType = vtkCellMetadata::RegisterType<vtkDGEdge>();

const std::array<std::array<double, 3>, 2> vtkDGEdge::Parameters{ {
  { -1., 0., 0. }, // node 0
  { +1., 0., 0. }, // node 1
} };

const std::array<int, vtkDGEdge::Dimension + 2> vtkDGEdge::SideOffsets{ { 0, 1,
  3 } }; // Note: this is not start of side list

const std::array<vtkDGCell::Shape, vtkDGEdge::Dimension + 2> vtkDGEdge::SideShapes{ { Shape::Edge,
  Shape::Vertex, Shape::None } };

const std::array<std::vector<vtkIdType>, 3> vtkDGEdge::Sides{ {
  { 0, 1 }, // edge itself
  { 0 },    // vertex 0
  { 1 }     // vertex 1
} };

const std::array<std::vector<vtkIdType>, 3> vtkDGEdge::SidesOfSides{ {
  { 0, 1 }, // edge itself
  {},       // vertex 0
  {}        // vertex 1
} };

vtkDGEdge::vtkDGEdge()
{
  this->CellSpec.SourceShape = this->GetShape();
}

vtkDGEdge::~vtkDGEdge() = default;

void vtkDGEdge::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

bool vtkDGEdge::IsInside(const vtkVector3d& rst, double tolerance)
{
  tolerance = std::abs(tolerance);
  return rst[0] >= -1.0 - tolerance && rst[0] <= 1.0 + tolerance && std::abs(rst[1]) < tolerance &&
    std::abs(rst[2]) < tolerance;
}

const std::array<double, 3>& vtkDGEdge::GetCornerParameter(int corner) const
{
  if (corner < 0 || corner >= 2)
  {
    static std::array<double, 3> dummy{ 0., 0., 0. }; // Maybe NaN would be better?
    return dummy;
  }
  return this->Parameters[corner];
}

int vtkDGEdge::GetNumberOfSideTypes() const
{
  return static_cast<int>(vtkDGEdge::SideOffsets.size() - 2);
}

std::pair<int, int> vtkDGEdge::GetSideRangeForType(int sideType) const
{
  if (sideType < -1)
  {
    return std::make_pair(SideOffsets[1] - 1, SideOffsets[vtkDGEdge::Dimension + 1] - 1);
  }
  if (sideType > vtkDGEdge::Dimension)
  {
    return std::make_pair(-1, -1);
  }
  return std::make_pair(SideOffsets[sideType + 1] - 1, SideOffsets[sideType + 2] - 1);
}

int vtkDGEdge::GetNumberOfSidesOfDimension(int dimension) const
{
  if (dimension < 0 || dimension >= this->Dimension)
  {
    return 0;
  }
  return this->SideOffsets[Dimension - dimension + 1] - this->SideOffsets[Dimension - dimension];
}

const std::vector<vtkIdType>& vtkDGEdge::GetSideConnectivity(int side) const
{
  if (side < -1 || side >= 2)
  {
    static std::vector<vtkIdType> dummy;
    return dummy;
  }
  return this->Sides[side + 1];
}

const std::vector<vtkIdType>& vtkDGEdge::GetSidesOfSide(int side) const
{
  if (side < -1 || side >= 2)
  {
    static std::vector<vtkIdType> dummy;
    return dummy;
  }
  return this->SidesOfSides[side + 1];
}

vtkTypeFloat32Array* vtkDGEdge::GetReferencePoints() const
{
  static vtkNew<vtkTypeFloat32Array> refPts;
  if (refPts->GetNumberOfTuples() == 0)
  {
    this->FillReferencePoints(refPts);
    refPts->SetName("EdgeReferencePoints");
  }
  return refPts;
}

vtkTypeInt32Array* vtkDGEdge::GetSideConnectivity() const
{
  static vtkNew<vtkTypeInt32Array> sideConn;
  if (sideConn->GetNumberOfTuples() == 0)
  {
    this->FillSideConnectivity(sideConn);
    sideConn->SetName("EdgeSideConn");
  }
  return sideConn;
}

vtkDGCell::Shape vtkDGEdge::GetSideShape(int side) const
{
  if (side < -1)
  {
    return None;
  }
  for (std::size_t ii = 0; ii < SideOffsets.size() - 1; ++ii)
  {
    if (side + 1 < vtkDGEdge::SideOffsets[ii + 1])
    {
      return vtkDGEdge::SideShapes[ii];
    }
  }
  return None;
}

vtkTypeInt32Array* vtkDGEdge::GetSideOffsetsAndShapes() const
{
  static vtkNew<vtkTypeInt32Array> sideOffsetsAndShapes;
  if (sideOffsetsAndShapes->GetNumberOfTuples() == 0)
  {
    this->FillSideOffsetsAndShapes(sideOffsetsAndShapes);
    sideOffsetsAndShapes->SetName("EdgeOffsetsAndShapes");
  }
  return sideOffsetsAndShapes;
}

VTK_ABI_NAMESPACE_END
