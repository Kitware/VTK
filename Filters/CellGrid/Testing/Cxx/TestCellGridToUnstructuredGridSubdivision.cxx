// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCell.h"
#include "vtkCellGrid.h"
#include "vtkCellGridComputeSides.h"
#include "vtkCellGridReader.h"
#include "vtkCellGridToUnstructuredGrid.h"
#include "vtkCellType.h"
#include "vtkDataArray.h"
#include "vtkFiltersCellGrid.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkTestUtilities.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace
{

/// The number of cells a single input cell of \a shape subdivides into when the
/// filter uses \a m subdivisions per parametric axis.
///
/// Every shape subdivides into m^d cells of its own type except the pyramid,
/// which subdivides into a mix of pyramids and tetrahedra.
vtkIdType SubCellCount(int cellType, int m)
{
  switch (cellType)
  {
    case VTK_VERTEX:
      return 1;
    case VTK_LINE:
      return m;
    case VTK_TRIANGLE:
    case VTK_QUAD:
      return m * m;
    case VTK_TETRA:
    case VTK_HEXAHEDRON:
    case VTK_WEDGE:
      return m * m * m;
    case VTK_PYRAMID:
    {
      // Layer n contributes n^2 + (n - 1)^2 pyramids and 2n(n - 1) tetrahedra,
      // i.e. (2n - 1)^2 cells.
      vtkIdType count = 0;
      for (int nn = 1; nn <= m; ++nn)
      {
        count += (2 * nn - 1) * (2 * nn - 1);
      }
      return count;
    }
    default:
      break;
  }
  return 0;
}

/// A decomposition of each volumetric cell type into tetrahedra, used to
/// measure cells and to verify that none of them is inverted.
///
/// A tetrahedron is the only cell whose volume is unambiguous, so everything
/// else is cut into tetrahedra and summed. That is exact when the cell's faces
/// are planar and an approximation otherwise. See CheckMeasureSeries, which
/// takes the difference into account.
const std::vector<std::array<int, 4>>& TetrahedraForCellType(int cellType)
{
  static const std::map<int, std::vector<std::array<int, 4>>> tets{
    { VTK_TETRA, { { 0, 1, 2, 3 } } },
    { VTK_PYRAMID, { { 0, 1, 2, 4 }, { 0, 2, 3, 4 } } },
    { VTK_WEDGE, { { 0, 1, 2, 3 }, { 1, 2, 3, 4 }, { 2, 3, 4, 5 } } },
    { VTK_HEXAHEDRON,
      { { 0, 1, 3, 4 }, { 1, 2, 3, 6 }, { 1, 4, 5, 6 }, { 3, 4, 6, 7 }, { 1, 3, 4, 6 } } },
  };
  static const std::vector<std::array<int, 4>> none;
  auto it = tets.find(cellType);
  return it == tets.end() ? none : it->second;
}

/// Return the signed volume of a volumetric \a cell, or 0 for other cells.
double SignedVolume(vtkCell* cell)
{
  double volume = 0.;
  std::array<double, 3> pt[4];
  for (const auto& tet : TetrahedraForCellType(cell->GetCellType()))
  {
    for (int ii = 0; ii < 4; ++ii)
    {
      cell->GetPoints()->GetPoint(tet[ii], pt[ii].data());
    }
    volume += vtkTetra::ComputeVolume(pt[0].data(), pt[1].data(), pt[2].data(), pt[3].data());
  }
  return volume;
}

/// Return the area of a triangle or quadrilateral \a cell.
///
/// Unlike a volume this is unsigned, because a surface in space has no inherent
/// "this way up" and so no orientation to check against.
double Area(vtkCell* cell)
{
  std::array<double, 3> p0, p1, p2, p3;
  cell->GetPoints()->GetPoint(0, p0.data());
  cell->GetPoints()->GetPoint(1, p1.data());
  cell->GetPoints()->GetPoint(2, p2.data());
  double area = vtkTriangle::TriangleArea(p0.data(), p1.data(), p2.data());
  if (cell->GetNumberOfPoints() == 4)
  {
    cell->GetPoints()->GetPoint(3, p3.data());
    area += vtkTriangle::TriangleArea(p0.data(), p2.data(), p3.data());
  }
  return area;
}

double Length(vtkCell* cell)
{
  std::array<double, 3> p0, p1;
  cell->GetPoints()->GetPoint(0, p0.data());
  cell->GetPoints()->GetPoint(1, p1.data());
  double sum = 0.;
  for (int ii = 0; ii < 3; ++ii)
  {
    sum += (p1[ii] - p0[ii]) * (p1[ii] - p0[ii]);
  }
  return std::sqrt(sum);
}

/// Measurements of a converted grid, used to check that subdividing a cell
/// really does partition it and nothing more.
struct GridMeasure
{
  /// Total volume (3D cells), area (2D cells), or length (1D cells).
  double Cells{ 0. };
  /// Total area (3D) or length (2D) of the boundary: the faces or edges that
  /// belong to exactly one cell.
  double Boundary{ 0. };
  /// Cells whose volume is zero or negative (i.e. degenerate or inverted).
  vtkIdType NonPositiveCells{ 0 };
  /// Faces or edges shared by three or more cells, which are never valid.
  vtkIdType OverusedFaces{ 0 };
  int Dimension{ 0 };
};

/// Measure \a grid, accumulating cell measures and the measure of its boundary.
///
/// Subdividing a cell has to partition it exactly, with no gaps, no overlaps
/// and no hanging nodes. Both totals catch a failure to do so. A gap or an
/// overlap changes the total cell measure, and a hanging node leaves an
/// interior face belonging to only one cell, which inflates the boundary.
///
/// This also covers the interfaces *between* input cells: the output is
/// conformal only if the samples two neighboring cells make of the face they
/// share merge into the same points. If they did not, that face would belong to
/// one cell on either side and count twice toward the boundary.
GridMeasure MeasureGrid(vtkUnstructuredGrid* grid)
{
  GridMeasure result;
  // Sorting the point IDs makes a face's identity independent of the winding
  // each of its cells sees it with, so the two cells sharing a face agree on
  // the key and the face gets counted twice.
  std::map<std::vector<vtkIdType>, int> boundaryUse;
  std::map<std::vector<vtkIdType>, double> boundaryMeasure;

  for (vtkIdType cc = 0; cc < grid->GetNumberOfCells(); ++cc)
  {
    vtkCell* cell = grid->GetCell(cc);
    int dim = cell->GetCellDimension();
    result.Dimension = std::max(result.Dimension, dim);
    if (dim == 3)
    {
      double volume = SignedVolume(cell);
      result.Cells += volume;
      if (volume <= 0.)
      {
        ++result.NonPositiveCells;
      }
    }
    else if (dim == 2)
    {
      double area = Area(cell);
      result.Cells += area;
      if (area <= 0.)
      {
        ++result.NonPositiveCells;
      }
    }
    else if (dim == 1)
    {
      result.Cells += Length(cell);
      // The "boundary" of a set of edges is a set of points, which has no
      // measure worth comparing, so there is nothing more to do for edges.
      continue;
    }
    else
    {
      continue;
    }

    // Record the cell's faces (3D) or edges (2D).
    int numBoundaries = dim == 3 ? cell->GetNumberOfFaces() : cell->GetNumberOfEdges();
    for (int bb = 0; bb < numBoundaries; ++bb)
    {
      vtkCell* boundary = dim == 3 ? cell->GetFace(bb) : cell->GetEdge(bb);
      std::vector<vtkIdType> key;
      key.reserve(boundary->GetNumberOfPoints());
      for (vtkIdType pp = 0; pp < boundary->GetNumberOfPoints(); ++pp)
      {
        key.push_back(boundary->GetPointId(pp));
      }
      std::sort(key.begin(), key.end());
      if (++boundaryUse[key] == 1)
      {
        boundaryMeasure[key] = dim == 3 ? Area(boundary) : Length(boundary);
      }
    }
  }

  for (const auto& entry : boundaryUse)
  {
    if (entry.second == 1)
    {
      result.Boundary += boundaryMeasure[entry.first];
    }
    else if (entry.second > 2)
    {
      ++result.OverusedFaces;
    }
  }
  return result;
}

/// Verify the output of the filter run on \a input at \a level.
///
/// \a cellType is the VTK cell type expected in the output and \a linearGeometry
/// indicates whether the input's shape attribute is linear (in which case
/// subdividing must not change the output's bounds).
bool CheckLevel(vtkAlgorithm* source, vtkCellGrid* input, vtkIdType numInputCells, int cellType,
  bool linearGeometry, int level, GridMeasure& measure)
{
  bool ok = true;
  int m = 1 << level;
  std::array<double, 6> bounds;
  input->GetBounds(bounds.data());

  vtkNew<vtkCellGridToUnstructuredGrid> convert;
  convert->SetInputConnection(source->GetOutputPort());
  convert->SetSubdivisionLevel(level);
  convert->Update();
  auto* output = convert->GetOutput();

  measure = MeasureGrid(output);
  std::cout << "  level " << level << " (" << m
            << " subdivisions/axis): " << output->GetNumberOfPoints() << " points, "
            << output->GetNumberOfCells() << " cells, measure " << measure.Cells << ", boundary "
            << measure.Boundary << "\n";

  if (convert->GetSubdivisionLevel() != level)
  {
    std::cerr << "    ERROR: Level was not stored (" << convert->GetSubdivisionLevel() << ").\n";
    ok = false;
  }

  // 1. Cell count.
  vtkIdType expectedCells = numInputCells * SubCellCount(cellType, m);
  if (output->GetNumberOfCells() != expectedCells)
  {
    std::cerr << "    ERROR: Expected " << expectedCells << " cells, got "
              << output->GetNumberOfCells() << ".\n";
    ok = false;
  }

  // 2. Output cell types: the input shape, plus tetrahedra when subdividing pyramids.
  std::set<unsigned char> allowed{ static_cast<unsigned char>(cellType) };
  if (cellType == VTK_PYRAMID)
  {
    allowed.insert(VTK_TETRA);
  }
  auto* types = vtkUnsignedCharArray::FastDownCast(output->GetCellTypes());
  for (vtkIdType ii = 0; ii < (types ? types->GetNumberOfValues() : 0); ++ii)
  {
    if (allowed.find(types->GetValue(ii)) == allowed.end())
    {
      std::cerr << "    ERROR: Unexpected output cell type "
                << static_cast<int>(types->GetValue(ii)) << ".\n";
      ok = false;
      break;
    }
  }

  // 3. Sub-cells are well-formed: none is degenerate or inverted, and no face
  //    is shared by more than two of them.
  if (output->GetNumberOfPoints() <= 0)
  {
    std::cerr << "    ERROR: No output points.\n";
    ok = false;
  }
  if (measure.NonPositiveCells > 0)
  {
    std::cerr << "    ERROR: " << measure.NonPositiveCells
              << " output cells are degenerate or inverted.\n";
    ok = false;
  }
  // NB: Only volumetric output has to be manifold. The sides of a solid form a
  //     surface whose edges may quite legitimately border more than two faces,
  //     so the same rule cannot be applied to two-dimensional output.
  if (measure.Dimension == 3 && measure.OverusedFaces > 0)
  {
    std::cerr << "    ERROR: " << measure.OverusedFaces
              << " faces are shared by more than two cells.\n";
    ok = false;
  }

  // 4. Every point-data array covers every point and holds finite values.
  auto* pointData = output->GetPointData();
  if (pointData->GetNumberOfArrays() == 0)
  {
    std::cerr << "    ERROR: No point-data arrays.\n";
    ok = false;
  }
  for (int aa = 0; aa < pointData->GetNumberOfArrays(); ++aa)
  {
    auto* array = pointData->GetArray(aa);
    if (!array)
    {
      continue;
    }
    if (array->GetNumberOfTuples() != output->GetNumberOfPoints())
    {
      std::cerr << "    ERROR: Array " << array->GetName() << " has " << array->GetNumberOfTuples()
                << " tuples, expected " << output->GetNumberOfPoints() << ".\n";
      ok = false;
      continue;
    }
    for (vtkIdType ii = 0; ii < array->GetNumberOfTuples(); ++ii)
    {
      for (int cc = 0; cc < array->GetNumberOfComponents(); ++cc)
      {
        if (!std::isfinite(array->GetComponent(ii, cc)))
        {
          std::cerr << "    ERROR: Array " << array->GetName() << " has a non-finite value at "
                    << ii << ".\n";
          ok = false;
          ii = array->GetNumberOfTuples();
          break;
        }
      }
    }
  }

  // 5. Subdividing a cell with linear geometry must not move its boundary.
  const std::array<double, 6>& inputBounds = bounds;
  std::array<double, 6> outputBounds;
  output->GetBounds(outputBounds.data());
  for (int ii = 0; ii < 6; ++ii)
  {
    double tol = 1e-8 * std::max(1., std::abs(inputBounds[ii]));
    bool inside = (ii % 2 == 0) ? (outputBounds[ii] > inputBounds[ii] - tol)
                                : (outputBounds[ii] < inputBounds[ii] + tol);
    bool matches = std::abs(outputBounds[ii] - inputBounds[ii]) < tol;
    if (linearGeometry ? !matches : !inside)
    {
      std::cerr << "    ERROR: Bound " << ii << " is " << outputBounds[ii] << ", expected "
                << (linearGeometry ? "" : "no more than ") << inputBounds[ii] << ".\n";
      ok = false;
    }
  }

  return ok;
}

/// Verify that a measurement taken at each level partitions the same region.
///
/// Subdividing a cell partitions it exactly, so these measures would be
/// identical at every level if the sub-cells could be measured exactly. They
/// are measured by cutting them into tetrahedra (or triangles in 2D), which is
/// exact only when the cell's faces are planar. A linear cell need not be
/// affine, since a trilinear hexahedron may have warped faces, and for those
/// the measurement converges as the sub-cells shrink instead. So the series has
/// to converge toward a limit and stay near the level-0 value, and only
/// simplices, whose linear map is always affine, have to match exactly.
bool CheckMeasureSeries(const char* what, const std::vector<double>& series, bool exact)
{
  bool ok = true;
  if (series.size() < 2)
  {
    return ok;
  }
  for (std::size_t ii = 1; ii < series.size(); ++ii)
  {
    double drift = std::abs(series[ii] - series[0]);
    if (exact && drift > 1e-9 * std::max(1., std::abs(series[0])))
    {
      std::cerr << "    ERROR: Level " << ii << " " << what << " is " << series[ii]
                << ", but level 0 gives " << series[0] << " (difference " << drift << ").\n";
      ok = false;
    }
    // A gap, an overlap, or a hanging node changes the measure by a fixed
    // fraction of the cell at every level, which this bound would catch.
    if (drift > 0.1 * std::max(1., std::abs(series[0])))
    {
      std::cerr << "    ERROR: Level " << ii << " " << what << " is " << series[ii]
                << ", too far from the level-0 value " << series[0] << ".\n";
      ok = false;
    }
    // Successive levels must approach a limit rather than drift away.
    if (ii >= 2)
    {
      double previous = std::abs(series[ii - 1] - series[ii - 2]);
      double current = std::abs(series[ii] - series[ii - 1]);
      if (current > 0.75 * previous && current > 1e-12)
      {
        std::cerr << "    ERROR: " << what << " is not converging: level " << ii << " moved by "
                  << current << " after level " << (ii - 1) << " moved by " << previous << ".\n";
        ok = false;
      }
    }
  }
  return ok;
}

bool CheckLevels(
  vtkAlgorithm* source, vtkCellGrid* input, vtkIdType numInputCells, int cellType, bool linear)
{
  bool ok = true;
  std::vector<double> cellMeasures;
  std::vector<double> boundaryMeasures;
  for (int level = 0; level <= 3; ++level)
  {
    GridMeasure measure;
    ok &= CheckLevel(source, input, numInputCells, cellType, linear, level, measure);
    cellMeasures.push_back(measure.Cells);
    boundaryMeasures.push_back(measure.Boundary);
  }
  if (linear)
  {
    bool simplex = cellType == VTK_LINE || cellType == VTK_TRIANGLE || cellType == VTK_TETRA;
    ok &= CheckMeasureSeries("cell measure", cellMeasures, simplex);
    ok &= CheckMeasureSeries("boundary measure", boundaryMeasures, simplex);
  }
  return ok;
}

bool CheckFile(int argc, char* argv[], const char* dataFile, int cellType, bool linearGeometry)
{
  std::cout << dataFile << ":\n";
  char* filename =
    vtkTestUtilities::ExpandDataFileName(argc, argv, (std::string("Data/") + dataFile).c_str(), 0);
  if (!filename)
  {
    std::cerr << "ERROR: No filename for " << dataFile << ".\n";
    return false;
  }
  vtkNew<vtkCellGridReader> reader;
  reader->SetFileName(filename);
  delete[] filename;
  reader->Update();
  auto* input = reader->GetOutput();
  if (!input || input->GetNumberOfCells() == 0)
  {
    std::cerr << "ERROR: Could not read " << dataFile << ".\n";
    return false;
  }
  return CheckLevels(reader, input, input->GetNumberOfCells(), cellType, linearGeometry);
}

/// Convert the *sides* of a cell-grid rather than its cells.
///
/// Sides are sampled in the reference coordinates of the side and then mapped
/// into the coordinates of the cell they bound, a code path the cell sources
/// never exercise. It is worth testing on its own, because a mistake there
/// draws each side using some other cell's geometry and still produces a
/// plausible-looking grid with the right number of cells in it.
bool CheckSides(int argc, char* argv[], const char* dataFile, int sideCellType)
{
  std::cout << dataFile << " (sides):\n";
  char* filename =
    vtkTestUtilities::ExpandDataFileName(argc, argv, (std::string("Data/") + dataFile).c_str(), 0);
  if (!filename)
  {
    std::cerr << "ERROR: No filename for " << dataFile << ".\n";
    return false;
  }
  vtkNew<vtkCellGridReader> reader;
  reader->SetFileName(filename);
  delete[] filename;

  vtkNew<vtkCellGridComputeSides> sides;
  sides->SetInputConnection(reader->GetOutputPort());
  sides->Update();
  auto* input = vtkCellGrid::SafeDownCast(sides->GetOutputDataObject(0));
  if (!input || input->GetNumberOfCells() == 0)
  {
    std::cerr << "ERROR: Could not compute sides of " << dataFile << ".\n";
    return false;
  }

  // The sides replace the (blanked) cells, so the grid reports one cell per side.
  return CheckLevels(sides, input, input->GetNumberOfCells(), sideCellType, true);
}

} // anonymous namespace

int TestCellGridToUnstructuredGridSubdivision(int argc, char* argv[])
{
  vtkFiltersCellGrid::RegisterCellsAndResponders();

  bool ok = true;
  ok &= CheckFile(argc, argv, "dgEdges.dg", VTK_LINE, true);
  ok &= CheckFile(argc, argv, "dgTriangle.dg", VTK_TRIANGLE, true);
  ok &= CheckFile(argc, argv, "dgQuadrilateral.dg", VTK_QUAD, true);
  ok &= CheckFile(argc, argv, "dgTetrahedra.dg", VTK_TETRA, true);
  ok &= CheckFile(argc, argv, "dgHexahedra.dg", VTK_HEXAHEDRON, true);
  ok &= CheckFile(argc, argv, "dgWedges.dg", VTK_WEDGE, true);
  ok &= CheckFile(argc, argv, "dgPyramids.dg", VTK_PYRAMID, true);
  // Quadratic geometry: subdividing approximates the curved shape, so the
  // output may only shrink toward (never grow beyond) the control-point hull.
  ok &= CheckFile(argc, argv, "dgQuadraticQuadrilaterals.dg", VTK_QUAD, false);

  // The quadrilateral sides of hexahedra, which exercise the mapping from a
  // side's reference coordinates into those of the cell it bounds.
  ok &= CheckSides(argc, argv, "dgHexahedra.dg", VTK_QUAD);

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
