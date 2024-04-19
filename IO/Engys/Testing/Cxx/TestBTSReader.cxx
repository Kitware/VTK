// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2019-2023 Engys Ltd.
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkBTSReader.h>
#include <vtkCellData.h>
#include <vtkErrorCode.h>
#include <vtkPartitionedDataSet.h>
#include <vtkPolyData.h>
#include <vtkTestUtilities.h>
#include <vtkTesting.h>
#include <vtkXMLPartitionedDataSetReader.h>
#include <vtkXMLPartitionedDataSetWriter.h>

#include <string>

int TestPolyDataPoints(vtkPolyData* data, vtkPolyData* expectedData, unsigned int partitionId)
{
  if (data->GetNumberOfPoints() != expectedData->GetNumberOfPoints())
  {
    std::cerr << "For partition " << partitionId << ", expecting "
              << expectedData->GetNumberOfPoints()
              << " points but got: " << data->GetNumberOfPoints() << std::endl;
    return EXIT_FAILURE;
  }

  for (vtkIdType pointId = 0; pointId < data->GetNumberOfPoints(); pointId++)
  {
    double coordinates[3];
    double expectedCoordinates[3];
    data->GetPoint(pointId, coordinates);
    expectedData->GetPoint(pointId, expectedCoordinates);
    if (coordinates[0] != expectedCoordinates[0] || coordinates[1] != expectedCoordinates[1] ||
      coordinates[2] != expectedCoordinates[2])
    {
      std::cerr << "For partition " << partitionId << ", point " << pointId << ", expecting ["
                << expectedCoordinates[0] << ", " << expectedCoordinates[1] << ", "
                << expectedCoordinates[2] << "]"
                << " but got [" << coordinates[0] << ", " << coordinates[1] << ", "
                << coordinates[2] << "]" << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

int TestPolyDataCells(vtkPolyData* data, vtkPolyData* expectedData, unsigned int partitionId)
{
  if (data->GetNumberOfCells() != expectedData->GetNumberOfCells())
  {
    std::cerr << "For partition " << partitionId << ", expecting "
              << expectedData->GetNumberOfCells() << " cells but got: " << data->GetNumberOfCells()
              << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkIdList> cellPoints;
  vtkNew<vtkIdList> expectedCellPoints;
  for (vtkIdType cellId = 0; cellId < data->GetNumberOfCells(); cellId++)
  {
    data->GetCellPoints(cellId, cellPoints);
    expectedData->GetCellPoints(cellId, expectedCellPoints);

    if (cellPoints->GetNumberOfIds() != expectedCellPoints->GetNumberOfIds())
    {
      std::cerr << "For partition " << partitionId << ", cell " << cellId << ", expecting "
                << expectedCellPoints->GetNumberOfIds() << " points but got "
                << cellPoints->GetNumberOfIds() << std::endl;
      return EXIT_FAILURE;
    }

    for (vtkIdType cellPointIndex = 0; cellPointIndex < cellPoints->GetNumberOfIds();
         cellPointIndex++)
    {
      if (cellPoints->GetId(cellPointIndex) != expectedCellPoints->GetId(cellPointIndex))
      {
        std::cerr << "For partition " << partitionId << ", cell " << cellId
                  << ", expecting point ID " << expectedCellPoints->GetId(cellPointIndex)
                  << " but got " << cellPoints->GetId(cellPointIndex) << std::endl;
        return EXIT_FAILURE;
      }
    }
  }
  return EXIT_SUCCESS;
}

int TestPolyData(vtkPolyData* data, vtkPolyData* expectedData, unsigned int partitionId)
{
  if (TestPolyDataPoints(data, expectedData, partitionId))
  {
    return EXIT_FAILURE;
  }

  if (TestPolyDataCells(data, expectedData, partitionId))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

void WriteDataToTemporary(vtkPartitionedDataSet* data, std::string tempDir)
{
  vtkNew<vtkXMLPartitionedDataSetWriter> w;
  w->SetInputData(data);
  std::string fn = tempDir + "/pds2.vtpd";
  w->SetFileName(fn.c_str());
  w->SetDataModeToAscii();
  std::cerr << "Writing temporary data to " << fn << std::endl;
  w->Update();
}

int TestPartitionedDataSet(
  vtkPartitionedDataSet* pds, vtkPartitionedDataSet* expectedPds, std::string tempDir)
{
  if (pds == nullptr || expectedPds == nullptr)
  {
    std::cerr << "Error: Data not in the format expected." << std::endl;
    return EXIT_FAILURE;
  }

  if (pds->GetNumberOfPartitions() != expectedPds->GetNumberOfPartitions())
  {
    std::cerr << "Expecting " << expectedPds->GetNumberOfPartitions()
              << " partitions but got: " << pds->GetNumberOfPartitions() << std::endl;
    WriteDataToTemporary(pds, tempDir);
    return EXIT_FAILURE;
  }

  for (unsigned int partitionId = 0; partitionId < pds->GetNumberOfPartitions(); partitionId++)
  {
    vtkPolyData* data = vtkPolyData::SafeDownCast(pds->GetPartition(partitionId));
    vtkPolyData* expectedData = vtkPolyData::SafeDownCast(expectedPds->GetPartition(partitionId));

    if (TestPolyData(data, expectedData, partitionId))
    {
      WriteDataToTemporary(pds, tempDir);
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

int TestReadData(
  const std::string& btsFile, const std::string& expectedDataFile, const std::string& tempDir)
{
  std::cout << "Testing: " << btsFile << std::endl;
  vtkNew<vtkBTSReader> reader;
  reader->SetFileName(btsFile.c_str());
  reader->Update();
  if (reader->GetErrorCode() != 0)
  {
    std::cerr << "Got an error when updating the reader: "
              << vtkErrorCode::GetStringFromErrorCode(reader->GetErrorCode()) << endl;
    return EXIT_FAILURE;
  }
  vtkPartitionedDataSet* pds = reader->GetOutput();

  std::cout << "Comparing with: " << expectedDataFile << std::endl;
  vtkNew<vtkXMLPartitionedDataSetReader> expectedDataReader;
  expectedDataReader->SetFileName(expectedDataFile.c_str());
  expectedDataReader->Update();
  if (expectedDataReader->GetErrorCode() != 0)
  {
    std::cerr << "Got an error when updating the comparison reader: "
              << vtkErrorCode::GetStringFromErrorCode(expectedDataReader->GetErrorCode()) << endl;
    WriteDataToTemporary(pds, tempDir);
    return EXIT_FAILURE;
  }

  vtkPartitionedDataSet* expectedPds =
    vtkPartitionedDataSet::SafeDownCast(expectedDataReader->GetOutput());

  if (TestPartitionedDataSet(pds, expectedPds, tempDir))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int TestBTSReader(int argc, char* argv[])
{
  vtkNew<vtkTesting> testHelper;
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
  {
    std::cerr << "Error: -D /path/to/data was not specified.";
    return EXIT_FAILURE;
  }

  std::string tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");

  std::string dataRoot = testHelper->GetDataRoot();
  std::string singlePatchBtsFile = dataRoot + "/Data/Engys/bts/2400-IDGH.bts";
  std::string expectedSinglePatchBtsFile = dataRoot + "/Data/Engys/vtpd/2400-IDGH.vtpd";
  if (TestReadData(singlePatchBtsFile, expectedSinglePatchBtsFile, tempDir))
  {
    return EXIT_FAILURE;
  }

  std::string multiplePatchesBtsFile = dataRoot + "/Data/Engys/bts/multiple_patches.bts";
  std::string expectedMultiplePatchesBtsFile = dataRoot + "/Data/Engys/vtpd/multiple_patches.vtpd";
  if (TestReadData(multiplePatchesBtsFile, expectedMultiplePatchesBtsFile, tempDir))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
