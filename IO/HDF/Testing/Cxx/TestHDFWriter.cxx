// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkConvertToPartitionedDataSetCollection.h"
#include "vtkHDFReader.h"
#include "vtkHDFWriter.h"
#include "vtkIOSSReader.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPolyData.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkXMLMultiBlockDataReader.h"
#include "vtkXMLPartitionedDataSetCollectionReader.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkXMLUnstructuredGridReader.h"

#include <string>

//----------------------------------------------------------------------------
bool TestEmptyPolyData(const std::string& tempDir)
{
  std::string filePath = tempDir + "/emptyPolyData.vtkhdf";
  vtkNew<vtkPolyData> pd;
  vtkNew<vtkHDFWriter> writer;
  writer->SetInputData(pd);
  writer->SetFileName(filePath.c_str());
  writer->Write();
  return true;
}

//----------------------------------------------------------------------------
bool TestWriteAndRead(vtkDataObject* data, const char* tempPath, bool outputAsMultiBlock = false)
{
  vtkNew<vtkHDFWriter> writer;
  writer->SetInputData(data);
  writer->SetFileName(tempPath);
  writer->SetOutputAsMultiBlockDataSet(outputAsMultiBlock);
  writer->Write();

  vtkNew<vtkHDFReader> reader;
  if (!reader->CanReadFile(tempPath))
  {
    std::cerr << "vtkHDFReader can not read file: " << tempPath << std::endl;
    return false;
  }
  reader->SetFileName(tempPath);
  reader->Update();
  vtkDataObject* output = vtkDataObject::SafeDownCast(reader->GetOutput());
  if (output == nullptr)
  {
    std::cerr << "vtkHDFReader does not output a vtkDataObject when reading: " << tempPath
              << std::endl;
    return false;
  }

  auto outputMB = vtkMultiBlockDataSet::SafeDownCast(output);
  auto inputMB = vtkMultiBlockDataSet::SafeDownCast(data);

  auto outputPDC = vtkPartitionedDataSetCollection::SafeDownCast(output);
  auto inputPDC = vtkPartitionedDataSetCollection::SafeDownCast(data);

  if (!vtkTestUtilities::CompareDataObjects(data, output))
  {
    std::cerr << "vtkDataObject does not match: " << tempPath << std::endl;
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool TestSpherePolyData(const std::string& tempDir)
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(100);
  sphere->SetPhiResolution(100);
  sphere->SetRadius(1);
  sphere->Update();
  vtkPolyData* spherePd = sphere->GetOutput();

  std::string filePath = tempDir + "/spherePolyData.vtkhdf";
  return TestWriteAndRead(spherePd, filePath.c_str());
}

//----------------------------------------------------------------------------
bool TestComplexPolyData(const std::string& tempDir, const std::string& dataRoot)
{
  std::vector<std::string> baseNames = { "cow.vtp", "isofill_0.vtp" };
  for (const auto& baseName : baseNames)
  {
    // Get a polydata from a VTP
    const std::string basePath = dataRoot + "/Data/" + baseName;
    vtkNew<vtkXMLPolyDataReader> baseReader;
    baseReader->SetFileName(basePath.c_str());
    baseReader->Update();
    vtkPolyData* baseData = vtkPolyData::SafeDownCast(baseReader->GetOutput());
    if (baseData == nullptr)
    {
      std::cerr << "Can't read base data from: " << basePath << std::endl;
      return false;
    }

    // Write and read the polydata in a temp file, compare with base
    std::string tempPath = tempDir + "/HDFWriter_" + baseName + ".vtkhdf";
    if (!TestWriteAndRead(baseData, tempPath.c_str()))
    {
      return false;
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool TestUnstructuredGrid(const std::string& tempDir, const std::string& dataRoot)
{
  std::vector<std::string> baseNames = { "explicitStructuredGrid.vtu",
    "explicitStructuredGridEmpty.vtu", "elements.vtu" };
  for (const auto& baseName : baseNames)
  {
    // Get an Unstructured grid from a VTU
    const std::string basePath = dataRoot + "/Data/" + baseName;
    vtkNew<vtkXMLUnstructuredGridReader> baseReader;
    baseReader->SetFileName(basePath.c_str());
    baseReader->Update();
    vtkUnstructuredGrid* baseData = vtkUnstructuredGrid::SafeDownCast(baseReader->GetOutput());
    if (baseData == nullptr)
    {
      std::cerr << "Can't read base data from: " << basePath << std::endl;
      return false;
    }

    // Write and read the unstructuredGrid in a temp file, compare with base
    std::string tempPath = tempDir + "/HDFWriter_" + baseName + ".vtkhdf";
    if (!TestWriteAndRead(baseData, tempPath.c_str()))
    {
      return false;
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool TestMultiBlock(const std::string& tempDir, const std::string& dataRoot)
{
  vtkLog(INFO, "TestMultiBlock");
  std::vector<std::string> baseNamesMB = { "test_multiblock_hdf.vtm" };
  for (const auto& baseName : baseNamesMB)
  {
    // Get an Unstructured grid from a VTU
    const std::string basePath = dataRoot + "/Data/vtkHDF/" + baseName;
    vtkNew<vtkXMLMultiBlockDataReader> baseReader;
    baseReader->SetFileName(basePath.c_str());
    baseReader->Update();
    vtkMultiBlockDataSet* baseData = vtkMultiBlockDataSet::SafeDownCast(baseReader->GetOutput());
    if (baseData == nullptr)
    {
      std::cerr << "Can't read base data from: " << basePath << std::endl;
      return false;
    }

    // Write and read the vtkMultiBlockDataSet in a temp file, compare with base
    std::string tempPath = tempDir + "/HDFWriter_" + baseName + ".vtkhdf";
    if (!TestWriteAndRead(baseData, tempPath.c_str(), true))
    {
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------------------
bool TestPartitionedDataSetCollection(const std::string& tempDir, const std::string& dataRoot)
{
  vtkLog(INFO, "TestPartitionedDataSetCollection");
  std::vector<std::string> baseNamesMB = { "dummy_pdc_structure.vtpc" };
  for (const auto& baseName : baseNamesMB)
  {
    // Get an Unstructured grid from a VTU
    const std::string basePath = dataRoot + "/Data/vtkHDF/" + baseName;
    vtkNew<vtkXMLPartitionedDataSetCollectionReader> baseReader;
    baseReader->SetFileName(basePath.c_str());
    baseReader->Update();
    vtkPartitionedDataSetCollection* baseData =
      vtkPartitionedDataSetCollection::SafeDownCast(baseReader->GetOutput());
    if (baseData == nullptr)
    {
      std::cerr << "Can't read base data from: " << basePath << std::endl;
      return false;
    }

    // Write and read the vtkPartitionedDataSetCollection in a temp file, compare with base
    std::string tempPath = tempDir + "/HDFWriter_" + baseName + ".vtkhdf";
    if (!TestWriteAndRead(baseData, tempPath.c_str()))
    {
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------------------
int TestHDFWriter(int argc, char* argv[])
{
  // Get temporary testing directory
  char* tempDirCStr =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  std::string tempDir{ tempDirCStr };
  delete[] tempDirCStr;

  // Get data directory
  vtkNew<vtkTesting> testHelper;
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
  {
    std::cerr << "Error: -D /path/to/data was not specified." << std::endl;
    return EXIT_FAILURE;
  }
  std::string dataRoot = testHelper->GetDataRoot();

  // Run tests
  bool testPasses = true;
  // testPasses &= TestEmptyPolyData(tempDir);
  // testPasses &= TestSpherePolyData(tempDir);
  // testPasses &= TestComplexPolyData(tempDir, dataRoot);
  // testPasses &= TestUnstructuredGrid(tempDir, dataRoot);
  // testPasses &= TestPartitionedDataSetCollection(tempDir, dataRoot);
  testPasses &= TestMultiBlock(tempDir, dataRoot);

  return testPasses ? EXIT_SUCCESS : EXIT_FAILURE;
}
