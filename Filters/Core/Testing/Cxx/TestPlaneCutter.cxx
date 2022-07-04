/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCutter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataSetTriangleFilter.h"
#include "vtkImageDataToPointSet.h"
#include "vtkMappedUnstructuredGridGenerator.h"
#include "vtkPlane.h"
#include "vtkPlaneCutter.h"
#include "vtkPointDataToCellData.h"
#include "vtkPolyData.h"
#include "vtkRTAnalyticSource.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridBase.h"

#define Compare(output, expected)                                                                  \
  do                                                                                               \
  {                                                                                                \
    if (output->GetNumberOfCells() != expected)                                                    \
    {                                                                                              \
      cerr << "Test " << __FUNCTION__ << " expected " << expected << " cells, got "                \
           << output->GetNumberOfCells() << endl;                                                  \
      return false;                                                                                \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      cout << "Test " << __FUNCTION__ << " succeeded with " << output->GetNumberOfCells()          \
           << " cells." << endl;                                                                   \
    }                                                                                              \
  } while (false)

bool TestPlaneCutterStructured(int type, int expected)
{
  vtkSmartPointer<vtkRTAnalyticSource> imageSource = vtkSmartPointer<vtkRTAnalyticSource>::New();
  imageSource->SetWholeExtent(-2, 2, -2, 2, -2, 2);

  vtkSmartPointer<vtkAlgorithm> filter;
  if (type == 0)
  {
    filter = imageSource;
  }
  else
  {
    filter = vtkSmartPointer<vtkImageDataToPointSet>::New();
    filter->SetInputConnection(imageSource->GetOutputPort());
  }

  vtkSmartPointer<vtkPlaneCutter> cutter = vtkSmartPointer<vtkPlaneCutter>::New();
  vtkSmartPointer<vtkPlane> p3d = vtkSmartPointer<vtkPlane>::New();
  p3d->SetOrigin(-1.5, -1.5, -1.5);
  p3d->SetNormal(1, 1, 1);

  cutter->SetPlane(p3d);
  cutter->SetInputConnection(0, filter->GetOutputPort());

  cutter->SetGeneratePolygons(true);
  cutter->Update();
  vtkPolyData* output = vtkPolyData::SafeDownCast(cutter->GetOutputDataObject(0));
  Compare(output, expected);

  cutter->SetGeneratePolygons(false);
  cutter->Update();
  output = vtkPolyData::SafeDownCast(cutter->GetOutputDataObject(0));
  Compare(output, 7);
  return true;
}

bool TestPlaneCutterUnmapped(int expected)
{
  vtkUnstructuredGrid* mg;
  vtkMappedUnstructuredGridGenerator::GenerateUnstructuredGrid(&mg);

  vtkSmartPointer<vtkPlaneCutter> cutter = vtkSmartPointer<vtkPlaneCutter>::New();
  vtkSmartPointer<vtkPlane> p3d = vtkSmartPointer<vtkPlane>::New();
  p3d->SetOrigin(0.25, 0, 0);
  p3d->SetNormal(1.0, 0.0, 0.0);

  cutter->SetPlane(p3d);
  cutter->SetInputData(mg);

  cutter->SetGeneratePolygons(true);
  cutter->Update();
  mg->Delete();

  vtkPolyData* output = vtkPolyData::SafeDownCast(cutter->GetOutputDataObject(0));
  Compare(output, expected);

  cutter->SetGeneratePolygons(false);
  cutter->Update();
  output = vtkPolyData::SafeDownCast(cutter->GetOutputDataObject(0));
  Compare(output, expected);
  return true;
}

bool TestPlaneCutterMapped(int expected)
{
  vtkUnstructuredGridBase* mg;
  vtkMappedUnstructuredGridGenerator::GenerateMappedUnstructuredGrid(&mg);

  vtkSmartPointer<vtkPlaneCutter> cutter = vtkSmartPointer<vtkPlaneCutter>::New();
  vtkSmartPointer<vtkPlane> p3d = vtkSmartPointer<vtkPlane>::New();
  p3d->SetOrigin(0.25, 0, 0);
  p3d->SetNormal(1.0, 0.0, 0.0);

  cutter->SetPlane(p3d);
  cutter->SetInputData(mg);

  cutter->SetGeneratePolygons(true);
  cutter->Update();
  mg->Delete();

  vtkPolyData* output = vtkPolyData::SafeDownCast(cutter->GetOutputDataObject(0));
  Compare(output, expected);

  cutter->SetGeneratePolygons(false);
  cutter->Update();
  output = vtkPolyData::SafeDownCast(cutter->GetOutputDataObject(0));
  Compare(output, expected);
  return true;
}

bool TestPlaneCutterUnstructured(int expected)
{
  vtkSmartPointer<vtkRTAnalyticSource> imageSource = vtkSmartPointer<vtkRTAnalyticSource>::New();
  imageSource->SetWholeExtent(-2, 2, -2, 2, -2, 2);

  vtkSmartPointer<vtkPointDataToCellData> dataFilter =
    vtkSmartPointer<vtkPointDataToCellData>::New();
  dataFilter->SetInputConnection(imageSource->GetOutputPort());

  vtkSmartPointer<vtkDataSetTriangleFilter> tetraFilter =
    vtkSmartPointer<vtkDataSetTriangleFilter>::New();
  tetraFilter->SetInputConnection(dataFilter->GetOutputPort());

  vtkSmartPointer<vtkPlaneCutter> cutter = vtkSmartPointer<vtkPlaneCutter>::New();
  vtkSmartPointer<vtkPlane> p3d = vtkSmartPointer<vtkPlane>::New();
  p3d->SetOrigin(-1.5, -1.5, -1.5);
  p3d->SetNormal(1, 1, 1);

  cutter->SetPlane(p3d);
  cutter->SetInputConnection(0, tetraFilter->GetOutputPort());

  // vtkPlaneCutter does not support polygon generation for unstructured grids
  // so the number of expected cells is always the number of triangles.
  cutter->SetGeneratePolygons(true);
  cutter->Update();
  vtkPolyData* output = vtkPolyData::SafeDownCast(cutter->GetOutputDataObject(0));
  Compare(output, expected);

  cutter->SetGeneratePolygons(false);
  cutter->Update();
  output = vtkPolyData::SafeDownCast(cutter->GetOutputDataObject(0));
  Compare(output, expected);
  return true;
}

int TestPlaneCutter(int, char*[])
{
  for (int type = 0; type < 2; type++)
  {
    if (!TestPlaneCutterStructured(type, 4))
    {
      cerr << "Cutting Structured failed" << endl;
      return EXIT_FAILURE;
    }
  }

  if (!TestPlaneCutterUnstructured(10))
  {
    cerr << "Cutting Unstructured failed" << endl;
    return EXIT_FAILURE;
  }

  if (!TestPlaneCutterUnmapped(6))
  {
    cerr << "Cutting Mapped Unstructured failed" << endl;
    return EXIT_FAILURE;
  }

  if (!TestPlaneCutterMapped(6))
  {
    cerr << "Cutting Mapped Unstructured failed" << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
