// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGVert.h"

#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkStringToken.h"
#include "vtkTypeFloat32Array.h"
#include "vtkTypeInt32Array.h"

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkDGVert);

// TODO: Use IMPLEMENTABLE/IMPLEMENTS and vtkObjectFactory or equivalent.
static bool registerType = vtkCellMetadata::RegisterType<vtkDGVert>();

const std::array<std::array<double, 3>, 1> vtkDGVert::Parameters{ {
  { 0., 0., 0. }, // node 0
} };

const std::array<int, vtkDGVert::Dimension + 2> vtkDGVert::SideOffsets{ { 0, 1 } };

const std::array<vtkDGCell::Shape, vtkDGVert::Dimension + 2> vtkDGVert::SideShapes{ { Shape::Vertex,
  Shape::None } };

const std::array<std::vector<vtkIdType>, 1> vtkDGVert::Sides{ {
  { 0 } // vertex 0
} };

vtkDGVert::vtkDGVert() = default;
vtkDGVert::~vtkDGVert() = default;

void vtkDGVert::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

bool vtkDGVert::IsInside(const vtkVector3d& rst, double tolerance)
{
  tolerance = std::abs(tolerance);
  return std::abs(rst[0]) < tolerance && std::abs(rst[1]) < tolerance &&
    std::abs(rst[2]) < tolerance;
}

const std::array<double, 3>& vtkDGVert::GetCornerParameter(int corner) const
{
  if (corner < 0 || corner >= 3)
  {
    static std::array<double, 3> dummy{ 0., 0., 0. }; // Maybe NaN would be better?
    return dummy;
  }
  return this->Parameters[corner];
}

int vtkDGVert::GetNumberOfSideTypes() const
{
  return static_cast<int>(vtkDGVert::SideOffsets.size() - 2);
}

std::pair<int, int> vtkDGVert::GetSideRangeForType(int sideType) const
{
  if (sideType < -1)
  {
    // Return self as the "-1"-th side.
    return std::make_pair(SideOffsets[1] - 1, SideOffsets[vtkDGVert::Dimension + 1] - 1);
  }
  if (sideType > vtkDGVert::Dimension)
  {
    return std::make_pair(-1, -1);
  }
  return std::make_pair(SideOffsets[sideType + 1] - 1, SideOffsets[sideType + 2] - 1);
}

int vtkDGVert::GetNumberOfSidesOfDimension(int dimension) const
{
  (void)dimension;
  return 0;
}

const std::vector<vtkIdType>& vtkDGVert::GetSideConnectivity(int side) const
{
  if (side < -1 || side >= 0)
  {
    static std::vector<vtkIdType> dummy;
    return dummy;
  }
  return this->Sides[side + 1];
}

vtkTypeFloat32Array* vtkDGVert::GetReferencePoints() const
{
  static vtkNew<vtkTypeFloat32Array> refPts;
  if (refPts->GetNumberOfTuples() == 0)
  {
    this->FillReferencePoints(refPts);
    refPts->SetName("VertReferencePoints");
  }
  return refPts;
}

vtkTypeInt32Array* vtkDGVert::GetSideConnectivity() const
{
  static vtkNew<vtkTypeInt32Array> sideConn;
  if (sideConn->GetNumberOfTuples() == 0)
  {
    this->FillSideConnectivity(sideConn);
    sideConn->SetName("VertSideConn");
  }
  return sideConn;
}

vtkDGCell::Shape vtkDGVert::GetSideShape(int side) const
{
  if (side < -1)
  {
    return None;
  }
  for (std::size_t ii = 0; ii < SideOffsets.size() - 1; ++ii)
  {
    if (side + 1 < vtkDGVert::SideOffsets[ii + 1])
    {
      return vtkDGVert::SideShapes[ii];
    }
  }
  return None;
}

vtkTypeInt32Array* vtkDGVert::GetSideOffsetsAndShapes() const
{
  static vtkNew<vtkTypeInt32Array> sideOffsetsAndShapes;
  if (sideOffsetsAndShapes->GetNumberOfTuples() == 0)
  {
    this->FillSideOffsetsAndShapes(sideOffsetsAndShapes);
    sideOffsetsAndShapes->SetName("VertOffsetsAndShapes");
  }
  return sideOffsetsAndShapes;
}

VTK_ABI_NAMESPACE_END
