// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAppendDataSets.h"
#include "vtkFloatArray.h"
#include "vtkHDFReader.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridSource.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkMathUtilities.h"
#include "vtkMergeBlocks.h"
#include "vtkNew.h"
#include "vtkOverlappingAMR.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRandomHyperTreeGridSource.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkUniformGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLHyperTreeGridReader.h"
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
vtkSmartPointer<vtkDataObject> GetMergedBlocks(vtkHDFReader* reader, int output_type)
{
  reader->Update();
  vtkPartitionedDataSet* pds = vtkPartitionedDataSet::SafeDownCast(reader->GetOutputDataObject(0));

  // Emulate the late "MergeParts" option of the VTKHDF Reader
  vtkNew<vtkAppendDataSets> append;
  append->SetOutputDataSetType(output_type);
  for (unsigned int iPiece = 0; iPiece < pds->GetNumberOfPartitions(); ++iPiece)
  {
    append->AddInputData(pds->GetPartition(iPiece));
  }
  append->Update();
  return append->GetOutputDataObject(0);
}

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

  if (parallel)
  {
    vtkPartitionedDataSet* pds =
      vtkPartitionedDataSet::SafeDownCast(reader->GetOutputDataObject(0));
    if (pds->GetNumberOfPartitions() != 3)
    {
      std::cerr << "Error: expected 3 partitions in unstructured grid but got "
                << pds->GetNumberOfPartitions() << std::endl;
      return EXIT_FAILURE;
    }
  }

  oreader->SetFileName(expectedName.c_str());
  oreader->Update();
  vtkUnstructuredGrid* expectedData =
    vtkUnstructuredGrid::SafeDownCast(oreader->GetOutputAsDataSet());
  if (parallel)
  {
    return !vtkTestUtilities::CompareDataObjects(
      GetMergedBlocks(reader, VTK_UNSTRUCTURED_GRID), expectedData);
  }
  else
  {
    return !vtkTestUtilities::CompareDataObjects(reader->GetOutputDataObject(0), expectedData);
  }
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
  reader->Update();

  vtkNew<vtkUnstructuredGrid> data;
  if (parallel)
  {
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

    data->ShallowCopy(vtkUnstructuredGrid::SafeDownCast(appender->GetOutput()));
  }
  else
  {
    data->ShallowCopy(vtkUnstructuredGrid::SafeDownCast(reader->GetOutput()));
  }

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

  vtkPartitionedDataSet* pds = vtkPartitionedDataSet::SafeDownCast(reader->GetOutputDataObject(0));
  if (pds->GetNumberOfPartitions() != 2)
  {
    std::cerr << "Error: expected 2 partitions in polydata but got " << pds->GetNumberOfPartitions()
              << std::endl;
    return EXIT_FAILURE;
  }

  return !vtkTestUtilities::CompareDataObjects(
    GetMergedBlocks(reader, VTK_POLY_DATA), expectedData);
}

//----------------------------------------------------------------------------
int TestNullTerminatedString(const std::string& dataRoot)
{
  // File contains a 'Type' attributed that ends with a '\0' character.
  // Make sure we erase it before using the string.
  const std::string fileName = dataRoot + "/Data/vtkHDF/null_term_string.vtkhdf";
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(fileName.c_str());
  reader->Update();

  if (!vtkPolyData::SafeDownCast(reader->GetOutputDataObject(0)))
  {
    std::cerr << "Error: could not read null-terminated 'Type' attribute";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------
int TestUTF8Type(const std::string& dataRoot)
{
  // File contains a UTF-8 'Type' attribute
  const std::string fileName = dataRoot + "/Data/vtkHDF/utf8_string.vtkhdf";
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(fileName.c_str());
  reader->Update();

  if (!vtkImageData::SafeDownCast(reader->GetOutputDataObject(0)))
  {
    std::cerr << "Error: could not read UTF-8 'Type' attribute";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int TestPartitionedPolyData(const std::string& dataRoot)
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
  // This dataset is composed of 4 blocks : 2 polydata, 1 image data, 1 unstructured grid, 1
  // HyperTreeGrid
  const std::string hdfPath = dataRoot + "/Data/vtkHDF/test_composite.hdf";
  std::cout << "Testing: " << hdfPath << std::endl;

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
int TestRandomHyperTreeGrid(const std::string& dataRoot)
{
  const std::string hdfPath = dataRoot + "/Data/vtkHDF/randomhtg.hdf";
  std::cout << "Testing: " << hdfPath << std::endl;

  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(hdfPath.c_str());
  reader->Update();
  vtkHyperTreeGrid* readData = vtkHyperTreeGrid::SafeDownCast(reader->GetOutput());

  vtkNew<vtkRandomHyperTreeGridSource> source;
  source->SetSeed(123);
  source->SetDimensions(3, 3, 3);
  source->SetSplitFraction(0.75);
  source->SetMaskedFraction(0.25);
  source->Update();
  vtkHyperTreeGrid* expectedHTG = source->GetHyperTreeGridOutput();

  return !vtkTestUtilities::CompareDataObjects(expectedHTG, readData);
}

//------------------------------------------------------------------------------
int TestSimpleHyperTreeGrid(const std::string& dataRoot)
{
  const std::string hdfPath = dataRoot + "/Data/vtkHDF/simple_htg.hdf";
  std::cout << "Testing: " << hdfPath << std::endl;

  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(hdfPath.c_str());
  reader->Update();
  vtkHyperTreeGrid* readData = vtkHyperTreeGrid::SafeDownCast(reader->GetOutput());

  if (readData->GetNumberOfCells() != 44)
  {
    std::cerr << "Error: expected 44 cells in HTG but got " << readData->GetNumberOfCells()
              << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestPartitionedHyperTreeGrid(const std::string& dataRoot)
{
  const std::string hdfPath = dataRoot + "/Data/vtkHDF/multipiece_htg.hdf";
  std::cout << "Testing: " << hdfPath << std::endl;

  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(hdfPath.c_str());
  reader->Update();
  vtkPartitionedDataSet* readData = vtkPartitionedDataSet::SafeDownCast(reader->GetOutput());

  vtkNew<vtkHyperTreeGridSource> htgSource;
  htgSource->SetBranchFactor(2);
  htgSource->SetDimensions(6, 4, 1);
  htgSource->SetMaxDepth(2);
  htgSource->SetUseMask(true);

  htgSource->SetDescriptor("... .R. ... ... ... | ....");
  htgSource->SetMask("111 111 111 000 000 | 1111");
  htgSource->Update();

  vtkHyperTreeGrid* expectedHTG = htgSource->GetHyperTreeGridOutput();
  vtkHyperTreeGrid* readHTG = vtkHyperTreeGrid::SafeDownCast(readData->GetPartitionAsDataObject(0));

  if (!vtkTestUtilities::CompareDataObjects(expectedHTG, readHTG))
  {
    std::cerr << "HyperTreeGrids are not the same for part 0" << std::endl;
    return EXIT_FAILURE;
  }

  htgSource->SetDescriptor("... ... ... .R. ... | ....");
  htgSource->SetMask("000 000 000 111 111 | 1111");
  htgSource->Update();

  expectedHTG = htgSource->GetHyperTreeGridOutput();
  readHTG = vtkHyperTreeGrid::SafeDownCast(readData->GetPartitionAsDataObject(1));

  if (!vtkTestUtilities::CompareDataObjects(expectedHTG, readHTG))
  {
    std::cerr << "HyperTreeGrids are not the same for part 1" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestHyperTreeGridWithInterfaces(const std::string& dataRoot)
{
  const std::string hdfPath = dataRoot + "/Data/vtkHDF/shell_3d.hdf";
  const std::string xmlPath = dataRoot + "/Data/HTG/shell_3d.htg";
  std::cout << "Testing: " << hdfPath << std::endl;

  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(hdfPath.c_str());
  reader->Update();
  vtkHyperTreeGrid* readData = vtkHyperTreeGrid::SafeDownCast(reader->GetOutput());

  vtkNew<vtkXMLHyperTreeGridReader> xmlReader;
  xmlReader->SetFileName(xmlPath.c_str());
  xmlReader->Update();
  vtkHyperTreeGrid* readDataXML = vtkHyperTreeGrid::SafeDownCast(xmlReader->GetOutput());

  if (!vtkTestUtilities::CompareDataObjects(readData, readDataXML))
  {
    std::cerr << "HyperTreeGrids are not the same for part 0" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
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

  if (TestNullTerminatedString(dataRoot))
  {
    return EXIT_FAILURE;
  }

  if (TestUTF8Type(dataRoot))
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

  if (TestSimpleHyperTreeGrid(dataRoot))
  {
    return EXIT_FAILURE;
  }
  if (TestRandomHyperTreeGrid(dataRoot))
  {
    return EXIT_FAILURE;
  }
  if (TestPartitionedHyperTreeGrid(dataRoot))
  {
    return EXIT_FAILURE;
  }
  if (TestHyperTreeGridWithInterfaces(dataRoot))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
