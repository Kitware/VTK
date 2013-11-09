/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTubeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkCellArray.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkSmartPointer.h>
#include <vtkTubeFilter.h>

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
  vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
  lines->InsertNextCell(4);

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
      vtkIdType pointId = points->InsertNextPoint(point);
      verts->InsertCellPoint(pointId);
      lines->InsertCellPoint(pointId);
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
      vtkIdType pointId = points->InsertNextPoint(point);
      verts->InsertCellPoint(pointId);
      lines->InsertCellPoint(pointId);
      }
    }

  points->Squeeze();
  polyData->SetPoints(points);
  verts->Squeeze();
  polyData->SetVerts(verts);
  lines->Squeeze();
  polyData->SetLines(lines);
}

int TubeFilter(int dataType, int outputPointsPrecision)
{
  vtkSmartPointer<vtkPolyData> inputPolyData
    = vtkSmartPointer<vtkPolyData>::New();
  InitializePolyData(inputPolyData, dataType);

  vtkSmartPointer<vtkTubeFilter> tubeFilter
    = vtkSmartPointer<vtkTubeFilter>::New();
  tubeFilter->SetOutputPointsPrecision(outputPointsPrecision);
  tubeFilter->SetInputData(inputPolyData);

  tubeFilter->Update();

  vtkSmartPointer<vtkPolyData> outputPolyData = tubeFilter->GetOutput();
  vtkSmartPointer<vtkPoints> points = outputPolyData->GetPoints();

  return points->GetDataType();
}
}

int TestTubeFilter(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  int dataType = TubeFilter(VTK_FLOAT, vtkAlgorithm::DEFAULT_PRECISION);

  if(dataType != VTK_FLOAT)
    {
    return EXIT_FAILURE;
    }

  dataType = TubeFilter(VTK_DOUBLE, vtkAlgorithm::DEFAULT_PRECISION);

  if(dataType != VTK_DOUBLE)
    {
    return EXIT_FAILURE;
    }

  dataType = TubeFilter(VTK_FLOAT, vtkAlgorithm::SINGLE_PRECISION);

  if(dataType != VTK_FLOAT)
    {
    return EXIT_FAILURE;
    }

  dataType = TubeFilter(VTK_DOUBLE, vtkAlgorithm::SINGLE_PRECISION);

  if(dataType != VTK_FLOAT)
    {
    return EXIT_FAILURE;
    }

  dataType = TubeFilter(VTK_FLOAT, vtkAlgorithm::DOUBLE_PRECISION);

  if(dataType != VTK_DOUBLE)
    {
    return EXIT_FAILURE;
    }

  dataType = TubeFilter(VTK_DOUBLE, vtkAlgorithm::DOUBLE_PRECISION);

  if(dataType != VTK_DOUBLE)
    {
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
