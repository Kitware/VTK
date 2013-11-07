/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHedgeHog.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkCellArray.h>
#include <vtkHedgeHog.h>
#include <vtkFloatArray.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>

namespace
{
void InitializeUnstructuredGrid(vtkUnstructuredGrid *unstructuredGrid, int dataType)
{
  vtkSmartPointer<vtkMinimalStandardRandomSequence> randomSequence
    = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
  cells->InsertNextCell(4);
  vtkSmartPointer<vtkFloatArray> vectors = vtkSmartPointer<vtkFloatArray>::New();
  vectors->SetNumberOfComponents(3);

  if(dataType == VTK_DOUBLE)
    {
    points->SetDataType(VTK_DOUBLE);
    for(unsigned int i = 0; i < 4; ++i)
      {
      double vector[3];
      for(unsigned int j = 0; j < 3; ++j)
        {
        randomSequence->Next();
        vector[j] = randomSequence->GetValue();
        }
      vectors->InsertNextTuple(vector);
      double point[3];
      for(unsigned int j = 0; j < 3; ++j)
        {
        randomSequence->Next();
        point[j] = randomSequence->GetValue();
        }
      cells->InsertCellPoint(points->InsertNextPoint(point));
      }
    }
  else
    {
    points->SetDataType(VTK_FLOAT);
    for(unsigned int i = 0; i < 4; ++i)
      {
      float vector[3];
      for(unsigned int j = 0; j < 3; ++j)
        {
        randomSequence->Next();
        vector[j] = static_cast<float>(randomSequence->GetValue());
        }
      vectors->InsertNextTuple(vector);
      float point[3];
      for(unsigned int j = 0; j < 3; ++j)
        {
        randomSequence->Next();
        point[j] = static_cast<float>(randomSequence->GetValue());
        }
      cells->InsertCellPoint(points->InsertNextPoint(point));
      }
    }

  vectors->Squeeze();
  unstructuredGrid->GetPointData()->SetVectors(vectors);
  points->Squeeze();
  unstructuredGrid->SetPoints(points);
  cells->Squeeze();
  unstructuredGrid->SetCells(VTK_VERTEX, cells);
}

int HedgeHog(int dataType, int outputPointsPrecision)
{
  vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid
    = vtkSmartPointer<vtkUnstructuredGrid>::New();
  InitializeUnstructuredGrid(unstructuredGrid, dataType);

  vtkSmartPointer<vtkHedgeHog> hedgeHog
    = vtkSmartPointer<vtkHedgeHog>::New();
  hedgeHog->SetOutputPointsPrecision(outputPointsPrecision);
  hedgeHog->SetInputData(unstructuredGrid);

  hedgeHog->Update();

  vtkSmartPointer<vtkPolyData> polyData = hedgeHog->GetOutput();
  vtkSmartPointer<vtkPoints> points = polyData->GetPoints();

  return points->GetDataType();
}
}

int TestHedgeHog(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  int dataType = HedgeHog(VTK_FLOAT, vtkAlgorithm::DEFAULT_PRECISION);

  if(dataType != VTK_FLOAT)
    {
    return EXIT_FAILURE;
    }

  dataType = HedgeHog(VTK_DOUBLE, vtkAlgorithm::DEFAULT_PRECISION);

  if(dataType != VTK_DOUBLE)
    {
    return EXIT_FAILURE;
    }

  dataType = HedgeHog(VTK_FLOAT, vtkAlgorithm::SINGLE_PRECISION);

  if(dataType != VTK_FLOAT)
    {
    return EXIT_FAILURE;
    }

  dataType = HedgeHog(VTK_DOUBLE, vtkAlgorithm::SINGLE_PRECISION);

  if(dataType != VTK_FLOAT)
    {
    return EXIT_FAILURE;
    }

  dataType = HedgeHog(VTK_FLOAT, vtkAlgorithm::DOUBLE_PRECISION);

  if(dataType != VTK_DOUBLE)
    {
    return EXIT_FAILURE;
    }

  dataType = HedgeHog(VTK_DOUBLE, vtkAlgorithm::DOUBLE_PRECISION);

  if(dataType != VTK_DOUBLE)
    {
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
