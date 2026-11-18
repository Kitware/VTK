// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Exercise vtkCellGridIsocontour against the example .dg datasets, one shape
// per file, over every scalar cell-attribute each carries, at several
// isovalues. The attributes are passed as they are; the filter converts them
// to Bezier itself.
//
// Every output must be a cell-grid of triangles (vtkDGTri, from volumetric
// cells), lines (vtkDGEdge, from surfaces) and vertices (vtkDGVert, from curves)
// whose connectivity refers to existing, distinct points.
// Each point must carry the cell and parametric coordinates it was found at,
// and every attribute of the input evaluated there; the contoured attribute
// must stay within the range of its Bezier coefficients.
// The number of output points and cells for each attribute at each isovalue is
// checked against values recorded from the filter itself - a regression
// baseline, not a hand-derived one.
//
// If the environment variable VTK_CELLGRID_ISOCONTOUR_OUTPUT_DIR names a
// directory, each output is written to it (with vtkCellGridWriter, as
// <file>_<attribute>_<isovalue fraction>.dg) and read back (with
// vtkCellGridReader), checking that the counts survive the round trip.

#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridChangeBasis.h"
#include "vtkCellGridIsocontour.h"
#include "vtkCellGridReader.h"
#include "vtkCellGridToUnstructuredGrid.h"
#include "vtkCellGridWriter.h"
#include "vtkContourFilter.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkFiltersCellGrid.h"
#include "vtkIdList.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStringToken.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
using namespace vtk::literals;

/// How many points and cells (of every type) an isosurface has.
struct IsocontourCounts
{
  vtkIdType Points{ 0 };
  vtkIdType Cells{ 0 };

  bool operator==(const IsocontourCounts& other) const
  {
    return this->Points == other.Points && this->Cells == other.Cells;
  }
};

std::ostream& operator<<(std::ostream& os, const IsocontourCounts& counts)
{
  os << counts.Points << " points in " << counts.Cells << " cells";
  return os;
}

/// The isovalues each attribute is contoured at, as fractions of the range of
/// its Bezier coefficients.
constexpr std::array<double, 3> Fractions{ { 0.25, 0.5, 0.75 } };

/// Return the range of \a attribute's Bezier coefficients (converting it to
/// Bezier, at its own order, if it is not already), which bounds its values.
bool CoefficientRange(vtkCellGrid* grid, vtkCellAttribute* attribute, double range[2])
{
  vtkSmartPointer<vtkCellGrid> bezierGrid = grid;
  vtkCellAttribute* bezierAttribute = attribute;
  for (const auto& cellTypeName : grid->CellTypeArray())
  {
    auto info = attribute->GetCellTypeInfo(cellTypeName);
    if (!info.FunctionSpace.IsValid())
    {
      continue;
    }
    if (info.FunctionSpace != "Bezier"_token)
    {
      vtkNew<vtkCellGridChangeBasis> converter;
      converter->SetInputDataObject(grid);
      converter->SetCellAttribute(attribute);
      converter->SetSourceFunctionSpace(info.FunctionSpace.Data().c_str());
      converter->SetSourceBasis(info.Basis.Data().c_str());
      converter->SetSourceOrder(info.Order);
      converter->SetTargetFunctionSpace("Bezier");
      converter->SetTargetBasis("A");
      converter->SetTargetOrder(info.Order);
      converter->Update();
      bezierGrid = vtkCellGrid::SafeDownCast(converter->GetOutputDataObject(0));
      bezierAttribute =
        bezierGrid ? bezierGrid->GetCellAttributeByName(attribute->GetName().Data()) : nullptr;
    }
    break;
  }
  if (!bezierAttribute)
  {
    return false;
  }

  range[0] = 1e300;
  range[1] = -1e300;
  bool found = false;
  for (const auto& cellTypeName : bezierGrid->CellTypeArray())
  {
    auto* values = bezierAttribute->GetCellTypeInfo(cellTypeName)
                     .GetArrayForRoleAs<vtkDataArray>("values"_token);
    if (!values)
    {
      continue;
    }
    // A cell's coefficients may all sit in one tuple (one component each), so
    // take the range over every component.
    for (int component = 0; component < values->GetNumberOfComponents(); ++component)
    {
      double r[2];
      values->GetRange(r, component);
      range[0] = std::min(range[0], r[0]);
      range[1] = std::max(range[1], r[1]);
    }
    found = true;
  }
  return found;
}

/// The cell types an isocontour has, and how many points each cell has.
constexpr std::array<std::pair<const char*, int>, 3> OutputCellTypes{ { { "vtkDGTri", 3 },
  { "vtkDGEdge", 2 }, { "vtkDGVert", 1 } } };

/// Count \a output's points and cells, and verify that every cell refers to
/// existing, distinct points.
bool CheckOutput(vtkCellGrid* output, IsocontourCounts& counts)
{
  auto* points = output->GetAttributes("points"_token);
  auto* coords = points ? points->GetScalars() : nullptr;
  counts.Points = coords ? coords->GetNumberOfTuples() : 0;
  counts.Cells = 0;
  bool ok = true;
  for (const auto& cellType : OutputCellTypes)
  {
    auto* cells = output->GetAttributes(vtkStringToken(cellType.first));
    auto* connectivity = cells ? cells->GetScalars() : nullptr;
    if (!connectivity)
    {
      continue;
    }
    const int numCorners = cellType.second;
    if (connectivity->GetNumberOfComponents() != numCorners)
    {
      std::cerr << "    ERROR: " << cellType.first << " cells do not have " << numCorners
                << " points.\n";
      return false;
    }
    const vtkIdType numCells = connectivity->GetNumberOfTuples();
    counts.Cells += numCells;
    for (vtkIdType cell = 0; cell < numCells; ++cell)
    {
      std::array<vtkIdType, 3> ids{ { -1, -1, -1 } };
      for (int corner = 0; corner < numCorners; ++corner)
      {
        ids[corner] = static_cast<vtkIdType>(connectivity->GetComponent(cell, corner));
        if (ids[corner] < 0 || ids[corner] >= counts.Points)
        {
          std::cerr << "    ERROR: " << cellType.first << " " << cell << " refers to point "
                    << ids[corner] << " of " << counts.Points << ".\n";
          return false;
        }
        for (int other = 0; other < corner; ++other)
        {
          if (ids[other] == ids[corner])
          {
            std::cerr << "    ERROR: " << cellType.first << " " << cell << " is degenerate.\n";
            ok = false;
          }
        }
      }
    }
  }
  return ok;
}

/// Verify the attributes \a output carries: each point's source cell type,
/// cell ID and parametric coordinates, and every non-shape attribute of
/// \a input, evaluated there. The points are on the isosurface of the linear
/// cells, not of the input's curved ones, and a point shared by cells where the
/// input is discontinuous takes the value of only one of them, so the contoured
/// attribute, \a attributeName, is not the isovalue at every point. It must lie
/// within \a range (within \a tolerance), which bounds the attribute, or be NaN
/// where no cell contains the point.
bool CheckAttributes(vtkCellGrid* input, vtkCellGrid* output, const std::string& attributeName,
  const double range[2], double tolerance)
{
  auto* points = output->GetAttributes("points"_token);
  const vtkIdType numPoints = points->GetScalars()->GetNumberOfTuples();
  for (const char* name : { "CellType", "CellId", "RST" })
  {
    auto* array = points->GetArray(name);
    if (!array || array->GetNumberOfTuples() != numPoints)
    {
      std::cerr << "    ERROR: missing or mis-sized \"" << name << "\" point array.\n";
      return false;
    }
  }
  for (const auto& inputAttribute : input->GetCellAttributeList())
  {
    if (inputAttribute == input->GetShapeAttribute())
    {
      continue;
    }
    const std::string name = inputAttribute->GetName().Data();
    auto* outputAttribute = output->GetCellAttributeByName(name);
    // The attribute is defined on every type of cell the output has, with the
    // same values at the points.
    vtkDataArray* values = nullptr;
    for (const char* cellType : { "vtkDGTri", "vtkDGEdge", "vtkDGVert" })
    {
      if (outputAttribute && !values)
      {
        values = outputAttribute->GetCellTypeInfo(vtkStringToken(cellType))
                   .GetArrayForRoleAs<vtkDataArray>("values"_token);
      }
    }
    if (!values || values->GetNumberOfTuples() != numPoints ||
      values->GetNumberOfComponents() != inputAttribute->GetNumberOfComponents())
    {
      std::cerr << "    ERROR: the output has no matching \"" << name << "\" attribute.\n";
      return false;
    }
    if (name != attributeName)
    {
      continue;
    }
    for (vtkIdType ii = 0; ii < numPoints; ++ii)
    {
      const double value = values->GetTuple1(ii);
      if (!std::isnan(value) && (value < range[0] - tolerance || value > range[1] + tolerance))
      {
        std::cerr << "    ERROR: \"" << name << "\" is " << value << " at point " << ii
                  << ", outside its range [" << range[0] << ", " << range[1] << "].\n";
        return false;
      }
    }
  }
  return true;
}

/// Write \a output to \a fileName, read it back and check its counts match.
bool RoundTrip(vtkCellGrid* output, const std::string& fileName, const IsocontourCounts& counts)
{
  vtkNew<vtkCellGridWriter> writer;
  writer->SetFileName(fileName.c_str());
  writer->SetInputDataObject(output);
  if (!writer->Write())
  {
    std::cerr << "    ERROR: could not write \"" << fileName << "\".\n";
    return false;
  }
  vtkNew<vtkCellGridReader> reader;
  reader->SetFileName(fileName.c_str());
  reader->Update();
  auto* readBack = vtkCellGrid::SafeDownCast(reader->GetOutputDataObject(0));
  IsocontourCounts readCounts;
  if (!readBack || !CheckOutput(readBack, readCounts) || !(readCounts == counts))
  {
    std::cerr << "    ERROR: \"" << fileName << "\" does not read back as written.\n";
    return false;
  }
  return true;
}

/// Contour every scalar cell-attribute of \a grid (read from \a fileName) at
/// each of Fractions, checking their counts against \a expected (keyed by
/// attribute name, one entry per fraction).
bool TestGrid(vtkCellGrid* grid, const std::string& fileName,
  const std::map<std::string, std::array<IsocontourCounts, 3>>& expected)
{
  std::cout << fileName << ":\n";
  if (!grid)
  {
    std::cerr << "  ERROR: could not read \"" << fileName << "\".\n";
    return false;
  }

  const char* outputDir = std::getenv("VTK_CELLGRID_ISOCONTOUR_OUTPUT_DIR");
  const std::string baseName = fileName.substr(fileName.rfind('/') + 1);

  bool ok = true;
  auto* shapeAttribute = grid->GetShapeAttribute();
  vtkNew<vtkCellGridIsocontour> contour;
  contour->SetInputDataObject(grid);
  for (const auto& attribute : grid->GetCellAttributeList())
  {
    if (attribute.GetPointer() == shapeAttribute || attribute->GetNumberOfComponents() != 1)
    {
      continue;
    }
    const std::string attributeName = attribute->GetName().Data();
    auto it = expected.find(attributeName);
    if (it == expected.end())
    {
      std::cerr << "  ERROR: no expected counts recorded for \"" << attributeName << "\".\n";
      ok = false;
    }

    double range[2];
    if (!CoefficientRange(grid, attribute, range))
    {
      std::cerr << "  ERROR: no coefficients for \"" << attributeName << "\".\n";
      ok = false;
      continue;
    }
    for (std::size_t ff = 0; ff < Fractions.size(); ++ff)
    {
      contour->SetInputAttributeToProcess(0, 0, 0, attributeName.c_str());
      const double isovalue = range[0] + Fractions[ff] * (range[1] - range[0]);
      contour->SetIsovalue(isovalue);
      contour->Update();
      auto* output = vtkCellGrid::SafeDownCast(contour->GetOutputDataObject(0));

      IsocontourCounts counts;
      ok &= output && CheckOutput(output, counts);
      if (output && counts.Points > 0)
      {
        const double scale = std::max({ 1., std::abs(range[0]), std::abs(range[1]) });
        ok &= CheckAttributes(grid, output, attributeName, range, 1e-6 * scale);
      }
      std::cout << "  " << attributeName << " @ " << Fractions[ff] << ": " << counts << "\n";
      if (it != expected.end() && !(counts == it->second[ff]))
      {
        std::cerr << "    ERROR: expected " << it->second[ff] << ", got " << counts << ".\n";
        ok = false;
      }
      if (outputDir && output)
      {
        std::ostringstream name;
        name << outputDir << "/" << baseName << "_" << attributeName << "_" << Fractions[ff]
             << ".dg";
        ok &= RoundTrip(output, name.str(), counts);
      }
    }
  }
  return ok;
}

/// Read \a fileName from VTK's test data and test it (see TestGrid()).
bool TestIsocontour(int argc, char* argv[], const char* fileName,
  const std::map<std::string, std::array<IsocontourCounts, 3>>& expected)
{
  char* filename = vtkTestUtilities::ExpandDataFileName(argc, argv, fileName, 0);
  vtkNew<vtkCellGridReader> reader;
  reader->SetFileName(filename);
  reader->Update();
  delete[] filename;
  return TestGrid(vtkCellGrid::SafeDownCast(reader->GetOutputDataObject(0)), fileName, expected);
}

/// The triangles of a surface, as pairs of the positions of their corners (rounded
/// to \a resolution) so that points that coincide are the same.
using Position = std::array<long long, 3>;
using Triangles = std::vector<std::array<Position, 3>>;

Position RoundedPosition(const double xyz[3], double resolution)
{
  return { { std::llround(xyz[0] / resolution), std::llround(xyz[1] / resolution),
    std::llround(xyz[2] / resolution) } };
}

/// Count the edges that only one of \a triangles uses (the surface is open
/// there) and the edges that more than two use, ignoring degenerate triangles.
void CountOpenEdges(const Triangles& triangles, vtkIdType& numTriangles, vtkIdType& numOpenEdges)
{
  std::map<std::pair<Position, Position>, int> uses;
  numTriangles = 0;
  for (const auto& triangle : triangles)
  {
    if (triangle[0] == triangle[1] || triangle[1] == triangle[2] || triangle[0] == triangle[2])
    {
      continue;
    }
    ++numTriangles;
    for (int corner = 0; corner < 3; ++corner)
    {
      Position a = triangle[corner];
      Position b = triangle[(corner + 1) % 3];
      ++uses[a < b ? std::make_pair(a, b) : std::make_pair(b, a)];
    }
  }
  numOpenEdges = 0;
  for (const auto& use : uses)
  {
    numOpenEdges += use.second == 1 ? 1 : 0;
  }
}

/// Check that the isosurface of the continuous \a attributeName of \a grid, at
/// each of \a fractions of its range \a range, is as closed as the contour of
/// the linear cells that the whole grid is transcribed into with its points
/// merged: neighboring cells find the same points on the face they share, and
/// the filter merges them, so the surface must not have more open edges. (Both
/// are subdivided \a level times; the fractions avoid isovalues that a field
/// takes at a sample, where the surfaces differ in degenerate triangles.)
bool CheckClosedLikeMergedContour(vtkCellGrid* grid, const char* attributeName,
  const double range[2], const std::vector<double>& fractions, int level)
{
  vtkNew<vtkCellGridToUnstructuredGrid> transcriber;
  transcriber->SetInputDataObject(grid);
  transcriber->SetSubdivisionLevel(level);
  transcriber->Update();
  vtkUnstructuredGrid* mesh = transcriber->GetOutput();
  double bounds[6];
  mesh->GetBounds(bounds);
  const double diagonal = std::sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
    (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
    (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));
  const double resolution = 1e-7 * diagonal;

  bool ok = true;
  vtkNew<vtkCellGridIsocontour> filter;
  filter->SetInputDataObject(grid);
  filter->SetInputAttributeToProcess(0, 0, 0, attributeName);
  filter->SetSubdivisionLevel(level);
  for (double fraction : fractions)
  {
    const double isovalue = range[0] + fraction * (range[1] - range[0]);
    filter->SetIsovalue(isovalue);
    filter->Update();
    auto* output = vtkCellGrid::SafeDownCast(filter->GetOutputDataObject(0));
    Triangles filterTriangles;
    auto* trianglePoints = output->GetAttributes("points"_token);
    auto* connectivity = output->GetAttributes("vtkDGTri"_token);
    if (connectivity && connectivity->GetScalars() && trianglePoints)
    {
      auto* coordinates = trianglePoints->GetScalars();
      auto* triangles = connectivity->GetScalars();
      for (vtkIdType tri = 0; tri < triangles->GetNumberOfTuples(); ++tri)
      {
        std::array<Position, 3> corners;
        for (int corner = 0; corner < 3; ++corner)
        {
          double xyz[3];
          coordinates->GetTuple(static_cast<vtkIdType>(triangles->GetComponent(tri, corner)), xyz);
          corners[corner] = RoundedPosition(xyz, resolution);
        }
        filterTriangles.push_back(corners);
      }
    }

    vtkNew<vtkContourFilter> contour;
    contour->SetInputData(mesh);
    contour->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, attributeName);
    contour->SetValue(0, isovalue);
    contour->ComputeNormalsOff();
    contour->ComputeScalarsOff();
    contour->Update();
    Triangles referenceTriangles;
    vtkPolyData* reference = contour->GetOutput();
    vtkNew<vtkIdList> corners;
    for (vtkIdType cell = 0; cell < reference->GetNumberOfCells(); ++cell)
    {
      reference->GetCellPoints(cell, corners);
      if (corners->GetNumberOfIds() != 3)
      {
        continue;
      }
      std::array<Position, 3> positions;
      for (int corner = 0; corner < 3; ++corner)
      {
        double xyz[3];
        reference->GetPoint(corners->GetId(corner), xyz);
        positions[corner] = RoundedPosition(xyz, resolution);
      }
      referenceTriangles.push_back(positions);
    }

    vtkIdType numTriangles, numOpen, numReferenceTriangles, numReferenceOpen;
    CountOpenEdges(filterTriangles, numTriangles, numOpen);
    CountOpenEdges(referenceTriangles, numReferenceTriangles, numReferenceOpen);
    std::cout << "    " << attributeName << " @ " << fraction << ": " << numTriangles
              << " triangles, " << numOpen
              << " open edges; merged contour: " << numReferenceTriangles << " triangles, "
              << numReferenceOpen << " open edges.\n";
    if (numOpen > numReferenceOpen)
    {
      std::cerr << "    ERROR: the isosurface has " << numOpen << " open edges, the contour of the"
                << " merged cells " << numReferenceOpen << ".\n";
      ok = false;
    }
    if (std::abs(numTriangles - numReferenceTriangles) > 2 + numReferenceTriangles / 100)
    {
      std::cerr << "    ERROR: the isosurface has " << numTriangles
                << " triangles, the contour of the merged cells " << numReferenceTriangles << ".\n";
      ok = false;
    }
  }
  return ok;
}

/// Read \a fileName and check the isosurface of each of its continuous
/// attributes \a attributeNames (see CheckClosedLikeMergedContour()).
bool TestClosedSurfaces(
  int argc, char* argv[], const char* fileName, const std::vector<const char*>& attributeNames)
{
  std::cout << fileName << " (closed like the merged contour):\n";
  char* filename = vtkTestUtilities::ExpandDataFileName(argc, argv, fileName, 0);
  vtkNew<vtkCellGridReader> reader;
  reader->SetFileName(filename);
  reader->Update();
  delete[] filename;
  auto* grid = vtkCellGrid::SafeDownCast(reader->GetOutputDataObject(0));
  bool ok = grid != nullptr;
  for (const char* attributeName : attributeNames)
  {
    double range[2];
    ok = ok && CoefficientRange(grid, grid->GetCellAttributeByName(attributeName), range) &&
      CheckClosedLikeMergedContour(grid, attributeName, range, { 0.31, 0.57, 0.83 }, 2);
  }
  return ok;
}

} // anonymous namespace

int TestCellGridIsocontour(int argc, char* argv[])
{
  vtkFiltersCellGrid::RegisterCellsAndResponders();
  bool ok = true;

  // clang-format off
  // Grouped by cell type, in vtkDGCell::Shape order (Edge, Triangle,
  // Quadrilateral, Tetrahedron, Hexahedron, Wedge, Pyramid), then Mixed.
  // Each attribute's counts at the isovalues in Fractions: { points, cells }.
  // Cells of every dimension are contoured: lines, vertices or triangles, from
  // surfaces, curves or volumes.

  // Edge
  ok &= TestIsocontour(argc, argv, "Data/dgEdges.dg", {
    { "scalar0", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "scalar1", { { { 5, 5 }, { 5, 5 }, { 4, 4 } } } },
    { "scalar2", { { { 1, 1 }, { 2, 2 }, { 2, 2 } } } },
    { "scalar3", { { { 3, 3 }, { 3, 3 }, { 2, 2 } } } },
  });
  ok &= TestIsocontour(argc, argv, "Data/dgEdge-GBasis.dg", {
    { "G1", { { { 2, 2 }, { 2, 2 }, { 2, 2 } } } },
    { "G2", { { { 0, 0 }, { 2, 2 }, { 2, 2 } } } },
    { "G3", { { { 0, 0 }, { 2, 2 }, { 2, 2 } } } },
    { "G4", { { { 0, 0 }, { 0, 0 }, { 2, 2 } } } },
    { "G5", { { { 0, 0 }, { 0, 0 }, { 2, 2 } } } },
  });

  // Triangle
  ok &= TestIsocontour(argc, argv, "Data/dgTriangle.dg", {
    { "scalar0", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "scalar1", { { { 4, 2 }, { 2, 1 }, { 2, 1 } } } },
    { "scalar2", { { { 2, 1 }, { 0, 0 }, { 2, 1 } } } },
    { "scalar3", { { { 2, 1 }, { 2, 1 }, { 2, 1 } } } },
  });
  ok &= TestIsocontour(argc, argv, "Data/dgTriangle-GBasis.dg", {
    { "G1", { { { 4, 2 }, { 4, 2 }, { 4, 2 } } } },
    { "G2", { { { 0, 0 }, { 4, 2 }, { 4, 2 } } } },
    { "G3", { { { 0, 0 }, { 8, 6 }, { 4, 2 } } } },
    { "G4", { { { 0, 0 }, { 16, 12 }, { 4, 2 } } } },
    { "G5", { { { 0, 0 }, { 12, 10 }, { 4, 2 } } } },
  });

  // Quadrilateral
  ok &= TestIsocontour(argc, argv, "Data/dgQuadrilateral.dg", {
    { "scalar0", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "scalar1", { { { 4, 2 }, { 2, 1 }, { 2, 1 } } } },
    { "scalar2", { { { 2, 1 }, { 0, 0 }, { 2, 1 } } } },
    { "scalar3", { { { 2, 1 }, { 2, 1 }, { 2, 1 } } } },
  });
  ok &= TestIsocontour(argc, argv, "Data/dgQuad-GBasis.dg", {
    { "G1", { { { 8, 4 }, { 4, 2 }, { 4, 2 } } } },
    { "G2", { { { 0, 0 }, { 4, 2 }, { 4, 2 } } } },
    { "G3", { { { 0, 0 }, { 8, 6 }, { 4, 2 } } } },
    { "G4", { { { 0, 0 }, { 8, 6 }, { 4, 2 } } } },
    { "G5", { { { 0, 0 }, { 12, 10 }, { 0, 0 } } } },
  });
  ok &= TestIsocontour(argc, argv, "Data/dgQuadraticQuadrilaterals.dg", {
    { "scalar0", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "scalar1", { { { 4, 2 }, { 14, 11 }, { 0, 0 } } } },
    { "scalar2", { { { 0, 0 }, { 9, 8 }, { 0, 0 } } } },
    { "scalar3", { { { 0, 0 }, { 14, 8 }, { 0, 0 } } } },
    { "linear", { { { 2, 1 }, { 0, 0 }, { 2, 1 } } } },
  });

  // Tetrahedron
  ok &= TestIsocontour(argc, argv, "Data/dgTetrahedra.dg", {
    { "scalar0", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "scalar1", { { { 7, 3 }, { 4, 2 }, { 3, 1 } } } },
    { "scalar2", { { { 3, 1 }, { 0, 0 }, { 3, 1 } } } },
    { "scalar3", { { { 3, 1 }, { 3, 1 }, { 4, 2 } } } },
    { "G1", { { { 3, 1 }, { 4, 2 }, { 3, 1 } } } },
    { "G2", { { { 0, 0 }, { 28, 28 }, { 6, 2 } } } },
    { "G3", { { { 0, 0 }, { 39, 52 }, { 0, 0 } } } },
    { "G4", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "G5", { { { 0, 0 }, { 1315, 2388 }, { 0, 0 } } } },
  });

  // Hexahedron
  ok &= TestIsocontour(argc, argv, "Data/dgHexahedra.dg", {
    { "scalar0", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "scalar1", { { { 10, 6 }, { 10, 6 }, { 10, 4 } } } },
    { "scalar2", { { { 4, 2 }, { 0, 0 }, { 4, 2 } } } },
    { "scalar3", { { { 6, 4 }, { 6, 4 }, { 6, 4 } } } },
    { "gauss_cubic", { { { 20, 16 }, { 99, 160 }, { 50, 40 } } } },
    { "quadratic", { { { 17, 8 }, { 9, 16 }, { 17, 8 } } } },
  });
  ok &= TestIsocontour(argc, argv, "Data/dgGhostHexahedra.dg", {
    { "scalar0", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "scalar1", { { { 10, 6 }, { 10, 6 }, { 10, 4 } } } },
    { "scalar2", { { { 4, 2 }, { 0, 0 }, { 4, 2 } } } },
    { "scalar3", { { { 6, 4 }, { 6, 4 }, { 6, 4 } } } },
    { "quadratic", { { { 17, 8 }, { 9, 16 }, { 17, 8 } } } },
  });
  ok &= TestIsocontour(argc, argv, "Data/dgHexahedra-GBasis.dg", {
    { "scalar0", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "scalar1", { { { 10, 6 }, { 10, 6 }, { 10, 4 } } } },
    { "scalar2", { { { 4, 2 }, { 0, 0 }, { 4, 2 } } } },
    { "scalar3", { { { 6, 4 }, { 6, 4 }, { 6, 4 } } } },
    { "G1", { { { 24, 8 }, { 6, 2 }, { 6, 2 } } } },
    { "G2", { { { 0, 0 }, { 6, 2 }, { 6, 2 } } } },
    { "G3", { { { 0, 0 }, { 18, 14 }, { 6, 2 } } } },
    { "G4", { { { 0, 0 }, { 0, 0 }, { 6, 2 } } } },
    { "G5", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "gauss_cubic", { { { 20, 16 }, { 99, 160 }, { 50, 40 } } } },
    { "quadratic", { { { 17, 8 }, { 9, 16 }, { 17, 8 } } } },
  });
  // fandisk.dg's only attribute ("curlyq") has 3 components, so nothing
  // here is scalar and there is nothing to check.
  ok &= TestIsocontour(argc, argv, "Data/fandisk.dg", {});

  // Wedge
  ok &= TestIsocontour(argc, argv, "Data/dgWedges.dg", {
    { "scalar0", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "scalar1", { { { 7, 3 }, { 7, 3 }, { 7, 3 } } } },
    { "scalar2", { { { 4, 2 }, { 4, 2 }, { 4, 2 } } } },
    { "scalar3", { { { 4, 2 }, { 6, 4 }, { 4, 2 } } } },
  });
  ok &= TestIsocontour(argc, argv, "Data/dgWedges-GBasis.dg", {
    { "scalar0", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "scalar1", { { { 7, 3 }, { 7, 3 }, { 7, 3 } } } },
    { "scalar2", { { { 4, 2 }, { 4, 2 }, { 4, 2 } } } },
    { "scalar3", { { { 4, 2 }, { 6, 4 }, { 4, 2 } } } },
    { "G1", { { { 12, 6 }, { 6, 2 }, { 6, 2 } } } },
    { "G2", { { { 0, 0 }, { 6, 2 }, { 6, 2 } } } },
    { "G3", { { { 0, 0 }, { 18, 14 }, { 6, 2 } } } },
    { "G4", { { { 0, 0 }, { 45, 42 }, { 6, 2 } } } },
    { "G5", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
  });

  // Pyramid
  ok &= TestIsocontour(argc, argv, "Data/dgPyramids.dg", {
    { "scalar0", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "scalar1", { { { 9, 5 }, { 6, 4 }, { 7, 3 } } } },
    { "scalar2", { { { 4, 2 }, { 0, 0 }, { 4, 2 } } } },
    { "scalar3", { { { 4, 2 }, { 0, 0 }, { 4, 2 } } } },
  });
  ok &= TestIsocontour(argc, argv, "Data/dgPyramids-GBasis.dg", {
    { "G1", { { { 4, 2 }, { 3, 1 }, { 4, 2 } } } },
    { "G2", { { { 0, 0 }, { 0, 0 }, { 10, 6 } } } },
    { "G3", { { { 0, 0 }, { 61, 88 }, { 0, 0 } } } },
    { "G4", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "G5", { { { 0, 0 }, { 1713, 3120 }, { 0, 0 } } } },
  });

  // Mixed (tetrahedra and quadrilaterals): each attribute is one
  // vtkCellAttribute spanning both cell types, so its counts here are the
  // two shapes' combined total, not per-shape.
  ok &= TestIsocontour(argc, argv, "Data/dgMixed.dg", {
    { "scalar0", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "scalar1", { { { 9, 4 }, { 6, 3 }, { 5, 2 } } } },
    { "scalar2", { { { 3, 1 }, { 0, 0 }, { 5, 2 } } } },
    { "scalar3", { { { 4, 2 }, { 4, 2 }, { 4, 2 } } } },
  });
  // clang-format on

  // Continuous attributes (where neighboring cells agree on the face they share).
  ok &= TestClosedSurfaces(argc, argv, "Data/dgHexahedra.dg", { "scalar3" });
  ok &= TestClosedSurfaces(argc, argv, "Data/dgTetrahedra.dg", { "scalar3" });
  ok &= TestClosedSurfaces(argc, argv, "Data/dgWedges.dg", { "scalar2", "scalar3" });

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
