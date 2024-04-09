// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHDFReader.h"
#include "vtkHDFWriter.h"
#include "vtkImageData.h"
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

#include "vtkHDF5ScopedHandle.h"
#include "vtk_hdf5.h"

#include <string>

//----------------------------------------------------------------------------
bool WriteMiscData(const std::string& filename)
{
  // Appending groups alongside "VTKHDF" in the file should not alter how the reader behaves
  vtkHDF::ScopedH5FHandle file{ H5Fopen(filename.c_str(), H5F_ACC_RDWR, H5P_DEFAULT) };
  if (file == H5I_INVALID_HID)
  {
    std::cerr << "Could not re-open " << filename << file << " for writing";
    return false;
  }

  // Create groups
  vtkHDF::ScopedH5GHandle misc{ H5Gcreate(file, "Misc", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) };
  vtkHDF::ScopedH5GHandle misc2{ H5Gcreate(file, "VTKHD", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) };

  // Create more groups inside
  vtkHDF::ScopedH5GHandle data1{ H5Gcreate(misc, "Data1", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) };
  vtkHDF::ScopedH5GHandle data2{ H5Gcreate(misc, "Data2", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) };
  vtkHDF::ScopedH5GHandle data3{ H5Gcreate(data1, "Data3", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) };

  // Create dataspace
  vtkHDF::ScopedH5SHandle dataspace{ H5Screate(H5S_SIMPLE) };
  std::array<hsize_t, 1> dimensions{ 3 };
  H5Sset_extent_simple(dataspace, 1, dimensions.data(), dimensions.data());

  // Create and fill dataset
  vtkHDF::ScopedH5DHandle dataset = H5Dcreate(
    data3, "MiscDataset", H5T_STD_I64LE, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  std::array<int, 3> values{ 4, 5, 3 };
  H5Dwrite(dataset, H5T_STD_I64LE, H5S_ALL, dataspace, H5P_DEFAULT, values.data());

  return true;
}

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
bool TestWriteAndRead(vtkDataObject* data, const std::string& tempPath)
{
  vtkNew<vtkHDFWriter> writer;
  writer->SetInputData(data);
  writer->SetFileName(tempPath.c_str());
  writer->Write();
  if (!WriteMiscData(tempPath))
  {
    return false;
  }

  vtkNew<vtkHDFReader> reader;
  if (!reader->CanReadFile(tempPath.c_str()))
  {
    std::cerr << "vtkHDFReader can not read file: " << tempPath << std::endl;
    return false;
  }
  reader->SetFileName(tempPath.c_str());
  reader->Update();
  vtkDataObject* output = vtkDataObject::SafeDownCast(reader->GetOutput());
  if (output == nullptr)
  {
    std::cerr << "vtkHDFReader does not output a vtkDataObject when reading: " << tempPath
              << std::endl;
    return false;
  }

  if (!vtkTestUtilities::CompareDataObjects(output, data))
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
  return TestWriteAndRead(spherePd, filePath);
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
    if (!TestWriteAndRead(baseData, tempPath))
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
    if (!TestWriteAndRead(baseData, tempPath))
    {
      return false;
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool TestMultiBlock(const std::string& tempDir, const std::string& dataRoot)
{
  std::vector<std::string> baseNamesMB = { "test_multiblock_hdf.vtm" };
  for (const auto& baseName : baseNamesMB)
  {
    // Read the multiblock from vtm file
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
    if (!TestWriteAndRead(baseData, tempPath))
    {
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------------------
bool TestPartitionedDataSetCollection(const std::string& tempDir, const std::string& dataRoot)
{
  std::vector<std::string> baseNamesMB = { "dummy_pdc_structure.vtpc", "multi_ds_pdc.vtpc" };
  for (const auto& baseName : baseNamesMB)
  {
    // Get a PDC from a vtpc file
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
    if (!TestWriteAndRead(baseData, tempPath))
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
  testPasses &= TestEmptyPolyData(tempDir);
  testPasses &= TestSpherePolyData(tempDir);
  testPasses &= TestComplexPolyData(tempDir, dataRoot);
  testPasses &= TestUnstructuredGrid(tempDir, dataRoot);
  testPasses &= TestPartitionedDataSetCollection(tempDir, dataRoot);
  testPasses &= TestMultiBlock(tempDir, dataRoot);

  return testPasses ? EXIT_SUCCESS : EXIT_FAILURE;
}
