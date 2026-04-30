// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkNew.h"
#include <vtkCellArray.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>

#include <iostream>

namespace
{
void InitializePolyData(vtkPolyData* polyData, int dataType)
{
  vtkNew<vtkMinimalStandardRandomSequence> randomSequence;
  randomSequence->SetSeed(1);

  vtkNew<vtkPoints> points;
  vtkNew<vtkCellArray> verts;
  verts->InsertNextCell(4);

  if (dataType == VTK_DOUBLE)
  {
    points->SetDataType(VTK_DOUBLE);
    for (unsigned int i = 0; i < 4; ++i)
    {
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
      float point[3];
      for (unsigned int j = 0; j < 3; ++j)
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

void InitializeTransform(vtkTransform* transform)
{
  vtkNew<vtkMinimalStandardRandomSequence> randomSequence;
  randomSequence->SetSeed(1);

  double elements[16];
  for (unsigned int i = 0; i < 16; ++i)
  {
    randomSequence->Next();
    elements[i] = randomSequence->GetValue();
  }
  transform->SetMatrix(elements);
}
}

int TransformPolyData(int dataType, int outputPointsPrecision)
{
  vtkNew<vtkPolyData> inputPolyData;
  InitializePolyData(inputPolyData, dataType);

  vtkNew<vtkTransform> transform;
  InitializeTransform(transform);

  vtkNew<vtkTransformFilter> transformPolyDataFilter;
  transformPolyDataFilter->SetOutputPointsPrecision(outputPointsPrecision);

  transformPolyDataFilter->SetTransform(transform);
  transformPolyDataFilter->SetInputData(inputPolyData);

  transformPolyDataFilter->Update();

  vtkPolyData* outputPolyData = transformPolyDataFilter->GetPolyDataOutput();
  vtkPoints* points = outputPolyData->GetPoints();

  return points->GetDataType();
}

bool IsFilterOutputEmpty(vtkTransformFilter* transformPolyDataFilter)
{
  transformPolyDataFilter->Update();
  vtkPoints* points = transformPolyDataFilter->GetOutput()->GetPoints();
  return (points == nullptr || points->GetNumberOfPoints() == 0);
}

int TransformEmptyPolyData()
{
  vtkNew<vtkTransformFilter> transformPolyDataFilter;

  vtkNew<vtkPolyData> inputPolyData;
  InitializePolyData(inputPolyData, VTK_DOUBLE);
  transformPolyDataFilter->SetInputData(inputPolyData);

  vtkNew<vtkTransform> transform;
  InitializeTransform(transform);
  transformPolyDataFilter->SetTransform(transform);

  if (IsFilterOutputEmpty(transformPolyDataFilter))
  {
    std::cerr << "Transformed output is expected to be not empty" << std::endl;
    return EXIT_FAILURE;
  }

  // Test if 0 input points produces empty output
  inputPolyData->GetPoints()->Initialize();
  if (!IsFilterOutputEmpty(transformPolyDataFilter))
  {
    std::cerr << "Transformed output should be empty if 0 points in input" << std::endl;
    return EXIT_FAILURE;
  }

  // Run the filter with non-empty output again to make sure that in the next test
  // the filter output is cleared
  InitializePolyData(inputPolyData, VTK_DOUBLE);
  if (IsFilterOutputEmpty(transformPolyDataFilter))
  {
    std::cerr << "Transformed output is expected to be not empty" << std::endl;
    return EXIT_FAILURE;
  }

  // Test if the filter produces empty output if no points object is in the input
  inputPolyData->SetPoints(nullptr);
  if (!IsFilterOutputEmpty(transformPolyDataFilter))
  {
    std::cerr << "Transformed output should be empty if no points object in input" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int TestTransformPolyDataFilter(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int dataType = TransformPolyData(VTK_FLOAT, vtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = TransformPolyData(VTK_DOUBLE, vtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = TransformPolyData(VTK_FLOAT, vtkAlgorithm::SINGLE_PRECISION);

  if (dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = TransformPolyData(VTK_DOUBLE, vtkAlgorithm::SINGLE_PRECISION);

  if (dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = TransformPolyData(VTK_FLOAT, vtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = TransformPolyData(VTK_DOUBLE, vtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  int result = TransformEmptyPolyData();
  if (result != EXIT_SUCCESS)
  {
    return result;
  }

  return EXIT_SUCCESS;
}
