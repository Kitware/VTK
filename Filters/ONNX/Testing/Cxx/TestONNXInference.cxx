// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkONNXInference.h"

#include "vtkCellData.h"
#include "vtkCellTypeSource.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMathUtilities.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

namespace
{
bool Assert(bool test, const std::string& msg)
{
  if (!test)
  {
    vtkLog(ERROR, "Test failed: " << msg);
  }
  return test;
}

bool TestGaussianKernel(int argc, char* argv[])
{
  bool test = true;
  char* dataPath =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ONNX/gaussian_kernel.onnx");

  vtkNew<vtkCellTypeSource> source;
  source->SetCellType(VTK_QUAD);
  source->SetBlocksDimensions(10, 10, 1);
  source->Update();

  vtkNew<vtkONNXInference> filter;
  filter->SetInputConnection(source->GetOutputPort());
  filter->SetNumberOfInputParameters(3);
  filter->SetInputParameter(0, 0.25);
  filter->SetInputParameter(1, 0.6);
  filter->SetInputParameter(2, 1.0);
  filter->SetOutputDimension(1);
  filter->SetModelFile(dataPath);
  filter->Update();

  vtkSmartPointer<vtkUnstructuredGrid> output =
    vtkUnstructuredGrid::SafeDownCast(filter->GetOutput());

  vtkFloatArray* prediction =
    vtkFloatArray::SafeDownCast(output->GetCellData()->GetArray("PredictedField"));
  test &= ::Assert(prediction->GetNumberOfTuples() == 100, "CELL DATA, Wrong output shape.");
  test &= ::Assert(
    vtkMathUtilities::FuzzyCompare(prediction->GetTuple1(0), 0.8095716238021850585993750, 0.0001),
    "CELL DATA, Wrong prediction value.");
  test &= ::Assert(
    vtkMathUtilities::FuzzyCompare(prediction->GetTuple1(62), 0.99739539623260498046875, 0.0001),
    "CELL DATA, Wrong prediction value.");

  return test;
}

bool TestGaussianKernelOnPoints(int argc, char* argv[])
{
  bool test = true;
  char* dataPath =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ONNX/gaussian_kernel.onnx");

  vtkNew<vtkCellTypeSource> source;
  source->SetCellType(VTK_QUAD);
  source->SetBlocksDimensions(9, 4, 1);
  source->Update();

  vtkNew<vtkONNXInference> filter;
  filter->SetInputConnection(source->GetOutputPort());
  filter->SetNumberOfInputParameters(3);
  filter->SetInputParameter(0, 0.5);
  filter->SetInputParameter(1, 0.5);
  filter->SetInputParameter(2, 1.5);
  filter->SetOutputDimension(2);
  filter->SetArrayAssociation(vtkDataObject::POINT);
  filter->SetModelFile(dataPath);
  filter->Update();

  vtkSmartPointer<vtkUnstructuredGrid> output =
    vtkUnstructuredGrid::SafeDownCast(filter->GetOutput());

  vtkFloatArray* prediction =
    vtkFloatArray::SafeDownCast(output->GetPointData()->GetArray("PredictedField"));
  test &= ::Assert(prediction->GetNumberOfTuples() == 50, "POINT DATA, Wrong output shape.");
  test &= ::Assert(
    vtkMathUtilities::FuzzyCompare(prediction->GetTuple2(0)[1], 0.874813258647918701171875, 0.0001),
    "POINT DATA, Wrong prediction value.");
  test &= ::Assert(
    vtkMathUtilities::FuzzyCompare(prediction->GetTuple2(31)[0], 0.965625286102294921875, 0.0001),
    "POINT DATA, Wrong prediction value.");

  return test;
}

bool TestGaussianKernelWithTime(int argc, char* argv[])
{
  bool test = true;
  char* dataPath =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ONNX/gaussian_kernel.onnx");

  vtkNew<vtkCellTypeSource> source;
  source->SetCellType(VTK_QUAD);
  source->SetBlocksDimensions(10, 10, 1);
  source->Update();

  std::vector<double> timeValues = { 1.1, 2.37 };

  vtkNew<vtkONNXInference> filter;
  filter->SetInputConnection(source->GetOutputPort());
  filter->SetNumberOfInputParameters(3);
  filter->SetInputParameter(0, 0.9);
  filter->SetInputParameter(1, 0.1);
  filter->SetInputParameter(2, 1.1);
  filter->SetNumberOfTimeStepValues(2);
  for (size_t i = 0; i < timeValues.size(); ++i)
  {
    filter->SetTimeStepValue(i, timeValues[i]);
  }
  filter->SetTimeStepIndex(2);
  filter->SetModelFile(dataPath);
  filter->UpdateTimeStep(timeValues[0]);

  vtkSmartPointer<vtkUnstructuredGrid> output =
    vtkUnstructuredGrid::SafeDownCast(filter->GetOutput());
  vtkFloatArray* prediction =
    vtkFloatArray::SafeDownCast(output->GetCellData()->GetArray("PredictedField"));

  test &= ::Assert(prediction->GetNumberOfTuples() == 100, "TIME, Wrong output shape.");
  test &= ::Assert(
    vtkMathUtilities::FuzzyCompare(prediction->GetTuple1(0), 0.688853085041046142578125, 0.0001),
    "TIME, Wrong prediction value.");
  test &= ::Assert(
    vtkMathUtilities::FuzzyCompare(prediction->GetTuple1(31), 0.735185921192169189453125, 0.0001),
    "TIME, Wrong prediction value.");

  filter->UpdateTimeStep(timeValues[1]);

  test &= ::Assert(prediction->GetNumberOfTuples() == 100, "TIME, Wrong output shape.");
  test &= ::Assert(
    vtkMathUtilities::FuzzyCompare(prediction->GetTuple1(0), 0.841141164302825927734375, 0.0001),
    "TIME, Wrong prediction value.");
  test &= ::Assert(
    vtkMathUtilities::FuzzyCompare(prediction->GetTuple1(31), 0.86694240570068359375, 0.0001),
    "TIME, Wrong prediction value.");

  return test;
}
}

int TestONNXInference(int argc, char* argv[])
{
  bool testVal = ::TestGaussianKernel(argc, argv);
  testVal &= ::TestGaussianKernelOnPoints(argc, argv);
  testVal &= ::TestGaussianKernelWithTime(argc, argv);

  return testVal ? EXIT_SUCCESS : EXIT_FAILURE;
}
