/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDelaunay3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkCellArray.h>
#include <vtkDelaunay3D.h>
#include <vtkMinimalStandardRandomSequence.h>
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

  if(dataType == VTK_DOUBLE)
  {
    points->SetDataType(VTK_DOUBLE);
    for(unsigned int i = 0; i < 4; ++i)
    {
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
      float point[3];
      for(unsigned int j = 0; j < 3; ++j)
      {
        randomSequence->Next();
        point[j] = static_cast<float>(randomSequence->GetValue());
      }
      cells->InsertCellPoint(points->InsertNextPoint(point));
    }
  }

  points->Squeeze();
  unstructuredGrid->SetPoints(points);
  cells->Squeeze();
  unstructuredGrid->SetCells(VTK_VERTEX, cells);
}

int Delaunay3D(int dataType, int outputPointsPrecision)
{
  vtkSmartPointer<vtkUnstructuredGrid> inputUnstructuredGrid
    = vtkSmartPointer<vtkUnstructuredGrid>::New();
  InitializeUnstructuredGrid(inputUnstructuredGrid, dataType);

  vtkSmartPointer<vtkDelaunay3D> delaunay
    = vtkSmartPointer<vtkDelaunay3D>::New();
  delaunay->SetOutputPointsPrecision(outputPointsPrecision);
  delaunay->SetInputData(inputUnstructuredGrid);

  delaunay->Update();

  vtkSmartPointer<vtkUnstructuredGrid> outputUnstructuredGrid = delaunay->GetOutput();
  vtkSmartPointer<vtkPoints> points = outputUnstructuredGrid->GetPoints();

  return points->GetDataType();
}
}

int TestDelaunay3D(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  int dataType = Delaunay3D(VTK_FLOAT, vtkAlgorithm::DEFAULT_PRECISION);

  if(dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = Delaunay3D(VTK_DOUBLE, vtkAlgorithm::DEFAULT_PRECISION);

  if(dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = Delaunay3D(VTK_FLOAT, vtkAlgorithm::SINGLE_PRECISION);

  if(dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = Delaunay3D(VTK_DOUBLE, vtkAlgorithm::SINGLE_PRECISION);

  if(dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = Delaunay3D(VTK_FLOAT, vtkAlgorithm::DOUBLE_PRECISION);

  if(dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = Delaunay3D(VTK_DOUBLE, vtkAlgorithm::DOUBLE_PRECISION);

  if(dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
