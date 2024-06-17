// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

namespace
{

int CompareTimes(
  vtkMTimeType before, vtkMTimeType after, bool expectUpdate, const std::string& modifiedObjectName)
{
  if (expectUpdate && before == after)
  {
    vtkLog(ERROR,
      "Modifying " << modifiedObjectName << " should modify MeshMTime. Before: " << before
                   << "; after: " << after);
    return EXIT_FAILURE;
  }

  if (!expectUpdate && before != after)
  {
    vtkLog(ERROR,
      "Modifying " << modifiedObjectName << " should not modify MeshMTime. Expected: " << before
                   << "; got: " << after);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int TestFieldDataModification(vtkDataSet* dataset, bool expectMeshMTimeUpdate)
{
  int result = EXIT_SUCCESS;

  // Create dummy point/cell data
  vtkNew<vtkFloatArray> floatArray;

  // Modify point data arrays
  vtkMTimeType initialMeshMTime = dataset->GetMeshMTime();
  dataset->GetPointData()->AddArray(floatArray);
  vtkMTimeType newMeshMTime = dataset->GetMeshMTime();

  result &= CompareTimes(initialMeshMTime, newMeshMTime, expectMeshMTimeUpdate, "point data");

  // Modify cell data arrays
  initialMeshMTime = dataset->GetMeshMTime();
  dataset->GetCellData()->AddArray(floatArray);
  newMeshMTime = dataset->GetMeshMTime();

  result &= CompareTimes(initialMeshMTime, newMeshMTime, expectMeshMTimeUpdate, "cell data");

  return result;
}

int TestPolyData()
{
  vtkLogScopeF(INFO, "Test vtkPolyData");

  int result = EXIT_SUCCESS;

  // Create dummy dataset
  float point[3] = { 0, 0, 0 };
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(1);
  points->SetPoint(0, point);

  vtkNew<vtkPolyData> polydata;
  polydata->SetPoints(points);

  // Modifying points/lines/verts/polys themselves should.
  // Modifying field data shouldn't modify MeshMTime;
  result &= TestFieldDataModification(polydata, false);

  // Modify lines
  vtkMTimeType initialMeshMTime = polydata->GetMeshMTime();
  polydata->SetLines(vtkNew<vtkCellArray>());
  vtkMTimeType newMeshMTime = polydata->GetMeshMTime();

  result &= CompareTimes(initialMeshMTime, newMeshMTime, true, "lines");

  // Modify strips
  initialMeshMTime = polydata->GetMeshMTime();
  polydata->SetStrips(vtkNew<vtkCellArray>());
  newMeshMTime = polydata->GetMeshMTime();

  result &= CompareTimes(initialMeshMTime, newMeshMTime, true, "strips");

  // Modify polys
  initialMeshMTime = polydata->GetMeshMTime();
  polydata->SetPolys(vtkNew<vtkCellArray>());
  newMeshMTime = polydata->GetMeshMTime();

  result &= CompareTimes(initialMeshMTime, newMeshMTime, true, "polys");

  // Modify verts
  initialMeshMTime = polydata->GetMeshMTime();
  polydata->SetVerts(vtkNew<vtkCellArray>());
  newMeshMTime = polydata->GetMeshMTime();

  result &= CompareTimes(initialMeshMTime, newMeshMTime, true, "verts");

  return result;
}

int TestUnstructuredGrid()
{
  vtkLogScopeF(INFO, "Test vtkUnstructuredGrid");

  int result = EXIT_SUCCESS;

  float point[3] = { 0, 0, 0 };
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(1);
  points->SetPoint(0, point);

  // Modifying field data shouldn't modify MeshMTime;
  // Modifying connectivity should.
  vtkNew<vtkUnstructuredGrid> grid;
  grid->SetPoints(points);

  result &= TestFieldDataModification(grid, false);

  // Modify connectivity
  vtkMTimeType initialMeshMTime = grid->GetMeshMTime();
  grid->SetCells(VTK_TRIANGLE, vtkNew<vtkCellArray>());
  vtkMTimeType newMeshMTime = grid->GetMeshMTime();

  result &= CompareTimes(initialMeshMTime, newMeshMTime, true, "connectivity");

  return result;
}

int TestDefaultImplementation(vtkDataSet* noMeshMTimeImplementationDataset)
{
  vtkLogScopeF(INFO, "Test default implementation");

  int result = EXIT_SUCCESS;
  result &= TestFieldDataModification(noMeshMTimeImplementationDataset, true);
  return result;
}
}

//------------------------------------------------------------------------------
int TestMeshMTime(int, char*[])
{
  int result = EXIT_SUCCESS;

  result &= TestPolyData();
  result &= TestUnstructuredGrid();

  // Note: Image data uses the default implementation; this might change in the future
  vtkNew<vtkImageData> imageData;
  result &= TestDefaultImplementation(imageData);

  return result;
}
