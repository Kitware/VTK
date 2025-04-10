// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "HDFTestUtilities.h"
#include "vtkAppendDataSets.h"
#include "vtkCleanUnstructuredGrid.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkDataObjectTree.h"
#include "vtkDataSet.h"
#include "vtkExtractSurface.h"
#include "vtkForceStaticMesh.h"
#include "vtkGenerateTimeSteps.h"
#include "vtkGroupDataSetsFilter.h"
#include "vtkHDFReader.h"
#include "vtkHDFWriter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMergeBlocks.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPartitionedDataSetCollectionAlgorithm.h"
#include "vtkPassArrays.h"
#include "vtkPointData.h"
#include "vtkPointDataToCellData.h"
#include "vtkPolyData.h"
#include "vtkSpatioTemporalHarmonicsSource.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridWriter.h"

namespace HDFTestUtilities
{
vtkStandardNewMacro(vtkAddAssembly);
}
namespace
{

enum supportedDataSetTypes
{
  vtkUnstructuredGridType,
  vtkPolyDataType
};

struct WriterConfigOptions
{
  bool UseExternalTimeSteps;
  bool UseExternalPartitions;
  std::string FileNameSuffix;
};
}

//----------------------------------------------------------------------------
bool TestTemporalData(const std::string& tempDir, const std::string& dataRoot,
  const std::string& baseName, const WriterConfigOptions& config, int datatype)
{
  // Open original temporal HDF data
  const std::string basePath = dataRoot + "/Data/" + baseName;
  vtkNew<vtkHDFReader> baseHDFReader;
  baseHDFReader->SetFileName(basePath.c_str());
  baseHDFReader->Update();

  vtkNew<vtkMergeBlocks> mergeBlocks;
  mergeBlocks->SetInputConnection(baseHDFReader->GetOutputPort());
  mergeBlocks->SetMergePoints(false);
  mergeBlocks->SetMergePartitionsOnly(true);
  mergeBlocks->SetOutputDataSetType(datatype);

  // Write the data to a file using the vtkHDFWriter
  vtkNew<vtkHDFWriter> HDFWriter;
  HDFWriter->SetInputConnection(
    datatype > 0 ? mergeBlocks->GetOutputPort() : baseHDFReader->GetOutputPort());
  std::string tempPath = tempDir + "/HDFWriter_";
  tempPath += baseName + ".vtkhdf" + config.FileNameSuffix;
  HDFWriter->SetFileName(tempPath.c_str());
  HDFWriter->SetUseExternalTimeSteps(config.UseExternalTimeSteps);
  HDFWriter->SetUseExternalPartitions(config.UseExternalPartitions);
  HDFWriter->SetWriteAllTimeSteps(true);
  HDFWriter->SetChunkSize(100);
  HDFWriter->SetCompressionLevel(4);
  HDFWriter->Write();

  vtkLog(INFO,
    "Testing " << tempPath << " with options Ext time steps: " << config.UseExternalTimeSteps
               << " ext partitions: " << config.UseExternalPartitions);
  // Read the data just written
  vtkNew<vtkHDFReader> HDFReader;
  if (!HDFReader->CanReadFile(tempPath.c_str()))
  {
    vtkLog(ERROR, "vtkHDFReader can not read file: " << tempPath);
    return false;
  }
  HDFReader->SetFileName(tempPath.c_str());
  HDFReader->Update();
  // Read the original data from the beginning
  vtkNew<vtkHDFReader> HDFReaderBaseline;
  HDFReaderBaseline->SetFileName(basePath.c_str());
  HDFReaderBaseline->Update();
  // Make sure both have the same number of timesteps
  int totalTimeStepsXML = HDFReaderBaseline->GetNumberOfSteps();
  int totalTimeStepsHDF = HDFReader->GetNumberOfSteps();
  if (totalTimeStepsXML != totalTimeStepsHDF)
  {
    vtkLog(ERROR,
      "total time steps in both HDF files do not match: " << totalTimeStepsHDF << " instead of "
                                                          << totalTimeStepsXML);
    return false;
  }

  // Compare the data at each timestep from both readers
  for (int step = 0; step < totalTimeStepsXML; step++)
  {
    HDFReaderBaseline->SetStep(step);
    HDFReaderBaseline->Update();

    HDFReader->SetStep(step);
    HDFReader->Update();

    // Time values must be the same
    if (HDFReader->GetTimeValue() != HDFReaderBaseline->GetTimeValue())
    {
      vtkLog(ERROR,
        "timestep value does not match : " << HDFReader->GetTimeValue() << " instead of "
                                           << HDFReaderBaseline->GetTimeValue());
      return false;
    }

    if (datatype > 0) // Working with a partitioned dataset
    {
      vtkPartitionedDataSet* baselineData =
        vtkPartitionedDataSet::SafeDownCast(HDFReaderBaseline->GetOutput());

      mergeBlocks->Update();
      vtkNew<vtkAppendDataSets> appendParts;
      appendParts->SetOutputDataSetType(datatype);
      for (unsigned int iPiece = 0; iPiece < baselineData->GetNumberOfPartitions(); ++iPiece)
      {
        appendParts->AddInputData(baselineData->GetPartition(iPiece));
      }
      appendParts->Update();
      if (!vtkTestUtilities::CompareDataObjects(appendParts->GetOutput(), HDFReader->GetOutput()))
      {
        vtkLog(ERROR, "data objects do not match");
        return false;
      }
    }
    else
    {
      if (!vtkTestUtilities::CompareDataObjects(
            HDFReaderBaseline->GetOutput(), HDFReader->GetOutput()))
      {
        vtkLog(ERROR, "data objects do not match");
        return false;
      }
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool TestTemporalStaticMesh(
  const std::string& tempDir, const std::string& baseName, int dataSetType)
{
  /*
   * At the time this test has been written, the reader only support static mesh for partitioned
   * data set. We can't use use both the merge parts & the cache at the same time, which cause every
   * static to be read as a partitioned dataset with at least one partition. The writer doesn't
   * support writing partitioned dataset yet so we can't test static mesh writing properly since we
   * can't read non partitioned static data.
   */
  // Custom static mesh source
  vtkNew<vtkSpatioTemporalHarmonicsSource> harmonics;
  harmonics->ClearHarmonics();
  harmonics->AddHarmonic(1, 0, 0.6283, 0.6283, 0.6283, 0);
  harmonics->AddHarmonic(3, 0, 0.6283, 0, 0, 1.5708);
  harmonics->AddHarmonic(2, 0, 0, 0.6283, 0, 3.1416);
  harmonics->AddHarmonic(1, 0, 0, 0, 0.6283, 4.1724);

  vtkSmartPointer<vtkAlgorithm> datasetTypeSpecificFilter;

  if (dataSetType == ::supportedDataSetTypes::vtkUnstructuredGridType)
  {
    datasetTypeSpecificFilter =
      vtkSmartPointer<vtkCleanUnstructuredGrid>::Take(vtkCleanUnstructuredGrid::New());
  }
  else if (dataSetType == ::supportedDataSetTypes::vtkPolyDataType)
  {
    datasetTypeSpecificFilter = vtkSmartPointer<vtkExtractSurface>::Take(vtkExtractSurface::New());
  }
  datasetTypeSpecificFilter->SetInputConnection(0, harmonics->GetOutputPort(0));

  vtkNew<vtkPointDataToCellData> pointDataToCellData;
  pointDataToCellData->SetPassPointData(true);
  pointDataToCellData->SetInputConnection(0, datasetTypeSpecificFilter->GetOutputPort(0));

  vtkNew<vtkForceStaticMesh> staticMesh;
  staticMesh->SetInputConnection(0, pointDataToCellData->GetOutputPort(0));

  // Write the data to a file using the vtkHDFWriter
  vtkNew<vtkHDFWriter> HDFWriter;
  HDFWriter->SetInputConnection(staticMesh->GetOutputPort());
  std::string tempPath = tempDir + "/HDFWriter_";
  tempPath += baseName + ".vtkhdf";
  HDFWriter->SetFileName(tempPath.c_str());
  HDFWriter->SetWriteAllTimeSteps(true);
  HDFWriter->SetCompressionLevel(1);
  if (!HDFWriter->Write())
  {
    vtkLog(ERROR, "An error occurred while writing the static mesh HDF file");
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
bool TestTemporalComposite(const std::string& tempDir, const std::string& dataRoot,
  const std::vector<std::string>& baseNames, int compositeType)
{
  std::vector<vtkSmartPointer<vtkHDFReader>> baselineReaders;
  std::vector<vtkSmartPointer<vtkMergeBlocks>> baselineReadersMerged;
  for (const auto& baseName : baseNames)
  {
    const std::string filePath = dataRoot + "/Data/" + baseName + ".hdf";
    vtkNew<vtkHDFReader> baseHDFReader;
    baseHDFReader->SetFileName(filePath.c_str());

    vtkNew<vtkMergeBlocks> mergeBlocks;
    mergeBlocks->SetInputConnection(baseHDFReader->GetOutputPort());
    mergeBlocks->SetMergePartitionsOnly(true);
    mergeBlocks->SetMergePoints(false);
    mergeBlocks->SetOutputDataSetType(VTK_UNSTRUCTURED_GRID);

    baselineReaders.emplace_back(baseHDFReader);
    baselineReadersMerged.emplace_back(mergeBlocks);
  }

  // Create a composite structure
  vtkNew<vtkGroupDataSetsFilter> groupDataSets;
  groupDataSets->SetOutputType(compositeType);
  for (int i = 0; i < static_cast<int>(baseNames.size()); i++)
  {
    if (baseNames[i] == "transient_sphere")
    {
      groupDataSets->AddInputConnection(baselineReadersMerged[i]->GetOutputPort());
    }
    else
    {
      groupDataSets->AddInputConnection(baselineReaders[i]->GetOutputPort());
    }
    groupDataSets->SetInputName(i, baseNames[i].c_str());
  }

  // vtkGroupDataSetsFilter does not create an assembly for PDC, but the VTKHDF requires one.
  vtkNew<HDFTestUtilities::vtkAddAssembly> addAssembly;
  addAssembly->SetInputConnection(groupDataSets->GetOutputPort());

  // Write out the composite temporal dataset
  vtkNew<vtkHDFWriter> HDFWriterGrouped;
  HDFWriterGrouped->SetInputConnection(compositeType == VTK_PARTITIONED_DATA_SET_COLLECTION
      ? addAssembly->GetOutputPort()
      : groupDataSets->GetOutputPort());

  std::string tempPath = tempDir + "/HDFWriter_";
  tempPath += "composite" + std::to_string(compositeType) + ".vtkhdf";
  HDFWriterGrouped->SetFileName(tempPath.c_str());
  HDFWriterGrouped->SetWriteAllTimeSteps(true);
  HDFWriterGrouped->Write();

  // Read back the grouped dataset
  vtkNew<vtkHDFReader> readerGrouped;
  readerGrouped->SetFileName(tempPath.c_str());
  readerGrouped->Update();

  // Make sure the number of timesteps match for all readers
  int totalTimeStepsGrouped = readerGrouped->GetNumberOfSteps();

  for (auto& readerPart : baselineReaders)
  {
    int totalTimeStepsPart = readerPart->GetNumberOfSteps();
    if (totalTimeStepsGrouped != totalTimeStepsPart)
    {
      vtkLog(ERROR,
        "total time steps in both HDF files do not match: "
          << totalTimeStepsPart << " instead of " << totalTimeStepsGrouped << " for dataset "
          << readerPart->GetFileName());
      return false;
    }
  }

  // Make sure we now control time manually using SetStep, don't let the pipeline handle it anymore
  for (auto& reader : baselineReaders)
  {
    reader->GetOutputInformation(0)->Remove(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  }

  // Compare the data at each timestep
  for (int step = 0; step < totalTimeStepsGrouped; step++)
  {
    readerGrouped->SetStep(step);
    readerGrouped->Update();

    auto composite = vtkCompositeDataSet::SafeDownCast(readerGrouped->GetOutputDataObject(0));
    vtkCompositeDataIterator* iter = vtkCompositeDataSet::SafeDownCast(composite)->NewIterator();
    iter->SkipEmptyNodesOn();
    iter->GoToFirstItem();

    for (int compositeID = 0; compositeID < static_cast<int>(baseNames.size()); compositeID++)
    {
      if (iter->IsDoneWithTraversal())
      {
        vtkLog(ERROR, "Wrong number of datasets in composite output");
        return false;
      }

      baselineReaders[compositeID]->SetStep(step);
      baselineReaders[compositeID]->Update();
      baselineReadersMerged[compositeID]->Update();

      auto currentGroupedDO = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      vtkDataSet* baselineDO = nullptr;
      if (baseNames[compositeID] == "transient_sphere")
      {
        baselineDO =
          vtkDataSet::SafeDownCast(baselineReadersMerged[compositeID]->GetOutputDataObject(0));
      }
      else
      {
        baselineDO = vtkDataSet::SafeDownCast(baselineReaders[compositeID]->GetOutputDataObject(0));
      }

      // After grouping datasets, field data (time values) are not expected to match with the
      // original dataset field values. Copy them to avoid failing comparison.
      currentGroupedDO->SetFieldData(baselineDO->GetFieldData());
      currentGroupedDO->GetPointData()->RemoveArray(0);
      currentGroupedDO->GetPointData()->AddArray(baselineDO->GetPointData()->GetArray(0));

      if (!vtkTestUtilities::CompareDataObjects(currentGroupedDO, baselineDO))
      {
        vtkLog(ERROR, << "data objects do not match for time step " << step);
        return false;
      }

      iter->GoToNextItem();
    }
    iter->Delete();
  }

  return true;
}

//----------------------------------------------------------------------------
int TestHDFWriterTemporal(int argc, char* argv[])
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
    vtkLog(ERROR, "-D /path/to/data was not specified.");
    return EXIT_FAILURE;
  }
  std::string dataRoot = testHelper->GetDataRoot();
  bool result = true;

  // Run tests : read data, write it, read the written data and compare to the original
  std::vector<std::string> baseNames = { "transient_sphere.hdf",
    "temporal_unstructured_grid.vtkhdf", "transient_harmonics.hdf" };
  std::vector<int> parallel_types{ VTK_UNSTRUCTURED_GRID,
    -1, // Not parallel
    -1 };
  std::vector<WriterConfigOptions> configs{ { false, false, "_NoExtTimeNoExtPart" },
    { false, true, "_NoExtTimeExtPart" }, { true, false, "_ExtTimeNoExtPart" },
    { true, true, "_ExtTimeExtPart" } };

  // Test the whole matrix "file" x "config"
  for (const auto& config : configs)
  {
    for (int i = 0; i < static_cast<int>(baseNames.size()); i++)
    {
      result &= TestTemporalData(tempDir, dataRoot, baseNames[i], config, parallel_types[i]);
    }
  }

  // Use a modified version of transient_harmonics to make sure that the time values match
  // between
  // both datasets
  std::vector<std::string> baseNamesComposite = { "transient_sphere", "transient_harmonics" };
  result &= TestTemporalComposite(tempDir, dataRoot, baseNamesComposite, VTK_MULTIBLOCK_DATA_SET);
  result &= TestTemporalComposite(
    tempDir, dataRoot, baseNamesComposite, VTK_PARTITIONED_DATA_SET_COLLECTION);

  result &= TestTemporalStaticMesh(
    tempDir, "transient_static_sphere_ug_source", ::supportedDataSetTypes::vtkUnstructuredGridType);
  result &= TestTemporalStaticMesh(
    tempDir, "transient_static_sphere_polydata_source", ::supportedDataSetTypes::vtkPolyDataType);
  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
