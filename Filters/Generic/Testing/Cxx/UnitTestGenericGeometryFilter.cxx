/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestGenericGeometryFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPointer.h"

#include "vtkGenericGeometryFilter.h"
#include "vtkBridgeDataSet.h"
#include "vtkPlaneSource.h"
#include "vtkVertex.h"
#include "vtkTetra.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPointLocator.h"

#include "vtkTestErrorObserver.h"

#include <sstream>

static vtkSmartPointer<vtkBridgeDataSet> CreatePolyData(const int xres, const int yres);
static vtkSmartPointer<vtkBridgeDataSet> CreateVertexData();
static vtkSmartPointer<vtkBridgeDataSet> CreateTetraData();

int UnitTestGenericGeometryFilter(int, char*[])
{
  const int xres = 20, yres = 10;
  int status = EXIT_SUCCESS;
  {
  std::cout << "Testing empty print...";
  vtkSmartPointer<vtkGenericGeometryFilter> filter =
    vtkSmartPointer<vtkGenericGeometryFilter>::New();
  std::ostringstream emptyPrint;
  filter->Print(emptyPrint);
  std::cout << "PASSED." << std::endl;
  }
  {
  std::cout << "Testing default settings...";
  vtkSmartPointer<vtkGenericGeometryFilter> filter =
    vtkSmartPointer<vtkGenericGeometryFilter>::New();
  filter->SetInputData (CreatePolyData(xres, yres));
  filter->Update();
  int got = filter->GetOutput()->GetNumberOfCells();
  std::cout << "# of cells: " << got;
  int expected = xres * yres;
  if (expected != got)
  {
    std::cout << " Expected " << expected << " cells"
              << " but got " << got << " cells."
              << " FAILED." << std::endl;
    status++;
  }
  else
  {
    std::cout << " PASSED." << std::endl;
  }
  }
  {
  std::cout << "Testing PointClippingOn()...";
  vtkSmartPointer<vtkPointLocator> locator =
    vtkSmartPointer<vtkPointLocator>::New();
  vtkSmartPointer<vtkGenericGeometryFilter> filter =
    vtkSmartPointer<vtkGenericGeometryFilter>::New();
  filter->SetInputData (CreatePolyData(xres, yres));
  filter->SetLocator(locator);
  filter->MergingOff();
  filter->PointClippingOn();
  filter->CellClippingOff();
  filter->ExtentClippingOff();
  filter->SetPointMinimum(0);
  filter->SetPointMaximum((xres+1)*(yres+1)-1);
  filter->Update();
  int got = filter->GetOutput()->GetNumberOfCells();
  std::cout << "# of cells: " << got;
  int expected = xres * yres;
  if (expected != got)
  {
    std::cout << " Expected " << expected << " cells"
              << " but got " << got << " cells."
              << " FAILED." << std::endl;
    status++;
  }
  else
  {
    std::cout << " PASSED." << std::endl;
  }
  std::ostringstream fullPrint;
  filter->Print(fullPrint);

  }
  {
  std::cout << "Testing CellClippingOn()...";
  vtkSmartPointer<vtkGenericGeometryFilter> filter =
    vtkSmartPointer<vtkGenericGeometryFilter>::New();
  filter->SetInputData (CreatePolyData(xres, yres));
  filter->PointClippingOff();
  filter->CellClippingOn();
  filter->ExtentClippingOff();
  filter->SetCellMinimum(xres);
  filter->SetCellMaximum(xres + 9);
  filter->Update();
  int got = filter->GetOutput()->GetNumberOfCells();
  std::cout << "# of cells: " << got;
  int expected = filter->GetCellMaximum() - filter->GetCellMinimum() + 1;
  if (expected != got)
  {
    std::cout << " Expected " << expected << " cells"
              << " but got " << got << " cells."
              << " FAILED" << std::endl;
    status++;
  }
  else
  {
    std::cout << " PASSED." << std::endl;
  }
  }
  {
  std::cout << "Testing ExtentClippingOn()...";
  vtkSmartPointer<vtkGenericGeometryFilter> filter =
    vtkSmartPointer<vtkGenericGeometryFilter>::New();
  filter->MergingOn();
  filter->SetInputData (CreatePolyData(xres, yres));
  filter->PointClippingOff();
  filter->CellClippingOff();
  filter->ExtentClippingOn();
  filter->PassThroughCellIdsOn();
  filter->SetExtent(.4, -.4, .4, -.4, .4, -.4);
  filter->SetExtent(-.499, .499, -.499, .499, 0.0, 0.0);
  filter->SetExtent(-.499, .499, -.499, .499, 0.0, 0.0);
  filter->Update();
  int got = filter->GetOutput()->GetNumberOfCells();
  std::cout << "# of cells: " << got;
  int expected = (xres * yres) - 2 * xres - 2 * (yres - 2);
  if (expected != got)
  {
    std::cout << " Expected " << expected << " cells"
              << " but got " << got << " cells."
              << " FAILED." << std::endl;
    status++;
  }
  else if (filter->GetOutput()->GetCellData()->GetArray("vtkOriginalCellIds") == NULL)
  {
    std::cout << " PassThroughCellIdsOn should produce vtkOriginalCellIds, but did not." << std::endl;
    std::cout << " FAILED." << std::endl;
    status++;
  }
  else
  {
    std::cout << " PASSED." << std::endl;
  }
  }
  {
  std::cout << "Testing with TetraData...";
  vtkSmartPointer<vtkGenericGeometryFilter> filter =
    vtkSmartPointer<vtkGenericGeometryFilter>::New();
  filter->SetInputData (CreateTetraData());
  filter->PointClippingOff();
  filter->CellClippingOff();
  filter->ExtentClippingOff();
  filter->PassThroughCellIdsOn();
  filter->Update();

  int got = filter->GetOutput()->GetNumberOfCells();
  std::cout << "# of cells: " << got;
  int expected = 4;
  if (expected != got)
  {
    std::cout << " Expected " << expected << " cells"
              << " but got " << got << " cells."
              << " FAILED." << std::endl;
    status++;
  }
  else
  {
    std::cout << " PASSED." << std::endl;
  }
  }
  {
  std::cout << "Testing errors...";
  vtkSmartPointer<vtkTest::ErrorObserver>  errorObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();

  vtkSmartPointer<vtkGenericGeometryFilter> filter =
    vtkSmartPointer<vtkGenericGeometryFilter>::New();
  filter->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  filter->SetInputData (vtkSmartPointer<vtkBridgeDataSet>::New());
  filter->Update();
  status += errorObserver->CheckErrorMessage("Number of cells is zero, no data to process.");

  filter->SetInputData (CreateVertexData());
  filter->Update();
  status += errorObserver->CheckErrorMessage("Cell of dimension 0 not handled yet.");

  if (status)
  {
    std::cout << "FAILED." << std::endl;
  }
  else
  {
    std::cout << "PASSED." << std::endl;
  }
  }
  return status;
}

vtkSmartPointer<vtkBridgeDataSet> CreatePolyData(const int xres, const int yres)
{
  vtkSmartPointer<vtkPlaneSource> plane =
    vtkSmartPointer<vtkPlaneSource>::New();
  plane->SetXResolution(xres);
  plane->SetYResolution(yres);
  plane->Update();
  vtkSmartPointer<vtkIntArray> cellData =
    vtkSmartPointer<vtkIntArray>::New();
  cellData->SetNumberOfTuples(xres * yres);
  cellData->SetName("CellDataTestArray");
  vtkIdType c = 0;
  for (int j = 0; j < yres; ++j)
  {
    for (int i = 0; i <xres; ++i)
    {
      cellData->SetTuple1(c++, i);
    }
  }
  vtkSmartPointer<vtkIntArray> pointData =
    vtkSmartPointer<vtkIntArray>::New();
  pointData->SetNumberOfTuples((xres + 1) * (yres + 1));
  pointData->SetName("PointDataTestArray");
  c = 0;
  for (int j = 0; j < yres + 1; ++j)
  {
    for (int i = 0; i <xres + 1; ++i)
    {
        pointData->SetTuple1(c++, i);
    }
  }

  vtkSmartPointer<vtkPolyData> pd =
    vtkSmartPointer<vtkPolyData>::New();
  pd = plane->GetOutput();
  pd->GetPointData()->SetScalars(pointData);
  pd->GetCellData()->SetScalars(cellData);

  vtkSmartPointer<vtkBridgeDataSet> bridge =
    vtkSmartPointer<vtkBridgeDataSet>::New();
  bridge->SetDataSet(plane->GetOutput());

  return bridge;
}
vtkSmartPointer<vtkBridgeDataSet> CreateVertexData()
{
  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>::New();
  points->InsertNextPoint(0,0,0);

  vtkSmartPointer<vtkVertex> vertex =
    vtkSmartPointer<vtkVertex>::New();
  vertex->GetPointIds()->SetId(0, 0);

  vtkSmartPointer<vtkCellArray> vertices =
    vtkSmartPointer<vtkCellArray>::New();
  vertices->InsertNextCell(vertex);

  vtkSmartPointer<vtkPolyData> polydata =
    vtkSmartPointer<vtkPolyData>::New();
  polydata->SetPoints(points);
  polydata->SetVerts(vertices);

  vtkSmartPointer<vtkBridgeDataSet> bridge =
    vtkSmartPointer<vtkBridgeDataSet>::New();
  bridge->SetDataSet(polydata);

  return bridge;
}

vtkSmartPointer<vtkBridgeDataSet> CreateTetraData()
{
  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>:: New();
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(1, 0, 0);
  points->InsertNextPoint(1, 1, 0);
  points->InsertNextPoint(0, 1, 1);
  points->InsertNextPoint(5, 5, 5);
  points->InsertNextPoint(6, 5, 5);
  points->InsertNextPoint(6, 6, 5);
  points->InsertNextPoint(5, 6, 6);

  vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid =
    vtkSmartPointer<vtkUnstructuredGrid>::New();
  unstructuredGrid->SetPoints(points);

  vtkSmartPointer<vtkTetra> tetra =
    vtkSmartPointer<vtkTetra>::New();
  tetra->GetPointIds()->SetId(0, 4);
  tetra->GetPointIds()->SetId(1, 5);
  tetra->GetPointIds()->SetId(2, 6);
  tetra->GetPointIds()->SetId(3, 7);

  vtkSmartPointer<vtkCellArray> cellArray =
    vtkSmartPointer<vtkCellArray>::New();
  cellArray->InsertNextCell(tetra);
  unstructuredGrid->SetCells(VTK_TETRA, cellArray);

  vtkSmartPointer<vtkIntArray> pointData =
    vtkSmartPointer<vtkIntArray>::New();
  pointData->SetNumberOfTuples(unstructuredGrid->GetNumberOfPoints());
  pointData->SetName("PointDataTestArray");
  int c = 0;
  for (vtkIdType id = 0; id < tetra->GetNumberOfPoints(); ++id)
  {
    pointData->SetTuple1(c++, id);
  }
  unstructuredGrid->GetPointData()->SetScalars(pointData);

  vtkSmartPointer<vtkBridgeDataSet> bridge =
    vtkSmartPointer<vtkBridgeDataSet>::New();
  bridge->SetDataSet(unstructuredGrid);

  return bridge;
}
