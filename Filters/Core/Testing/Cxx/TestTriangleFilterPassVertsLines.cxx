// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Check that vtkTriangleFilter honours PassVerts and PassLines, and keeps the cell data with its
// cells, when the vertices, lines and polygons of the input are already in the form the filter
// outputs. Those inputs take the shortcuts of the filter, which are what this test covers.

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkTriangleFilter.h"

#include <iostream>
#include <vector>

namespace
{
constexpr int VertLabel = 100;
constexpr int LineLabel = 200;
constexpr int Triangle1Label = 300;
constexpr int Triangle2Label = 301;

// A vertex, a line (or a polyline of three points) and two triangles, with cell data that labels
// each cell
vtkSmartPointer<vtkPolyData> MakeInput(bool polyLine)
{
  vtkNew<vtkPoints> points;
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(1, 0, 0);
  points->InsertNextPoint(0, 1, 0);
  points->InsertNextPoint(1, 1, 0);
  points->InsertNextPoint(2, 0, 0);
  points->InsertNextPoint(2, 1, 0);

  vtkNew<vtkCellArray> verts;
  // Not InsertNextCell({ 4 }): with a single value, MSVC calls InsertNextCell(int npts), which adds
  // a cell of 4 point ids that are never set
  const vtkIdType vertexPointId = 4;
  verts->InsertNextCell(1, &vertexPointId);
  vtkNew<vtkCellArray> lines;
  if (polyLine)
  {
    lines->InsertNextCell({ 3, 4, 5 });
  }
  else
  {
    lines->InsertNextCell({ 3, 4 });
  }
  vtkNew<vtkCellArray> polys;
  polys->InsertNextCell({ 0, 1, 2 });
  polys->InsertNextCell({ 1, 3, 2 });

  auto input = vtkSmartPointer<vtkPolyData>::New();
  input->SetPoints(points);
  input->SetVerts(verts);
  input->SetLines(lines);
  input->SetPolys(polys);

  // The cell data follows the cell order of vtkPolyData: vertices, lines, polygons
  vtkNew<vtkIntArray> labels;
  labels->SetName("Label");
  for (int label : { VertLabel, LineLabel, Triangle1Label, Triangle2Label })
  {
    labels->InsertNextValue(label);
  }
  input->GetCellData()->AddArray(labels);
  return input;
}

bool Check(bool passVerts, bool passLines, bool polyLine, const std::vector<int>& expectedTypes,
  const std::vector<int>& expectedLabels)
{
  vtkNew<vtkTriangleFilter> filter;
  filter->SetPassVerts(passVerts);
  filter->SetPassLines(passLines);
  filter->SetInputData(MakeInput(polyLine));
  filter->Update();
  vtkPolyData* output = filter->GetOutput();
  vtkIntArray* labels = vtkIntArray::SafeDownCast(output->GetCellData()->GetArray("Label"));

  bool ok = labels && output->GetNumberOfCells() == static_cast<vtkIdType>(expectedTypes.size()) &&
    labels->GetNumberOfTuples() == output->GetNumberOfCells();
  for (vtkIdType cellId = 0; ok && cellId < output->GetNumberOfCells(); ++cellId)
  {
    ok = output->GetCellType(cellId) == expectedTypes[cellId] &&
      labels->GetValue(cellId) == expectedLabels[cellId];
  }
  if (!ok)
  {
    std::cerr << "Wrong output for PassVerts " << passVerts << ", PassLines " << passLines
              << ", polyline " << polyLine << ":";
    for (vtkIdType cellId = 0; cellId < output->GetNumberOfCells(); ++cellId)
    {
      std::cerr << " (type " << output->GetCellType(cellId) << ", label "
                << (labels && cellId < labels->GetNumberOfTuples() ? labels->GetValue(cellId) : -1)
                << ")";
    }
    std::cerr << std::endl;
  }
  return ok;
}
}

int TestTriangleFilterPassVertsLines(int, char*[])
{
  bool ok = true;
  ok &=
    Check(false, false, false, { VTK_TRIANGLE, VTK_TRIANGLE }, { Triangle1Label, Triangle2Label });
  ok &= Check(true, false, false, { VTK_VERTEX, VTK_TRIANGLE, VTK_TRIANGLE },
    { VertLabel, Triangle1Label, Triangle2Label });
  ok &= Check(false, true, false, { VTK_LINE, VTK_TRIANGLE, VTK_TRIANGLE },
    { LineLabel, Triangle1Label, Triangle2Label });
  ok &= Check(true, true, false, { VTK_VERTEX, VTK_LINE, VTK_TRIANGLE, VTK_TRIANGLE },
    { VertLabel, LineLabel, Triangle1Label, Triangle2Label });
  // A polyline is split into lines, which the triangles must not replace with the polyline again
  ok &= Check(true, true, true, { VTK_VERTEX, VTK_LINE, VTK_LINE, VTK_TRIANGLE, VTK_TRIANGLE },
    { VertLabel, LineLabel, LineLabel, Triangle1Label, Triangle2Label });
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
