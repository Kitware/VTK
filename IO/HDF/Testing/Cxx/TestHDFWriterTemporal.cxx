// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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
#include "vtkNew.h"
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

/**
 * Simple filter that adds a vtkDataAssembly to a PDC that does not have one.
 * This can be removed when vtkGroupDataSetsFilter will support generating an assembly automatically
 * for PartitionedDataSetCollections
 */
class vtkAddAssembly : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  static vtkAddAssembly* New();
  vtkTypeMacro(vtkAddAssembly, vtkDataObjectAlgorithm);
  vtkAddAssembly()
    : vtkPartitionedDataSetCollectionAlgorithm(){

    };

protected:
  int RequestData(
    vtkInformation* request, vtkInformationVector** inVector, vtkInformationVector* ouInfo) override
  {
    this->vtkPartitionedDataSetCollectionAlgorithm::RequestData(request, inVector, ouInfo);
    auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(vtkDataObject::GetData(ouInfo, 0));
    vtkPartitionedDataSetCollection* input = vtkPartitionedDataSetCollection::SafeDownCast(
      inVector[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));

    vtkNew<vtkDataAssembly> hierarchy;
    vtkDataAssemblyUtilities::GenerateHierarchy(input, hierarchy, pdc);
    return 1;
  }
};

vtkStandardNewMacro(vtkAddAssembly);

}

//----------------------------------------------------------------------------
bool TestTemporalData(const std::string& tempDir, const std::string& dataRoot,
  const std::string& baseName, const WriterConfigOptions& config)
{
  // Open original temporal HDF data
  const std::string basePath = dataRoot + "/Data/" + baseName;
  vtkNew<vtkHDFReader> baseHDFReader;
  baseHDFReader->SetFileName(basePath.c_str());

  // Write the data to a file using the vtkHDFWriter
  vtkNew<vtkHDFWriter> HDFWriter;
  HDFWriter->SetInputConnection(baseHDFReader->GetOutputPort());
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

    if (!vtkTestUtilities::CompareDataObjects(
          HDFReaderBaseline->GetOutput(), HDFReader->GetOutput()))
    {
      vtkLog(ERROR, << "data objects do not match for time step " << step);
      return false;
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
  /* TODO
   * Once the reader supports both MergeParts & UseCache used together,
   * this test will need to be updated by reading the output file and checking
   * it corresponds to the source, as well as checking the MeshMTime values.
   */
  return true;
}

//----------------------------------------------------------------------------
bool TestTemporalComposite(const std::string& tempDir, const std::string& dataRoot,
  const std::vector<std::string>& baseNames, int compositeType)
{
  std::vector<vtkSmartPointer<vtkHDFReader>> baselineReaders;
  for (const auto& baseName : baseNames)
  {
    const std::string filePath = dataRoot + "/Data/" + baseName + ".hdf";
    vtkNew<vtkHDFReader> baseHDFReader;
    baseHDFReader->SetFileName(filePath.c_str());
    baselineReaders.emplace_back(baseHDFReader);
  }

  // Create a composite structure
  vtkNew<vtkGroupDataSetsFilter> groupDataSets;
  groupDataSets->SetOutputType(compositeType);
  for (int i = 0; i < static_cast<int>(baseNames.size()); i++)
  {
    groupDataSets->AddInputConnection(baselineReaders[i]->GetOutputPort());
    groupDataSets->SetInputName(i, baseNames[i].c_str());
  }

  // vtkGroupDataSetsFilter does not create an assembly for PDC, but the VTKHDF requires one.
  // vtkNew<vtkPassInputTypeAlgorithm> addAssembly;
  vtkNew<::vtkAddAssembly> addAssembly;
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
  HDFWriterGrouped->SetUseExternalComposite(false); // TODO: variabilize
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

      auto currentGroupedDO = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      auto baselineDO =
        vtkDataSet::SafeDownCast(baselineReaders[compositeID]->GetOutputDataObject(0));

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
  std::vector<WriterConfigOptions> configs{ { false, false, "_NoExtTimeNoExtPart" },
    { false, true, "_NoExtTimeExtPart" }, { true, false, "_ExtTimeNoExtPart" },
    { true, true, "_ExtTimeExtPart" } };

  // Test the whole matrix "file" x "config"
  for (const auto& config : configs)
  {
    for (const auto& fileName : baseNames)
    {
      result &= TestTemporalData(tempDir, dataRoot, fileName, config);
    }
  }

  // Use a modified version of transient_harmonics to make sure that the time values match between
  // both datasets
  std::vector<std::string> baseNamesComposite = { "transient_sphere",
    "transient_harmonics_timevalues" };
  result &= TestTemporalComposite(tempDir, dataRoot, baseNamesComposite, VTK_MULTIBLOCK_DATA_SET);
  result &= TestTemporalComposite(
    tempDir, dataRoot, baseNamesComposite, VTK_PARTITIONED_DATA_SET_COLLECTION);

  result &= TestTemporalStaticMesh(
    tempDir, "transient_static_sphere_ug_source", ::supportedDataSetTypes::vtkUnstructuredGridType);
  result &= TestTemporalStaticMesh(
    tempDir, "transient_static_sphere_polydata_source", ::supportedDataSetTypes::vtkPolyDataType);
  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
