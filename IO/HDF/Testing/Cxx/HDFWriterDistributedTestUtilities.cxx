// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "HDFWriterDistributedTestUtilities.h"

#include "vtkCommunicator.h"
#include "vtkDataObjectTree.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkForceStaticMesh.h"
#include "vtkHDFReader.h"
#include "vtkHDFWriter.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointSet.h"
#include "vtkStringFormatter.h"
#include "vtkTestUtilities.h"

#include <cstdlib>
#include <iostream>
#include <numeric>
#include <utility>

namespace HDFWriterDistributedTestUtilities
{
void WriteToFile(vtkHDFWriter* writer, vtkDataObjectAlgorithm* source, const Configuration& config,
  vtkMPIController* controller, const std::string& filebase)
{
  const std::string filename = filebase + ".vtkhdf";
  writer->SetFileName(filename.c_str());
  writer->SetUseExternalComposite(config.External & ExternalType::Composite);
  writer->SetUseExternalPartitions(config.External & ExternalType::Partitions);
  writer->SetUseExternalTimeSteps(config.External & ExternalType::Time);
  writer->SetUseExternalDistributed(config.External & ExternalType::Distributed);
  writer->SetWriteAllTimeSteps(true);
  writer->SetController(controller);
  writer->SetCompressionLevel(3); // Don't take too much space on disk
  if (config.StaticMesh == StaticMeshType::Static)
  {
    // Could be done in the source, but much easier to force meshmtime here
    vtkNew<vtkForceStaticMesh> forceStatic;
    forceStatic->SetInputConnection(source->GetOutputPort());
    writer->SetInputConnection(forceStatic->GetOutputPort(0));
  }
  else
  {
    writer->SetInputConnection(source->GetOutputPort(0));
  }
  writer->Write();
}

bool ComparePDS(vtkPartitionedDataSet* actual, vtkPartitionedDataSet* expected, unsigned int offset,
  unsigned int total)
{
  if (!actual || !expected || actual->GetNumberOfPartitions() != total ||
    total < offset + expected->GetNumberOfPartitions())
  {
    return false;
  }
  for (unsigned int part = 0; part < expected->GetNumberOfPartitions(); ++part)
  {
    auto* readDO = actual->GetPartitionAsDataObject(offset + part);
    auto* refDO = expected->GetPartitionAsDataObject(part);
    if (readDO)
    {
      // Added by the reader
      readDO->GetFieldData()->RemoveArray("Time");
    }
    auto* expectedData = vtkDataSet::SafeDownCast(refDO);
    if (expectedData && expectedData->GetNumberOfPoints() == 0 &&
      expectedData->GetNumberOfCells() == 0)
    {
      auto* actualData = vtkDataSet::SafeDownCast(readDO);
      if (!actualData || !actualData->IsA(expectedData->GetClassName()) ||
        actualData->GetNumberOfPoints() != 0 || actualData->GetNumberOfCells() != 0)
      {
        return false;
      }
      continue;
    }
    if ((!readDO || !refDO) ? readDO != refDO
                            : !vtkTestUtilities::CompareDataObjects(readDO, refDO))
    {
      return false;
    }
  }
  return true;
}

vtkDataObject* GetReadPartition(vtkDataObject* actual, unsigned int partition)
{
  if (auto* partitions = vtkPartitionedDataSet::SafeDownCast(actual))
  {
    return partition < partitions->GetNumberOfPartitions()
      ? partitions->GetPartitionAsDataObject(partition)
      : nullptr;
  }
  return partition == 0 ? actual : nullptr;
}

bool ComparePointSet(vtkDataObject* actual, vtkPointSet* expected, unsigned int partition)
{
  auto* pointSet = vtkPointSet::SafeDownCast(GetReadPartition(actual, partition));
  if (!pointSet || !expected || pointSet->GetDataObjectType() != expected->GetDataObjectType())
  {
    return false;
  }
  pointSet->GetFieldData()->RemoveArray("Time");
  if (expected->GetNumberOfPoints() == 0)
  {
    return pointSet->GetNumberOfPoints() == 0 && pointSet->GetNumberOfCells() == 0;
  }
  return vtkTestUtilities::CompareDataObjects(pointSet, expected);
}

bool CompareReadOutput(vtkDataObject* actual, vtkDataObject* expected, const Configuration& config,
  int rank, bool mainFile);

bool CompareComposite(vtkDataObject* actual, vtkDataObjectTree* expected,
  const Configuration& config, int rank, bool mainFile)
{
  // Compare block by block, because we may need to remove some metadata for accurate comparisons
  if (auto* blocks = vtkMultiBlockDataSet::SafeDownCast(expected))
  {
    auto* output = vtkMultiBlockDataSet::SafeDownCast(actual);
    if (!output || output->GetNumberOfBlocks() != blocks->GetNumberOfBlocks())
    {
      return false;
    }
    for (unsigned int block = 0; block < blocks->GetNumberOfBlocks(); ++block)
    {
      if (!CompareReadOutput(
            output->GetBlock(block), blocks->GetBlock(block), config, rank, mainFile))
      {
        return false;
      }
    }
    return true;
  }
  auto* collections = vtkPartitionedDataSetCollection::SafeDownCast(expected);
  auto* output = vtkPartitionedDataSetCollection::SafeDownCast(actual);
  if (!collections || !output ||
    output->GetNumberOfPartitionedDataSets() != collections->GetNumberOfPartitionedDataSets())
  {
    return false;
  }
  for (unsigned int block = 0; block < collections->GetNumberOfPartitionedDataSets(); ++block)
  {
    if (!CompareReadOutput(output->GetPartitionedDataSet(block),
          collections->GetPartitionedDataSet(block), config, rank, mainFile))
    {
      return false;
    }
  }
  return true;
}

bool CompareReadOutput(vtkDataObject* actual, vtkDataObject* expected, const Configuration& config,
  int rank, bool mainFile)
{
  if (auto* partitions = vtkPartitionedDataSet::SafeDownCast(expected))
  {
    // The reader re-balances partition counts, so we need to re-map pieces manually for comparison
    const unsigned int offset = mainFile
      ? std::accumulate(config.PartitionCounts.begin(), config.PartitionCounts.begin() + rank, 0u)
      : 0;
    const unsigned int total = mainFile
      ? std::accumulate(config.PartitionCounts.begin(), config.PartitionCounts.end(), 0u)
      : partitions->GetNumberOfPartitions();
    vtkNew<vtkPartitionedDataSet> singlePartition;
    auto* actualPartitions = vtkPartitionedDataSet::SafeDownCast(actual);
    if (actual && !actualPartitions)
    {
      singlePartition->SetPartition(0, actual);
      actualPartitions = singlePartition;
    }
    return ComparePDS(actualPartitions, partitions, offset, total);
  }
  const unsigned int partition = mainFile ? rank : 0;
  if (auto* pointSet = vtkPointSet::SafeDownCast(expected))
  {
    return ComparePointSet(actual, pointSet, partition);
  }
  return CompareComposite(
    actual, vtkDataObjectTree::SafeDownCast(expected), config, rank, mainFile);
}

bool ReadAndCompareStep(vtkDataObjectAlgorithm* source, const Configuration& config,
  vtkMPIController* controller, const std::string& filebase, int step, int numberOfSteps)
{
  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  const std::string mainFilename = filebase + ".vtkhdf";
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(mainFilename.c_str());
  reader->UpdateInformation();
  if (reader->GetNumberOfSteps() != numberOfSteps)
  {
    vtkLog(ERROR, << "Wrong timestep count for " << mainFilename);
    return false;
  }
  reader->SetStep(step);

  reader->UpdatePiece(0, 1, 0);
  vtkDataObject* mainReaderOutput = reader->GetOutputDataObject(0);

  source->UpdateTimeStep(step, myRank, nbRanks, 0);
  vtkDataObject* mainSourceOutput = source->GetOutputDataObject(0);

  if ((numberOfSteps > 1 && reader->GetTimeValue() != step) ||
    !CompareReadOutput(mainReaderOutput, mainSourceOutput, config, myRank, true))
  {
    vtkLog(ERROR, << "Failed comparison for " << mainFilename << " at step " << step);
    return false;
  }

  // Compare _partX.vtkhdf files from external distributed
  if ((config.External & ExternalType::Distributed) &&
    (config.PartitionCounts.empty() || config.PartitionCounts[myRank] > 0))
  {
    const std::string partFilename = filebase + "_part" + vtk::to_string(myRank) + ".vtkhdf";
    vtkNew<vtkHDFReader> partReader;
    partReader->SetFileName(partFilename.c_str());
    partReader->UpdateInformation();
    if (partReader->GetNumberOfSteps() != numberOfSteps)
    {
      vtkLog(ERROR, << "Wrong timestep count for " << partFilename);
      return false;
    }
    partReader->SetStep(step);
    partReader->Update();
    vtkDataObject* partOutput = partReader->GetOutputDataObject(0);
    if ((numberOfSteps > 1 && partReader->GetTimeValue() != step) ||
      !CompareReadOutput(partOutput, mainSourceOutput, config, myRank, false))
    {
      vtkLog(ERROR, << "Failed comparison for " << partFilename << " at step " << step);
      return false;
    }
  }

  return true;
}

bool ReadAndCompareAllSteps(vtkDataObjectAlgorithm* source, const Configuration& config,
  vtkMPIController* controller, const std::string& filebase)
{
  const int numberOfSteps = config.Temporal == TemporalType::Temporal ? NumberOfSteps : 1;
  bool success = true;
  for (int step = 0; step < numberOfSteps; ++step)
  {
    success &= ReadAndCompareStep(source, config, controller, filebase, step, numberOfSteps);
  }
  return success;
}

/**
 * Write, read and assert on result for a given configuration
 */
bool RunTestConfig(const Configuration& config, std::size_t id, vtkMPIController* controller,
  const std::string& tmpDir)
{
  auto source = MakeConfigurationSource(config);

  std::string filebase = tmpDir + "/hdfwriter_config_" + vtk::to_string(id) + "_" + config.Name;

  vtkNew<vtkHDFWriter> writer;
  WriteToFile(writer, source, config, controller, filebase);
  controller->Barrier();

  return ReadAndCompareAllSteps(source, config, controller, filebase);
}

/**
 * Run all configurations, return the number of failures
 */
std::size_t RunAllConfigs(vtkMPIController* controller, const std::string& tmpDir,
  std::vector<Configuration> configs, const std::string& caseName)
{
  std::size_t numFailed = 0;
  for (std::size_t i = 0; i < configs.size(); ++i)
  {
    if (!caseName.empty() && configs[i].Name != caseName)
    {
      continue;
    }
    const int localResult = RunTestConfig(configs[i], i, controller, tmpDir) ? 1 : 0;
    int result = 0;
    controller->AllReduce(&localResult, &result, 1, vtkCommunicator::MIN_OP);
    numFailed += result ? 0 : 1;
    if (controller->GetLocalProcessId() == 0)
    {
      vtkLog(INFO, << i + 1 << "/" << configs.size() << " " << configs[i].Name
                   << (result ? ": PASS " : ": FAIL"));
    }
  }
  if (controller->GetLocalProcessId() == 0)
  {
    std::cout << "Failed: " << numFailed << " / " << configs.size() << '\n' << std::flush;
  }

  return numFailed;
}
}

bool HDFWriterDistributedTestUtilities::RunDistributedConfigurations(
  int argc, char* argv[], std::vector<Configuration> configs)
{
  // Initialize MPI Controller
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);

  std::string threadName = "rank #";
  threadName += vtk::to_string(controller->GetLocalProcessId());
  vtkLogger::SetThreadName(threadName);

  char* temp =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  const std::string tmpDir(temp);
  delete[] temp;

  // Helper: run a single test case by its name using --case <test_case>
  std::string caseName;
  for (int arg = 1; arg < argc; ++arg)
  {
    if (std::string(argv[arg]) == "--case")
    {
      if (arg + 1 == argc)
      {
        break;
      }
      caseName = argv[++arg];
      break;
    }
  }

  const std::size_t numFailed = RunAllConfigs(controller, tmpDir, std::move(configs), caseName);
  vtkMultiProcessController::SetGlobalController(nullptr);
  controller->Finalize();
  return numFailed == 0;
}
