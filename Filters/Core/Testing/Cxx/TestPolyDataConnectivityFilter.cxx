/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPolyDataConnectivityFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkAppendPolyData.h>
#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkPointData.h>
#include <vtkPolyDataConnectivityFilter.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>

namespace
{
void InitializePolyData(vtkPolyData* polyData, int dataType)
{
  vtkSmartPointer<vtkMinimalStandardRandomSequence> randomSequence =
    vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> verts = vtkSmartPointer<vtkCellArray>::New();
  verts->InsertNextCell(4);
  vtkSmartPointer<vtkFloatArray> scalars = vtkSmartPointer<vtkFloatArray>::New();

  if (dataType == VTK_DOUBLE)
  {
    points->SetDataType(VTK_DOUBLE);
    for (unsigned int i = 0; i < 4; ++i)
    {
      randomSequence->Next();
      scalars->InsertNextValue(randomSequence->GetValue());
      double point[3];
      for (unsigned int j = 0; j < 3; ++j)
      {
        randomSequence->Next();
        point[j] = randomSequence->GetValue();
      }
      verts->InsertCellPoint(points->InsertNextPoint(point));
    }
  }
  else
  {
    points->SetDataType(VTK_FLOAT);
    for (unsigned int i = 0; i < 4; ++i)
    {
      randomSequence->Next();
      scalars->InsertNextValue(randomSequence->GetValue());
      float point[3];
      for (unsigned int j = 0; j < 3; ++j)
      {
        randomSequence->Next();
        point[j] = static_cast<float>(randomSequence->GetValue());
      }
      verts->InsertCellPoint(points->InsertNextPoint(point));
    }
  }

  scalars->Squeeze();
  polyData->GetPointData()->SetScalars(scalars);
  points->Squeeze();
  polyData->SetPoints(points);
  verts->Squeeze();
  polyData->SetVerts(verts);
}

int FilterPolyDataConnectivity(int dataType, int outputPointsPrecision)
{
  vtkSmartPointer<vtkPolyData> inputPolyData = vtkSmartPointer<vtkPolyData>::New();
  InitializePolyData(inputPolyData, dataType);

  vtkSmartPointer<vtkPolyDataConnectivityFilter> polyDataConnectivityFilter =
    vtkSmartPointer<vtkPolyDataConnectivityFilter>::New();
  polyDataConnectivityFilter->SetOutputPointsPrecision(outputPointsPrecision);
  polyDataConnectivityFilter->ScalarConnectivityOn();
  polyDataConnectivityFilter->SetScalarRange(0.25, 0.75);
  polyDataConnectivityFilter->SetInputData(inputPolyData);

  polyDataConnectivityFilter->Update();

  vtkSmartPointer<vtkPolyData> outputPolyData = polyDataConnectivityFilter->GetOutput();
  vtkSmartPointer<vtkPoints> points = outputPolyData->GetPoints();

  return points->GetDataType();
}

bool MarkVisitedPoints()
{
  // Set up two disconnected spheres.
  vtkNew<vtkSphereSource> sphere1;
  sphere1->SetCenter(-1, 0, 0);
  sphere1->Update();
  vtkIdType numPtsSphere1 = sphere1->GetOutput()->GetNumberOfPoints();

  vtkNew<vtkSphereSource> sphere2;
  sphere2->SetCenter(1, 0, 0);
  sphere2->SetPhiResolution(32);

  vtkNew<vtkAppendPolyData> spheres;
  spheres->SetInputConnection(sphere1->GetOutputPort());
  spheres->AddInputConnection(sphere2->GetOutputPort());
  spheres->Update();

  // Test VTK_EXTRACT_CLOSEST_POINT_REGION mode.
  // Select the one with the highest points IDS so we can ensure marked visited points
  // use the original indices.
  vtkPolyDataConnectivityFilter* connectivity = vtkPolyDataConnectivityFilter::New();
  connectivity->SetInputConnection(spheres->GetOutputPort());
  connectivity->SetExtractionModeToClosestPointRegion();
  connectivity->SetClosestPoint(1, 0, 0);
  connectivity->MarkVisitedPointIdsOn();
  connectivity->Update();

  // Check that the marked point IDs fall in the range of the second sphere, which is closest.
  vtkIdList* visitedPts = connectivity->GetVisitedPointIds();
  for (vtkIdType id = 0; id < visitedPts->GetNumberOfIds(); ++id)
  {
    vtkIdType visitedPt = visitedPts->GetId(id);
    if (visitedPts->GetId(id) < numPtsSphere1)
    {
      std::cerr << "Visited point id " << visitedPt << " is from sphere1 and not sphere2 "
                << "in VTK_EXTRACT_CLOSEST_POINT_REGION mode." << std::endl;
      return false;
    }
  }

  connectivity->Delete();

  // Test VTK_EXTRACT_SPECIFIED_REGIONS mode.
  connectivity = vtkPolyDataConnectivityFilter::New();
  connectivity->SetInputConnection(spheres->GetOutputPort());
  connectivity->SetExtractionModeToSpecifiedRegions();
  connectivity->InitializeSpecifiedRegionList();
  connectivity->AddSpecifiedRegion(1);
  connectivity->Update();

  // Check that the marked point IDs fall in the range of the second sphere, which is region 1.
  bool succeeded = true;
  visitedPts = connectivity->GetVisitedPointIds();
  for (vtkIdType id = 0; id < visitedPts->GetNumberOfIds(); ++id)
  {
    vtkIdType visitedPt = visitedPts->GetId(id);
    if (visitedPt < numPtsSphere1)
    {
      std::cerr << "Visited point id " << visitedPt << " is from sphere1 and not sphere2 "
                << "in VTK_EXTRACT_SPECIFIED_REGIONS mode." << std::endl;
      succeeded = false;
    }
  }

  connectivity->Delete();

  // Test VTK_EXTRACT_LARGEST_REGION mode.
  connectivity = vtkPolyDataConnectivityFilter::New();
  connectivity->SetInputConnection(spheres->GetOutputPort());
  connectivity->SetExtractionModeToLargestRegion();
  connectivity->Update();

  // Check that the marked point IDs fall in the range of the second sphere, which is biggest.
  visitedPts = connectivity->GetVisitedPointIds();
  for (vtkIdType id = 0; id < visitedPts->GetNumberOfIds(); ++id)
  {
    vtkIdType visitedPt = visitedPts->GetId(id);
    if (visitedPt < numPtsSphere1)
    {
      std::cerr << "Visited point id " << visitedPt << " is from sphere1 and not sphere2 "
                << "in VTK_EXTRACT_SPECIFIED_REGIONS mode." << std::endl;
      succeeded = false;
    }
  }

  connectivity->Delete();

  return succeeded;
}
}

int TestPolyDataConnectivityFilter(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int dataType = FilterPolyDataConnectivity(VTK_FLOAT, vtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = FilterPolyDataConnectivity(VTK_DOUBLE, vtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = FilterPolyDataConnectivity(VTK_FLOAT, vtkAlgorithm::SINGLE_PRECISION);

  if (dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = FilterPolyDataConnectivity(VTK_DOUBLE, vtkAlgorithm::SINGLE_PRECISION);

  if (dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = FilterPolyDataConnectivity(VTK_FLOAT, vtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = FilterPolyDataConnectivity(VTK_DOUBLE, vtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  if (!MarkVisitedPoints())
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
