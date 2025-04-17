// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "HDFTestUtilities.h"

#include "vtkAppendDataSets.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkForceStaticMesh.h"
#include "vtkGenerateTimeSteps.h"
#include "vtkGroupDataSetsFilter.h"
#include "vtkHDFReader.h"
#include "vtkHDFWriter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkMergeBlocks.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPartitionedDataSetCollectionAlgorithm.h"
#include "vtkPassArrays.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRedistributeDataSetFilter.h"
#include "vtkSpatioTemporalHarmonicsAttribute.h"
#include "vtkSphereSource.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkWarpScalar.h"
#include "vtkXMLMultiBlockDataReader.h"
#include "vtkXMLPolyDataReader.h"

namespace HDFTestUtilities
{
vtkStandardNewMacro(vtkAddAssembly);
}

namespace
{
vtkSmartPointer<vtkAlgorithm> SetupCompositePipeline(const vtkIdType compositeType)
{
  // Create a sphere source
  vtkNew<vtkSphereSource> sphere;
  sphere->SetPhiResolution(50);
  sphere->SetThetaResolution(50);
  sphere->SetRadius(5.0);

  // Distribute it
  vtkNew<vtkRedistributeDataSetFilter> redistribute;
  redistribute->SetGenerateGlobalCellIds(false);
  redistribute->SetInputConnection(sphere->GetOutputPort());

  // Extract surface to get a poly data again
  vtkNew<vtkDataSetSurfaceFilter> surface;
  surface->SetInputConnection(redistribute->GetOutputPort());

  vtkNew<vtkTransform> transform;
  transform->Translate(100.0, 10.0, 10.0);
  vtkNew<vtkTransformFilter> transformFilter;
  transformFilter->SetTransform(transform);
  transformFilter->SetInputConnection(surface->GetOutputPort());

  // Create a composite structure
  vtkNew<vtkGroupDataSetsFilter> group;
  group->SetOutputType(compositeType);
  group->AddInputConnection(redistribute->GetOutputPort());
  group->AddInputConnection(transformFilter->GetOutputPort());

  return group;
}

//------------------------------------------------------------------------------
/**
 * Pipeline used for this test:
 * Cow > Redistribute > (usePolyData ? SurfaceFilter ) > Generate Time steps > Harmonics >
 * (!staticMesh ? warp by scalar) > Pass arrays > VTKHDF Writer > Read whole/part
 *
 * No animals were harmed in the making of this test.
 */
bool TestDistributedObject(
  vtkMPIController* controller, const std::string& tempDir, bool usePolyData)
{
  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  // Create a sphere source
  vtkNew<vtkSphereSource> sphere;
  sphere->SetPhiResolution(50);
  sphere->SetThetaResolution(50);

  // Distribute it
  vtkNew<vtkRedistributeDataSetFilter> redistribute;
  redistribute->SetGenerateGlobalCellIds(false);
  redistribute->SetInputConnection(sphere->GetOutputPort());

  // Extract surface to get a poly data again
  vtkNew<vtkDataSetSurfaceFilter> surface;
  surface->SetInputConnection(redistribute->GetOutputPort());

  // Write it to disk
  std::string prefix = tempDir + "/parallel_sphere_" + (usePolyData ? "PD" : "UG");
  std::string filePath = prefix + ".vtkhdf";
  std::string filePathPart = prefix + "_part" + std::to_string(myRank) + ".vtkhdf";

  vtkNew<vtkHDFWriter> writer;
  writer->SetInputConnection(
    usePolyData ? surface->GetOutputPort() : redistribute->GetOutputPort());
  writer->SetFileName(filePath.c_str());
  writer->Write();

  // Wait for all processes to be done writing
  controller->Barrier();

  // Reopen file and compare it to the source
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(filePath.c_str());
  reader->UpdatePiece(myRank, nbRanks, 0);

  vtkNew<vtkHDFReader> readerPart;
  readerPart->SetFileName(filePathPart.c_str());
  readerPart->Update();

  vtkDataObject* readPiece = reader->GetOutputDataObject(0);
  vtkDataObject* originalPiece =
    usePolyData ? surface->GetOutputDataObject(0) : redistribute->GetOutputDataObject(0);
  vtkDataObject* readPart = readerPart->GetOutputDataObject(0);

  auto partitionedPiece = vtkPartitionedDataSet::SafeDownCast(readPiece);

  if (!vtkTestUtilities::CompareDataObjects(originalPiece, partitionedPiece->GetPartition(0)))
  {
    vtkLog(ERROR, "Original and read piece do not match");
    return false;
  }

  if (!vtkTestUtilities::CompareDataObjects(partitionedPiece->GetPartition(0), readPart))
  {
    vtkLog(ERROR, "Read piece and read part do not match");
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool TestCompositeDistributedObject(
  vtkMPIController* controller, const std::string& tempDir, const vtkIdType compositeType)
{
  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  vtkSmartPointer<vtkGroupDataSetsFilter> group =
    vtkGroupDataSetsFilter::SafeDownCast(::SetupCompositePipeline(compositeType));
  group->UpdatePiece(myRank, nbRanks, 0);

  vtkNew<HDFTestUtilities::vtkAddAssembly> addAssembly;
  addAssembly->SetInputConnection(group->GetOutputPort());

  // Write it to disk
  std::string prefix = tempDir + "/parallel_composite_" + std::to_string(compositeType);
  std::string filePath = prefix + ".vtkhdf";
  std::string filePathPart = prefix + "_part" + std::to_string(myRank) + ".vtkhdf";

  vtkNew<vtkHDFWriter> writer;
  writer->SetInputConnection(compositeType == VTK_PARTITIONED_DATA_SET_COLLECTION
      ? addAssembly->GetOutputPort()
      : group->GetOutputPort());
  writer->SetFileName(filePath.c_str());
  writer->Write();

  // Wait for all processes to be done writing
  controller->Barrier();

  // Reopen file and compare it to the source
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(filePath.c_str());
  reader->UpdatePiece(myRank, nbRanks, 0);

  vtkNew<vtkHDFReader> readerPart;
  readerPart->SetFileName(filePathPart.c_str());
  readerPart->Update();

  if (compositeType == VTK_MULTIBLOCK_DATA_SET)
  {
    auto originalPiece = vtkMultiBlockDataSet::SafeDownCast(group->GetOutputDataObject(0));
    auto readPart = vtkMultiBlockDataSet::SafeDownCast(readerPart->GetOutputDataObject(0));
    auto readTotal = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutputDataObject(0));

    vtkMultiPieceDataSet* ugMP = vtkMultiPieceDataSet::SafeDownCast(readTotal->GetBlock(0));
    vtkMultiPieceDataSet* pdMP = vtkMultiPieceDataSet::SafeDownCast(readTotal->GetBlock(1));
    vtkUnstructuredGrid* ugBlock = vtkUnstructuredGrid::SafeDownCast(ugMP->GetPartition(0));
    vtkPolyData* pdBlock = vtkPolyData::SafeDownCast(pdMP->GetPartition(0));

    if (!vtkTestUtilities::CompareDataObjects(readPart->GetBlock(0), ugBlock))
    {
      vtkLog(ERROR, "Read block 0 and read part do not match");
      return false;
    }
    if (!vtkTestUtilities::CompareDataObjects(readPart->GetBlock(1), pdBlock))
    {
      vtkLog(ERROR, "Read block 1 and read part do not match");
      return false;
    }

    if (!vtkTestUtilities::CompareDataObjects(originalPiece, readPart))
    {
      vtkLog(ERROR, "Original and read part do not match");
      return false;
    }
  }
  else
  {
    auto originalPiece =
      vtkPartitionedDataSetCollection::SafeDownCast(addAssembly->GetOutputDataObject(0));
    auto readPart =
      vtkPartitionedDataSetCollection::SafeDownCast(readerPart->GetOutputDataObject(0));
    auto readTotal = vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutputDataObject(0));

    if (!vtkTestUtilities::CompareDataObjects(readPart, readTotal))
    {
      vtkLog(ERROR, "Original and read global assembly do not match");
      return false;
    }

    if (!vtkTestUtilities::CompareDataObjects(originalPiece, readPart))
    {
      vtkLog(ERROR, "Original and read part do not match");
      return false;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool TestDistributedTemporal(vtkMPIController* controller, const std::string& tempDir,
  const std::string& dataRoot, bool usePolyData, bool staticMesh, bool nullPart)
{
  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  const std::string basePath = dataRoot + "/Data/cow.vtp";
  vtkNew<vtkXMLPolyDataReader> baseReader;
  baseReader->SetFileName(basePath.c_str());

  // Redistribute cow
  vtkNew<vtkRedistributeDataSetFilter> redistribute;
  redistribute->SetGenerateGlobalCellIds(true);
  redistribute->SetInputConnection(baseReader->GetOutputPort());

  // Extract surface to get a poly data again
  vtkNew<vtkDataSetSurfaceFilter> surface;
  surface->SetInputConnection(redistribute->GetOutputPort());

  // Generate several time steps
  vtkNew<vtkGenerateTimeSteps> generateTimeSteps;
  const std::array timeValues{ 1.0, 3.0, 5.0 };
  for (const double& value : timeValues)
  {
    generateTimeSteps->AddTimeStepValue(value);
  }
  generateTimeSteps->SetInputConnection(
    usePolyData ? surface->GetOutputPort() : redistribute->GetOutputPort());

  // Generate a time-varying point field: use default ParaView weights
  vtkNew<vtkSpatioTemporalHarmonicsAttribute> harmonics;
  harmonics->AddHarmonic(1.0, 1.0, 0.6283, 0.6283, 0.6283, 0.0);
  harmonics->AddHarmonic(3.0, 1.0, 0.6283, 0.0, 0.0, 1.5708);
  harmonics->AddHarmonic(2.0, 2.0, 0.0, 0.6283, 0.0, 3.1416);
  harmonics->AddHarmonic(1.0, 3.0, 0.0, 0.0, 0.6283, 4.7124);
  harmonics->SetInputConnection(generateTimeSteps->GetOutputPort());

  // Warp by scalar
  vtkNew<vtkWarpScalar> warp;
  warp->SetInputConnection(harmonics->GetOutputPort());

  // Write data in parallel to disk
  std::string prefix = tempDir + "/parallel_time_cow" + (usePolyData ? "_PD" : "_UG") +
    (staticMesh ? "_static" : "") + (nullPart ? "_null" : "");
  std::string filePath = prefix + ".vtkhdf";
  std::string filePathPart = prefix + "_part" + std::to_string(myRank) + ".vtkhdf";

  vtkNew<vtkHDFWriter> writer;

  harmonics->Update();
  warp->Update();
  vtkDataObject* output =
    staticMesh ? harmonics->GetOutputDataObject(0) : warp->GetOutputDataObject(0);
  // Set a null part in the middle of the others, to make sure it is handled well
  if (nullPart && myRank == 2)
  {
    if (usePolyData)
    {
      vtkNew<vtkPolyData> pd;
      writer->SetInputDataObject(pd);
    }
    else
    {
      vtkNew<vtkUnstructuredGrid> ug;
      vtkNew<vtkPoints> points;
      ug->SetPoints(points);
      writer->SetInputDataObject(ug);
    }
  }
  else
  {
    writer->SetInputDataObject(output);
  }
  writer->SetWriteAllTimeSteps(true);
  writer->SetFileName(filePath.c_str());
  writer->Write();

  // All processes have written their pieces to disk
  controller->Barrier();

  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(filePath.c_str());
  reader->UpdatePiece(myRank, nbRanks, 0);

  vtkNew<vtkHDFReader> readerPart;
  readerPart->SetFileName(filePathPart.c_str());
  readerPart->Update();

  for (int time = 0; time < static_cast<int>(timeValues.size()); time++)
  {
    reader->SetStep(time);
    reader->UpdatePiece(myRank, nbRanks, 0);

    readerPart->SetStep(time);
    readerPart->Update();

    vtkPartitionedDataSet* readPartitionedPiece =
      vtkPartitionedDataSet::SafeDownCast(reader->GetOutputDataObject(0));
    if (usePolyData)
    {
      vtkPolyData* readPiece = vtkPolyData::SafeDownCast(readPartitionedPiece->GetPartition(0));
      vtkPolyData* readPart = vtkPolyData::SafeDownCast(readerPart->GetOutputDataObject(0));

      if (readPiece == nullptr || readPart == nullptr)
      {
        vtkLog(ERROR, "Piece should not be null");
        return false;
      }
    }
    else
    {
      vtkUnstructuredGrid* readPiece =
        vtkUnstructuredGrid::SafeDownCast(readPartitionedPiece->GetPartition(0));
      vtkUnstructuredGrid* readPart =
        vtkUnstructuredGrid::SafeDownCast(readerPart->GetOutputDataObject(0));

      if (readPiece == nullptr || readPart == nullptr)
      {
        vtkLog(ERROR, "Piece should not be null");
        return false;
      }
    }

    vtkDataObject* readPiece = readPartitionedPiece->GetPartition(0);
    vtkDataObject* readPart = readerPart->GetOutputDataObject(0);

    if (nullPart && myRank == 2)
    {
      if (readPiece->GetNumberOfElements(vtkDataSet::POINT) +
          readPart->GetNumberOfElements(vtkDataSet::POINT) +
          readPiece->GetNumberOfElements(vtkDataSet::CELL) +
          readPart->GetNumberOfElements(vtkDataSet::CELL) >
        0)
      {
        vtkLog(ERROR, "Read piece or read part do not have 0 elements each when partition is null");
        return false;
      }
    }
    else if (!vtkTestUtilities::CompareDataObjects(readPiece, readPart))
    {
      vtkLog(ERROR, "Read piece and read part do not match");
      return false;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool TestCompositeTemporalDistributedObject(
  vtkMPIController* controller, const std::string& tempDir, const vtkIdType compositeType)
{
  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  vtkSmartPointer<vtkGroupDataSetsFilter> group =
    vtkGroupDataSetsFilter::SafeDownCast(::SetupCompositePipeline(compositeType));
  group->UpdatePiece(myRank, nbRanks, 0);

  vtkNew<HDFTestUtilities::vtkAddAssembly> addAssembly;
  addAssembly->SetInputConnection(group->GetOutputPort());

  // Generate several time steps
  vtkNew<vtkGenerateTimeSteps> generateTimeSteps;
  const std::array timeValues{ 1.0, 3.0, 5.0 };
  for (const double& value : timeValues)
  {
    generateTimeSteps->AddTimeStepValue(value);
  }
  generateTimeSteps->SetInputConnection(compositeType == VTK_PARTITIONED_DATA_SET_COLLECTION
      ? addAssembly->GetOutputPort()
      : group->GetOutputPort());

  // Generate a time-varying point field: use default ParaView weights
  vtkNew<vtkSpatioTemporalHarmonicsAttribute> harmonics;
  harmonics->AddHarmonic(1.0, 1.0, 0.6283, 0.6283, 0.6283, 0.0);
  harmonics->AddHarmonic(3.0, 1.0, 0.6283, 0.0, 0.0, 1.5708);
  harmonics->AddHarmonic(2.0, 2.0, 0.0, 0.6283, 0.0, 3.1416);
  harmonics->AddHarmonic(1.0, 3.0, 0.0, 0.0, 0.6283, 4.7124);
  harmonics->SetInputConnection(generateTimeSteps->GetOutputPort());

  // Write it to disk
  std::string prefix = tempDir + "/parallel_temporal_composite_" + std::to_string(compositeType);
  std::string filePath = prefix + ".vtkhdf";
  std::string filePathPart = prefix + "_part" + std::to_string(myRank) + ".vtkhdf";

  vtkNew<vtkHDFWriter> writer;
  writer->SetWriteAllTimeSteps(true);
  writer->SetFileName(filePath.c_str());
  writer->SetInputConnection(harmonics->GetOutputPort());
  writer->Write();

  // All processes have written their pieces to disk
  controller->Barrier();

  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(filePath.c_str());

  vtkNew<vtkHDFReader> readerPart;
  readerPart->SetFileName(filePathPart.c_str());
  readerPart->Update();

  for (int time = 0; time < static_cast<int>(timeValues.size()); time++)
  {
    reader->SetStep(time);
    reader->UpdatePiece(myRank, nbRanks, 0);

    readerPart->SetStep(time);
    readerPart->Update();

    if (compositeType == VTK_MULTIBLOCK_DATA_SET)
    {
      auto readPart = vtkMultiBlockDataSet::SafeDownCast(readerPart->GetOutputDataObject(0));
      auto readTotal = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutputDataObject(0));

      // Distributed Multi-block yields a vtkMultiPiece but the single part version does not,
      // So we need to decompose them to be able to compare.
      vtkMultiPieceDataSet* ugMP = vtkMultiPieceDataSet::SafeDownCast(readTotal->GetBlock(0));
      vtkUnstructuredGrid* ugBlock2 = vtkUnstructuredGrid::SafeDownCast(readPart->GetBlock(0));
      vtkMultiPieceDataSet* pdMP = vtkMultiPieceDataSet::SafeDownCast(readTotal->GetBlock(1));
      vtkUnstructuredGrid* ugBlock = vtkUnstructuredGrid::SafeDownCast(ugMP->GetPartition(0));
      vtkPolyData* pdBlock = vtkPolyData::SafeDownCast(pdMP->GetPartition(0));
      vtkPolyData* pdBlock2 = vtkPolyData::SafeDownCast(readPart->GetBlock(1));

      if (!vtkTestUtilities::CompareDataObjects(pdBlock, pdBlock2))
      {
        vtkLog(ERROR, "Original and read part do not match");
        return false;
      }
      if (!vtkTestUtilities::CompareDataObjects(ugBlock, ugBlock2))
      {
        vtkLog(ERROR, "Original and read part do not match");
        return false;
      }
    }
    else
    {
      auto readPart =
        vtkPartitionedDataSetCollection::SafeDownCast(readerPart->GetOutputDataObject(0));
      auto readTotal =
        vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutputDataObject(0));

      if (!vtkTestUtilities::CompareDataObjects(readPart, readTotal))
      {
        vtkLog(ERROR, "Original and read part do not match");
        return false;
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool TestDistributedMultiBlockMissingBlocks(
  vtkMPIController* controller, const std::string& tempDir, const std::string& dataRoot)
{
  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  // This data has VTU and VTP blocks, that are automatically read on different ranks.
  // MPI Rank 0 will have block 'Sub1', Rank 1 'Sub2' and Rank 2 have no data.
  // We make sure that the written VTKHDF dataset has the right data on the right processes.

  vtkNew<vtkXMLMultiBlockDataReader> reader;
  const std::string sourcefile = dataRoot + "/Data/vtkHDF/distributed.vtm";
  reader->SetFileName(sourcefile.c_str());
  reader->UpdatePiece(myRank, nbRanks, 0);

  const std::string writtenFile = tempDir + "/distributed_missing_blocks.vtkhdf";
  vtkNew<vtkHDFWriter> writer;
  writer->SetWriteAllTimeSteps(false);
  writer->SetFileName(writtenFile.c_str());
  writer->SetInputConnection(reader->GetOutputPort());
  writer->SetDebug(true);
  writer->Write();

  controller->Barrier();

  vtkNew<vtkHDFReader> readerHDF;
  readerHDF->SetFileName(writtenFile.c_str());
  readerHDF->UpdatePiece(myRank, nbRanks, 0);

  vtkNew<vtkMergeBlocks> merge;
  merge->SetInputConnection(readerHDF->GetOutputPort());
  merge->SetMergePartitionsOnly(true);
  merge->Update();

  auto inputData = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutputDataObject(0));
  auto outputData = vtkMultiBlockDataSet::SafeDownCast(merge->GetOutputDataObject(0));

  if (!vtkTestUtilities::CompareDataObjects(inputData, outputData))
  {
    vtkLog(ERROR, "Original and read part do not match");
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool TestDistributedTemporalStaticMultiBlockMissingBlocks(
  vtkMPIController* controller, const std::string& tempDir, const std::string& dataRoot)
{
  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  // We read from a VTKHDF file, because it is the only VTK-native format
  // that supports temporal multiblock right now.

  vtkNew<vtkHDFReader> baselineReader;
  const std::string fileExt = "/test_multiblock_static_multipiece.vtkhdf";
  const std::string sourcefile = dataRoot + "/Data/vtkHDF" + fileExt;
  baselineReader->SetFileName(sourcefile.c_str());
  baselineReader->UpdatePiece(myRank, nbRanks, 0);

  vtkNew<vtkForceStaticMesh> forceStatic;
  forceStatic->SetInputConnection(baselineReader->GetOutputPort());

  std::string writtenFile = tempDir + fileExt;
  vtkNew<vtkHDFWriter> writer;
  writer->SetWriteAllTimeSteps(true);
  writer->SetFileName(writtenFile.c_str());
  writer->SetInputConnection(forceStatic->GetOutputPort());
  writer->Write();

  controller->Barrier();

  vtkNew<vtkHDFReader> readerHDF;
  readerHDF->SetFileName(writtenFile.c_str());
  readerHDF->SetUseCache(true);
  readerHDF->UpdatePiece(myRank, nbRanks, 0);

  if (baselineReader->GetNumberOfSteps() != 2)
  {
    std::cerr << "Expected 2 time steps but got " << baselineReader->GetNumberOfSteps()
              << " in baseline reader." << std::endl;
    return false;
  }
  if (readerHDF->GetNumberOfSteps() != 2)
  {
    std::cerr << "Expected 2 time steps but got " << readerHDF->GetNumberOfSteps() << std::endl;
    return false;
  }

  baselineReader->GetOutputInformation(0)->Remove(
    vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

  for (int time = 0; time < static_cast<int>(baselineReader->GetNumberOfSteps()); time++)
  {
    vtkLog(INFO, "Processing time step " << time);
    baselineReader->SetStep(time);
    baselineReader->UpdatePiece(myRank, nbRanks, 0);

    readerHDF->SetStep(time);
    readerHDF->UpdatePiece(myRank, nbRanks, 0);

    if (readerHDF->GetTimeValue() != baselineReader->GetTimeValue())
    {
      std::cerr << "Wrong time value: " << readerHDF->GetTimeValue()
                << " != " << baselineReader->GetTimeValue() << std::endl;
      return false;
    }

    auto inputData = vtkMultiBlockDataSet::SafeDownCast(baselineReader->GetOutputDataObject(0));
    auto outputData = vtkMultiBlockDataSet::SafeDownCast(readerHDF->GetOutputDataObject(0));

    if (!vtkTestUtilities::CompareDataObjects(inputData, outputData))
    {
      vtkLog(ERROR, "Original and read part do not match");
      return false;
    }

    // TODO: Fix composite MeshMTime
    // Currently, we disable data object cache for composite structures, because the cache can only
    // handle one object at a time. Data reading is still cached, but MTime will change.
    // See https://gitlab.kitware.com/vtk/vtk/-/issues/19658
    auto outputMeshTime = vtkDataSet::SafeDownCast(
      vtkMultiPieceDataSet::SafeDownCast(outputData->GetBlock(0))->GetPartition(0))
                            ->GetMeshMTime();

    vtkLog(INFO, << "MeshMtime is " << outputMeshTime);
  }
  return true;
}

//------------------------------------------------------------------------------
bool TestDistributedPolyData(vtkMPIController* controller, const std::string& tempDir)
{
  return TestDistributedObject(controller, tempDir, true);
}

//------------------------------------------------------------------------------
bool TestDistributedUnstructuredGrid(vtkMPIController* controller, const std::string& tempDir)
{
  return TestDistributedObject(controller, tempDir, false);
}

//------------------------------------------------------------------------------
bool TestDistributedMultiBlock(vtkMPIController* controller, const std::string& tempDir)
{
  return TestCompositeDistributedObject(controller, tempDir, VTK_MULTIBLOCK_DATA_SET);
}

//------------------------------------------------------------------------------
bool TestDistributedPartitionedDataSetCollection(
  vtkMPIController* controller, const std::string& tempDir)
{
  return TestCompositeDistributedObject(controller, tempDir, VTK_PARTITIONED_DATA_SET_COLLECTION);
}

//------------------------------------------------------------------------------
bool TestDistributedUnstructuredGridTemporal(
  vtkMPIController* controller, const std::string& tempDir, const std::string& dataRoot)
{
  return TestDistributedTemporal(controller, tempDir, dataRoot, false, false, false);
}

//------------------------------------------------------------------------------
bool TestDistributedUnstructuredGridTemporalStatic(
  vtkMPIController* controller, const std::string& tempDir, const std::string& dataRoot)
{
  return TestDistributedTemporal(controller, tempDir, dataRoot, false, true, false);
}

//------------------------------------------------------------------------------
bool TestDistributedUnstructuredGridTemporalNullPart(
  vtkMPIController* controller, const std::string& tempDir, const std::string& dataRoot)
{
  return TestDistributedTemporal(controller, tempDir, dataRoot, false, false, true);
}

//------------------------------------------------------------------------------
bool TestDistributedPolyDataTemporal(
  vtkMPIController* controller, const std::string& tempDir, const std::string& dataRoot)
{
  return TestDistributedTemporal(controller, tempDir, dataRoot, true, false, false);
}

//------------------------------------------------------------------------------
bool TestDistributedPolyDataTemporalStatic(
  vtkMPIController* controller, const std::string& tempDir, const std::string& dataRoot)
{
  return TestDistributedTemporal(controller, tempDir, dataRoot, true, true, false);
}

//------------------------------------------------------------------------------
bool TestDistributedTemporalMultiBlock(vtkMPIController* controller, const std::string& tempDir)
{
  return TestCompositeTemporalDistributedObject(controller, tempDir, VTK_MULTIBLOCK_DATA_SET);
}

//------------------------------------------------------------------------------
bool TestDistributedTemporalPartitionedDataSetCollection(
  vtkMPIController* controller, const std::string& tempDir)
{
  return TestCompositeTemporalDistributedObject(
    controller, tempDir, VTK_PARTITIONED_DATA_SET_COLLECTION);
}
}

int TestHDFWriterDistributed(int argc, char* argv[])
{
  // Initialize MPI Controller
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);

  std::string threadName = "rank #";
  threadName += std::to_string(controller->GetLocalProcessId());
  vtkLogger::SetThreadName(threadName);

  // Retrieve temporary testing directory
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

  bool res = true;
  res &= ::TestDistributedPolyData(controller, tempDir);
  res &= ::TestDistributedUnstructuredGrid(controller, tempDir);
  res &= ::TestDistributedMultiBlock(controller, tempDir);
  res &= ::TestDistributedMultiBlockMissingBlocks(controller, tempDir, dataRoot);
  res &= ::TestDistributedPartitionedDataSetCollection(controller, tempDir);
  res &= ::TestDistributedUnstructuredGridTemporal(controller, tempDir, dataRoot);
  res &= ::TestDistributedUnstructuredGridTemporalStatic(controller, tempDir, dataRoot);
  res &= ::TestDistributedUnstructuredGridTemporalNullPart(controller, tempDir, dataRoot);
  res &= ::TestDistributedPolyDataTemporal(controller, tempDir, dataRoot);
  res &= ::TestDistributedPolyDataTemporalStatic(controller, tempDir, dataRoot);
  res &= ::TestDistributedTemporalMultiBlock(controller, tempDir);
  res &= ::TestDistributedTemporalStaticMultiBlockMissingBlocks(controller, tempDir, dataRoot);
  res &= ::TestDistributedTemporalPartitionedDataSetCollection(controller, tempDir);
  controller->Finalize();
  return res ? EXIT_SUCCESS : EXIT_FAILURE;
}
