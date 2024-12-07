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

const std::array<std::array<double, 3>, 8> vtkDGHex::Parameters{ {
  { -1., -1., -1. }, // node 0
  { +1., -1., -1. }, // node 1
  { +1., +1., -1. }, // node 2
  { -1., +1., -1. }, // node 3
  { -1., -1., +1. }, // node 4
  { +1., -1., +1. }, // node 5
  { +1., +1., +1. }, // node 6
  { -1., +1., +1. }  // node 7
} };

const std::array<int, vtkDGHex::Dimension + 2> vtkDGHex::SideOffsets{ { 0, 1, 7, 19, 27 } };

const std::array<vtkDGCell::Shape, vtkDGHex::Dimension + 2> vtkDGHex::SideShapes{
  { Shape::Hexahedron, Shape::Quadrilateral, Shape::Edge, Shape::Vertex, Shape::None }
};

// WARNING: The order of sides **must** match the IOSS (Exodus) side order or side sets
//   from Exodus files will not be rendered properly. Note that this order **coincidentally**
//   matches the Intrepid face ordering for HDiv face-coefficients but does **not** match
//   the Intrepid edge ordering (the vertical +T edges are last for intrepid). Also, this side
//   ordering does **not** necessarily match VTK's face ordering because the side-array
//   passed by the IOSS reader is **not** translated into VTK's order.
const std::array<std::vector<vtkIdType>, 27> vtkDGHex::Sides{ {
  { 0, 1, 2, 3, 4, 5, 6, 7 }, // hexahedron itself
  { 0, 1, 5, 4 },             // face 2 (-S normal)
  { 1, 2, 6, 5 },             // face 1 (+R normal)
  { 3, 7, 6, 2 },             // face 3 (+S normal)
  { 0, 4, 7, 3 },             // face 0 (-R normal)
  { 0, 3, 2, 1 },             // face 4 (-T normal)
  { 4, 5, 6, 7 },             // face 5 (+T normal)
  { 0, 1 },                   // edge 0 (-S-T planes, +R dir) 7
  { 1, 2 },                   // edge 1 (+R-T planes, +S dir) 8
  { 3, 2 },                   // edge 2 (+S-T planes, +R dir) 9
  { 0, 3 },                   // edge 3 (-R-T planes, +S dir)10
  { 0, 4 },                   // edge 8 (-R-S planes, +T dir)11
  { 1, 5 },                   // edge 9 (+R-S planes, +T dir)12
  { 3, 7 },                   // edge 10 (+R+S planes, +T dir)13
  { 2, 6 },                   // edge 11 (-R+S planes, +T dir)14
  { 4, 5 },                   // edge 4 (-S+T planes, +R dir)15
  { 5, 6 },                   // edge 5 (+R+T planes, +S dir)16
  { 7, 6 },                   // edge 6 (+S+T planes, +R dir)17
  { 4, 7 },                   // edge 7 (-R+T planes, +S dir)18
  { 0 },                      // vertex 0 19
  { 1 },                      // vertex 1 20
  { 2 },                      // vertex 2 21
  { 3 },                      // vertex 3 22
  { 4 },                      // vertex 4 23
  { 5 },                      // vertex 5 24
  { 6 },                      // vertex 6 25
  { 7 }                       // vertex 7 26
} };

// This array of arrays takes a side ID (-1 for the element itself, 0
// for the first side, 1 for the second side, etc.).The resulting array
// is the list of indices into vtkDGHex::Sides that holds the connectivity
// of the side's sides.
// Note that vertices have no sides (i.e., their side arrays are empty).
//
// For example, given face 3 (+S normal, side #3 above, whose nodes
// are (3, 7, 6, 2)), we discover from SidesOfSides[3] that edges 13,
// 17, 14, and 9 are the sides of face 3.
// We can then look up edge 13 as vtkDGHex::SidesOfSides[13] to see
// its sides are side 22 (vertex 3) and 26 (vertex 7) *or* we can
// directly look up vtkDGHex::Sides[13] to obtain its endpoint nodes
// (vertices 3 and 7).
// Similarly, side 17 is bounded by sides 26 (vertex 7) and 25 (vertex 6).
const std::array<std::vector<vtkIdType>, 27> vtkDGHex::SidesOfSides{ { { 0, 1, 2, 3, 4, 5 },
  { 6, 11, 14, 10 }, { 7, 13, 15, 11 }, { 12, 16, 13, 8 }, { 10, 17, 12, 9 }, { 9, 8, 7, 6 },
  { 14, 15, 16, 17 }, { 18, 19 }, { 19, 20 }, { 21, 20 }, { 18, 21 }, { 18, 22 }, { 19, 23 },
  { 21, 25 }, { 20, 24 }, { 22, 23 }, { 23, 24 }, { 25, 24 }, { 22, 25 }, {}, {}, {}, {}, {}, {},
  {}, {} } };

vtkDGHex::vtkDGHex()
{
  this->CellSpec.SourceShape = this->GetShape();
}

vtkDGHex::~vtkDGHex() = default;

void vtkDGHex::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

bool vtkDGHex::IsInside(const vtkVector3d& rst, double tolerance)
{
  tolerance = std::abs(tolerance);
  double pb = 1 + tolerance;
  double nb = -1 - tolerance;
  return rst[0] >= nb && rst[0] <= pb && rst[1] >= nb && rst[1] <= pb && rst[2] >= nb &&
    rst[2] <= pb;
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
  return static_cast<int>(vtkDGHex::SideOffsets.size() - 2);
}

std::pair<int, int> vtkDGHex::GetSideRangeForType(int sideType) const
{
  if (sideType < -1)
  {
    return std::make_pair(SideOffsets[1] - 1, SideOffsets[vtkDGHex::Dimension + 1] - 1);
  }
  if (sideType > vtkDGHex::Dimension)
  {
    return std::make_pair(-1, -1);
  }
  return std::make_pair(SideOffsets[sideType + 1] - 1, SideOffsets[sideType + 2] - 1);
}

int vtkDGHex::GetNumberOfSidesOfDimension(int dimension) const
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

const std::vector<vtkIdType>& vtkDGHex::GetSideConnectivity(int side) const
{
  if (side < -1 || side >= 26)
  {
    static std::vector<vtkIdType> dummy;
    return dummy;
  }
  return this->Sides[side + 1];
}

const std::vector<vtkIdType>& vtkDGHex::GetSidesOfSide(int side) const
{
  if (side < -1 || side >= 26)
  {
    static std::vector<vtkIdType> dummy;
    return dummy;
  }
  return this->SidesOfSides[side + 1];
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
  if (side < -1)
  {
    return None;
  }
  for (std::size_t ii = 0; ii < SideOffsets.size() - 1; ++ii)
  {
    if (side + 1 < vtkDGHex::SideOffsets[ii + 1])
    {
      return vtkDGHex::SideShapes[ii];
    }
  }
  return None;
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
