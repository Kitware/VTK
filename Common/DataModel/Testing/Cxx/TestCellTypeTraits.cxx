// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests vtkCellTypeTraits against vtkCellTypeUtilities and the cells built by
// vtkGenericCell::InstantiateCell(): their class, dimension, linearity, number of points, edges
// and faces, the sizes of their edges and faces, and their corners.

#include "vtkBezierCurve.h"
#include "vtkBezierHexahedron.h"
#include "vtkBezierQuadrilateral.h"
#include "vtkBezierTetra.h"
#include "vtkBezierTriangle.h"
#include "vtkBezierWedge.h"
#include "vtkBiQuadraticQuad.h"
#include "vtkBiQuadraticQuadraticHexahedron.h"
#include "vtkBiQuadraticQuadraticWedge.h"
#include "vtkBiQuadraticTriangle.h"
#include "vtkCell.h"
#include "vtkCellTypeTraits.h"
#include "vtkCellTypeUtilities.h"
#include "vtkConvexPointSet.h"
#include "vtkCubicLine.h"
#include "vtkEmptyCell.h"
#include "vtkGenericCell.h"
#include "vtkHexagonalPrism.h"
#include "vtkHexahedron.h"
#include "vtkHigherOrderCurve.h"
#include "vtkHigherOrderHexahedron.h"
#include "vtkHigherOrderQuadrilateral.h"
#include "vtkHigherOrderTetra.h"
#include "vtkHigherOrderTriangle.h"
#include "vtkHigherOrderWedge.h"
#include "vtkIdList.h"
#include "vtkLagrangeCurve.h"
#include "vtkLagrangeHexahedron.h"
#include "vtkLagrangeQuadrilateral.h"
#include "vtkLagrangeTetra.h"
#include "vtkLagrangeTriangle.h"
#include "vtkLagrangeWedge.h"
#include "vtkLine.h"
#include "vtkPentagonalPrism.h"
#include "vtkPixel.h"
#include "vtkPolyLine.h"
#include "vtkPolyVertex.h"
#include "vtkPolygon.h"
#include "vtkPolyhedron.h"
#include "vtkPyramid.h"
#include "vtkQuad.h"
#include "vtkQuadraticEdge.h"
#include "vtkQuadraticHexahedron.h"
#include "vtkQuadraticLinearQuad.h"
#include "vtkQuadraticLinearWedge.h"
#include "vtkQuadraticPolygon.h"
#include "vtkQuadraticPyramid.h"
#include "vtkQuadraticQuad.h"
#include "vtkQuadraticTetra.h"
#include "vtkQuadraticTriangle.h"
#include "vtkQuadraticWedge.h"
#include "vtkSmartPointer.h"
#include "vtkStringFormatter.h"
#include "vtkTetra.h"
#include "vtkTriQuadraticHexahedron.h"
#include "vtkTriQuadraticPyramid.h"
#include "vtkTriangle.h"
#include "vtkTriangleStrip.h"
#include "vtkVertex.h"
#include "vtkVoxel.h"
#include "vtkWedge.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace
{
static_assert(vtkCellTypeTraits<VTK_HEXAHEDRON>::NumberOfFaces == 6);
static_assert(vtkCellTypeTraits<VTK_QUADRATIC_TETRA>::NumberOfCorners == 4);
static_assert(!vtkCellTypeTraits<VTK_POLYHEDRON>::HasFixedSize);

// The members of the class of the cell type, see vtkCellTypeTraitsMembersMacro.
template <class Traits>
void CheckClassMembers()
{
  using CellClass = typename Traits::CellClass;
  if constexpr (!std::is_void_v<CellClass>)
  {
    static_assert(std::is_same_v<typename CellClass::Traits, Traits>, "wrong traits of a class");
    static_assert(CellClass::CellType == Traits::CellType &&
        CellClass::Dimension == Traits::Dimension && CellClass::IsLinearCell == Traits::IsLinear &&
        CellClass::HasFixedSize == Traits::HasFixedSize &&
        CellClass::NumberOfPoints == Traits::NumberOfPoints &&
        CellClass::NumberOfCorners == Traits::NumberOfCorners &&
        CellClass::NumberOfEdges == Traits::NumberOfEdges &&
        CellClass::NumberOfFaces == Traits::NumberOfFaces &&
        CellClass::MinimumEdgeSize == Traits::MinimumEdgeSize &&
        CellClass::MaximumEdgeSize == Traits::MaximumEdgeSize &&
        CellClass::MinimumFaceSize == Traits::MinimumFaceSize &&
        CellClass::MaximumFaceSize == Traits::MaximumFaceSize &&
        CellClass::MaximumFaceCorners == Traits::MaximumFaceCorners &&
        CellClass::MinimumFaceCorners == Traits::MinimumFaceCorners &&
        CellClass::MaximumValence == Traits::MaximumValence,
      "wrong members of a class");
    static_assert(std::is_same_v<std::remove_cv_t<decltype(CellClass::NumberOfPoints)>, vtkIdType>,
      "the sizes of a class should be vtkIdType");
  }
}

void Check(bool condition, const std::string& message)
{
  if (!condition)
  {
    throw std::runtime_error(message);
  }
}

// The traits against vtkCellTypeUtilities.
template <class Traits>
void CheckUtilities(const std::string& name)
{
  const int type = Traits::CellType;
  Check(vtkCellTypeUtilities::GetDimension(static_cast<unsigned char>(type)) == Traits::Dimension,
    name + ": wrong GetDimension");
  Check((vtkCellTypeUtilities::IsLinear(static_cast<unsigned char>(type)) != 0) == Traits::IsLinear,
    name + ": wrong IsLinear");
  const char* className = vtkCellTypeUtilities::GetClassNameFromTypeId(type);
  Check(Traits::ClassName ? std::strcmp(className, Traits::ClassName) == 0
                          : std::strcmp(className, "UnknownClass") == 0,
    name + ": wrong GetClassNameFromTypeId");
  if constexpr (Traits::ClassName != nullptr)
  {
    Check(vtkCellTypeUtilities::GetTypeIdFromClassName(Traits::ClassName) == type,
      name + ": wrong GetTypeIdFromClassName");
  }
  const std::string typeName = vtkCellTypeUtilities::GetTypeAsString(type);
  Check(
    typeName == (Traits::Name ? Traits::Name : "Unknown Cell"), name + ": wrong GetTypeAsString");
  if constexpr (Traits::Name != nullptr)
  {
    Check(vtkCellTypeUtilities::GetTypeIdFromName(Traits::Name) == type,
      name + ": wrong GetTypeIdFromName");
  }
}

// The number of corners of the cells of every cell type with a fixed size, -1 otherwise.
constexpr std::array<int, VTK_NUMBER_OF_CELL_TYPES> MakeCorners()
{
  std::array<int, VTK_NUMBER_OF_CELL_TYPES> corners{};
  for (auto& c : corners)
  {
    c = -1;
  }
  vtkForEachCellType(
    [&corners](auto traits)
    {
      using Traits = decltype(traits);
      corners[Traits::CellType] = Traits::NumberOfCorners;
    });
  return corners;
}
constexpr std::array<int, VTK_NUMBER_OF_CELL_TYPES> Corners = MakeCorners();

// The number of corners of a side, its points if it is linear.
int GetSideCorners(vtkCell* side)
{
  return side->IsLinear() ? static_cast<int>(side->GetNumberOfPoints())
                          : Corners[side->GetCellType()];
}

// The traits against the cell built for the cell type, whose point ids are their indices, so that
// the points of its sides are local points.
template <class Traits>
void CheckCell(const std::string& name)
{
  auto cell = vtkSmartPointer<vtkCell>::Take(vtkGenericCell::InstantiateCell(Traits::CellType));
  if (!cell)
  {
    // The higher order cells, whose classes are abstract, and the pyramids that are not
    // implemented.
    return;
  }
  if constexpr (Traits::ClassName != nullptr)
  {
    Check(std::strcmp(cell->GetClassName(), Traits::ClassName) == 0, name + ": wrong class");
  }
  else
  {
    Check(false, name + ": a cell type without a class should not build a cell");
  }
  Check(cell->GetCellType() == Traits::CellType, name + ": wrong cell type");
  Check(cell->GetCellDimension() == Traits::Dimension, name + ": wrong dimension");
  Check((cell->IsLinear() != 0) == Traits::IsLinear, name + ": wrong linearity");
  if constexpr (Traits::NumberOfEdges >= 0)
  {
    Check(cell->GetNumberOfEdges() == Traits::NumberOfEdges, name + ": wrong number of edges");
  }
  if constexpr (Traits::NumberOfFaces >= 0)
  {
    Check(cell->GetNumberOfFaces() == Traits::NumberOfFaces, name + ": wrong number of faces");
  }
  if constexpr (Traits::HasFixedSize)
  {
    Check(cell->GetNumberOfPoints() == Traits::NumberOfPoints, name + ": wrong number of points");
    Check(Traits::NumberOfCorners <= Traits::NumberOfPoints &&
        (!Traits::IsLinear || Traits::NumberOfCorners == Traits::NumberOfPoints),
      name + ": wrong number of corners");
    for (vtkIdType i = 0; i < cell->GetNumberOfPoints(); ++i)
    {
      cell->GetPointIds()->SetId(i, i);
    }
    int minEdgeSize = Traits::NumberOfEdges > 0 ? VTK_INT_MAX : 0;
    int maxEdgeSize = 0;
    // The number of edges incident to every corner.
    std::array<int, 32> valences{};
    for (int e = 0; e < cell->GetNumberOfEdges(); ++e)
    {
      vtkCell* edge = cell->GetEdge(e);
      const auto numEdgePts = static_cast<int>(edge->GetNumberOfPoints());
      minEdgeSize = std::min(minEdgeSize, numEdgePts);
      maxEdgeSize = std::max(maxEdgeSize, numEdgePts);
      ++valences[edge->GetPointId(0)];
      ++valences[edge->GetPointId(1)];
      // The corners of an edge are its first points, which are corners of the cell.
      for (int k = 0; k < GetSideCorners(edge); ++k)
      {
        Check(edge->GetPointId(k) < Traits::NumberOfCorners,
          name + ": edge " + vtk::to_string(e) + " has a corner that is not a corner of the cell");
      }
    }
    Check(minEdgeSize == Traits::MinimumEdgeSize, name + ": wrong MinimumEdgeSize");
    Check(maxEdgeSize == Traits::MaximumEdgeSize, name + ": wrong MaximumEdgeSize");
    Check(*std::max_element(valences.begin(), valences.end()) == Traits::MaximumValence,
      name + ": wrong MaximumValence");
    int minFaceSize = Traits::NumberOfFaces > 0 ? VTK_INT_MAX : 0;
    int maxFaceSize = 0;
    int maxFaceCorners = 0;
    int minFaceCorners = Traits::NumberOfFaces > 0 ? VTK_INT_MAX : 0;
    for (int f = 0; f < cell->GetNumberOfFaces(); ++f)
    {
      vtkCell* face = cell->GetFace(f);
      const int faceCorners = GetSideCorners(face);
      minFaceSize = std::min(minFaceSize, static_cast<int>(face->GetNumberOfPoints()));
      maxFaceSize = std::max(maxFaceSize, static_cast<int>(face->GetNumberOfPoints()));
      maxFaceCorners = std::max(maxFaceCorners, faceCorners);
      minFaceCorners = std::min(minFaceCorners, faceCorners);
      for (int k = 0; k < faceCorners; ++k)
      {
        Check(face->GetPointId(k) < Traits::NumberOfCorners,
          name + ": face " + vtk::to_string(f) + " has a corner that is not a corner of the cell");
      }
    }
    Check(minFaceSize == Traits::MinimumFaceSize, name + ": wrong MinimumFaceSize");
    Check(maxFaceSize == Traits::MaximumFaceSize, name + ": wrong MaximumFaceSize");
    Check(maxFaceCorners == Traits::MaximumFaceCorners, name + ": wrong MaximumFaceCorners");
    Check(minFaceCorners == Traits::MinimumFaceCorners, name + ": wrong MinimumFaceCorners");
  }
  else
  {
    Check(Traits::NumberOfPoints == -1 && Traits::MinimumEdgeSize == -1 &&
        Traits::MaximumEdgeSize == -1 && Traits::MinimumFaceSize == -1 &&
        Traits::MaximumFaceSize == -1,
      name + ": a cell type without a fixed size should have no number of points");
  }
}
} // namespace

int TestCellTypeTraits(int, char*[])
{
  int numTypes = 0;
  try
  {
    vtkForEachCellType(
      [&numTypes](auto traits)
      {
        using Traits = decltype(traits);
        const std::string name = vtkCellTypeUtilities::GetTypeAsString(Traits::CellType) + " (" +
          vtk::to_string(Traits::CellType) + ")";
        CheckClassMembers<Traits>();
        CheckUtilities<Traits>(name);
        CheckCell<Traits>(name);
        ++numTypes;
      });
    // Every cell type of vtkCellType.h but the deprecated parametric ones.
    Check(numTypes == 57, "wrong number of cell types: " + vtk::to_string(numTypes));
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
