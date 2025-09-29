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

std::tuple<int, vtkIdType, vtkIdType> ThresholdPolyDataPoints(double lowerThreshold,
  double upperThreshold, int component, int dataType, int outputPointsPrecision, int thresholdMode)
{
  vtkSmartPointer<vtkPolyData> inputPolyData = vtkSmartPointer<vtkPolyData>::New();
  InitializePolyData(inputPolyData, dataType);

  vtkSmartPointer<vtkThresholdPoints> thresholdPoints = vtkSmartPointer<vtkThresholdPoints>::New();
  thresholdPoints->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "vectors");
  thresholdPoints->SetOutputPointsPrecision(outputPointsPrecision);
  thresholdPoints->SetLowerThreshold(lowerThreshold);
  thresholdPoints->SetUpperThreshold(upperThreshold);
  thresholdPoints->SetThresholdFunction(thresholdMode);
  thresholdPoints->SetInputArrayComponent(component);
  thresholdPoints->SetInputData(inputPolyData);

  thresholdPoints->Update();

  vtkSmartPointer<vtkPolyData> outputPolyData = thresholdPoints->GetOutput();
  vtkSmartPointer<vtkPoints> points = outputPolyData->GetPoints();

  return std::make_tuple<int, vtkIdType, vtkIdType>(
    points->GetDataType(), outputPolyData->GetNumberOfPoints(), outputPolyData->GetNumberOfCells());
}

const char* ThresholdModeName(int mode)
{
  switch (mode)
  {
    case vtkThresholdPoints::THRESHOLD_UPPER:
      return "THRESHOLD_UPPER";
    case vtkThresholdPoints::THRESHOLD_LOWER:
      return "THRESHOLD_LOWER";
    case vtkThresholdPoints::THRESHOLD_BETWEEN:
      return "THRESHOLD_BETWEEN";
    default:
      return "THRESHOLD_UNKNOWN";
  }
}
} // end anonymous namespace

int TestThresholdPoints(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  double thresholdValue = 0.5;
  double lowerBetween = 0.25;
  double upperBetween = 0.75;
  int dataType;
  vtkIdType numPoints, numCells;

  // Each element is the number of expected points and cells for a given array component
  const std::array<std::pair<vtkIdType, vtkIdType>, 4> componentExpectationsUpper{ {
    { 3, 3 },
    { 3, 3 },
    { 2, 2 },
    { 4, 4 },
  } };

  const std::array<std::pair<vtkIdType, vtkIdType>, 4> componentExpectationsLower{ {
    { 1, 1 },
    { 1, 1 },
    { 2, 2 },
    { 0, 0 },
  } };

  const std::array<std::pair<vtkIdType, vtkIdType>, 4> componentExpectationsBetween{ {
    { 3, 3 },
    { 4, 4 },
    { 1, 1 },
    { 1, 1 },
  } };

  struct ThresholdConfig
  {
    int inputType;
    int outputPrecision;
    int expectedDataType;
    const char* description;
  };

  const std::array<ThresholdConfig, 6> thresholdConfigs{ {
    { VTK_FLOAT, vtkAlgorithm::DEFAULT_PRECISION, VTK_FLOAT, "float/default" },
    { VTK_DOUBLE, vtkAlgorithm::DEFAULT_PRECISION, VTK_DOUBLE, "double/default" },
    { VTK_FLOAT, vtkAlgorithm::SINGLE_PRECISION, VTK_FLOAT, "float/single" },
    { VTK_DOUBLE, vtkAlgorithm::SINGLE_PRECISION, VTK_FLOAT, "double/single" },
    { VTK_FLOAT, vtkAlgorithm::DOUBLE_PRECISION, VTK_DOUBLE, "float/double" },
    { VTK_DOUBLE, vtkAlgorithm::DOUBLE_PRECISION, VTK_DOUBLE, "double/double" },
  } };

  auto runThresholdCases = [&](double lower, double upper, int thresholdMode,
                             const std::pair<vtkIdType, vtkIdType>& expectedCounts, int component)
  {
    bool modeSuccess = true;
    for (const auto& config : thresholdConfigs)
    {
      std::tie(dataType, numPoints, numCells) = ThresholdPolyDataPoints(
        lower, upper, component, config.inputType, config.outputPrecision, thresholdMode);

      if (dataType != config.expectedDataType)
      {
        vtkLog(ERROR, << ThresholdModeName(thresholdMode) << " component " << component << " ("
                      << config.description << ") expected data type " << config.expectedDataType
                      << ", got " << dataType);
        modeSuccess = false;
      }
      if (expectedCounts.first != numPoints)
      {
        vtkLog(ERROR, << ThresholdModeName(thresholdMode) << " component " << component << " ("
                      << config.description << ") expected " << expectedCounts.first
                      << " points, got " << numPoints);
        modeSuccess = false;
      }
      if (expectedCounts.second != numCells)
      {
        vtkLog(ERROR, << ThresholdModeName(thresholdMode) << " component " << component << " ("
                      << config.description << ") expected " << expectedCounts.second
                      << " cells, got " << numCells);
        modeSuccess = false;
      }
    }
    return modeSuccess;
  };

  // Iterate over array component number. Go one past the number of components to test vector
  // magnitude
  bool success = true;
  for (int component = 0; component < 4; ++component)
  {
    if (!runThresholdCases(thresholdValue, thresholdValue, vtkThresholdPoints::THRESHOLD_UPPER,
          componentExpectationsUpper[component], component))
    {
      success = false;
    }

    if (!runThresholdCases(thresholdValue, thresholdValue, vtkThresholdPoints::THRESHOLD_LOWER,
          componentExpectationsLower[component], component))
    {
      success = false;
    }

    if (!runThresholdCases(lowerBetween, upperBetween, vtkThresholdPoints::THRESHOLD_BETWEEN,
          componentExpectationsBetween[component], component))
    {
      success = false;
    }
  }

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
