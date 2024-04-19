// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkCellArray.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

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
  vtkSmartPointer<vtkMinimalStandardRandomSequence> randomSequence =
    vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
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
  vtkSmartPointer<vtkPolyData> inputPolyData = vtkSmartPointer<vtkPolyData>::New();
  InitializePolyData(inputPolyData, dataType);

  vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
  InitializeTransform(transform);

  vtkSmartPointer<vtkTransformPolyDataFilter> transformPolyDataFilter =
    vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  transformPolyDataFilter->SetOutputPointsPrecision(outputPointsPrecision);

  transformPolyDataFilter->SetTransform(transform);
  transformPolyDataFilter->SetInputData(inputPolyData);

  transformPolyDataFilter->Update();

  vtkSmartPointer<vtkPolyData> outputPolyData = transformPolyDataFilter->GetOutput();
  vtkSmartPointer<vtkPoints> points = outputPolyData->GetPoints();

  return points->GetDataType();
}

bool IsFilterOutputEmpty(vtkTransformPolyDataFilter* transformPolyDataFilter)
{
  transformPolyDataFilter->Update();
  vtkPoints* points = transformPolyDataFilter->GetOutput()->GetPoints();
  return (points == nullptr || points->GetNumberOfPoints() == 0);
}

int TransformEmptyPolyData()
{
  vtkNew<vtkTransformPolyDataFilter> transformPolyDataFilter;

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
  inputPolyData->GetPoints()->SetNumberOfPoints(0);
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
