// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCleanUnstructuredGrid.h"
#include "vtkDataObjectTree.h"
#include "vtkDataSet.h"
#include "vtkExtractSurface.h"
#include "vtkForceStaticMesh.h"
#include "vtkGenerateTimeSteps.h"
#include "vtkGroupDataSetsFilter.h"
#include "vtkHDFReader.h"
#include "vtkHDFWriter.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSetCollection.h"
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
  for (int i = 0; i < totalTimeStepsXML; i++)
  {
    std::cout << "Comparing timestep " << i << std::endl;
    HDFReaderBaseline->SetStep(i);
    HDFReaderBaseline->Update();

    HDFReader->SetStep(i);
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
      vtkLog(ERROR, "data objects do not match");
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
  // Open all non-composite parts and group them in a composite dataset
  vtkNew<vtkGroupDataSetsFilter> group;
  group->SetOutputType(compositeType);

  // std::vector<std::string> baseFiles{ "transient_sphere.hdf", "transient_harmonics.hdf" };
  for (auto& baseName : baseNames)
  {
    const std::string basePath = dataRoot + "/Data/" + baseName;
    vtkNew<vtkHDFReader> baseHDFReader;
    baseHDFReader->SetFileName(basePath.c_str());
    group->AddInputConnection(baseHDFReader->GetOutputPort());
  }

  // vtkGroupDataSetsFilter does not create a vtkDataAssembly for the PDC, so we create it manually
  group->Update();
  // auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(group->GetOutputDataObject(0));
  // vtkNew<vtkDataAssembly> assembly;
  // assembly->Initialize();
  // assembly->SetRootNodeName("RootGroup");
  // auto nodeIds = assembly->AddNodes(baseNames);
  // for (int i = 0; i < baseNames.size(); i++)
  // {
  //   assembly->AddDataSetIndex(nodeIds[i], i);
  // }
  // pdc->SetDataAssembly(assembly);

  // Write the data to a file using the vtkHDFWriter
  vtkNew<vtkHDFWriter> HDFWriter;
  std::string tempPath = tempDir + "/HDFWriter_TemporalComposite.vtkhdf";
  HDFWriter->SetFileName(tempPath.c_str());
  HDFWriter->SetWriteAllTimeSteps(true);
  HDFWriter->SetUseExternalComposite(false); // TODO: variabilize
  HDFWriter->SetInputConnection(group->GetOutputPort());
  if (!HDFWriter->Write())
  {
    vtkLog(ERROR, "An error occured while writing the composite temporal mesh HDF file");
    return false;
  }

  // Read again the data
  vtkNew<vtkHDFReader> HDFReader;
  HDFReader->SetFileName(tempPath.c_str());
  HDFReader->Update();

  // Check that the number of time steps is right
  vtkInformation* info = group->GetOutputInformation(0);
  if (!info->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    vtkLog(ERROR, "Unable to retrieve time steps from test data.");
    return false;
  }

  // TODO: also check for values
  std::vector<double> timeSteps;
  const int nbOfTimesteps = info->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  const double* timeStepsPtr = info->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  if (HDFReader->GetNumberOfSteps() != nbOfTimesteps)
  {
    vtkLog(ERROR,
      "Number of timesteps do not match: got " << HDFReader->GetNumberOfSteps() << " instead of "
                                               << nbOfTimesteps);
    return false;
  }

  // Check for equality between datasets for each timestep
  for (int i = 0; i < nbOfTimesteps; i++)
  {
    info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), i);
    group->Update();
    vtkDataObjectTree* compositeRef =
      vtkDataObjectTree::SafeDownCast(group->GetOutputDataObject(0));

    HDFReader->SetStep(i);
    HDFReader->Update();
    vtkDataObjectTree* composite =
      vtkDataObjectTree::SafeDownCast(HDFReader->GetOutputDataObject(0));

    // After grouping datasets, field data (time values) are not expected to match with the original
    // dataset field values. Copy them to avoid failing comparison.
    currentGroupedDO->SetFieldData(baselineDO->GetFieldData());
    currentGroupedDO->GetPointData()->RemoveArray(0);
    currentGroupedDO->GetPointData()->AddArray(baselineDO->GetPointData()->GetArray(0));
    if (!vtkTestUtilities::CompareDataObjects(composite, compositeRef))
    {
      vtkLog(ERROR, "Failed comparison for timestep." << i);
      return false;
    }
  }
  return true;
}

// bool TestTemporalComposite(const std::string& tempDir, const std::string& dataRoot,
//   const std::vector<std::string>& baseNames, int outputType)
// {
//   std::vector<vtkSmartPointer<vtkHDFReader>> baselineReaders;
//   for (const auto& baseName : baseNames)
//   {
//     const std::string filePath = dataRoot + "/Data/" + baseName;
//     vtkNew<vtkHDFReader> baseHDFReader;
//     baseHDFReader->SetFileName(filePath.c_str());
//     baselineReaders.emplace_back(baseHDFReader);
//   }

//   // Create a composite structure
//   vtkNew<vtkGroupDataSetsFilter> groupDataSets;
//   groupDataSets->SetOutputType(outputType);
//   for (int i = 0; i < static_cast<int>(baseNames.size()); i++)
//   {
//     groupDataSets->AddInputConnection(baselineReaders[i]->GetOutputPort());
//     groupDataSets->SetInputName(i, baseNames[i].c_str());
//   }
//   auto groupedData = groupDataSets->GetOutputDataObject(0);

//   // PDC Output does not produce the hierarchy we need to write out the file.
//   if (outputType == VTK_PARTITIONED_DATA_SET_COLLECTION)
//   {
//     auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(groupedData);
//     vtkNew<vtkDataAssembly> assembly;
//     assembly->Initialize();
//     assembly->
//   }

//   // Write out the composite temporal dataset
//   vtkNew<vtkHDFWriter> HDFWriterGrouped;
//   HDFWriterGrouped->SetInputDataObject(groupedData);
//   std::string tempPath = tempDir + "/HDFWriter_";
//   tempPath += "composite" + std::to_string(outputType) + ".vtkhdf";
//   HDFWriterGrouped->SetFileName(tempPath.c_str());
//   HDFWriterGrouped->SetWriteAllTimeSteps(true);
//   HDFWriterGrouped->SetChunkSize(100);
//   HDFWriterGrouped->SetCompressionLevel(1);
//   HDFWriterGrouped->SetDebug(true);
//   HDFWriterGrouped->Write();

//   // Read back the grouped dataset
//   vtkNew<vtkHDFReader> readerGrouped;
//   readerGrouped->SetFileName(tempPath.c_str());
//   readerGrouped->Update();

//   // Make sure the number of timesteps match for all readers
//   int totalTimeStepsGrouped = readerGrouped->GetNumberOfSteps();

//   for (auto& readerPart : baselineReaders)
//   {
//     int totalTimeStepsPart = readerPart->GetNumberOfSteps();
//     if (totalTimeStepsGrouped != totalTimeStepsPart)
//     {
//       vtkLog(ERROR,
//         "total time steps in both HDF files do not match: "
//           << totalTimeStepsPart << " instead of " << totalTimeStepsGrouped << " for dataset "
//           << readerPart->GetFileName());
//       return false;
//     }
//   }

//   // Compare the data at each timestep
//   for (int i = 0; i < totalTimeStepsGrouped; i++)
//   {
//     std::cout << "Comparing timestep " << i << std::endl;

//     readerGrouped->SetStep(i);
//     readerGrouped->Update();

//     auto composite = vtkCompositeDataSet::SafeDownCast(readerGrouped->GetOutputDataObject(0));
//     vtkCompositeDataIterator* iter = vtkCompositeDataSet::SafeDownCast(composite)->NewIterator();
//     iter->SkipEmptyNodesOn();
//     iter->GoToFirstItem();

//     for (int i = 0; i < static_cast<int>(baseNames.size()); i++)
//     {
//       if (iter->IsDoneWithTraversal())
//       {
//         vtkLog(ERROR, "Wrong number of datasets in composite output");
//         return false;
//       }

//       baselineReaders[i]->SetStep(i);
//       baselineReaders[i]->Update();

//       auto currentGroupedDO = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
//       auto baselineDO = vtkDataSet::SafeDownCast(baselineReaders[i]->GetOutputDataObject(0));

//       // After grouping datasets, field data (time values) are not expected to match with the
//       // original dataset field values. Copy them to avoid failing comparison.
//       currentGroupedDO->SetFieldData(baselineDO->GetFieldData());
//       currentGroupedDO->GetPointData()->RemoveArray(0);
//       currentGroupedDO->GetPointData()->AddArray(baselineDO->GetPointData()->GetArray(0));

//       if (!vtkTestUtilities::CompareDataObjects(currentGroupedDO, baselineDO))
//       {
//         vtkLog(ERROR, "data objects do not match");
//         // return false;
//       }

//       iter->GoToNextItem();
//     }
//     iter->Delete();
//   }

//   return true;
// }

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
      // result &= TestTemporalData(tempDir, dataRoot, fileName, config);
    }
  }

  // result &= TestTemporalComposite(tempDir, dataRoot, VTK_PARTITIONED_DATA_SET_COLLECTION);

  std::vector<std::string> baseNamesComposite = { "transient_sphere.hdf",
    "transient_harmonics.hdf" };
  // result &= TestTemporalComposite(tempDir, dataRoot, baseNamesComposite,
  // VTK_MULTIBLOCK_DATA_SET);
  //   result &=
  TestTemporalComposite(tempDir, dataRoot, baseNamesComposite, VTK_PARTITIONED_DATA_SET_COLLECTION);

  // result &= TestTemporalStaticMesh(
  //   tempDir, "transient_static_sphere_ug_source",
  //   ::supportedDataSetTypes::vtkUnstructuredGridType);
  // result &= TestTemporalStaticMesh(
  //   tempDir, "transient_static_sphere_polydata_source",
  //   ::supportedDataSetTypes::vtkPolyDataType);
  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
