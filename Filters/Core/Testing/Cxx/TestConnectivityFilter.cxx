// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkAppendDataSets.h>
#include <vtkCellData.h>
#include <vtkConnectivityFilter.h>
#include <vtkDataArray.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkRTAnalyticSource.h>
#include <vtkTestUtilities.h>
#include <vtkUnstructuredGrid.h>

namespace
{
//------------------------------------------------------------------------------
void AddRegion(vtkAppendDataSets* filter)
{
  constexpr int maxExtent = 5;
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

//------------------------------------------------------------------------------
template <typename T>
bool CompareValues(const std::string& context, T actual, T expected)
{
  if (actual != expected)
  {
    std::cerr << "ERROR: Wrong " << context << ". Has " << actual << " instead of " << expected
              << "\n";
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
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
  constexpr int nbOfRegions = 10;
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

//------------------------------------------------------------------------------
bool CheckScalarsArray(vtkDataSetAttributes* fieldData, long long nbOfRegions)
{
  vtkDataArray* scalars = fieldData->GetScalars();
  std::string expectedName = "RegionId";
  std::string actualName = scalars->GetName();
  if (!CompareValues("cell scalars array", actualName, expectedName))
  {
    return false;
  }

  double range[2];
  scalars->GetRange(range);
  double expected = nbOfRegions;
  if (!CompareValues("number of regions", range[1] + 1, expected))
  {
    return false;
  }

  int expectedDataType = VTK_CHAR;
  if (nbOfRegions <= std::numeric_limits<char>::max())
  {
    expectedDataType = VTK_CHAR;
  }
  else if (nbOfRegions <= std::numeric_limits<unsigned char>::max())
  {
    expectedDataType = VTK_UNSIGNED_CHAR;
  }
  else if (nbOfRegions <= std::numeric_limits<short>::max())
  {
    expectedDataType = VTK_SHORT;
  }
  // we do not test with more regions because it take too much time.
  unsigned int elementSize = vtkDataArray::GetDataTypeSize(expectedDataType);

  unsigned int expectedSize = scalars->GetSize() * elementSize;
  unsigned int expectedKiBSize =
    static_cast<unsigned int>(std::ceil(static_cast<double>(expectedSize) / 1024.0));
  unsigned int actualSize = scalars->GetActualMemorySize();
  if (!CompareValues("region id memory size (in KiB)", actualSize, expectedKiBSize))
  {
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool TestRegionIdArraySize()
{
  std::cout << "TestRegionIdArraySize\n";
  // stop at max(unsigned char) + 1, to requires short.
  // With bigger type it takes really more time to execute (~90 sec for the whole test with short)
  // If changing, one should also update inner checks (see CheckScalarsArray)
  std::vector<long long> availableRegions = { 1, 10, std::numeric_limits<char>::max() + 1,
    std::numeric_limits<unsigned char>::max() + 1 };
  for (long long nbOfRegions : availableRegions)
  {
    vtkNew<vtkUnstructuredGrid> inputUnstructuredGrid;
    InitializeUnstructuredGrid(inputUnstructuredGrid, VTK_DOUBLE, nbOfRegions);

    vtkNew<vtkConnectivityFilter> connectivityFilter;
    connectivityFilter->SetInputData(inputUnstructuredGrid);
    connectivityFilter->ColorRegionsOn();
    connectivityFilter->Update();
    vtkUnstructuredGrid* output = connectivityFilter->GetUnstructuredGridOutput();

    vtkCellData* cellData = output->GetCellData();
    if (!CheckScalarsArray(cellData, nbOfRegions))
    {
      std::cerr << "CellData failed for " << nbOfRegions << " regions.\n";
      return false;
    }

    vtkPointData* pointData = output->GetPointData();
    if (!CheckScalarsArray(pointData, nbOfRegions))
    {
      std::cerr << "PointData failed for " << nbOfRegions << " regions.\n";
      return false;
    }
  }

  return true;
}
}

//------------------------------------------------------------------------------
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

  if (!TestRegionIdArraySize())
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
