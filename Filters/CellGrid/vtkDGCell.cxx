// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGCell.h"

#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkDataSetAttributes.h"
#include "vtkStringToken.h"
#include "vtkTypeFloat32Array.h"
#include "vtkTypeInt32Array.h"

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkDGCell::vtkDGCell()
{
  static bool registeredSideShapes = false;
  if (!registeredSideShapes)
  {
    registeredSideShapes = true;
    // Constructing these inserts the strings into the
    // vtkStringManager so they are available for printing
    // even though GetShapeName() computes the hash at
    // compile time (which cannot insert strings into the
    // manager).
    (void)vtkStringToken("vertex");
    (void)vtkStringToken("edge");
    (void)vtkStringToken("triangle");
    (void)vtkStringToken("quadrilateral");
    (void)vtkStringToken("tetrahedron");
    (void)vtkStringToken("hexahedron");
    (void)vtkStringToken("wedge");
    (void)vtkStringToken("pyramid");
    (void)vtkStringToken("unknown");
  }
}

vtkDGCell::~vtkDGCell() = default;

void vtkDGCell::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

int vtkDGCell::GetShapeCornerCount(Shape shape)
{
  switch (shape)
  {
    case Vertex:
      return 1;
    case Edge:
      return 2;
    case Triangle:
      return 3;
    case Quadrilateral:
      return 4;
    case Tetrahedron:
      return 4;
    case Hexahedron:
      return 8;
    case Wedge:
      return 6;
    case Pyramid:
      return 5;
    default:
      break;
  }
  return 0;
}

vtkStringToken vtkDGCell::GetShapeName(Shape shape)
{
  switch (shape)
  {
    case Vertex:
      return "vertex"_token;
    case Edge:
      return "edge"_token;
    case Triangle:
      return "triangle"_token;
    case Quadrilateral:
      return "quadrilateral"_token;
    case Tetrahedron:
      return "tetrahedron"_token;
    case Hexahedron:
      return "hexahedron"_token;
    case Wedge:
      return "wedge"_token;
    case Pyramid:
      return "pyramid"_token;
    default:
      break;
  }
  return "unknown"_token;
}

int vtkDGCell::GetShapeDimension(Shape shape)
{
  switch (shape)
  {
    case Vertex:
      return 0;

    case Edge:
      return 1;

    case Triangle:
    case Quadrilateral:
      return 2;

    case Tetrahedron:
    case Hexahedron:
    case Wedge:
    case Pyramid:
      return 3;

    default:
      break;
  }
  return -1;
}

void vtkDGCell::FillReferencePoints(vtkTypeFloat32Array* arr) const
{
  int nn = this->GetNumberOfCorners();
  arr->SetNumberOfComponents(3);
  arr->SetNumberOfTuples(nn);
  for (int ii = 0; ii < nn; ++ii)
  {
    const auto& dCoord = this->GetCornerParameter(ii);
    std::array<float, 3> fCoord{ { static_cast<float>(dCoord[0]), static_cast<float>(dCoord[1]),
      static_cast<float>(dCoord[2]) } };
    arr->SetTypedTuple(ii, fCoord.data());
  }
}

void vtkDGCell::FillSideConnectivity(vtkTypeInt32Array* arr) const
{
  arr->SetNumberOfComponents(1);
  int tt = this->GetNumberOfSideTypes();
  int vv = 0; // Number of values to hold all side connectivities.
  for (int ii = 0; ii < tt; ++ii)
  {
    auto range = this->GetSideRangeForType(ii);
    if (range.second <= range.first)
    {
      continue;
    } // Ignore empty ranges.

    int pointsPerSide = vtkDGCell::GetShapeCornerCount(this->GetSideShape(range.first));
    vv += pointsPerSide * (range.second - range.first);
  }
  arr->SetNumberOfTuples(vv);

  // Fill in the array.
  vv = 0; // Index of current value.
  for (int ii = 0; ii < tt; ++ii)
  {
    auto range = this->GetSideRangeForType(ii);

    for (int jj = range.first; jj < range.second; ++jj)
    {
      const auto& conn = this->GetSideConnectivity(jj);
      for (const auto& pointId : conn)
      {
        auto pointIdInt = static_cast<std::int32_t>(pointId);
        arr->SetTypedTuple(vv++, &pointIdInt);
      }
    }
  }
}

void vtkDGCell::FillSideOffsetsAndShapes(vtkTypeInt32Array* arr) const
{
  int tt = this->GetNumberOfSideTypes();
  arr->SetNumberOfComponents(2);
  arr->SetNumberOfTuples(tt + 1);

  std::array<std::int32_t, 2> tuple{ { 0, tt == 0 ? this->GetShape() : this->GetSideShape(0) } };
  for (int ii = 0; ii < tt; ++ii)
  {
    arr->SetTypedTuple(ii, tuple.data());

    auto range = this->GetSideRangeForType(ii);
    int pointsPerSide = vtkDGCell::GetShapeCornerCount(static_cast<Shape>(tuple[1]));
    tuple[0] += pointsPerSide * (range.second - range.first);
    tuple[1] = this->GetSideShape(range.second);
  }
  tuple[1] = this->GetShape(); // The final shape is the cell's shape.
  arr->SetTypedTuple(tt, tuple.data());
}

VTK_ABI_NAMESPACE_END
