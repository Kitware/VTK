// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAppendDataSets.h"
#include "vtkFloatArray.h"
#include "vtkHDFReader.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkOverlappingAMR.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkUniformGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLImageDataReader.h"
#include "vtkXMLPUnstructuredGridReader.h"
#include "vtkXMLPartitionedDataSetCollectionReader.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkXMLUniformGridAMRReader.h"
#include "vtkXMLUnstructuredGridReader.h"

#include <cstdlib>
#include <iterator>
#include <string>

//----------------------------------------------------------------------------
vtkSmartPointer<vtkImageData> ReadImageData(const std::string& fileName)
{
  vtkNew<vtkXMLImageDataReader> reader;
  reader->SetFileName(fileName.c_str());
  reader->Update();
  vtkSmartPointer<vtkImageData> data = vtkImageData::SafeDownCast(reader->GetOutput());
  return data;
}

//----------------------------------------------------------------------------
int TestImageData(const std::string& dataRoot)
{
  // ImageData file
  // ------------------------------------------------------------
  std::string fileName = dataRoot + "/Data/mandelbrot-vti.hdf";
  std::cout << "Testing: " << fileName << std::endl;
  vtkNew<vtkHDFReader> reader;
  if (!reader->CanReadFile(fileName.c_str()))
  {
    return EXIT_FAILURE;
  }
  reader->SetFileName(fileName.c_str());
  reader->Update();
  vtkImageData* data = vtkImageData::SafeDownCast(reader->GetOutput());
  vtkSmartPointer<vtkImageData> expectedData = ReadImageData(dataRoot + "/Data/mandelbrot.vti");

  int* dims = data->GetDimensions();
  int* edims = expectedData->GetDimensions();
  if (dims[0] != edims[0] || dims[1] != edims[1] || dims[2] != edims[2])
  {
    std::cerr << "Error: vtkImageData with wrong dimensions: "
              << "expecting "
              << "[" << edims[0] << ", " << edims[1] << ", " << edims[2] << "]"
              << " got "
              << "[" << dims[0] << ", " << dims[1] << ", " << dims[2] << "]" << std::endl;
    return EXIT_FAILURE;
  }

  return !vtkTestUtilities::CompareDataObjects(data, expectedData, true);
}

//----------------------------------------------------------------------------
int TestImageCellData(const std::string& dataRoot)
{
  // ImageData file with cell data
  // ------------------------------------------------------------
  std::string fileName = dataRoot + "/Data/wavelet_cell_data.hdf";
  std::cout << "Testing: " << fileName << std::endl;
  vtkNew<vtkHDFReader> reader;
  if (!reader->CanReadFile(fileName.c_str()))
  {
    return EXIT_FAILURE;
  }
  reader->SetFileName(fileName.c_str());
  reader->Update();
  vtkImageData* data = vtkImageData::SafeDownCast(reader->GetOutput());
  vtkSmartPointer<vtkImageData> expectedData =
    ReadImageData(dataRoot + "/Data/wavelet_cell_data.vti");

  int* dims = data->GetDimensions();
  int* edims = expectedData->GetDimensions();
  if (dims[0] != edims[0] || dims[1] != edims[1] || dims[2] != edims[2])
  {
    std::cerr << "Error: vtkImageData with wrong dimensions: "
              << "expecting "
              << "[" << edims[0] << ", " << edims[1] << ", " << edims[2] << "]"
              << " got "
              << "[" << dims[0] << ", " << dims[1] << ", " << dims[2] << "]" << std::endl;
    return EXIT_FAILURE;
  }

  return !vtkTestUtilities::CompareDataObjects(data, expectedData);
}

//----------------------------------------------------------------------------
int TestUnstructuredGrid(const std::string& dataRoot, bool parallel)
{
  std::string fileName, expectedName;
  vtkNew<vtkHDFReader> reader;
  vtkNew<vtkXMLUnstructuredGridReader> expectedReader;
  vtkNew<vtkXMLPUnstructuredGridReader> expectedPReader;
  vtkXMLReader* oreader;
  if (parallel)
  {
    fileName = dataRoot + "/Data/can-pvtu.hdf";
    expectedName = dataRoot + "/Data/can.pvtu";
    oreader = expectedPReader;
  }
  else
  {
    // This file intentionally has Type attribute in variable-length string
    fileName = dataRoot + "/Data/can-vtu.hdf";
    expectedName = dataRoot + "/Data/can.vtu";
    oreader = expectedReader;
  }
  std::cout << "Testing: " << fileName << std::endl;
  if (!reader->CanReadFile(fileName.c_str()))
  {
    return EXIT_FAILURE;
  }
  reader->SetFileName(fileName.c_str());
  reader->Update();
  vtkUnstructuredGrid* data = vtkUnstructuredGrid::SafeDownCast(reader->GetOutputAsDataSet());

  oreader->SetFileName(expectedName.c_str());
  oreader->Update();
  vtkUnstructuredGrid* expectedData =
    vtkUnstructuredGrid::SafeDownCast(oreader->GetOutputAsDataSet());
  return !vtkTestUtilities::CompareDataObjects(data, expectedData);
}

//----------------------------------------------------------------------------
int TestPartitionedUnstructuredGrid(const std::string& dataRoot, bool parallel)
{
  std::string fileName, expectedName;
  vtkNew<vtkHDFReader> reader;
  vtkNew<vtkXMLUnstructuredGridReader> expectedReader;
  vtkNew<vtkXMLPUnstructuredGridReader> expectedPReader;
  vtkXMLReader* oreader;
  if (parallel)
  {
    fileName = dataRoot + "/Data/can-pvtu.hdf";
    expectedName = dataRoot + "/Data/can.pvtu";
    oreader = expectedPReader;
  }
  else
  {
    fileName = dataRoot + "/Data/can-vtu.hdf";
    expectedName = dataRoot + "/Data/can.vtu";
    oreader = expectedReader;
  }
  std::cout << "Testing: " << fileName << std::endl;
  if (!reader->CanReadFile(fileName.c_str()))
  {
    return EXIT_FAILURE;
  }
  reader->SetFileName(fileName.c_str());
  reader->SetMergeParts(false);
  reader->Update();

  auto pds = vtkPartitionedDataSet::SafeDownCast(reader->GetOutput());
  if (!pds)
  {
    return EXIT_FAILURE;
  }
  vtkNew<vtkAppendDataSets> appender;
  for (unsigned int iPiece = 0; iPiece < pds->GetNumberOfPartitions(); ++iPiece)
  {
    auto piece = vtkUnstructuredGrid::SafeDownCast(pds->GetPartition(iPiece));
    appender->AddInputData(piece);
  }
  appender->Update();

  auto data = vtkUnstructuredGrid::SafeDownCast(appender->GetOutput());

  oreader->SetFileName(expectedName.c_str());
  oreader->Update();
  vtkUnstructuredGrid* expectedData =
    vtkUnstructuredGrid::SafeDownCast(oreader->GetOutputAsDataSet());
  return !vtkTestUtilities::CompareDataObjects(data, expectedData);
}

//----------------------------------------------------------------------------
int TestPolyData(const std::string& dataRoot)
{
  const std::string expectedName = dataRoot + "/Data/hdf_poly_data_twin.vtp";
  vtkNew<vtkXMLPolyDataReader> expectedReader;
  expectedReader->SetFileName(expectedName.c_str());
  expectedReader->Update();
  auto expectedData = vtkPolyData::SafeDownCast(expectedReader->GetOutput());

  const std::string fileName = dataRoot + "/Data/test_poly_data.hdf";
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(fileName.c_str());
  reader->Update();
  auto data = vtkPolyData::SafeDownCast(reader->GetOutputAsDataSet());

  return !vtkTestUtilities::CompareDataObjects(data, expectedData);
}

//----------------------------------------------------------------------------
int TestPartitionedPolyData(const std::string& dataRoot)
{
  const std::string expectedName = dataRoot + "/Data/hdf_poly_data_twin.vtp";
  vtkNew<vtkXMLPolyDataReader> expectedReader;
  expectedReader->SetFileName(expectedName.c_str());
  expectedReader->Update();
  auto expectedData = vtkPolyData::SafeDownCast(expectedReader->GetOutput());

  const std::string fileName = dataRoot + "/Data/test_poly_data.hdf";
  vtkNew<vtkHDFReader> reader;
  reader->SetMergeParts(false);
  reader->SetFileName(fileName.c_str());
  reader->Update();

  auto pds = vtkPartitionedDataSet::SafeDownCast(reader->GetOutput());
  if (!pds)
  {
    return EXIT_FAILURE;
  }
  vtkNew<vtkAppendDataSets> appender;
  appender->SetOutputDataSetType(VTK_POLY_DATA);
  for (unsigned int iPiece = 0; iPiece < pds->GetNumberOfPartitions(); ++iPiece)
  {
    auto piece = vtkPolyData::SafeDownCast(pds->GetPartition(iPiece));
    appender->AddInputData(piece);
  }
  appender->Update();

  auto data = vtkPolyData::SafeDownCast(appender->GetOutput());

  return !vtkTestUtilities::CompareDataObjects(data, expectedData);
}

//----------------------------------------------------------------------------
int TestOverlappingAMR(const std::string& dataRoot)
{
  std::string fileName = dataRoot + "/Data/amr_gaussian_pulse.hdf";
  std::cout << "Testing: " << fileName << std::endl;
  vtkNew<vtkHDFReader> reader;
  if (!reader->CanReadFile(fileName.c_str()))
  {
    return EXIT_FAILURE;
  }
  reader->SetFileName(fileName.c_str());
  reader->Update();
  auto data = vtkOverlappingAMR::SafeDownCast(reader->GetOutput());

  vtkNew<vtkXMLUniformGridAMRReader> outputReader;
  std::string expectedFileName = dataRoot + "/Data/amr_gaussian_pulse.vthb";
  outputReader->SetFileName(expectedFileName.c_str());
  outputReader->SetMaximumLevelsToReadByDefault(0);
  outputReader->Update();
  auto expectedData = vtkOverlappingAMR::SafeDownCast(outputReader->GetOutput());

  if (data->GetNumberOfLevels() != expectedData->GetNumberOfLevels())
  {
    std::cerr << "Number of levels does not match. Expected: " << expectedData->GetNumberOfLevels()
              << " got: " << data->GetNumberOfLevels() << std::endl;
    return EXIT_FAILURE;
  }

  for (unsigned int levelIndex = 0; levelIndex < expectedData->GetNumberOfLevels(); ++levelIndex)
  {
    if (data->GetNumberOfDataSets(levelIndex) != expectedData->GetNumberOfDataSets(levelIndex))
    {
      std::cerr << "Number of datasets does not match for level " << levelIndex
                << ". Expected: " << expectedData->GetNumberOfDataSets(0)
                << " got: " << data->GetNumberOfDataSets(0) << std::endl;
      return EXIT_FAILURE;
    }

    for (unsigned int datasetIndex = 0;
         datasetIndex < expectedData->GetNumberOfDataSets(levelIndex); ++datasetIndex)
    {
      auto dataset = data->GetDataSet(levelIndex, datasetIndex);
      auto expectedDataset = expectedData->GetDataSet(levelIndex, datasetIndex);
      if (!vtkTestUtilities::CompareDataObjects(dataset, expectedDataset))
      {
        std::cerr << "Datasets does not match for level " << levelIndex << " dataset "
                  << datasetIndex << std::endl;
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestCompositeDataSet(const std::string& dataRoot)
{
  // This dataset is composed of 4 blocks : 2 polydata, 1 unstructured grid, 1 image data
  const std::string hdfPath = dataRoot + "/Data/vtkHDF/test_composite.hdf";
  vtkNew<vtkHDFReader> expectedReader;
  expectedReader->SetFileName(hdfPath.c_str());
  expectedReader->Update();
  auto expectedData = vtkPartitionedDataSetCollection::SafeDownCast(expectedReader->GetOutput());

  const std::string vtpcPath = dataRoot + "/Data/vtkHDF/test_composite.hdf_000000.vtpc";
  vtkNew<vtkXMLPartitionedDataSetCollectionReader> reader;
  reader->SetFileName(vtpcPath.c_str());
  reader->Update();
  auto data = vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutput());

  return !vtkTestUtilities::CompareDataObjects(data, expectedData);
}

//------------------------------------------------------------------------------
int TestHDFReader(int argc, char* argv[])
{
  vtkNew<vtkTesting> testHelper;
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
  {
    std::cerr << "Error: -D /path/to/data was not specified.";
    return EXIT_FAILURE;
  }

  std::string dataRoot = testHelper->GetDataRoot();
  if (TestImageData(dataRoot))
  {
    return EXIT_FAILURE;
  }

  if (TestImageCellData(dataRoot))
  {
    return EXIT_FAILURE;
  }

  if (TestUnstructuredGrid(dataRoot, false))
  {
    return EXIT_FAILURE;
  }
  if (TestUnstructuredGrid(dataRoot, true))
  {
    return EXIT_FAILURE;
  }

  if (TestPolyData(dataRoot))
  {
    return EXIT_FAILURE;
  }

  if (TestOverlappingAMR(dataRoot))
  {
    return EXIT_FAILURE;
  }

  if (TestPartitionedPolyData(dataRoot))
  {
    return EXIT_FAILURE;
  }

  if (TestPartitionedUnstructuredGrid(dataRoot, false))
  {
    return EXIT_FAILURE;
  }

  if (TestPartitionedUnstructuredGrid(dataRoot, true))
  {
    return EXIT_FAILURE;
  }

  if (TestCompositeDataSet(dataRoot))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
