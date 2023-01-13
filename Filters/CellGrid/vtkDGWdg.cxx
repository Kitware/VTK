// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGWdg.h"

#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkStringToken.h"
#include "vtkTypeFloat32Array.h"
#include "vtkTypeInt32Array.h"

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkDGWdg);

// TODO: Use IMPLEMENTABLE/IMPLEMENTS and vtkObjectFactory or equivalent.
static bool registerType = vtkCellMetadata::RegisterType<vtkDGWdg>();

const std::array<std::array<double, 3>, 6> vtkDGWdg::Parameters{ {
  { 0., 0., -1. },  // node 0
  { +1., 0., -1. }, // node 1
  { 0., +1., -1. }, // node 2
  { 0., 0., +1. },  // node 3
  { +1., 0., +1. }, // node 4
  { 0., +1., +1. }  // node 5
} };

const std::array<int, vtkDGWdg::Dimension + 3> vtkDGWdg::SideOffsets{ { 0, 1, 4, 6, 15, 21 } };

const std::array<vtkDGCell::Shape, vtkDGWdg::Dimension + 3> vtkDGWdg::SideShapes{ { Shape::Wedge,
  Shape::Quadrilateral, Shape::Triangle, Shape::Edge, Shape::Vertex, Shape::None } };

// WARNING: The order of sides **must** match the IOSS (Exodus) side order or side sets
//   from Exodus files will not be rendered properly. Note that this order **coincidentally**
//   matches the Intrepid face ordering for HDiv face-coefficients but does **not** match
//   the Intrepid edge ordering (the vertical +T edges are last for intrepid). Also, this side
//   ordering does **not** necessarily match VTK's face ordering because the side-array
//   passed by the IOSS reader is **not** translated into VTK's order.
const std::array<std::vector<vtkIdType>, 21> vtkDGWdg::Sides{ {
  { 0, 1, 2, 3, 4, 5 }, // wedge itself
  { 0, 1, 4, 3 },       // face 0 (-S normal)
  { 1, 2, 5, 4 },       // face 1 (+RS normal)
  { 0, 3, 5, 2 },       // face 2 (-R normal)
  { 0, 2, 1 },          // face 4 (-T normal)
  { 3, 4, 5 },          // face 5 (+T normal)
  { 0, 1 },             // edge 0 (-S-T planes, +R dir)
  { 1, 2 },             // edge 1 (+RS-T planes, -R+S dir)
  { 0, 2 },             // edge 2 (-R-T planes, +S dir)
  { 0, 3 },             // edge 3 (-R-S planes, +T dir)
  { 1, 4 },             // edge 4 (+RS-S planes, +T dir)
  { 2, 5 },             // edge 5 (-R+RS planes, +T dir)
  { 3, 4 },             // edge 6 (-S+T planes, +R dir)
  { 4, 5 },             // edge 7 (+RS+T planes, -R+S dir)
  { 5, 3 },             // edge 8 (+R+T planes, +S dir)
  { 0 },                // vertex 0
  { 1 },                // vertex 1
  { 2 },                // vertex 2
  { 3 },                // vertex 3
  { 4 },                // vertex 4
  { 5 }                 // vertex 5
} };

vtkDGWdg::vtkDGWdg() = default;
vtkDGWdg::~vtkDGWdg() = default;

void vtkDGWdg::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

bool vtkDGWdg::IsInside(const vtkVector3d& rst, double tolerance)
{
  tolerance = std::abs(tolerance);
  double pb = 1 + tolerance;
  double nb = -1 - tolerance;
  double n0 = -tolerance;
  double rs = 1.0 - rst[0] - rst[1];
  return rst[0] >= n0 && rst[0] <= pb && rst[1] >= n0 && rst[1] <= pb && rs >= n0 && rs <= pb &&
    rst[2] >= nb && rst[2] <= pb;
}

const std::array<double, 3>& vtkDGWdg::GetCornerParameter(int corner) const
{
  if (corner < 0 || corner >= 6)
  {
    static std::array<double, 3> dummy{ 0., 0., 0. }; // Maybe NaN would be better?
    return dummy;
  }
  return this->Parameters[corner];
}

int vtkDGWdg::GetNumberOfSideTypes() const
{
  return static_cast<int>(vtkDGWdg::SideOffsets.size() - 2);
}

std::pair<int, int> vtkDGWdg::GetSideRangeForType(int sideType) const
{
  if (sideType < -1)
  {
    return std::make_pair(SideOffsets[1] - 1, SideOffsets[vtkDGWdg::Dimension + 2] - 1);
  }
  if (sideType > vtkDGWdg::Dimension)
  {
    return std::make_pair(-1, -1);
  }
  return std::make_pair(SideOffsets[sideType + 1] - 1, SideOffsets[sideType + 2] - 1);
}

int vtkDGWdg::GetNumberOfSidesOfDimension(int dimension) const
{
  if (dimension < 0 || dimension >= this->Dimension)
  {
    return 0;
  }
  return this->SideOffsets[Dimension - dimension + 1] - this->SideOffsets[Dimension - dimension];
}

const std::vector<vtkIdType>& vtkDGWdg::GetSideConnectivity(int side) const
{
  if (side < -1 || side >= 21)
  {
    static std::vector<vtkIdType> dummy;
    return dummy;
  }
  return this->Sides[side + 1];
}

vtkTypeFloat32Array* vtkDGWdg::GetReferencePoints() const
{
  static vtkNew<vtkTypeFloat32Array> refPts;
  if (refPts->GetNumberOfTuples() == 0)
  {
    this->FillReferencePoints(refPts);
    refPts->SetName("WdgReferencePoints");
  }
  return refPts;
}

vtkTypeInt32Array* vtkDGWdg::GetSideConnectivity() const
{
  static vtkNew<vtkTypeInt32Array> sideConn;
  if (sideConn->GetNumberOfTuples() == 0)
  {
    this->FillSideConnectivity(sideConn);
    sideConn->SetName("WdgSideConn");
  }
  return sideConn;
}

vtkDGWdg::Shape vtkDGWdg::GetSideShape(int side) const
{
  if (side < -1)
  {
    return None;
  }
  for (std::size_t ii = 0; ii < SideOffsets.size() - 1; ++ii)
  {
    if (side + 1 < vtkDGWdg::SideOffsets[ii + 1])
    {
      return vtkDGWdg::SideShapes[ii];
    }
  }
  return None;
}

vtkTypeInt32Array* vtkDGWdg::GetSideOffsetsAndShapes() const
{
  static vtkNew<vtkTypeInt32Array> sideOffsetsAndShapes;
  if (sideOffsetsAndShapes->GetNumberOfTuples() == 0)
  {
    this->FillSideOffsetsAndShapes(sideOffsetsAndShapes);
    sideOffsetsAndShapes->SetName("WdgOffsetsAndShapes");
  }
  return sideOffsetsAndShapes;
}

VTK_ABI_NAMESPACE_END
