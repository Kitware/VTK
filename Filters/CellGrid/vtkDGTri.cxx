// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGTri.h"

#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkStringToken.h"
#include "vtkTypeFloat32Array.h"
#include "vtkTypeInt32Array.h"

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkDGTri);

// TODO: Use IMPLEMENTABLE/IMPLEMENTS and vtkObjectFactory or equivalent.
static bool registerType = vtkCellMetadata::RegisterType<vtkDGTri>();

// Note to Jacob, verify against vtk
const std::array<std::array<double, 3>, 3> vtkDGTri::Parameters{ {
  { 0., 0., 0. }, // node 0
  { 1., 0., 0. }, // node 1
  { 0., 1., 0. }, // node 2
} };

// Note to Jacob, Match to VTK
const std::array<int, vtkDGTri::Dimension + 2> vtkDGTri::SideOffsets{ { 0, 1, 4,
  7 } }; // Note: this is not start of side list

const std::array<vtkDGCell::Shape, vtkDGTri::Dimension + 2> vtkDGTri::SideShapes{ { Shape::Triangle,
  Shape::Edge, Shape::Vertex, Shape::None } };

const std::array<std::vector<vtkIdType>, 7> vtkDGTri::Sides{ {
  { 0, 1, 2 }, // triangle itself
  { 0, 1 },    // edge 0
  { 1, 2 },    // edge 1
  { 2, 0 },    // edge 2
  { 0 },       // vertex 0
  { 1 },       // vertex 1
  { 2 }        // vertex 2
} };

/// SidesOfSides is generated from Sides by TestCellGridSideInfo.
const std::array<std::vector<vtkIdType>, 7> vtkDGTri::SidesOfSides{ { { 0, 1, 2 }, { 3, 4 },
  { 4, 5 }, { 5, 3 }, {}, {}, {} } };

vtkDGTri::vtkDGTri()
{
  this->CellSpec.SourceShape = this->GetShape();
}

vtkDGTri::~vtkDGTri() = default;

void vtkDGTri::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

bool vtkDGTri::IsInside(const vtkVector3d& rst, double tolerance)
{
  tolerance = std::abs(tolerance);
  double u = 1.0 - rst[0] - rst[1];
  double pb = 1.0 + tolerance;
  double nb = -tolerance;
  return rst[0] >= nb && rst[0] <= pb && rst[1] >= nb && rst[1] <= pb && u >= nb && u <= pb &&
    std::abs(rst[2]) < tolerance;
}

const std::array<double, 3>& vtkDGTri::GetCornerParameter(int corner) const
{
  if (corner < 0 || corner >= 3)
  {
    static std::array<double, 3> dummy{ 0., 0., 0. }; // Maybe NaN would be better?
    return dummy;
  }
  return this->Parameters[corner];
}

int vtkDGTri::GetNumberOfSideTypes() const
{
  return static_cast<int>(vtkDGTri::SideOffsets.size() - 2);
}

std::pair<int, int> vtkDGTri::GetSideRangeForType(int sideType) const
{
  if (sideType < -1)
  {
    return std::make_pair(SideOffsets[1] - 1, SideOffsets[vtkDGTri::Dimension + 1] - 1);
  }
  if (sideType > vtkDGTri::Dimension)
  {
    return std::make_pair(-1, -1);
  }
  return std::make_pair(SideOffsets[sideType + 1] - 1, SideOffsets[sideType + 2] - 1);
}

int vtkDGTri::GetNumberOfSidesOfDimension(int dimension) const
{
  if (dimension < -1 || dimension >= this->Dimension)
  {
    return 0;
  }
  else if (dimension == -1)
  {
    return 1;
  }
  return this->SideOffsets[Dimension - dimension + 1] - this->SideOffsets[Dimension - dimension];
}

const std::vector<vtkIdType>& vtkDGTri::GetSideConnectivity(int side) const
{
  if (side < -1 || side >= 7)
  {
    static std::vector<vtkIdType> dummy;
    return dummy;
  }
  return this->Sides[side + 1];
}

const std::vector<vtkIdType>& vtkDGTri::GetSidesOfSide(int side) const
{
  if (side < -1 || side >= 7)
  {
    static std::vector<vtkIdType> dummy;
    return dummy;
  }
  return this->SidesOfSides[side + 1];
}

vtkTypeFloat32Array* vtkDGTri::GetReferencePoints() const
{
  static vtkNew<vtkTypeFloat32Array> refPts;
  if (refPts->GetNumberOfTuples() == 0)
  {
    this->FillReferencePoints(refPts);
    refPts->SetName("TriReferencePoints");
  }
  return refPts;
}

vtkTypeInt32Array* vtkDGTri::GetSideConnectivity() const
{
  static vtkNew<vtkTypeInt32Array> sideConn;
  if (sideConn->GetNumberOfTuples() == 0)
  {
    this->FillSideConnectivity(sideConn);
    sideConn->SetName("TriSideConn");
  }
  return sideConn;
}

vtkDGCell::Shape vtkDGTri::GetSideShape(int side) const
{
  if (side < -1)
  {
    return None;
  }
  for (std::size_t ii = 0; ii < SideOffsets.size() - 1; ++ii)
  {
    if (side + 1 < vtkDGTri::SideOffsets[ii + 1])
    {
      return vtkDGTri::SideShapes[ii];
    }
  }
  return None;
}

vtkTypeInt32Array* vtkDGTri::GetSideOffsetsAndShapes() const
{
  static vtkNew<vtkTypeInt32Array> sideOffsetsAndShapes;
  if (sideOffsetsAndShapes->GetNumberOfTuples() == 0)
  {
    this->FillSideOffsetsAndShapes(sideOffsetsAndShapes);
    sideOffsetsAndShapes->SetName("TriOffsetsAndShapes");
  }
  return sideOffsetsAndShapes;
}

VTK_ABI_NAMESPACE_END
