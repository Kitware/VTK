/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTransformFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkMinimalStandardRandomSequence.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>

namespace
{
void InitializePointSet(vtkPointSet *pointSet, int dataType)
{
  vtkSmartPointer<vtkMinimalStandardRandomSequence> randomSequence
    = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

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
      points->InsertNextPoint(point);
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
      points->InsertNextPoint(point);
    }
  }

  points->Squeeze();
  pointSet->SetPoints(points);
}

void InitializeTransform(vtkTransform *transform)
{
  vtkSmartPointer<vtkMinimalStandardRandomSequence> randomSequence
    = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  double elements[16];
  for(unsigned int i = 0; i < 16; ++i)
  {
    randomSequence->Next();
    elements[i] = randomSequence->GetValue();
  }
  transform->SetMatrix(elements);
}
}

int TransformPointSet(int dataType, int outputPointsPrecision)
{
  vtkSmartPointer<vtkPointSet> inputPointSet
    = vtkSmartPointer<vtkPolyData>::New();
  InitializePointSet(inputPointSet, dataType);

  vtkSmartPointer<vtkTransform> transform
    = vtkSmartPointer<vtkTransform>::New();
  InitializeTransform(transform);

  vtkSmartPointer<vtkTransformFilter> transformFilter
    = vtkSmartPointer<vtkTransformFilter>::New();
  transformFilter->SetOutputPointsPrecision(outputPointsPrecision);

  transformFilter->SetTransform(transform);
  transformFilter->SetInputData(inputPointSet);

  transformFilter->Update();

  vtkSmartPointer<vtkPointSet> outputPointSet = transformFilter->GetOutput();
  vtkSmartPointer<vtkPoints> points = outputPointSet->GetPoints();

  return points->GetDataType();
}

int TestTransformFilter(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  int dataType = TransformPointSet(VTK_FLOAT, vtkAlgorithm::DEFAULT_PRECISION);

  if(dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = TransformPointSet(VTK_DOUBLE, vtkAlgorithm::DEFAULT_PRECISION);

  if(dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = TransformPointSet(VTK_FLOAT, vtkAlgorithm::SINGLE_PRECISION);

  if(dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = TransformPointSet(VTK_DOUBLE, vtkAlgorithm::SINGLE_PRECISION);

  if(dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = TransformPointSet(VTK_FLOAT, vtkAlgorithm::DOUBLE_PRECISION);

  if(dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = TransformPointSet(VTK_DOUBLE, vtkAlgorithm::DOUBLE_PRECISION);

  if(dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
