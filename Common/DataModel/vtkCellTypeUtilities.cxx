// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellTypeUtilities.h"

#include "vtkCellType.h"
#include "vtkGenericCell.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"

#include <map>

namespace
{
std::map<int, std::string> CellTypesClasseName = { { VTK_EMPTY_CELL, "vtkEmptyCell" },
  { VTK_VERTEX, "vtkVertex" }, { VTK_POLY_VERTEX, "vtkPolyVertex" }, { VTK_LINE, "vtkLine" },
  { VTK_POLY_LINE, "vtkPolyLine" }, { VTK_TRIANGLE, "vtkTriangle" },
  { VTK_TRIANGLE_STRIP, "vtkTriangleStrip" }, { VTK_POLYGON, "vtkPolygon" },
  { VTK_PIXEL, "vtkPixel" }, { VTK_QUAD, "vtkQuad" }, { VTK_TETRA, "vtkTetra" },
  { VTK_VOXEL, "vtkVoxel" }, { VTK_HEXAHEDRON, "vtkHexahedron" }, { VTK_WEDGE, "vtkWedge" },
  { VTK_PYRAMID, "vtkPyramid" }, { VTK_PENTAGONAL_PRISM, "vtkPentagonalPrism" },
  { VTK_HEXAGONAL_PRISM, "vtkHexagonalPrism" }, { VTK_QUADRATIC_EDGE, "vtkQuadraticEdge" },
  { VTK_QUADRATIC_TRIANGLE, "vtkQuadraticTriangle" }, { VTK_QUADRATIC_QUAD, "vtkQuadraticQuad" },
  { VTK_QUADRATIC_TETRA, "vtkQuadraticTetra" },
  { VTK_QUADRATIC_HEXAHEDRON, "vtkQuadraticHexahedron" },
  { VTK_QUADRATIC_WEDGE, "vtkQuadraticWedge" }, { VTK_QUADRATIC_PYRAMID, "vtkQuadraticPyramid" },
  { VTK_BIQUADRATIC_QUAD, "vtkBiQuadraticQuad" },
  { VTK_TRIQUADRATIC_HEXAHEDRON, "vtkTriQuadraticHexahedron" },
  { VTK_QUADRATIC_LINEAR_QUAD, "vtkQuadraticLinearQuad" },
  { VTK_QUADRATIC_LINEAR_WEDGE, "vtkQuadraticLinearWedge" },
  { VTK_BIQUADRATIC_QUADRATIC_WEDGE, "vtkBiQuadraticQuadraticWedge" },
  { VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON, "vtkBiQuadraticQuadraticHexahedron" },
  { VTK_BIQUADRATIC_TRIANGLE, "vtkBiQuadraticTriangle" }, { VTK_CUBIC_LINE, "vtkCubicLine" },
  { VTK_QUADRATIC_POLYGON, "vtkQuadraticPolygon" },
  { VTK_TRIQUADRATIC_PYRAMID, "vtkTriQuadraticPyramid" },
  { VTK_CONVEX_POINT_SET, "vtkConvexPointSet" }, { VTK_POLYHEDRON, "vtkPolyhedron" },
  { VTK_PARAMETRIC_CURVE, "vtkParametricCurve" },
  { VTK_PARAMETRIC_SURFACE, "vtkParametricSurface" },
  { VTK_PARAMETRIC_TRI_SURFACE, "vtkParametricTriSurface" },
  { VTK_PARAMETRIC_QUAD_SURFACE, "vtkParametricQuadSurface" },
  { VTK_PARAMETRIC_TETRA_REGION, "vtkParametricTetraRegion" },
  { VTK_PARAMETRIC_HEX_REGION, "vtkParametricHexRegion" },
  { VTK_HIGHER_ORDER_EDGE, "vtkHigherOrderEdge" },
  { VTK_HIGHER_ORDER_TRIANGLE, "vtkHigherOrderTriangle" },
  { VTK_HIGHER_ORDER_QUAD, "vtkHigherOrderQuad" },
  { VTK_HIGHER_ORDER_POLYGON, "vtkHigherOrderPolygon" },
  { VTK_HIGHER_ORDER_TETRAHEDRON, "vtkHigherOrderTetrahedron" },
  { VTK_HIGHER_ORDER_WEDGE, "vtkHigherOrderWedge" },
  { VTK_HIGHER_ORDER_PYRAMID, "vtkHigherOrderPyramid" },
  { VTK_HIGHER_ORDER_HEXAHEDRON, "vtkHigherOrderHexahedron" },
  { VTK_LAGRANGE_CURVE, "vtkLagrangeCurve" },
  { VTK_LAGRANGE_QUADRILATERAL, "vtkLagrangeQuadrilateral" },
  { VTK_LAGRANGE_TRIANGLE, "vtkLagrangeTriangle" },
  { VTK_LAGRANGE_TETRAHEDRON, "vtkLagrangeTetra" },
  { VTK_LAGRANGE_HEXAHEDRON, "vtkLagrangeHexahedron" }, { VTK_LAGRANGE_WEDGE, "vtkLagrangeWedge" },
  { VTK_LAGRANGE_PYRAMID, "vtkLagrangePyramid" }, { VTK_BEZIER_CURVE, "vtkBezierCurve" },
  { VTK_BEZIER_QUADRILATERAL, "vtkBezierQuadrilateral" },
  { VTK_BEZIER_TRIANGLE, "vtkBezierTriangle" }, { VTK_BEZIER_TETRAHEDRON, "vtkBezierTetra" },
  { VTK_BEZIER_HEXAHEDRON, "vtkBezierHexahedron" }, { VTK_BEZIER_WEDGE, "vtkBezierWedge" },
  { VTK_BEZIER_PYRAMID, "vtkBezierPyramid" } };

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
  if (::CellTypesClasseName.find(type) != ::CellTypesClasseName.end())
  {
    return ::CellTypesClasseName[type].c_str();
  }

  return "UnknownClass";
}

//------------------------------------------------------------------------------
int vtkCellTypeUtilities::GetTypeIdFromClassName(const char* classname)
{
  if (!classname)
  {
    return -1;
  }

  for (const auto& classStr : ::CellTypesClasseName)
  {
    if (classStr.second == classname)
    {
      return classStr.first;
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

//------------------------------------------------------------------------------
int vtkCellTypeUtilities::IsLinear(unsigned char type)
{
  return (
    (type < VTK_HEXAGONAL_PRISM) || (type == VTK_CONVEX_POINT_SET) || (type == VTK_POLYHEDRON));
}

VTK_ABI_NAMESPACE_END
