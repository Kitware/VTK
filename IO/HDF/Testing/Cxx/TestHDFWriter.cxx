// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "HDFTestUtilities.h"

#include "vtkCellData.h"
#include "vtkConeSource.h"
#include "vtkDataArray.h"
#include "vtkDataArraySelection.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkGroupDataSetsFilter.h"
#include "vtkHDF5ScopedHandle.h"
#include "vtkHDFReader.h"
#include "vtkHDFWriter.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridSource.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRandomHyperTreeGridSource.h"
#include "vtkRectilinearGrid.h"
#include "vtkSphereSource.h"
#include "vtkStructuredGrid.h"
#include "vtkTable.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLHyperTreeGridWriter.h"
#include "vtkXMLMultiBlockDataReader.h"
#include "vtkXMLPartitionedDataSetCollectionReader.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkXMLTableReader.h"
#include "vtkXMLUnstructuredGridReader.h"

#include <cstddef>
#include <iostream>
#include <string>

namespace
{
struct WriterConfigOptions
{
  bool UseExternalPartitions;
  bool UseExternalComposite;
  std::string FileNameSuffix;
  int CompressionLevel;
};
}
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
  std::array<hsize_t, 3> values{ 4, 5, 3 };
  H5Dwrite(dataset, H5T_STD_I64LE, H5S_ALL, dataspace, H5P_DEFAULT, values.data());

  return true;
}

//----------------------------------------------------------------------------
bool TestEmptyPolyData(const std::string& tempDir)
{
  std::string filePath = tempDir + "/emptyPolyData.vtkhdf";
  vtkNew<vtkPolyData> pd;
  vtkNew<vtkDoubleArray> arr;
  pd->GetCellData()->AddArray(arr);
  vtkNew<vtkHDFWriter> writer;
  writer->SetInputData(pd);
  writer->SetFileName(filePath.c_str());
  writer->Write();
  return true;
}

//----------------------------------------------------------------------------
bool TestWriteAndRead(
  vtkDataObject* data, const std::string& tempPath, WriterConfigOptions* options = nullptr)
{
  std::string fullPath = tempPath;
  vtkNew<vtkHDFWriter> writer;
  writer->SetInputData(data);
  if (options)
  {
    fullPath = tempPath + options->FileNameSuffix + ".vtkhdf";
    writer->SetUseExternalComposite(options->UseExternalComposite);
    writer->SetUseExternalPartitions(options->UseExternalPartitions);
    writer->SetCompressionLevel(options->CompressionLevel);

    vtkLog(INFO,
      "Testing " << fullPath << " with options Ext composite: " << options->UseExternalComposite
                 << " ext partitions: " << options->UseExternalPartitions << " compression "
                 << options->CompressionLevel);
  }

  writer->SetFileName(fullPath.c_str());
  writer->Write();

  // Append data that should be ignored by the reader
  if (!WriteMiscData(fullPath))
  {
    return false;
  }

  vtkNew<vtkHDFReader> reader;
  if (!reader->CanReadFile(fullPath.c_str()))
  {
    std::cerr << "vtkHDFReader can not read file: " << fullPath << std::endl;
    return false;
  }
  reader->SetFileName(fullPath.c_str());
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
bool TestWriteAndReadConfigurations(vtkDataObject* data, const std::string& path)
{
  std::vector<WriterConfigOptions> options{ { false, false, "_NoExtPartNoExtComp", 3 },
    { false, true, "_NoExtPartExtComp", 1 }, { true, true, "_ExtPartExtComp", 2 },
    { true, false, "_ExtPartNoExtComp", 5 } };

  for (auto& optionSet : options)
  {
    if (!TestWriteAndRead(data, path, &optionSet))
    {
      return false;
    }
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
  const std::vector<std::string> baseNames = { "cow.vtp", "isofill_0.vtp" };
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
bool TestImageDataWriteRead(const std::string& tempDir)
{
  vtkNew<vtkImageData> imageData;
  imageData->SetExtent(0, 3, 0, 2, 0, 1);
  imageData->SetOrigin(1.0, 2.0, 3.0);
  imageData->SetSpacing(0.5, 1.0, 2.0);

  vtkIdType numPoints = imageData->GetNumberOfPoints();
  vtkNew<vtkDoubleArray> scalars;
  scalars->SetName("PointScalars");
  scalars->SetNumberOfComponents(1);
  scalars->SetNumberOfTuples(numPoints);
  for (vtkIdType idx = 0; idx < numPoints; ++idx)
  {
    scalars->SetValue(idx, static_cast<double>(idx));
  }
  imageData->GetPointData()->SetScalars(scalars);

  vtkNew<vtkDoubleArray> vectors;
  vectors->SetName("PointVectors");
  vectors->SetNumberOfComponents(3);
  vectors->SetNumberOfTuples(numPoints);
  for (vtkIdType idx = 0; idx < numPoints; ++idx)
  {
    double tuple[3] = { 1.0 * idx, 2.0 * idx, 3.0 * idx };
    vectors->SetTypedTuple(idx, tuple);
  }
  imageData->GetPointData()->SetVectors(vectors);

  std::string filePath = tempDir + "/HDFWriter_imageData.vtkhdf";
  return TestWriteAndRead(imageData, filePath);
}

//----------------------------------------------------------------------------
bool TestRectilinearGridWriteRead(const std::string& tempDir)
{
  vtkNew<vtkRectilinearGrid> rectilinearGrid;
  int dimensions[3] = { 4, 3, 2 };
  rectilinearGrid->SetDimensions(dimensions);

  vtkNew<vtkDoubleArray> xCoords;
  xCoords->SetName("XCoordinates");
  xCoords->SetNumberOfTuples(dimensions[0]);
  for (int i = 0; i < dimensions[0]; ++i)
  {
    xCoords->SetValue(i, static_cast<double>(i));
  }

  vtkNew<vtkDoubleArray> yCoords;
  yCoords->SetName("YCoordinates");
  yCoords->SetNumberOfTuples(dimensions[1]);
  for (int j = 0; j < dimensions[1]; ++j)
  {
    yCoords->SetValue(j, static_cast<double>(j) * 2.0);
  }

  vtkNew<vtkDoubleArray> zCoords;
  zCoords->SetName("ZCoordinates");
  zCoords->SetNumberOfTuples(dimensions[2]);
  for (int k = 0; k < dimensions[2]; ++k)
  {
    zCoords->SetValue(k, static_cast<double>(k) * 3.0);
  }

  rectilinearGrid->SetXCoordinates(xCoords);
  rectilinearGrid->SetYCoordinates(yCoords);
  rectilinearGrid->SetZCoordinates(zCoords);

  vtkIdType numPoints = static_cast<vtkIdType>(dimensions[0]) * dimensions[1] * dimensions[2];
  vtkNew<vtkDoubleArray> scalars;
  scalars->SetName("PointScalars");
  scalars->SetNumberOfComponents(1);
  scalars->SetNumberOfTuples(numPoints);
  for (vtkIdType idx = 0; idx < numPoints; ++idx)
  {
    scalars->SetValue(idx, static_cast<double>(idx));
  }
  rectilinearGrid->GetPointData()->SetScalars(scalars);

  vtkNew<vtkDoubleArray> vectors;
  vectors->SetName("PointVectors");
  vectors->SetNumberOfComponents(3);
  vectors->SetNumberOfTuples(numPoints);
  for (vtkIdType idx = 0; idx < numPoints; ++idx)
  {
    double tuple[3] = { 1.0 * idx, 2.0 * idx, 3.0 * idx };
    vectors->SetTypedTuple(idx, tuple);
  }
  rectilinearGrid->GetPointData()->SetVectors(vectors);

  std::string filePath = tempDir + "/HDFWriter_rectilinearGrid.vtkhdf";
  return TestWriteAndRead(rectilinearGrid, filePath);
}

//----------------------------------------------------------------------------
bool TestStructuredGridWriteRead(const std::string& tempDir)
{
  vtkNew<vtkStructuredGrid> structuredGrid;
  int dimensions[3] = { 3, 3, 2 };
  structuredGrid->SetDimensions(dimensions);

  vtkNew<vtkPoints> points;
  vtkIdType numPoints = static_cast<vtkIdType>(dimensions[0]) * dimensions[1] * dimensions[2];
  points->SetNumberOfPoints(numPoints);
  vtkIdType pointIndex = 0;
  for (int k = 0; k < dimensions[2]; ++k)
  {
    for (int j = 0; j < dimensions[1]; ++j)
    {
      for (int i = 0; i < dimensions[0]; ++i)
      {
        points->SetPoint(pointIndex++, i * 1.0, j * 2.0, k * 3.0);
      }
    }
  }
  structuredGrid->SetPoints(points);

  vtkNew<vtkDoubleArray> scalars;
  scalars->SetName("PointScalars");
  scalars->SetNumberOfComponents(1);
  scalars->SetNumberOfTuples(numPoints);
  for (vtkIdType idx = 0; idx < numPoints; ++idx)
  {
    scalars->SetValue(idx, static_cast<double>(idx));
  }
  structuredGrid->GetPointData()->SetScalars(scalars);

  vtkNew<vtkDoubleArray> vectors;
  vectors->SetName("PointVectors");
  vectors->SetNumberOfComponents(3);
  vectors->SetNumberOfTuples(numPoints);
  for (vtkIdType idx = 0; idx < numPoints; ++idx)
  {
    double tuple[3] = { 1.0 * idx, 2.0 * idx, 3.0 * idx };
    vectors->SetTypedTuple(idx, tuple);
  }
  structuredGrid->GetPointData()->SetVectors(vectors);

  std::string filePath = tempDir + "/HDFWriter_structuredGrid.vtkhdf";
  return TestWriteAndRead(structuredGrid, filePath);
}

//----------------------------------------------------------------------------
bool TestDataSetAttributes(const std::string& tempDir)
{
  vtkNew<vtkUnstructuredGrid> ug;
  vtkNew<vtkFloatArray> floats;
  floats->SetName("scals");
  ug->GetPointData()->SetScalars(floats);

  vtkNew<vtkIdTypeArray> ids;
  ids->SetName("GlobIds");
  ug->GetCellData()->SetGlobalIds(ids);

  vtkNew<vtkHDFWriter> writer;
  writer->SetInputData(ug);
  const std::string filename = tempDir + "/HDFWriter_attrs.vtkhdf";
  writer->SetFileName(filename.c_str());
  writer->Write();

  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();

  vtkUnstructuredGrid* readUg = vtkUnstructuredGrid::SafeDownCast(reader->GetOutputAsDataSet());
  vtkDataArray* readScalars = readUg->GetPointData()->GetScalars();
  vtkDataArray* readGlobIds = readUg->GetCellData()->GetGlobalIds();

  if (readScalars->GetName() != std::string("scals"))
  {
    std::cerr << "Expected scalars array 'scals'" << std::endl;
    return false;
  }

  if (readGlobIds->GetName() != std::string("GlobIds"))
  {
    std::cerr << "Expected Global Ids array 'scals'" << std::endl;
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool TestSanitizeName(const std::string& tempDir, const std::string& dataRoot)
{
  // Write data with a field name using slashes, that must be replaced to comply with the VTKHDF
  // standard.
  std::string baseName = "vtkHDF/sanitization.vtu";
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

  std::string fullPath = tempDir + "/HDFWriter_sanitization.vtkhdf";
  vtkNew<vtkHDFWriter> writer;
  writer->SetFileName(fullPath.c_str());
  writer->SetInputConnection(baseReader->GetOutputPort());
  writer->Write();

  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(fullPath.c_str());
  reader->Update();
  auto readData = vtkUnstructuredGrid::SafeDownCast(reader->GetOutput());
  const std::string expectedName = "NAME_WITH_SLASH";
  const std::string realName = readData->GetCellData()->GetArray(0)->GetName();
  if (realName != expectedName)
  {
    std::cerr << "Written data does not contain sanitized field named " << expectedName
              << ". Found " << realName << " instead." << std::endl;
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool TestPartitionedUnstructuredGrid(const std::string& tempDir, const std::string& dataRoot)
{
  std::string baseName = "can-pvtu.vtkhdf";

  // Get an Partitioned Unstructured grid from a VTKHDF file
  const std::string basePath = dataRoot + "/Data/vtkHDF/" + baseName;
  vtkNew<vtkHDFReader> baseReader;
  baseReader->SetFileName(basePath.c_str());
  baseReader->Update();
  auto baseData = vtkPartitionedDataSet::SafeDownCast(baseReader->GetOutput());
  if (baseData == nullptr)
  {
    std::cerr << "Can't read base data from: " << basePath << std::endl;
    return false;
  }

  // Write and read the partitioned unstructuredGrid in a temp file, compare with base
  std::string tempPath = tempDir + "/HDFWriter_" + baseName;
  if (!TestWriteAndReadConfigurations(baseData, tempPath))
  {
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool TestPartitionedPolyData(const std::string& tempDir, const std::string& dataRoot)
{
  std::string baseName = "test_poly_data.vtkhdf";

  // Get an Partitioned PolyData from a VTKHDF file
  const std::string basePath = dataRoot + "/Data/vtkHDF/" + baseName;
  vtkNew<vtkHDFReader> baseReader;
  baseReader->SetFileName(basePath.c_str());
  baseReader->Update();
  auto baseData = vtkPartitionedDataSet::SafeDownCast(baseReader->GetOutput());
  if (baseData == nullptr)
  {
    std::cerr << "Can't read base data from: " << basePath << std::endl;
    return false;
  }

  // Write and read the partitioned PolyData in a temp file, compare with base
  std::string tempPath = tempDir + "/HDFWriter_" + baseName;
  if (!TestWriteAndReadConfigurations(baseData, tempPath))
  {
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool TestMultiBlock(const std::string& tempDir, const std::string& dataRoot)
{
  std::string baseName = "test_multiblock_hdf.vtm";

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
  std::string tempPath = tempDir + "/HDFWriter_" + baseName;
  if (!TestWriteAndReadConfigurations(baseData, tempPath))
  {
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool TestMultiBlockIdenticalBlockNames(const std::string& tempDir, const std::string& dataRoot)
{
  std::string baseName = "test_poly_data.vtkhdf";
  const std::string basePath = dataRoot + "/Data/vtkHDF/" + baseName;
  vtkNew<vtkHDFReader> baseReader;
  baseReader->SetFileName(basePath.c_str());
  baseReader->Update();
  vtkPartitionedDataSet* pds =
    vtkPartitionedDataSet::SafeDownCast(baseReader->GetOutputDataObject(0));
  vtkPolyData* pd = vtkPolyData::SafeDownCast(pds->GetPartition(0));

  // Create a nested MultiBlock with several times the same block
  vtkNew<vtkMultiBlockDataSet> subSubBlock;
  subSubBlock->SetNumberOfBlocks(2);
  subSubBlock->SetBlock(0u, pd);
  subSubBlock->SetBlock(1u, pd);
  subSubBlock->GetMetaData(0u)->Set(vtkCompositeDataSet::NAME(), "PolyData");
  subSubBlock->GetMetaData(1u)->Set(vtkCompositeDataSet::NAME(), "PolyData2");

  vtkNew<vtkMultiBlockDataSet> subBlock;
  subBlock->SetNumberOfBlocks(2);
  subBlock->SetBlock(0, subSubBlock);
  subBlock->SetBlock(1, pd);
  subBlock->GetMetaData(0u)->Set(vtkCompositeDataSet::NAME(), "Group");
  subBlock->GetMetaData(1u)->Set(vtkCompositeDataSet::NAME(), "PolyData");

  vtkNew<vtkMultiBlockDataSet> multiBlock;
  multiBlock->SetNumberOfBlocks(2);
  multiBlock->SetBlock(0, pd);
  multiBlock->SetBlock(1, subBlock);
  multiBlock->GetMetaData(0u)->Set(vtkCompositeDataSet::NAME(), "PolyData");
  multiBlock->GetMetaData(1u)->Set(vtkCompositeDataSet::NAME(), "Group");

  // Write and read the vtkMultiBlockDataSet in a temp file, compare with base
  std::string tempPath = tempDir + "/HDFWriter_multiblock_identical";
  if (!TestWriteAndReadConfigurations(multiBlock, tempPath))
  {
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool TestRandomHTG(const std::string& tempDir)
{
  vtkNew<vtkRandomHyperTreeGridSource> htgSource;
  htgSource->SetDimensions(5, 5, 5);
  htgSource->SetSplitFraction(0.5);
  htgSource->SetMaskedFraction(0.5);
  htgSource->Update();

  vtkHyperTreeGrid* htg = htgSource->GetHyperTreeGridOutput();
  // Write and read the vtkMultiBlockDataSet in a temp file, compare with base
  std::string tempPath = tempDir + "/HDFWriter_randomhtg";
  if (!TestWriteAndReadConfigurations(htg, tempPath))
  {
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool TestNullHTG(const std::string& tempDir)
{
  // Test that unititialized HTG can be written & read properly
  vtkNew<vtkHyperTreeGrid> htg; // Keep it uninitialized

  std::string tempPath = tempDir + "/HDFWriter_nullhtg";
  if (!TestWriteAndReadConfigurations(htg, tempPath))
  {
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool TestNoValidPartHTG(const std::string& tempDir)
{
  // Test that partitioned inside multiblock without vtkDataset (HTG is vtkDataObject) does not
  // cause an error
  vtkNew<vtkHyperTreeGrid> htg;
  htg->Initialize();

  vtkNew<vtkMultiPieceDataSet> multipiece;
  multipiece->SetNumberOfPieces(2);
  multipiece->SetPartition(0, htg);
  multipiece->SetPartition(1, htg);

  vtkNew<vtkMultiBlockDataSet> mbds;
  mbds->SetNumberOfBlocks(1);
  mbds->SetBlock(0, multipiece);

  std::string tempPath = tempDir + "/HDFWriter_nullpart";
  if (!TestWriteAndReadConfigurations(mbds, tempPath))
  {
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool TestSimpleHTG(const std::string& tempDir)
{
  vtkNew<vtkHyperTreeGridSource> htgSource;
  htgSource->SetDimensions(3, 3, 2);
  htgSource->SetBranchFactor(2);
  htgSource->SetMaxDepth(3);
  htgSource->SetDescriptor(".RRR|..R..... .R...... ........ | ........ ........");
  htgSource->SetUseMask(true);
  htgSource->SetMask("0111|11111111 11111111 11100111 | 01111111 11111101");
  htgSource->Update();

  vtkHyperTreeGrid* htg = htgSource->GetHyperTreeGridOutput();
  // Write and read the vtkMultiBlockDataSet in a temp file, compare with base
  std::string tempPath = tempDir + "/HDFWriter_simplehtg";
  if (!TestWriteAndReadConfigurations(htg, tempPath))
  {
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool TestPDCCompositeHTG(const std::string& tempDir)
{
  vtkNew<vtkHyperTreeGridSource> htgSource1;
  htgSource1->SetBranchFactor(2);
  htgSource1->SetDimensions(6, 4, 1);
  htgSource1->SetMaxDepth(2);
  htgSource1->SetUseMask(true);

  htgSource1->SetDescriptor("... .R. ... ... ... | ....");
  htgSource1->SetMask("111 111 111 000 000 | 1111");
  htgSource1->Update();

  vtkNew<vtkHyperTreeGridSource> htgSource2;
  htgSource2->SetBranchFactor(2);
  htgSource2->SetDimensions(6, 4, 1);
  htgSource2->SetMaxDepth(2);
  htgSource2->SetUseMask(true);

  htgSource2->SetDescriptor("... ... ... .R. ... | ....");
  htgSource2->SetMask("000 000 000 111 111 | 1111");
  htgSource2->Update();

  // Hyper-Tree Art: 3D Recursion (2026)
  vtkNew<vtkHyperTreeGridSource> htgSource3;
  htgSource3->SetBranchFactor(2);
  htgSource3->SetDimensions(3, 3, 3);
  htgSource3->SetMaxDepth(4);
  htgSource3->SetDescriptor(".......R|.......R|.......R|........");
  htgSource3->SetMask("11011011|11011011|11011011|11011011");
  htgSource3->SetUseMask(true);

  vtkNew<vtkGroupDataSetsFilter> pdsGroup;
  pdsGroup->SetOutputTypeToPartitionedDataSet();
  pdsGroup->AddInputConnection(htgSource1->GetOutputPort());
  pdsGroup->AddInputConnection(htgSource2->GetOutputPort());

  vtkNew<vtkGroupDataSetsFilter> pdsGroup2;
  pdsGroup2->SetOutputTypeToPartitionedDataSet();
  pdsGroup2->AddInputConnection(htgSource3->GetOutputPort());

  vtkNew<vtkGroupDataSetsFilter> pdcGroup;
  pdcGroup->SetOutputTypeToPartitionedDataSetCollection();
  pdcGroup->AddInputConnection(pdsGroup->GetOutputPort());
  pdcGroup->AddInputConnection(pdsGroup2->GetOutputPort());

  vtkPartitionedDataSetCollection* pdc =
    vtkPartitionedDataSetCollection::SafeDownCast(pdcGroup->GetOutputDataObject(0));

  // Original PDC has no assembly set, but VTKHDF writer sets one by default, so we create one.
  vtkNew<vtkDataAssembly> hierarchy;
  vtkDataAssemblyUtilities::GenerateHierarchy(pdc, hierarchy, nullptr);
  pdc->SetDataAssembly(hierarchy);

  // Write and read the vtkMultiBlockDataSet in a temp file, compare with base
  std::string tempPath = tempDir + "/HDFWriter_pdcHTG";
  if (!TestWriteAndReadConfigurations(pdc, tempPath))
  {
    return false;
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
    std::string tempPath = tempDir + "/HDFWriter_" + baseName;
    if (!TestWriteAndReadConfigurations(baseData, tempPath))
    {
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------------------
bool TestFieldDataReadWrite(const std::string& tempDir)
{
  vtkNew<vtkIntArray> arr;
  std::string arrayName = "MyAwesomeFieldData";
  arr->SetName(arrayName.c_str());
  arr->SetNumberOfComponents(3);
  arr->SetNumberOfTuples(2);
  std::array<int, 3> tup1{ 1, 2, 3 };
  arr->InsertNextTypedTuple(tup1.data());
  std::array<int, 3> tup2{ 2, 3, 4 };
  arr->InsertNextTypedTuple(tup2.data());

  vtkNew<vtkFieldData> fd;
  fd->AddArray(arr);

  vtkNew<vtkUnstructuredGrid> ug;
  ug->SetFieldData(fd);

  std::string tempPath = tempDir + "/HDFWriter_field_data.vtkhdf";
  vtkNew<vtkHDFWriter> writer;
  writer->SetInputData(ug);
  writer->SetFileName(tempPath.c_str());
  writer->Write();

  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(tempPath.c_str());
  reader->Update();

  vtkUnstructuredGrid* ug_read = vtkUnstructuredGrid::SafeDownCast(reader->GetOutputAsDataSet());

  if (!vtkTestUtilities::CompareFieldData(ug_read->GetFieldData(), ug->GetFieldData()))
  {
    std::cerr << "vtkDataObject does not match: " << tempPath << std::endl;
    return false;
  }

  vtkDataArraySelection* select = reader->GetFieldDataArraySelection();
  select->DisableArray(arrayName.c_str());
  reader->Update();

  ug_read = vtkUnstructuredGrid::SafeDownCast(reader->GetOutputAsDataSet());
  if (ug_read->GetFieldData()->GetNumberOfArrays() != 0)
  {
    std::cerr << "vtkDataObject does not match: " << tempPath << std::endl;
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool TestWriteAfterReadComposite(const std::string& tempDir)
{
  // Test that HDF Reader and writer properly release the file lock after they are done
  std::string writtenName = tempDir + "/pdc_read_write.vtkhdf";

  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkConeSource> cone;
  vtkNew<vtkGroupDataSetsFilter> group;
  group->AddInputConnection(sphere->GetOutputPort());
  group->AddInputConnection(cone->GetOutputPort());
  group->SetOutputTypeToMultiBlockDataSet();

  vtkNew<vtkHDFWriter> writer;
  writer->SetFileName(writtenName.c_str());
  writer->SetInputConnection(group->GetOutputPort());
  writer->Write();

  // Read the file we just wrote
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(writtenName.c_str());
  reader->Update();

  // Overwrite the file, check that reader correctly released resources
  // Test errors if write operation did not finish because file lock was not released;
  writer->Write();

  return true;
}

//----------------------------------------------------------------------------
bool TestTable(const std::string& tempDir, const std::string& dataRoot)
{
  const std::string baseName = "table.vtt";
  const std::string basePath = dataRoot + "/Data/vtkHDF/" + baseName;
  vtkNew<vtkXMLTableReader> baseReader;
  baseReader->SetFileName(basePath.c_str());
  baseReader->Update();
  auto baseData = vtkTable::SafeDownCast(baseReader->GetOutput());

  std::string tempPath = tempDir + "/HDFWriter_" + baseName;
  if (!TestWriteAndReadConfigurations(baseData, tempPath))
  {
    return false;
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
  testPasses &= TestSimpleHTG(tempDir);
  testPasses &= TestRandomHTG(tempDir);
  testPasses &= TestNullHTG(tempDir);
  testPasses &= TestNoValidPartHTG(tempDir);
  testPasses &= TestPDCCompositeHTG(tempDir);
  testPasses &= TestComplexPolyData(tempDir, dataRoot);
  testPasses &= TestUnstructuredGrid(tempDir, dataRoot);
  testPasses &= TestImageDataWriteRead(tempDir);
  testPasses &= TestRectilinearGridWriteRead(tempDir);
  testPasses &= TestStructuredGridWriteRead(tempDir);
  testPasses &= TestDataSetAttributes(tempDir);
  testPasses &= TestSanitizeName(tempDir, dataRoot);
  testPasses &= TestPartitionedUnstructuredGrid(tempDir, dataRoot);
  testPasses &= TestPartitionedPolyData(tempDir, dataRoot);
  testPasses &= TestPartitionedDataSetCollection(tempDir, dataRoot);
  testPasses &= TestMultiBlock(tempDir, dataRoot);
  testPasses &= TestMultiBlockIdenticalBlockNames(tempDir, dataRoot);
  testPasses &= TestFieldDataReadWrite(tempDir);
  testPasses &= TestWriteAfterReadComposite(tempDir);
  testPasses &= TestTable(tempDir, dataRoot);

  return testPasses ? EXIT_SUCCESS : EXIT_FAILURE;
}
