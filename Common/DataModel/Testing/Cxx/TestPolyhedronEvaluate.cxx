// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// A unit cube, and the same cube with its top pushed in to a point at height 0.6. A point in
// the dent is outside the concave cell, and linear fields are interpolated exactly and
// differentiated exactly everywhere in both, the corners of the box included.

#include "vtkCellType.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyhedron.h"
#include "vtkSmartPointer.h"
#include "vtkStringFormatter.h"
#include "vtkUnstructuredGrid.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace
{
int Failures = 0;

void Check(bool condition, const std::string& message)
{
  if (!condition)
  {
    std::cerr << message << std::endl;
    ++Failures;
  }
}

vtkSmartPointer<vtkUnstructuredGrid> MakeCube(bool dented)
{
  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  const double corners[8][3] = { { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 }, { 0, 0, 1 },
    { 1, 0, 1 }, { 1, 1, 1 }, { 0, 1, 1 } };
  for (const auto& corner : corners)
  {
    points->InsertNextPoint(corner);
  }
  std::vector<std::vector<vtkIdType>> faces = { { 0, 3, 2, 1 }, { 0, 1, 5, 4 }, { 1, 2, 6, 5 },
    { 2, 3, 7, 6 }, { 3, 0, 4, 7 } };
  if (dented)
  {
    points->InsertNextPoint(0.5, 0.5, 0.6);
    faces.insert(faces.end(), { { 4, 5, 8 }, { 5, 6, 8 }, { 6, 7, 8 }, { 7, 4, 8 } });
  }
  else
  {
    faces.push_back({ 4, 5, 6, 7 });
  }
  vtkNew<vtkIdList> stream;
  stream->InsertNextId(static_cast<vtkIdType>(faces.size()));
  for (const auto& face : faces)
  {
    stream->InsertNextId(static_cast<vtkIdType>(face.size()));
    for (vtkIdType id : face)
    {
      stream->InsertNextId(id);
    }
  }
  auto grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  grid->SetPoints(points);
  grid->InsertNextCell(VTK_POLYHEDRON, stream);
  return grid;
}

int Evaluate(vtkPolyhedron* cell, double x, double y, double z, double& dist2)
{
  const double point[3] = { x, y, z };
  double closest[3], pcoords[3];
  std::vector<double> weights(cell->GetNumberOfPoints());
  int subId;
  return cell->EvaluatePosition(point, closest, subId, pcoords, dist2, weights.data());
}

double Linear(const double p[3])
{
  return 1 + 2 * p[0] + 3 * p[1] + 4 * p[2];
}

void CheckInterpolation(vtkPolyhedron* cell, double x, double y, double z)
{
  const double point[3] = { x, y, z };
  std::vector<double> weights(cell->GetNumberOfPoints());
  cell->InterpolateFunctions(point, weights.data());
  double value = 0;
  for (vtkIdType i = 0; i < cell->GetNumberOfPoints(); i++)
  {
    double p[3];
    cell->GetPoints()->GetPoint(i, p);
    value += weights[i] * Linear(p);
  }
  Check(std::abs(value - Linear(point)) <= 1e-9,
    "interpolated value at " + vtk::to_string(x) + "," + vtk::to_string(y) + "," +
      vtk::to_string(z) + " is " + vtk::to_string(value) + " instead of " +
      vtk::to_string(Linear(point)));
}

// Two linear fields, f = 1 + 2x + 3y + 4z and g = 5 - x + 2y - z, with two padding components.
void CheckDerivatives(vtkPolyhedron* cell, const double pcoords[3], const std::string& where)
{
  constexpr int dim = 4;
  const vtkIdType numPts = cell->GetNumberOfPoints();
  std::vector<double> values(dim * numPts);
  for (vtkIdType i = 0; i < numPts; i++)
  {
    double p[3];
    cell->GetPoints()->GetPoint(i, p);
    values[dim * i] = Linear(p);
    values[dim * i + 1] = 5 - p[0] + 2 * p[1] - p[2];
    values[dim * i + 2] = 0;
    values[dim * i + 3] = 7;
  }
  double derivs[3 * dim];
  cell->Derivatives(0, pcoords, values.data(), dim, derivs);
  const double expected[3 * dim] = { 2, 3, 4, -1, 2, -1, 0, 0, 0, 0, 0, 0 };
  for (int i = 0; i < 3 * dim; i++)
  {
    Check(std::abs(derivs[i] - expected[i]) <= 1e-6,
      where + ": derivative " + vtk::to_string(i) + " is " + vtk::to_string(derivs[i]) +
        " instead of " + vtk::to_string(expected[i]));
  }
}
} // anonymous namespace

int TestPolyhedronEvaluate(int, char*[])
{
  vtkNew<vtkGenericCell> cell;

  auto dented = MakeCube(true);
  dented->GetCell(0, cell);
  auto* polyhedron = vtkPolyhedron::SafeDownCast(cell->GetRepresentativeCell());
  double dist2;
  Check(Evaluate(polyhedron, 0.5, 0.5, 0.8, dist2) == 0 && dist2 > 0.0,
    "a point in the dent should be outside the cell");
  Check(Evaluate(polyhedron, 0.5, 0.5, 0.3, dist2) == 1 && dist2 == 0.0,
    "a point under the dent should be inside the cell");
  Check(Evaluate(polyhedron, 0.5, 0.5, 0.0, dist2) == 1, "a point on a face should be on the cell");
  Check(
    Evaluate(polyhedron, 0.5, 0.5, 0.6, dist2) == 1, "the tip of the dent should be on the cell");
  CheckInterpolation(polyhedron, 0.2, 0.2, 0.5);
  CheckInterpolation(polyhedron, 0.3, 0.5, 0.7);
  CheckInterpolation(polyhedron, 0.45, 0.5, 0.59);
  const double underDent[3] = { 0.5, 0.5, 0.3 };
  CheckDerivatives(polyhedron, underDent, "under the dent");
  const double nearCorner[3] = { 0.9, 0.95, 0.1 };
  CheckDerivatives(polyhedron, nearCorner, "near a corner of the dented cube");
  const double offAxis[3] = { 0.2, 0.2, 0.5 };
  CheckDerivatives(polyhedron, offAxis, "off the axis of the dented cube");
  // The step in x from here crosses into the dent, out of the cell.
  const double belowDent[3] = { 0.3, 0.5, 0.755 };
  CheckDerivatives(polyhedron, belowDent, "just below the dent");

  auto box = MakeCube(false);
  box->GetCell(0, cell);
  polyhedron = vtkPolyhedron::SafeDownCast(cell->GetRepresentativeCell());
  for (int corner = 0; corner < 8; corner++)
  {
    const double pcoords[3] = { static_cast<double>(corner & 1),
      static_cast<double>((corner >> 1) & 1), static_cast<double>((corner >> 2) & 1) };
    CheckDerivatives(polyhedron, pcoords,
      "box corner " + vtk::to_string(pcoords[0]) + "," + vtk::to_string(pcoords[1]) + "," +
        vtk::to_string(pcoords[2]));
  }
  const double center[3] = { 0.5, 0.5, 0.5 };
  CheckDerivatives(polyhedron, center, "box center");

  if (Failures > 0)
  {
    std::cerr << Failures << " failure(s)" << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
