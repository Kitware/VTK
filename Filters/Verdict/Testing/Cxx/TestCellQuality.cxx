// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellQuality.h"
#include "vtkDoubleArray.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPixel.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

#include "vtkXMLRectilinearGridWriter.h"
#include "vtkXMLUnstructuredGridWriter.h"

#include <array>

namespace
{

//-----------------------------------------------------------------------------
// Generate a rectilinear grid which contains only pixels.
vtkSmartPointer<vtkRectilinearGrid> GenerateRectilinearGrid()
{
  vtkLog(INFO, "Generate vtkRectilinearGrid");
  vtkNew<vtkRectilinearGrid> grid;
  double x[10] = { 0.0, 0.5, 1.0, 2.0, 4.0, 6.0, 8.0, 9.0, 9.5, 10.0 };
  vtkNew<vtkDoubleArray> xCoords;
  for (int i = 0; i < 10; i++)
  {
    xCoords->InsertNextValue(x[i]);
  }

  double y[4] = { 0.0, 1.0, 4.0, 9.0 };
  vtkNew<vtkDoubleArray> yCoords;
  for (int i = 0; i < 4; i++)
  {
    yCoords->InsertNextValue(y[i]);
  }

  grid->SetDimensions(10, 4, 0);
  grid->SetXCoordinates(xCoords);
  grid->SetYCoordinates(yCoords);

  grid->SetExtent(0, 5, 0, 3, 0, 0);

  return grid;
}

//-----------------------------------------------------------------------------
// Generate an unstructured grid which contains only triangle strip to represent a quad.
vtkSmartPointer<vtkUnstructuredGrid> GenerateUG()
{
  vtkLog(INFO, "Generate vtkUnstructuredGrid");

  vtkNew<vtkPoints> points;
  points->InsertNextPoint(0.0, 0.0, 0.0);
  points->InsertNextPoint(1.0, 0.0, 0.0);
  points->InsertNextPoint(1.25, 1.5, 0.0);
  points->InsertNextPoint(0.0, 1.0, 0.0);

  vtkIdType triangleStrip[3] = { 0, 1, 3 };
  vtkIdType triangleStrip2[3] = { 3, 1, 2 };

  vtkNew<vtkUnstructuredGrid> ug;
  ug->SetPoints(points);
  ug->InsertNextCell(VTK_TRIANGLE_STRIP, 3, triangleStrip);
  ug->InsertNextCell(VTK_TRIANGLE_STRIP, 3, triangleStrip2);
  return ug;
}

//-----------------------------------------------------------------------------
// Generate an unstructured grid which contains unsupported cell
vtkSmartPointer<vtkUnstructuredGrid> GenerateUnsupportedCell()
{
  vtkLog(INFO, "Generate Unsupported Cell");

  vtkNew<vtkPoints> points;

  points->InsertNextPoint(0.0, 0.0, 0.0);
  points->InsertNextPoint(1.0, 0.0, 0.0);
  points->InsertNextPoint(1.0, 1.0, 0.0);
  points->InsertNextPoint(0.0, 1.0, 0.0);
  points->InsertNextPoint(0.0, 0.0, 1.0);
  points->InsertNextPoint(1.0, 0.0, 1.0);
  points->InsertNextPoint(1.0, 1.0, 1.0);
  points->InsertNextPoint(0.0, 1.0, 1.0);

  vtkNew<vtkUnstructuredGrid> unstructuredGrid;
  unstructuredGrid->SetPoints(points);

  vtkIdType indices[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

  unstructuredGrid->InsertNextCell(VTK_VOXEL, 8, indices);

  return unstructuredGrid;
}

//-----------------------------------------------------------------------------
// Compute vtkCellQuality on a data containing only VTK_PIXEL
bool CheckAreaMetricForPixel()
{
  vtkLog(INFO, "CheckAreaMetricForPixel");
  bool areaIsValid = true;

  auto grid = ::GenerateRectilinearGrid();

  vtkLog(INFO, "Compute the vtkCellQuality for Area metrics");
  vtkNew<vtkCellQuality> cellQualityFilter;
  cellQualityFilter->SetInputData(grid);
  cellQualityFilter->SetQualityMeasureToArea();
  cellQualityFilter->Update();

  vtkLog(INFO, "Verify metrics outputted on some indices");
  std::array<int, 5> indicesChecked = { 0, 2, 4, 9, 14 };
  std::array<double, 5> expectedValues = { 0.5, 1, 2, 6, 10 };

  auto* resultGrid = vtkRectilinearGrid::SafeDownCast(cellQualityFilter->GetOutputDataObject(0));
  auto* qualities =
    vtkDoubleArray::SafeDownCast(resultGrid->GetCellData()->GetArray("CellQuality"));

  for (int i = 0; i < 5; i++)
  {
    vtkIdType indiceChecked = indicesChecked[i];
    if (qualities->GetValue(indiceChecked) != expectedValues[i])
    {
      vtkLog(ERROR,
        "Wrong result at index " << indiceChecked << ". Expected " << expectedValues[i]
                                 << " but got " << qualities->GetValue(indiceChecked));
      areaIsValid = false;
    }
  }

  return areaIsValid;
}

//-----------------------------------------------------------------------------
// Compute vtkCellQuality on a data containing only VTK_TRIANGLE_STRIP
bool CheckAreaMetricForTriangleStrip()
{
  vtkLog(INFO, "CheckAreaMetricForTriangleStrip");
  bool areaIsValid = true;

  auto ug = ::GenerateUG();

  vtkLog(INFO, "Compute the vtkCellQuality for Area metrics");
  vtkNew<vtkCellQuality> cellQualityFilter;
  cellQualityFilter->SetInputData(ug);
  cellQualityFilter->SetQualityMeasureToArea();
  cellQualityFilter->Update();

  vtkLog(INFO, "Verify metrics outputted on some indices");
  std::array<double, 5> expectedValues = { 0.5, 0.875 };

  auto* result = vtkUnstructuredGrid::SafeDownCast(cellQualityFilter->GetOutputDataObject(0));
  auto* qualities = vtkDoubleArray::SafeDownCast(result->GetCellData()->GetArray("CellQuality"));

  for (int i = 0; i < 2; i++)
  {
    if (qualities->GetValue(i) != expectedValues[i])
    {
      vtkLog(ERROR,
        "Wrong result at index " << i << ". Expected " << expectedValues[i] << " but got "
                                 << qualities->GetValue(i));
      areaIsValid = false;
    }
  }

  return areaIsValid;
}

//-----------------------------------------------------------------------------
// Check that unsupported quality measure will output the correct default value set by the user
bool CheckUnsupportedQuality()
{
  vtkLog(INFO, "CheckUnsupportedQuality");
  bool unsupportedMetricIsValid = true;

  auto grid = ::GenerateRectilinearGrid();

  vtkLog(INFO, "Compute the vtkCellQuality with unsupported cells");
  vtkNew<vtkCellQuality> cellQualityFilter;
  cellQualityFilter->SetInputData(grid);
  // arbitrary value used here different to the default one (which is -1)
  double undefinedMetricValue = -2;
  cellQualityFilter->SetUndefinedQuality(undefinedMetricValue);
  cellQualityFilter->SetQualityMeasureToAspectRatio();
  cellQualityFilter->Update();

  auto* resultGrid = vtkRectilinearGrid::SafeDownCast(cellQualityFilter->GetOutputDataObject(0));
  auto* qualities =
    vtkDoubleArray::SafeDownCast(resultGrid->GetCellData()->GetArray("CellQuality"));

  for (vtkIdType i = 0; i < qualities->GetNumberOfValues(); i++)
  {
    if (qualities->GetValue(i) != undefinedMetricValue)
    {
      vtkLog(ERROR,
        "Wrong result at index " << i << ". Expected " << undefinedMetricValue << " but got "
                                 << qualities->GetValue(i));
      unsupportedMetricIsValid = false;
    }
  }

  return unsupportedMetricIsValid;
}

//-----------------------------------------------------------------------------
// Check that unsupported quality measure will output the correct default value set by the user
bool CheckUnsupportedCell()
{
  vtkLog(INFO, "CheckUnsupportedCell");
  bool unsupportedMetricIsValid = true;

  auto grid = ::GenerateUnsupportedCell();

  vtkNew<vtkCellQuality> cellQualityFilter;
  cellQualityFilter->SetInputData(grid);
  // arbitrary value used here different to the default one (which is -1)
  double unsupportedCell = -2;
  cellQualityFilter->SetUnsupportedGeometry(unsupportedCell);
  cellQualityFilter->SetQualityMeasureToAspectRatio();
  cellQualityFilter->Update();

  auto* resultGrid = vtkUnstructuredGrid::SafeDownCast(cellQualityFilter->GetOutputDataObject(0));
  auto* qualities =
    vtkDoubleArray::SafeDownCast(resultGrid->GetCellData()->GetArray("CellQuality"));

  for (vtkIdType i = 0; i < qualities->GetNumberOfValues(); i++)
  {
    if (qualities->GetValue(i) != unsupportedCell)
    {
      vtkLog(ERROR,
        "Wrong result at index " << i << ". Expected " << unsupportedCell << " but got "
                                 << qualities->GetValue(i));
      unsupportedMetricIsValid = false;
    }
  }

  return unsupportedMetricIsValid;
}

}

//-----------------------------------------------------------------------------
int TestCellQuality(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  bool succeed = true;
  succeed &= ::CheckAreaMetricForPixel();
  succeed &= ::CheckUnsupportedQuality();
  succeed &= ::CheckAreaMetricForTriangleStrip();
  succeed &= ::CheckUnsupportedCell();

  if (succeed)
  {
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}
