// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkLogger.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkThresholdPoints.h>

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
  vtkSmartPointer<vtkFloatArray> vectors = vtkSmartPointer<vtkFloatArray>::New();
  vectors->SetName("vectors");
  vectors->SetNumberOfComponents(3);

  points->SetDataType(dataType);

  for (unsigned int i = 0; i < 4; ++i)
  {
    float v[3];
    for (unsigned int j = 0; j < 3; ++j)
    {
      randomSequence->Next();
      v[j] = static_cast<float>(randomSequence->GetValue());
    }
    vectors->InsertNextTuple(v);

    if (dataType == VTK_DOUBLE)
    {
      double point[3];
      for (unsigned int j = 0; j < 3; ++j)
      {
        randomSequence->Next();
        point[j] = randomSequence->GetValue();
      }
      verts->InsertCellPoint(points->InsertNextPoint(point));
    }
    else
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

  vectors->Squeeze();
  auto pointData = polyData->GetPointData();
  pointData->AddArray(vectors);

  points->Squeeze();
  polyData->SetPoints(points);
  verts->Squeeze();
  polyData->SetVerts(verts);
}

std::tuple<int, vtkIdType, vtkIdType> ThresholdPolyDataPoints(
  double upperThreshold, int component, int dataType, int outputPointsPrecision)
{
  vtkSmartPointer<vtkPolyData> inputPolyData = vtkSmartPointer<vtkPolyData>::New();
  InitializePolyData(inputPolyData, dataType);

  vtkSmartPointer<vtkThresholdPoints> thresholdPoints = vtkSmartPointer<vtkThresholdPoints>::New();
  thresholdPoints->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "vectors");
  thresholdPoints->SetOutputPointsPrecision(outputPointsPrecision);
  thresholdPoints->ThresholdByUpper(upperThreshold);
  thresholdPoints->SetInputArrayComponent(component);
  thresholdPoints->SetInputData(inputPolyData);

  thresholdPoints->Update();

  vtkSmartPointer<vtkPolyData> outputPolyData = thresholdPoints->GetOutput();
  vtkSmartPointer<vtkPoints> points = outputPolyData->GetPoints();

  return std::make_tuple<int, vtkIdType, vtkIdType>(
    points->GetDataType(), outputPolyData->GetNumberOfPoints(), outputPolyData->GetNumberOfCells());
}
} // end anonymous namespace

int TestThresholdPoints(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  double thresholdValue = 0.5;
  int dataType;
  vtkIdType numPoints, numCells;

  // Each element is the number of expected points and cells for a given array component
  std::vector<std::pair<vtkIdType, vtkIdType>> componentExpectations;
  componentExpectations.emplace_back(std::make_pair<vtkIdType, vtkIdType>(3, 3));
  componentExpectations.emplace_back(std::make_pair<vtkIdType, vtkIdType>(3, 3));
  componentExpectations.emplace_back(std::make_pair<vtkIdType, vtkIdType>(2, 2));
  componentExpectations.emplace_back(std::make_pair<vtkIdType, vtkIdType>(4, 4));

  // Iterate over array component number. Go one past the number of components to test vector
  // magnitude
  bool success = true;
  for (int component = 0; component < 4; ++component)
  {
    std::tie(dataType, numPoints, numCells) = ThresholdPolyDataPoints(
      thresholdValue, component, VTK_FLOAT, vtkAlgorithm::DEFAULT_PRECISION);

    if (dataType != VTK_FLOAT)
    {
      vtkLog(ERROR, "Expected VTK_FLOAT points, got " << dataType);
      success = false;
    }
    if (componentExpectations[component].first != numPoints)
    {
      vtkLog(ERROR,
        "Expected " << componentExpectations[component].first << " points, got " << numPoints);
      success = false;
    }
    if (componentExpectations[component].second != numCells)
    {
      vtkLog(ERROR,
        "Expected " << componentExpectations[component].second << " cells, got " << numCells);
      success = false;
    }

    std::tie(dataType, numPoints, numCells) = ThresholdPolyDataPoints(
      thresholdValue, component, VTK_DOUBLE, vtkAlgorithm::DEFAULT_PRECISION);

    if (dataType != VTK_DOUBLE)
    {
      vtkLog(ERROR, "Expected VTK_DOUBLE points, got " << dataType);
      success = false;
    }
    if (componentExpectations[component].first != numPoints)
    {
      vtkLog(ERROR,
        "Expected " << componentExpectations[component].first << " points, got " << numPoints);
      success = false;
    }
    if (componentExpectations[component].second != numCells)
    {
      vtkLog(ERROR,
        "Expected " << componentExpectations[component].second << " cells, got " << numCells);
      success = false;
    }

    std::tie(dataType, numPoints, numCells) =
      ThresholdPolyDataPoints(thresholdValue, component, VTK_FLOAT, vtkAlgorithm::SINGLE_PRECISION);

    if (dataType != VTK_FLOAT)
    {
      vtkLog(ERROR, "Expected VTK_FLOAT points, got " << dataType);
      success = false;
    }
    if (componentExpectations[component].first != numPoints)
    {
      vtkLog(ERROR,
        "Expected " << componentExpectations[component].first << " points, got " << numPoints);
      success = false;
    }
    if (componentExpectations[component].second != numCells)
    {
      vtkLog(ERROR,
        "Expected " << componentExpectations[component].second << " cells, got " << numCells);
      success = false;
    }

    std::tie(dataType, numPoints, numCells) = ThresholdPolyDataPoints(
      thresholdValue, component, VTK_DOUBLE, vtkAlgorithm::SINGLE_PRECISION);

    if (dataType != VTK_FLOAT)
    {
      vtkLog(ERROR, "Expected VTK_FLOAT points, got " << dataType);
      success = false;
    }
    if (componentExpectations[component].first != numPoints)
    {
      vtkLog(ERROR,
        "Expected " << componentExpectations[component].first << " points, got " << numPoints);
      success = false;
    }
    if (componentExpectations[component].second != numCells)
    {
      vtkLog(ERROR,
        "Expected " << componentExpectations[component].second << " cells, got " << numCells);
      success = false;
    }

    std::tie(dataType, numPoints, numCells) =
      ThresholdPolyDataPoints(thresholdValue, component, VTK_FLOAT, vtkAlgorithm::DOUBLE_PRECISION);

    if (dataType != VTK_DOUBLE)
    {
      vtkLog(ERROR, "Expected VTK_DOUBLE points, got " << dataType);
      success = false;
    }
    if (componentExpectations[component].first != numPoints)
    {
      vtkLog(ERROR,
        "Expected " << componentExpectations[component].first << " points, got " << numPoints);
      success = false;
    }
    if (componentExpectations[component].second != numCells)
    {
      vtkLog(ERROR,
        "Expected " << componentExpectations[component].second << " cells, got " << numCells);
      success = false;
    }

    std::tie(dataType, numPoints, numCells) = ThresholdPolyDataPoints(
      thresholdValue, component, VTK_DOUBLE, vtkAlgorithm::DOUBLE_PRECISION);

    if (dataType != VTK_DOUBLE)
    {
      vtkLog(ERROR, "Expected VTK_DOUBLE points, got " << dataType);
      success = false;
    }
    if (componentExpectations[component].first != numPoints)
    {
      vtkLog(ERROR,
        "Expected " << componentExpectations[component].first << " points, got " << numPoints);
      success = false;
    }
    if (componentExpectations[component].second != numCells)
    {
      vtkLog(ERROR,
        "Expected " << componentExpectations[component].second << " cells, got " << numCells);
      success = false;
    }
  }

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
