// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkAppendDataSets.h>
#include <vtkCellData.h>
#include <vtkConnectivityFilter.h>
#include <vtkDataArray.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkRTAnalyticSource.h>
#include <vtkUnstructuredGrid.h>

namespace
{
//------------------------------------------------------------------------------
void AddRegion(vtkAppendDataSets* filter)
{
  const int maxExtent = 5;
  vtkNew<vtkRTAnalyticSource> dataSource;
  dataSource->SetWholeExtent(0, maxExtent, 0, maxExtent, 0, maxExtent);
  dataSource->Update();
  filter->AddInputData(dataSource->GetOutput());
}

//------------------------------------------------------------------------------
void InitializeUnstructuredGrid(
  vtkUnstructuredGrid* unstructuredGrid, int pointsType, long long nbOfRegions)
{
  vtkNew<vtkAppendDataSets> filter;
  filter->SetOutputDataSetType(VTK_UNSTRUCTURED_GRID);
  filter->SetOutputPointsPrecision(
    pointsType == VTK_FLOAT ? vtkAlgorithm::SINGLE_PRECISION : vtkAlgorithm::DOUBLE_PRECISION);

  for (long long region = 0; region < nbOfRegions; region++)
  {
    AddRegion(filter);
  }
  filter->Update();

  unstructuredGrid->ShallowCopy(filter->GetOutput());
}

bool CheckOutputPointsType(vtkUnstructuredGrid* output, int inputType, int expectedPrecision)
{
  vtkPoints* points = output->GetPoints();
  int outputDataType = points->GetDataType();

  int expectedType = expectedPrecision == vtkAlgorithm::DEFAULT_PRECISION
    ? inputType
    : expectedPrecision == vtkAlgorithm::SINGLE_PRECISION ? VTK_FLOAT : VTK_DOUBLE;

  if (expectedType != outputDataType)
  {
    std::cerr << "ERROR: wrong output point type. Has " << outputDataType << " instead of "
              << expectedType << "\n";
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool TestFilterOutputPrecision(int pointsType, int outputPointsPrecision)
{
  vtkNew<vtkUnstructuredGrid> inputUnstructuredGrid;
  const int nbOfRegions = 10;
  InitializeUnstructuredGrid(inputUnstructuredGrid, pointsType, nbOfRegions);

  vtkNew<vtkConnectivityFilter> connectivityFilter;
  connectivityFilter->SetOutputPointsPrecision(outputPointsPrecision);
  connectivityFilter->ScalarConnectivityOn();
  connectivityFilter->SetScalarRange(0.25, 0.75);
  connectivityFilter->SetInputData(inputUnstructuredGrid);
  connectivityFilter->Update();

  vtkUnstructuredGrid* outputUnstructuredGrid = connectivityFilter->GetUnstructuredGridOutput();

  return CheckOutputPointsType(outputUnstructuredGrid, pointsType, outputPointsPrecision);
}

}

bool TestOutputPointsType()
{
  std::cout << "TestOutputPointsType\n";
  std::vector<int> dataTypes = { VTK_FLOAT, VTK_DOUBLE };
  std::vector<int> outputPrecisions = { vtkAlgorithm::DEFAULT_PRECISION,
    vtkAlgorithm::SINGLE_PRECISION, vtkAlgorithm::DOUBLE_PRECISION };
  for (int type : dataTypes)
  {
    for (int precision : outputPrecisions)
    {
      bool success = TestFilterOutputPrecision(type, precision);
      if (!success)
      {
        std::cerr << "Connectivity fails for type " << type << " and precision " << precision
                  << "\n";
        return false;
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------
int TestConnectivityFilter(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  if (!TestOutputPointsType())
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
