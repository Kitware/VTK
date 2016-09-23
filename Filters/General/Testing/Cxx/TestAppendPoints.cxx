/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAppendPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkAppendPoints.h>
#include <vtkCellArray.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkSmartPointer.h>

namespace
{
void InitializePolyData(vtkPolyData *polyData, int dataType)
{
  vtkSmartPointer<vtkMinimalStandardRandomSequence> randomSequence
    = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> verts = vtkSmartPointer<vtkCellArray>::New();
  verts->InsertNextCell(4);

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
      verts->InsertCellPoint(points->InsertNextPoint(point));
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
      verts->InsertCellPoint(points->InsertNextPoint(point));
    }
  }

  points->Squeeze();
  polyData->SetPoints(points);
  verts->Squeeze();
  polyData->SetVerts(verts);
}

int AppendPolyDataPoints(int dataType0, int dataType1,
  int outputPointsPrecision)
{
  vtkSmartPointer<vtkPolyData> polyData0 = vtkSmartPointer<vtkPolyData>::New();
  InitializePolyData(polyData0, dataType0);

  vtkSmartPointer<vtkPolyData> polyData1 = vtkSmartPointer<vtkPolyData>::New();
  InitializePolyData(polyData1, dataType1);

  vtkSmartPointer<vtkAppendPoints> appendPoints
    = vtkSmartPointer<vtkAppendPoints>::New();
  appendPoints->SetOutputPointsPrecision(outputPointsPrecision);

  appendPoints->AddInputData(polyData0);
  appendPoints->AddInputData(polyData1);

  appendPoints->Update();

  vtkSmartPointer<vtkPointSet> pointSet = appendPoints->GetOutput();
  vtkSmartPointer<vtkPoints> points = pointSet->GetPoints();

  return points->GetDataType();
}
}

int TestAppendPoints(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  int dataType = AppendPolyDataPoints(VTK_FLOAT, VTK_FLOAT,
    vtkAlgorithm::DEFAULT_PRECISION);

  if(dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = AppendPolyDataPoints(VTK_DOUBLE, VTK_FLOAT,
    vtkAlgorithm::DEFAULT_PRECISION);

  if(dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = AppendPolyDataPoints(VTK_DOUBLE, VTK_DOUBLE,
    vtkAlgorithm::DEFAULT_PRECISION);

  if(dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = AppendPolyDataPoints(VTK_FLOAT, VTK_FLOAT,
    vtkAlgorithm::SINGLE_PRECISION);

  if(dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = AppendPolyDataPoints(VTK_DOUBLE, VTK_FLOAT,
    vtkAlgorithm::SINGLE_PRECISION);

  if(dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = AppendPolyDataPoints(VTK_DOUBLE, VTK_DOUBLE,
    vtkAlgorithm::SINGLE_PRECISION);

  if(dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = AppendPolyDataPoints(VTK_FLOAT, VTK_FLOAT,
    vtkAlgorithm::DOUBLE_PRECISION);

  if(dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = AppendPolyDataPoints(VTK_DOUBLE, VTK_FLOAT,
    vtkAlgorithm::DOUBLE_PRECISION);

  if(dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = AppendPolyDataPoints(VTK_DOUBLE, VTK_DOUBLE,
    vtkAlgorithm::DOUBLE_PRECISION);

  if(dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
