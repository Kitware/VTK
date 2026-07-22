// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "HDFTestUtilities.h"

#include "vtkAlgorithm.h"
#include "vtkCellArray.h"
#include "vtkDataObject.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkFieldData.h"
#include "vtkGroupDataSetsFilter.h"
#include "vtkHDFReader.h"
#include "vtkHDFWriter.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridAlgorithm.h"
#include "vtkHyperTreeGridSource.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkRandomHyperTreeGridSource.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringFormatter.h"
#include "vtkTestUtilities.h"

#include <iostream>
#include <string>

namespace HDFTestUtilities
{
vtkStandardNewMacro(vtkHTGChangingDescriptorSource);
}

namespace
{

//----------------------------------------------------------------------------
bool TestHDFWriterHTGTDistributedTemporalPDC(
  vtkMPIController* controller, const std::string& tempDir, bool extTime)
{
  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  std::string filePath =
    tempDir + "/HDFWriterHTGTemporalPDC" + (extTime ? "ExtTime" : "NoExtTime") + ".vtkhdf";

  vtkNew<HDFTestUtilities::vtkHTGChangingDescriptorSource> source1;
  source1->SetDescriptors({ "1.R0R.|........", "0...R|...R|....", "2.0.1.R|..R.|...." });
  source1->SetMasks({ "0110|01000010", "0001|0001|0001", "1011|0010|1101" });
  source1->SetDimensions({ 3, 3, 1 });
  source1->SetBranchFactor(2);

  vtkNew<vtkRandomHyperTreeGridSource> htgSource;
  htgSource->SetOutputBounds(-6, -3, -4, -1, -5, -1);
  htgSource->SetMaxDepth(4);

  vtkNew<vtkGroupDataSetsFilter> pdcGroup;
  pdcGroup->SetOutputTypeToPartitionedDataSetCollection();
  pdcGroup->AddInputConnection(source1->GetOutputPort());
  pdcGroup->AddInputConnection(htgSource->GetOutputPort());

  vtkNew<vtkHDFWriter> writer;
  writer->SetFileName(filePath.c_str());
  writer->SetInputConnection(pdcGroup->GetOutputPort());
  writer->SetChunkSize(100);
  writer->SetWriteAllTimeSteps(true);
  writer->Write();

  controller->Barrier();

  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(filePath.c_str());
  reader->UpdatePiece(myRank, nbRanks, 0);

  constexpr int nbSteps = 3;
  if (reader->GetNumberOfSteps() != nbSteps)
  {
    vtkLog(ERROR, "Unexpected number of steps: " << reader->GetNumberOfSteps());
    return false;
  }

  for (int step = 0; step < nbSteps; step++)
  {
    reader->SetStep(step);
    reader->UpdatePiece(myRank, nbRanks, 0);
    auto* pdcRead = vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutputDataObject(0));

    pdcGroup->GetOutputInformation(0)->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), static_cast<double>(step));
    pdcGroup->Modified();
    pdcGroup->UpdatePiece(myRank, nbRanks, 0);
    auto pdcSource =
      vtkPartitionedDataSetCollection::SafeDownCast(pdcGroup->GetOutputDataObject(0));

    // VTKHDF Writer automatically creates an assembly
    pdcSource->SetDataAssembly(pdcRead->GetDataAssembly());

    // Remove added "Time" field data so the comparison is correct
    // And rearrange number of partitions so it matches exactly the input
    vtkNew<vtkFieldData> fd;
    for (unsigned int part = 0; part < pdcRead->GetNumberOfPartitionedDataSets(); part++)
    {
      auto pds = pdcRead->GetPartitionedDataSet(part);
      pds->SetPartition(0, pds->GetPartitionAsDataObject(myRank));
      pds->GetPartitionAsDataObject(0)->SetFieldData(fd);
      pds->SetNumberOfPartitions(1);
    }

    if (!vtkTestUtilities::CompareDataObjects(pdcRead, pdcSource))
    {
      std::cerr << "Partitioned data does not match: " << filePath << " for time step " << step
                << std::endl;
      return false;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool TestDistributedSimpleHTG(vtkMPIController* controller, const std::string& tempDir)
{
  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  vtkNew<vtkHyperTreeGridSource> htgSource;
  htgSource->SetDescriptor("2.R0R.|........");
  htgSource->SetDimensions(3, 3, 1);
  htgSource->SetBranchFactor(2);
  htgSource->SetMask("0110|01000010");
  htgSource->SetUseMask(true);
  htgSource->SetMaxDepth(2);

  const std::string writtenFile = tempDir + "/distributed_simplehtg.vtkhdf";
  vtkNew<vtkHDFWriter> writer;
  writer->SetFileName(writtenFile.c_str());
  writer->SetInputConnection(htgSource->GetOutputPort());
  writer->SetChunkSize(100);
  writer->Write();

  controller->Barrier();

  vtkNew<vtkHDFReader> readerHDF;
  readerHDF->SetFileName(writtenFile.c_str());
  readerHDF->UpdatePiece(myRank, nbRanks, 0);

  auto inputData = vtkHyperTreeGrid::SafeDownCast(htgSource->GetOutputDataObject(0));
  auto outputData = vtkPartitionedDataSet::SafeDownCast(readerHDF->GetOutputDataObject(0));

  if (!vtkTestUtilities::CompareDataObjects(
        inputData, outputData->GetPartitionAsDataObject(myRank)))
  {
    vtkLog(ERROR, "Original and read part do not match");
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool TestDistributedPartitionedHTG(vtkMPIController* controller, const std::string& tempDir)
{
  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  vtkNew<vtkRandomHyperTreeGridSource> htgSource;
  htgSource->SetDimensions(5, 6, 4);
  htgSource->SetSplitFraction(0.5);
  htgSource->SetMaskedFraction(0.5);
  htgSource->SetMaxDepth(3);
  htgSource->Update();

  const std::string writtenFile = tempDir + "/distributed_randomhtg.vtkhdf";
  vtkNew<vtkHDFWriter> writer;
  writer->SetFileName(writtenFile.c_str());
  writer->SetInputConnection(htgSource->GetOutputPort());
  writer->SetChunkSize(100);
  writer->Write();

  controller->Barrier();

  vtkNew<vtkHDFReader> readerHDF;
  readerHDF->SetFileName(writtenFile.c_str());
  readerHDF->UpdatePiece(myRank, nbRanks, 0);

  auto inputData = vtkHyperTreeGrid::SafeDownCast(htgSource->GetOutputDataObject(0));
  auto outputData = vtkPartitionedDataSet::SafeDownCast(readerHDF->GetOutputDataObject(0));

  if (!vtkTestUtilities::CompareDataObjects(
        inputData, outputData->GetPartitionAsDataObject(myRank)))
  {
    vtkLog(ERROR, "Original and read part do not match");
    return false;
  }

  controller->Barrier();

  // Write to disk again the partitioned HTG that was just read
  const std::string writtenFile2 = tempDir + "/distributed_randomhtg2.vtkhdf";
  vtkNew<vtkHDFWriter> writer2;
  writer2->SetInputDataObject(outputData);
  writer2->SetFileName(writtenFile2.c_str());
  writer2->SetChunkSize(100);
  writer2->Write();

  controller->Barrier();

  readerHDF->SetFileName(writtenFile2.c_str());
  readerHDF->UpdatePiece(myRank, nbRanks, 0);

  auto outputData2 = vtkPartitionedDataSet::SafeDownCast(readerHDF->GetOutputDataObject(0));

  if (!vtkTestUtilities::CompareDataObjects(outputData2->GetPartitionAsDataObject(myRank),
        outputData->GetPartitionAsDataObject(myRank)))
  {
    vtkLog(ERROR, "Original and read part do not match");
    return false;
  }

  return true;
}

bool TestHDFWriterDistributedMissingBlocksMB(
  vtkMPIController* controller, const std::string& tempDir)
{
  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  // Create a multiblock, with non null HTG partition on 1 different block on each rank
  vtkNew<vtkMultiBlockDataSet> mb;
  mb->SetNumberOfBlocks(nbRanks);

  vtkNew<vtkRandomHyperTreeGridSource> randomHTG;
  randomHTG->SetDimensions(3, 3, 3);
  randomHTG->SetMaskedFraction(0.2);
  randomHTG->SetOutputBounds(10.0 * myRank, 10.0 * (myRank + 1), 0.0, 10.0, 0.0, 10.0);
  randomHTG->SetSeed(myRank);
  randomHTG->Update();
  mb->SetBlock(myRank, randomHTG->GetHyperTreeGridOutput());

  const std::string writtenFile = tempDir + "/composite_distrib_htg.vtkhdf";
  vtkNew<vtkHDFWriter> writer;
  writer->SetFileName(writtenFile.c_str());
  writer->SetInputDataObject(mb);
  writer->SetChunkSize(100);
  writer->Write();

  controller->Barrier();

  vtkNew<vtkHDFReader> readerHDF;
  readerHDF->SetFileName(writtenFile.c_str());
  // Get all pieces on the current rank
  // Currently the vtkHDFReader does not distributed pieces among ranks like XML does
  // See  https://gitlab.kitware.com/vtk/vtk/-/work_items/20118
  readerHDF->Update();

  auto outputData = vtkMultiBlockDataSet::SafeDownCast(readerHDF->GetOutputDataObject(0));
  if (!vtkTestUtilities::CompareDataObjects(outputData->GetBlock(myRank), mb->GetBlock(myRank)))
  {
    vtkLog(ERROR, "Original and read part do not match");
    return false;
  }

  return true;
}
}

//----------------------------------------------------------------------------
int TestHDFWriterHTGDistributed(int argc, char* argv[])
{
  // Initialize MPI Controller
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);

  std::string threadName = "rank #";
  threadName += vtk::to_string(controller->GetLocalProcessId());
  vtkLogger::SetThreadName(threadName);

  char* tempDirCStr =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  std::string tempDir{ tempDirCStr };
  delete[] tempDirCStr;

  bool testPasses = true;
  testPasses &= TestHDFWriterDistributedMissingBlocksMB(controller, tempDir);
  testPasses &= TestHDFWriterHTGTDistributedTemporalPDC(controller, tempDir, false);
  testPasses &= TestHDFWriterHTGTDistributedTemporalPDC(controller, tempDir, true);
  testPasses &= ::TestDistributedSimpleHTG(controller, tempDir);
  testPasses &= ::TestDistributedPartitionedHTG(controller, tempDir);

  controller->Finalize();

  return testPasses ? EXIT_SUCCESS : EXIT_FAILURE;
}
