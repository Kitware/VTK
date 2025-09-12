// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellTypeUtilities.h"

#include "vtkCellType.h"
#include "vtkGenericCell.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"

#include <cstring>
#include <map>

namespace
{
// This list should contain the cell class names in
// the same order as the enums in vtkCellType.h. Make sure
// this list is nullptr terminated.
const char* vtkCellTypesStrings[] = { "vtkEmptyCell", "vtkVertex", "vtkPolyVertex", "vtkLine",
  "vtkPolyLine", "vtkTriangle", "vtkTriangleStrip", "vtkPolygon", "vtkPixel", "vtkQuad", "vtkTetra",
  "vtkVoxel", "vtkHexahedron", "vtkWedge", "vtkPyramid", "vtkPentagonalPrism", "vtkHexagonalPrism",
  "UnknownClass", "UnknownClass", "UnknownClass", "UnknownClass", "vtkQuadraticEdge",
  "vtkQuadraticTriangle", "vtkQuadraticQuad", "vtkQuadraticTetra", "vtkQuadraticHexahedron",
  "vtkQuadraticWedge", "vtkQuadraticPyramid", "vtkBiQuadraticQuad", "vtkTriQuadraticHexahedron",
  "vtkQuadraticLinearQuad", "vtkQuadraticLinearWedge", "vtkBiQuadraticQuadraticWedge",
  "vtkBiQuadraticQuadraticHexahedron", "vtkBiQuadraticTriangle", "vtkCubicLine",
  "vtkQuadraticPolygon", "vtkTriQuadraticPyramid", "UnknownClass", "UnknownClass", "UnknownClass",
  "vtkConvexPointSet", "vtkPolyhedron", "UnknownClass", "UnknownClass", "UnknownClass",
  "UnknownClass", "UnknownClass", "UnknownClass", "UnknownClass", "UnknownClass",
  "vtkParametricCurve", "vtkParametricSurface", "vtkParametricTriSurface",
  "vtkParametricQuadSurface", "vtkParametricTetraRegion", "vtkParametricHexRegion", "UnknownClass",
  "UnknownClass", "UnknownClass", "vtkHigherOrderEdge", "vtkHigherOrderTriangle",
  "vtkHigherOrderQuad", "vtkHigherOrderPolygon", "vtkHigherOrderTetrahedron", "vtkHigherOrderWedge",
  "vtkHigherOrderPyramid", "vtkHigherOrderHexahedron", "vtkLagrangeCurve",
  "vtkLagrangeQuadrilateral", "vtkLagrangeTriangle", "vtkLagrangeTetra", "vtkLagrangeHexahedron",
  "vtkLagrangeWedge", "vtkLagrangePyramid", "vtkBezierCurve", "vtkBezierQuadrilateral",
  "vtkBezierTriangle", "vtkBezierTetra", "vtkBezierHexahedron", "vtkBezierWedge",
  "vtkBezierPyramid", nullptr };

std::map<int, std::string> CellTypesName = { { VTK_VERTEX, "Vertex" },
  { VTK_POLY_VERTEX, "Polyvertex" }, { VTK_LINE, "Line" }, { VTK_POLY_LINE, "Polyline" },
  { VTK_TRIANGLE, "Triangle" }, { VTK_TRIANGLE_STRIP, "Triangle Strip" },
  { VTK_POLYGON, "Polygon" }, { VTK_PIXEL, "Pixel" }, { VTK_QUAD, "Quadrilateral" },
  { VTK_TETRA, "Tetrahedron" }, { VTK_VOXEL, "Voxel" }, { VTK_HEXAHEDRON, "Hexahedron" },
  { VTK_WEDGE, "Wedge" }, { VTK_PYRAMID, "Pyramid" }, { VTK_PENTAGONAL_PRISM, "Pentagonal Prism" },
  { VTK_HEXAGONAL_PRISM, "Hexagonal Prism" }, { VTK_POLYHEDRON, "Polyhedron" },
  { VTK_QUADRATIC_EDGE, "Quadratic Edge" }, { VTK_QUADRATIC_TRIANGLE, "Quadratic Triangle" },
  { VTK_QUADRATIC_QUAD, "Quadratic Quadrilateral" }, { VTK_QUADRATIC_POLYGON, "Quadratic Polygon" },
  { VTK_QUADRATIC_TETRA, "Quadratic Tetrahedron" },
  { VTK_QUADRATIC_HEXAHEDRON, "Quadratic Hexahedron" }, { VTK_QUADRATIC_WEDGE, "Quadratic Wedge" },
  { VTK_QUADRATIC_PYRAMID, "Quadratic Pyramid" },
  { VTK_BIQUADRATIC_QUAD, "Bi-Quadratic Quadrilateral" },
  { VTK_TRIQUADRATIC_HEXAHEDRON, "Tri-Quadratic Hexahedron" },
  { VTK_TRIQUADRATIC_PYRAMID, "Tri-Quadratic Pyramid" },
  { VTK_QUADRATIC_LINEAR_QUAD, "Quadratic Linear Quadrilateral" },
  { VTK_QUADRATIC_LINEAR_WEDGE, "Quadratic Linear Wedge" },
  { VTK_BIQUADRATIC_QUADRATIC_WEDGE, "Bi-Quadratic Quadratic Wedge" },
  { VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON, "Bi-Quadratic Quadratic Hexahedron" },
  { VTK_BIQUADRATIC_TRIANGLE, "Bi-Quadratic Triangle" }, { VTK_CUBIC_LINE, "Cubic Line" },
  { VTK_LAGRANGE_CURVE, "Lagrange Curve" }, { VTK_LAGRANGE_TRIANGLE, "Lagrange Triangle" },
  { VTK_LAGRANGE_QUADRILATERAL, "Lagrange Quadrilateral" },
  { VTK_LAGRANGE_TETRAHEDRON, "Lagrange Tetrahedron" },
  { VTK_LAGRANGE_HEXAHEDRON, "Lagrange Hexahedron" }, { VTK_LAGRANGE_WEDGE, "Lagrange Wedge" },
  { VTK_LAGRANGE_PYRAMID, "Lagrange Pyramid" }, { VTK_BEZIER_CURVE, "Bezier Curve" },
  { VTK_BEZIER_TRIANGLE, "Bezier Triangle" }, { VTK_BEZIER_QUADRILATERAL, "Bezier Quadrilateral" },
  { VTK_BEZIER_TETRAHEDRON, "Bezier Tetrahedron" }, { VTK_BEZIER_HEXAHEDRON, "Bezier Hexahedron" },
  { VTK_BEZIER_WEDGE, "Bezier Wedge" }, { VTK_BEZIER_PYRAMID, "Bezier Pyramid" } };

} // anonymous namespace

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCellTypeUtilities);

//------------------------------------------------------------------------------
void vtkCellTypeUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
std::string vtkCellTypeUtilities::GetTypeAsString(int typeId)
{
  if (::CellTypesName.find(typeId) != ::CellTypesName.end())
  {
    return ::CellTypesName[typeId];
  }

  return "Unknown Cell";
}

//------------------------------------------------------------------------------
int vtkCellTypeUtilities::GetTypeIdFromName(const std::string& name)
{
  int id = VTK_EMPTY_CELL;
  for (const auto& type : ::CellTypesName)
  {
    if (type.second == name)
    {
      return type.first;
    }
  }

  return id;
}

//------------------------------------------------------------------------------
const char* vtkCellTypeUtilities::GetClassNameFromTypeId(int type)
{
  static int numClasses = 0;

  // find length of table
  if (numClasses == 0)
  {
    while (vtkCellTypesStrings[numClasses] != nullptr)
    {
      numClasses++;
    }
  }

  if (type < numClasses)
  {
    return vtkCellTypesStrings[type];
  }
  else
  {
    return "UnknownClass";
  }
}

//------------------------------------------------------------------------------
int vtkCellTypeUtilities::GetTypeIdFromClassName(const char* classname)
{
  if (!classname)
  {
    return -1;
  }

  for (int idx = 0; vtkCellTypesStrings[idx] != nullptr; idx++)
  {
    if (strcmp(vtkCellTypesStrings[idx], classname) == 0)
    {
      return idx;
    }
  }

  return -1;
}

//------------------------------------------------------------------------------
int vtkCellTypeUtilities::GetDimension(unsigned char type)
{
  // For the most common cell types, this is a fast call. If the cell type is
  // more exotic, then the cell must be grabbed and queried directly, which is
  // slow.
  switch (type)
  {
    case VTK_EMPTY_CELL:
    case VTK_VERTEX:
    case VTK_POLY_VERTEX:
      return 0;
    case VTK_LINE:
    case VTK_POLY_LINE:
    case VTK_QUADRATIC_EDGE:
    case VTK_CUBIC_LINE:
    case VTK_PARAMETRIC_CURVE:
    case VTK_HIGHER_ORDER_EDGE:
    case VTK_LAGRANGE_CURVE:
    case VTK_BEZIER_CURVE:
      return 1;
    case VTK_TRIANGLE:
    case VTK_QUAD:
    case VTK_PIXEL:
    case VTK_POLYGON:
    case VTK_TRIANGLE_STRIP:
    case VTK_QUADRATIC_TRIANGLE:
    case VTK_QUADRATIC_QUAD:
    case VTK_QUADRATIC_POLYGON:
    case VTK_QUADRATIC_LINEAR_QUAD:
    case VTK_BIQUADRATIC_QUAD:
    case VTK_BIQUADRATIC_TRIANGLE:
    case VTK_PARAMETRIC_SURFACE:
    case VTK_PARAMETRIC_TRI_SURFACE:
    case VTK_PARAMETRIC_QUAD_SURFACE:
    case VTK_HIGHER_ORDER_TRIANGLE:
    case VTK_HIGHER_ORDER_QUAD:
    case VTK_HIGHER_ORDER_POLYGON:
    case VTK_LAGRANGE_TRIANGLE:
    case VTK_LAGRANGE_QUADRILATERAL:
    case VTK_BEZIER_TRIANGLE:
    case VTK_BEZIER_QUADRILATERAL:
      return 2;
    case VTK_TETRA:
    case VTK_VOXEL:
    case VTK_HEXAHEDRON:
    case VTK_WEDGE:
    case VTK_PYRAMID:
    case VTK_PENTAGONAL_PRISM:
    case VTK_HEXAGONAL_PRISM:
    case VTK_QUADRATIC_TETRA:
    case VTK_QUADRATIC_HEXAHEDRON:
    case VTK_QUADRATIC_WEDGE:
    case VTK_QUADRATIC_PYRAMID:
    case VTK_QUADRATIC_LINEAR_WEDGE:
    case VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
    case VTK_BIQUADRATIC_QUADRATIC_WEDGE:
    case VTK_TRIQUADRATIC_HEXAHEDRON:
    case VTK_TRIQUADRATIC_PYRAMID:
    case VTK_CONVEX_POINT_SET:
    case VTK_POLYHEDRON:
    case VTK_PARAMETRIC_TETRA_REGION:
    case VTK_PARAMETRIC_HEX_REGION:
    case VTK_HIGHER_ORDER_TETRAHEDRON:
    case VTK_HIGHER_ORDER_WEDGE:
    case VTK_HIGHER_ORDER_PYRAMID:
    case VTK_HIGHER_ORDER_HEXAHEDRON:
    case VTK_LAGRANGE_TETRAHEDRON:
    case VTK_LAGRANGE_HEXAHEDRON:
    case VTK_LAGRANGE_WEDGE:
    case VTK_LAGRANGE_PYRAMID:
    case VTK_BEZIER_TETRAHEDRON:
    case VTK_BEZIER_HEXAHEDRON:
    case VTK_BEZIER_WEDGE:
    case VTK_BEZIER_PYRAMID:
      return 3;
    default:
    {
      if (type < VTK_NUMBER_OF_CELL_TYPES)
      {
        return 3;
      }
      vtkNew<vtkGenericCell> cell;
      cell->SetCellType(type);
      return cell->GetCellDimension();
    }
  }
}
VTK_ABI_NAMESPACE_END
