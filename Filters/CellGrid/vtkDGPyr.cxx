// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGPyr.h"

#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkStringToken.h"
#include "vtkTypeFloat32Array.h"
#include "vtkTypeInt32Array.h"

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkDGPyr);

// TODO: Use IMPLEMENTABLE/IMPLEMENTS and vtkObjectFactory or equivalent.
static bool registerType = vtkCellMetadata::RegisterType<vtkDGPyr>();

const std::array<std::array<double, 3>, 5> vtkDGPyr::Parameters{ {
  { -1., -1., 0. }, // node 0
  { +1., -1., 0. }, // node 1
  { +1., +1., 0. }, // node 2
  { -1., +1., 0. }, // node 3
  { 0., 0., +1. }   // node 4
} };

const std::array<int, vtkDGPyr::Dimension + 3> vtkDGPyr::SideOffsets{ { 0, 1, 5, 6, 14, 19 } };

const std::array<vtkDGCell::Shape, vtkDGPyr::Dimension + 3> vtkDGPyr::SideShapes{ { Shape::Pyramid,
  Shape::Triangle, Shape::Quadrilateral, Shape::Edge, Shape::Vertex, Shape::None } };

const std::array<int, vtkDGPyr::Dimension + 1> vtkDGPyr::SidesOfDimension{ { 1, 5, 8, 5 } };

// WARNING: The order of sides **must** match the IOSS (Exodus) side order or side sets
//   from Exodus files will not be rendered properly. Note that this order **coincidentally**
//   matches the Intrepid face ordering for HDiv face-coefficients but does **not** match
//   the Intrepid edge ordering (the vertical +T edges are last for intrepid). Also, this side
//   ordering does **not** necessarily match VTK's face ordering because the side-array
//   passed by the IOSS reader is **not** translated into VTK's order.
const std::array<std::vector<vtkIdType>, 19> vtkDGPyr::Sides{ {
  { 0, 1, 2, 3, 4 }, // pyramid itself
  { 0, 1, 4 },       // face 0
  { 1, 2, 4 },       // face 1
  { 2, 3, 4 },       // face 2
  { 3, 0, 4 },       // face 3
  { 0, 3, 2, 1 },    // face 4 (-T normal)
  { 0, 1 },          // edge 0 (-S-T planes, +R dir)
  { 1, 2 },          // edge 1 (+R-T planes, +S dir)
  { 3, 2 },          // edge 2 (+S-T planes, +R dir)
  { 0, 3 },          // edge 3 (-R-T planes, +S dir)
  { 0, 4 },          // edge 4
  { 1, 4 },          // edge 5
  { 2, 4 },          // edge 6
  { 3, 4 },          // edge 7
  { 0 },             // vertex 0
  { 1 },             // vertex 1
  { 2 },             // vertex 2
  { 3 },             // vertex 3
  { 4 }              // vertex 4
} };

/// SidesOfSides is generated from Sides by TestCellGridSideInfo.
const std::array<std::vector<vtkIdType>, 19> vtkDGPyr::SidesOfSides{ { { 0, 1, 2, 3, 4 },
  { 5, 10, 9 }, { 6, 11, 10 }, { 7, 12, 11 }, { 8, 9, 12 }, { 8, 7, 6, 5 }, { 13, 14 }, { 14, 15 },
  { 16, 15 }, { 13, 16 }, { 13, 17 }, { 14, 17 }, { 15, 17 }, { 16, 17 }, {}, {}, {}, {}, {} } };

vtkDGPyr::vtkDGPyr()
{
  this->CellSpec.SourceShape = this->GetShape();
}

vtkDGPyr::~vtkDGPyr() = default;

void vtkDGPyr::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

bool vtkDGPyr::IsInside(const vtkVector3d& rst, double tolerance)
{
  tolerance = std::abs(tolerance);
  double pb = 1 + tolerance;
  double nb = -1 - tolerance;
  return rst[0] >= nb && rst[0] <= pb && rst[1] >= nb && rst[1] <= pb && rst[2] >= -tolerance &&
    rst[2] <= pb;
}

const std::array<double, 3>& vtkDGPyr::GetCornerParameter(int corner) const
{
  if (corner < 0 || corner >= 5)
  {
    static std::array<double, 3> dummy{ 0., 0., 0. }; // Maybe NaN would be better?
    return dummy;
  }
  return this->Parameters[corner];
}

int vtkDGPyr::GetNumberOfSideTypes() const
{
  return static_cast<int>(vtkDGPyr::SideOffsets.size() - 2);
}

std::pair<int, int> vtkDGPyr::GetSideRangeForType(int sideType) const
{
  if (sideType < -1)
  {
    return std::make_pair(SideOffsets[1] - 1, SideOffsets[vtkDGPyr::Dimension + 2] - 1);
  }
  if (sideType > vtkDGPyr::Dimension)
  {
    return std::make_pair(-1, -1);
  }
  return std::make_pair(SideOffsets[sideType + 1] - 1, SideOffsets[sideType + 2] - 1);
}

int vtkDGPyr::GetNumberOfSidesOfDimension(int dimension) const
{
  if (dimension < -1 || dimension >= this->Dimension)
  {
    return 0;
  }
  return this->SidesOfDimension[dimension + 1];
}

const std::vector<vtkIdType>& vtkDGPyr::GetSideConnectivity(int side) const
{
  if (side < -1 || side >= 19)
  {
    static std::vector<vtkIdType> dummy;
    return dummy;
  }
  return this->Sides[side + 1];
}

const std::vector<vtkIdType>& vtkDGPyr::GetSidesOfSide(int side) const
{
  if (side < -1 || side >= 19)
  {
    static std::vector<vtkIdType> dummy;
    return dummy;
  }
  return this->SidesOfSides[side + 1];
}

vtkTypeFloat32Array* vtkDGPyr::GetReferencePoints() const
{
  static vtkNew<vtkTypeFloat32Array> refPts;
  if (refPts->GetNumberOfTuples() == 0)
  {
    this->FillReferencePoints(refPts);
    refPts->SetName("PyrReferencePoints");
  }
  return refPts;
}

vtkTypeInt32Array* vtkDGPyr::GetSideConnectivity() const
{
  static vtkNew<vtkTypeInt32Array> sideConn;
  if (sideConn->GetNumberOfTuples() == 0)
  {
    this->FillSideConnectivity(sideConn);
    sideConn->SetName("PyrSideConn");
  }
  return sideConn;
}

vtkDGPyr::Shape vtkDGPyr::GetSideShape(int side) const
{
  if (side < -1)
  {
    return None;
  }
  for (std::size_t ii = 0; ii < SideOffsets.size() - 1; ++ii)
  {
    if (side + 1 < vtkDGPyr::SideOffsets[ii + 1])
    {
      return vtkDGPyr::SideShapes[ii];
    }
  }
  return None;
}

vtkTypeInt32Array* vtkDGPyr::GetSideOffsetsAndShapes() const
{
  static vtkNew<vtkTypeInt32Array> sideOffsetsAndShapes;
  if (sideOffsetsAndShapes->GetNumberOfTuples() == 0)
  {
    this->FillSideOffsetsAndShapes(sideOffsetsAndShapes);
    sideOffsetsAndShapes->SetName("PyrOffsetsAndShapes");
  }
  return sideOffsetsAndShapes;
}

VTK_ABI_NAMESPACE_END
