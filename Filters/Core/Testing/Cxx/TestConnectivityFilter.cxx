/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestConnectivityFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkCellArray.h>
#include <vtkConnectivityFilter.h>
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
  vtkSmartPointer<vtkFloatArray> scalars = vtkSmartPointer<vtkFloatArray>::New();

  if(dataType == VTK_DOUBLE)
    {
    points->SetDataType(VTK_DOUBLE);
    for(unsigned int i = 0; i < 4; ++i)
      {
      randomSequence->Next();
      scalars->InsertNextValue(randomSequence->GetValue());
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
      randomSequence->Next();
      scalars->InsertNextValue(randomSequence->GetValue());
      float point[3];
      for(unsigned int j = 0; j < 3; ++j)
        {
        randomSequence->Next();
        point[j] = static_cast<float>(randomSequence->GetValue());
        }
      cells->InsertCellPoint(points->InsertNextPoint(point));
      }
    }

  scalars->Squeeze();
  unstructuredGrid->GetPointData()->SetScalars(scalars);
  points->Squeeze();
  unstructuredGrid->SetPoints(points);
  cells->Squeeze();
  unstructuredGrid->SetCells(VTK_VERTEX, cells);
}

int FilterUnstructuredGridConnectivity(int dataType, int outputPointsPrecision)
{
  vtkSmartPointer<vtkUnstructuredGrid> inputUnstructuredGrid
    = vtkSmartPointer<vtkUnstructuredGrid>::New();
  InitializeUnstructuredGrid(inputUnstructuredGrid, dataType);

  vtkSmartPointer<vtkConnectivityFilter> connectivityFilter
    = vtkSmartPointer<vtkConnectivityFilter>::New();
  connectivityFilter->SetOutputPointsPrecision(outputPointsPrecision);
  connectivityFilter->ScalarConnectivityOn();
  connectivityFilter->SetScalarRange(0.25, 0.75);
  connectivityFilter->SetInputData(inputUnstructuredGrid);

  connectivityFilter->Update();

  vtkSmartPointer<vtkUnstructuredGrid> outputUnstructuredGrid = connectivityFilter->GetOutput();
  vtkSmartPointer<vtkPoints> points = outputUnstructuredGrid->GetPoints();

  return points->GetDataType();
}
}

int TestConnectivityFilter(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  int dataType = FilterUnstructuredGridConnectivity(VTK_FLOAT, vtkAlgorithm::DEFAULT_PRECISION);

  if(dataType != VTK_FLOAT)
    {
    return EXIT_FAILURE;
    }

  dataType = FilterUnstructuredGridConnectivity(VTK_DOUBLE, vtkAlgorithm::DEFAULT_PRECISION);

  if(dataType != VTK_DOUBLE)
    {
    return EXIT_FAILURE;
    }

  dataType = FilterUnstructuredGridConnectivity(VTK_FLOAT, vtkAlgorithm::SINGLE_PRECISION);

  if(dataType != VTK_FLOAT)
    {
    return EXIT_FAILURE;
    }

  dataType = FilterUnstructuredGridConnectivity(VTK_DOUBLE, vtkAlgorithm::SINGLE_PRECISION);

  if(dataType != VTK_FLOAT)
    {
    return EXIT_FAILURE;
    }

  dataType = FilterUnstructuredGridConnectivity(VTK_FLOAT, vtkAlgorithm::DOUBLE_PRECISION);

  if(dataType != VTK_DOUBLE)
    {
    return EXIT_FAILURE;
    }

  dataType = FilterUnstructuredGridConnectivity(VTK_DOUBLE, vtkAlgorithm::DOUBLE_PRECISION);

  if(dataType != VTK_DOUBLE)
    {
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
