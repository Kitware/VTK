// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAppendDataSets.h"
#include "vtkCellData.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkForceStaticMesh.h"
#include "vtkGenerateTimeSteps.h"
#include "vtkGroupDataSetsFilter.h"
#include "vtkHDFReader.h"
#include "vtkHDFWriter.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkMergeBlocks.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPartitionedDataSetCollectionAlgorithm.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkRedistributeDataSetFilter.h"
#include "vtkSetGet.h"
#include "vtkSpatioTemporalHarmonicsAttribute.h"
#include "vtkSphereSource.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringFormatter.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkWarpScalar.h"
#include "vtkXMLMultiBlockDataReader.h"
#include "vtkXMLPolyDataReader.h"

#include <cstddef>
#include <iostream>

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

enum class ExternalDistributed
{
  Enabled,
  Disabled,
};

std::string OutputPrefix(const std::string& tempDir, ExternalDistributed external)
{
  return tempDir + (external == ExternalDistributed::Enabled ? "/external_" : "/");
}

//------------------------------------------------------------------------------
bool TestDistributedObject(vtkMPIController* controller, const std::string& tempDir,
  const vtkIdType datatype, ExternalDistributed external)
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
  std::string prefix = OutputPrefix(tempDir, external) + "parallel_sphere_" +
    (datatype == VTK_POLY_DATA ? "PD" : "UG");
  std::string filePath = prefix + ".vtkhdf";
  std::string filePathPart = prefix + "_part" + vtk::to_string(myRank) + ".vtkhdf";

  vtkNew<vtkHDFWriter> writer;
  writer->SetInputConnection(
    datatype == VTK_POLY_DATA ? surface->GetOutputPort() : redistribute->GetOutputPort());
  writer->SetFileName(filePath.c_str());
  writer->SetUseExternalDistributed(external == ExternalDistributed::Enabled);
  writer->Write();

  // Wait for all processes to be done writing
  controller->Barrier();

  // Reopen file and compare it to the source
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(filePath.c_str());
  reader->UpdatePiece(myRank, nbRanks, 0);

  vtkDataObject* readPiece = reader->GetOutputDataObject(0);
  vtkDataObject* originalPiece = datatype == VTK_POLY_DATA ? surface->GetOutputDataObject(0)
                                                           : redistribute->GetOutputDataObject(0);
  auto partitionedPiece = vtkPartitionedDataSet::SafeDownCast(readPiece);
  if (!partitionedPiece ||
    partitionedPiece->GetNumberOfPartitions() != static_cast<unsigned int>(nbRanks) ||
    !partitionedPiece->GetPartition(myRank))
  {
    vtkLog(ERROR, "Unexpected number of distributed partitions");
    return false;
  }

  if (!vtkTestUtilities::CompareDataObjects(originalPiece, partitionedPiece->GetPartition(myRank)))
  {
    vtkLog(ERROR, "Read piece and read part do not match");
    return false;
  }

  if (external == ExternalDistributed::Enabled)
  {
    vtkNew<vtkHDFReader> readerPart;
    readerPart->SetFileName(filePathPart.c_str());
    readerPart->Update();
    vtkDataObject* readPart = readerPart->GetOutputDataObject(0);
    if (!vtkTestUtilities::CompareDataObjects(partitionedPiece->GetPartition(myRank), readPart))
    {
      vtkLog(ERROR, "Original and read piece do not match");
      return false;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool TestDistributedArrayNames(vtkMPIController* controller, const std::string& tempDir,
  ExternalDistributed external, bool temporal = false)
{
  /*
  Test that missing or mismatched arrays on some ranks don't deadlock the writer.
  */

  const int rank = controller->GetLocalProcessId();
  const int ranks = controller->GetNumberOfProcesses();
  bool success = true;
  for (bool polyData : { false, true })
  {
    vtkNew<vtkPoints> points;
    points->InsertNextPoint(rank, 0, 0);
    points->InsertNextPoint(rank, 1, 0);
    points->InsertNextPoint(rank, 0, 1);
    const vtkIdType ids[] = { 0, 1, 2 };
    vtkSmartPointer<vtkPointSet> input;
    if (polyData)
    {
      vtkNew<vtkPolyData> data;
      data->SetPoints(points);
      vtkNew<vtkCellArray> cells;
      cells->InsertNextCell(3, ids);
      data->SetPolys(cells);
      input = data;
    }
    else
    {
      vtkNew<vtkUnstructuredGrid> data;
      data->SetPoints(points);
      data->Allocate(1);
      data->InsertNextCell(VTK_TRIANGLE, 3, ids);
      input = data;
    }
    for (int association : { vtkDataObject::POINT, vtkDataObject::CELL })
    {
      auto* attributes = input->GetAttributes(association);
      const vtkIdType tuples = input->GetNumberOfElements(association);
      vtkNew<vtkDoubleArray> common;
      common->SetName("Common");
      common->SetNumberOfTuples(tuples);
      common->Fill(rank + 1);
      vtkNew<vtkDoubleArray> local;
      local->SetName(("Rank" + vtk::to_string(rank)).c_str());
      local->SetNumberOfComponents(2);
      local->SetNumberOfTuples(tuples);
      local->Fill(rank + 1);

      // Use different local array array names
      attributes->AddArray(rank % 2 ? local.GetPointer() : common.GetPointer());
      attributes->AddArray(rank % 2 ? common.GetPointer() : local.GetPointer());
      attributes->SetActiveScalars("Common");
    }

    const int steps = temporal ? 3 : 1;
    vtkNew<vtkHDFWriter> writer;
    if (temporal)
    {
      vtkNew<vtkGenerateTimeSteps> timeSteps;
      timeSteps->SetInputDataObject(input);
      for (int step = 0; step < steps; ++step)
      {
        timeSteps->AddTimeStepValue(step);
      }
      writer->SetInputConnection(timeSteps->GetOutputPort());
    }
    else
    {
      writer->SetInputDataObject(input);
    }

    const std::string prefix = OutputPrefix(tempDir, external) +
      (polyData ? "array_names_pd" : "array_names_ug") + (temporal ? "_temporal" : "");
    const std::string filename = prefix + ".vtkhdf";
    writer->SetFileName(filename.c_str());
    writer->SetUseExternalDistributed(external == ExternalDistributed::Enabled);
    writer->Write();
    controller->Barrier();

    if (external == ExternalDistributed::Enabled)
    {
      // Read individual rank files
      vtkNew<vtkHDFReader> readerPart;
      const std::string partFilename = prefix + "_part" + vtk::to_string(rank) + ".vtkhdf";
      readerPart->SetFileName(partFilename.c_str());
      for (int step = 0; step < steps; ++step)
      {
        readerPart->SetStep(step);
        readerPart->UpdatePiece(0, 1, 0);
        auto* part = readerPart->GetOutputDataObject(0);
        part->GetFieldData()->RemoveArray("Time");
        if (!vtkTestUtilities::CompareDataObjects(input, part))
        {
          vtkLog(ERROR, "External array-name test part does not match its source");
          success = false;
        }
      }
    }

    vtkNew<vtkHDFReader> reader;
    reader->SetFileName(filename.c_str());
    for (int association : { vtkDataObject::POINT, vtkDataObject::CELL })
    {
      if (input->GetAttributes(association)->GetNumberOfArrays() != 2)
      {
        vtkLog(ERROR, "Array synchronization modified the source dataset");
        return false;
      }
      for (int sourceRank = 0; sourceRank < ranks; ++sourceRank)
      {
        if (sourceRank == rank)
        {
          continue;
        }

        // Add missing arrays to input for comparison to succeed
        vtkNew<vtkDoubleArray> missing;
        missing->SetName(("Rank" + vtk::to_string(sourceRank)).c_str());
        missing->SetNumberOfComponents(2);
        missing->SetNumberOfTuples(input->GetNumberOfElements(association));
        missing->Fill(0);
        input->GetAttributes(association)->AddArray(missing);
      }
    }
    reader->UpdateInformation();
    if (reader->GetNumberOfSteps() != steps)
    {
      vtkLog(ERROR, "Unexpected number of array-name test timesteps");
      success = false;
    }
    for (int step = 0; step < steps; ++step)
    {
      reader->SetStep(step);
      reader->UpdatePiece(rank, ranks, 0);
      auto* partitions = vtkPartitionedDataSet::SafeDownCast(reader->GetOutputDataObject(0));
      auto* actual = partitions ? partitions->GetPartition(rank) : nullptr;
      if (actual)
      {
        actual->GetFieldData()->RemoveArray("Time");
      }
      if (!actual || !vtkTestUtilities::CompareDataObjects(input, actual))
      {
        vtkLog(ERROR, "Read array-name test partition does not match its source");
        success = false;
      }
    }
  }
  return success;
}

//------------------------------------------------------------------------------
bool TestCompositeDistributedObject(vtkMPIController* controller, const std::string& tempDir,
  const vtkIdType compositeType, ExternalDistributed external)
{
  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  vtkSmartPointer<vtkGroupDataSetsFilter> group =
    vtkGroupDataSetsFilter::SafeDownCast(::SetupCompositePipeline(compositeType));
  group->UpdatePiece(myRank, nbRanks, 0);

  // Write it to disk
  std::string prefix =
    OutputPrefix(tempDir, external) + "parallel_composite_" + vtk::to_string(compositeType);
  std::string filePath = prefix + ".vtkhdf";

  vtkNew<vtkHDFWriter> writer;
  writer->SetInputConnection(group->GetOutputPort());
  writer->SetFileName(filePath.c_str());
  writer->SetUseExternalDistributed(external == ExternalDistributed::Enabled);
  writer->Write();

  // Wait for all processes to be done writing
  controller->Barrier();

  // Reopen file and compare it to the source
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(filePath.c_str());
  reader->UpdatePiece(myRank, nbRanks, 0);

  vtkNew<vtkHDFReader> readerPart;
  if (external == ExternalDistributed::Enabled)
  {
    const std::string filePathPart = prefix + "_part" + vtk::to_string(myRank) + ".vtkhdf";
    readerPart->SetFileName(filePathPart.c_str());
    readerPart->UpdatePiece(0, 1, 0);
  }

  if (compositeType == VTK_MULTIBLOCK_DATA_SET)
  {
    auto originalPiece = vtkMultiBlockDataSet::SafeDownCast(group->GetOutputDataObject(0));
    auto readTotal = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutputDataObject(0));

    vtkMultiPieceDataSet* ugMP = vtkMultiPieceDataSet::SafeDownCast(readTotal->GetBlock(0));
    vtkMultiPieceDataSet* pdMP = vtkMultiPieceDataSet::SafeDownCast(readTotal->GetBlock(1));
    vtkUnstructuredGrid* ugBlock = vtkUnstructuredGrid::SafeDownCast(ugMP->GetPartition(myRank));
    vtkPolyData* pdBlock = vtkPolyData::SafeDownCast(pdMP->GetPartition(myRank));

    if (!vtkTestUtilities::CompareDataObjects(originalPiece->GetBlock(0), ugBlock))
    {
      vtkLog(ERROR, "Read block 0 and read part do not match");
      return false;
    }
    if (!vtkTestUtilities::CompareDataObjects(originalPiece->GetBlock(1), pdBlock))
    {
      vtkLog(ERROR, "Read block 1 and read part do not match");
      return false;
    }

    if (external == ExternalDistributed::Enabled)
    {
      auto readPart = vtkMultiBlockDataSet::SafeDownCast(readerPart->GetOutputDataObject(0));
      if (!readPart || !vtkTestUtilities::CompareDataObjects(originalPiece, readPart) ||
        !vtkTestUtilities::CompareDataObjects(readPart->GetBlock(0), ugBlock) ||
        !vtkTestUtilities::CompareDataObjects(readPart->GetBlock(1), pdBlock))
      {
        vtkLog(ERROR, "External composite part does not match source or main-file blocks");
        return false;
      }
    }
  }
  else
  {
    auto originalPiece =
      vtkPartitionedDataSetCollection::SafeDownCast(group->GetOutputDataObject(0));
    auto readTotal = vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutputDataObject(0));

    vtkPartitionedDataSet* ugPD = readTotal->GetPartitionedDataSet(0);
    vtkPartitionedDataSet* pdPD = readTotal->GetPartitionedDataSet(1);
    vtkUnstructuredGrid* ugBlock = vtkUnstructuredGrid::SafeDownCast(ugPD->GetPartition(myRank));
    vtkPolyData* pdBlock = vtkPolyData::SafeDownCast(pdPD->GetPartition(myRank));

    if (!vtkTestUtilities::CompareDataObjects(
          originalPiece->GetPartitionedDataSet(0)->GetPartition(0), ugBlock))
    {
      vtkLog(ERROR, "Read block 0 and read part do not match");
      return false;
    }
    if (!vtkTestUtilities::CompareDataObjects(
          originalPiece->GetPartitionedDataSet(1)->GetPartition(0), pdBlock))
    {
      vtkLog(ERROR, "Read block 1 and read part do not match");
      return false;
    }

    if (external == ExternalDistributed::Enabled)
    {
      auto readPart =
        vtkPartitionedDataSetCollection::SafeDownCast(readerPart->GetOutputDataObject(0));
      vtkNew<vtkPartitionedDataSetCollection> expectedPart;
      expectedPart->ShallowCopy(originalPiece);
      if (readPart)
      {
        expectedPart->SetDataAssembly(readPart->GetDataAssembly());
      }
      if (!readPart || !vtkTestUtilities::CompareDataObjects(expectedPart, readPart) ||
        !vtkTestUtilities::CompareDataObjects(
          readPart->GetPartitionedDataSet(0)->GetPartition(0), ugBlock) ||
        !vtkTestUtilities::CompareDataObjects(
          readPart->GetPartitionedDataSet(1)->GetPartition(0), pdBlock))
      {
        vtkLog(ERROR, "External composite part does not match source or main-file partitions");
        return false;
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool TestDistributedTemporal(vtkMPIController* controller, const std::string& tempDir,
  const std::string& dataRoot, bool usePolyData, bool staticMesh, bool nullPart,
  ExternalDistributed external)
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
  std::string prefix = OutputPrefix(tempDir, external) + "parallel_time_cow" +
    (usePolyData ? "_PD" : "_UG") + (staticMesh ? "_static" : "") + (nullPart ? "_null" : "");
  std::string filePath = prefix + ".vtkhdf";
  std::string filePathPart = prefix + "_part" + vtk::to_string(myRank) + ".vtkhdf";

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
  writer->SetUseExternalDistributed(external == ExternalDistributed::Enabled);
  writer->SetFileName(filePath.c_str());
  writer->Write();

  // All processes have written their pieces to disk
  controller->Barrier();

  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(filePath.c_str());
  reader->UpdatePiece(myRank, nbRanks, 0);

  vtkNew<vtkHDFReader> readerPart;
  if (external == ExternalDistributed::Enabled)
  {
    readerPart->SetFileName(filePathPart.c_str());
    readerPart->UpdatePiece(0, 1, 0);
  }

  for (int time = 0; time < static_cast<int>(timeValues.size()); time++)
  {
    reader->SetStep(time);
    reader->UpdatePiece(myRank, nbRanks, 0);

    if (external == ExternalDistributed::Enabled)
    {
      readerPart->SetStep(time);
      readerPart->UpdatePiece(0, 1, 0);
    }
    vtkDataObject* expected = external == ExternalDistributed::Enabled
      ? readerPart->GetOutputDataObject(0)
      : writer->GetInputDataObject(0, 0);

    vtkPartitionedDataSet* readPartitionedPiece =
      vtkPartitionedDataSet::SafeDownCast(reader->GetOutputDataObject(0));
    if (usePolyData)
    {
      vtkPolyData* readPiece =
        vtkPolyData::SafeDownCast(readPartitionedPiece->GetPartition(myRank));
      vtkPolyData* readPart = vtkPolyData::SafeDownCast(expected);

      if (readPiece == nullptr || readPart == nullptr)
      {
        vtkLog(ERROR, "Piece should not be null");
        return false;
      }
    }
    else
    {
      vtkUnstructuredGrid* readPiece =
        vtkUnstructuredGrid::SafeDownCast(readPartitionedPiece->GetPartition(myRank));
      vtkUnstructuredGrid* readPart = vtkUnstructuredGrid::SafeDownCast(expected);

      if (readPiece == nullptr || readPart == nullptr)
      {
        vtkLog(ERROR, "Piece should not be null");
        return false;
      }
    }

    vtkDataObject* readPiece = readPartitionedPiece->GetPartition(myRank);
    vtkDataObject* readPart = expected;
    if (external == ExternalDistributed::Disabled)
    {
      readPiece->GetFieldData()->RemoveArray("Time");
      readPart->GetFieldData()->RemoveArray("Time");
    }

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

// ------------------------------------------------------------------------------
bool TestCompositeTemporalDistributedObject(vtkMPIController* controller,
  const std::string& tempDir, const vtkIdType compositeType, ExternalDistributed external)
{
  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  vtkSmartPointer<vtkGroupDataSetsFilter> group =
    vtkGroupDataSetsFilter::SafeDownCast(::SetupCompositePipeline(compositeType));
  group->UpdatePiece(myRank, nbRanks, 0);

  // Generate several time steps
  vtkNew<vtkGenerateTimeSteps> generateTimeSteps;
  const std::array timeValues{ 1.0, 3.0, 5.0 };
  for (const double& value : timeValues)
  {
    generateTimeSteps->AddTimeStepValue(value);
  }
  generateTimeSteps->SetInputConnection(group->GetOutputPort());

  // Generate a time-varying point field: use default ParaView weights
  vtkNew<vtkSpatioTemporalHarmonicsAttribute> harmonics;
  harmonics->AddHarmonic(1.0, 1.0, 0.6283, 0.6283, 0.6283, 0.0);
  harmonics->AddHarmonic(3.0, 1.0, 0.6283, 0.0, 0.0, 1.5708);
  harmonics->AddHarmonic(2.0, 2.0, 0.0, 0.6283, 0.0, 3.1416);
  harmonics->AddHarmonic(1.0, 3.0, 0.0, 0.0, 0.6283, 4.7124);
  harmonics->SetInputConnection(generateTimeSteps->GetOutputPort());

  // Write it to disk
  std::string prefix = OutputPrefix(tempDir, external) + "parallel_temporal_composite_" +
    vtk::to_string(compositeType);
  std::string filePath = prefix + ".vtkhdf";

  vtkNew<vtkHDFWriter> writer;
  writer->SetWriteAllTimeSteps(true);
  writer->SetFileName(filePath.c_str());
  writer->SetInputConnection(harmonics->GetOutputPort());
  writer->SetUseExternalDistributed(external == ExternalDistributed::Enabled);
  writer->Write();

  // All processes have written their pieces to disk
  controller->Barrier();

  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(filePath.c_str());

  vtkNew<vtkHDFReader> readerPart;
  if (external == ExternalDistributed::Enabled)
  {
    const std::string filePathPart = prefix + "_part" + vtk::to_string(myRank) + ".vtkhdf";
    readerPart->SetFileName(filePathPart.c_str());
  }

  for (int time = 0; time < static_cast<int>(timeValues.size()); time++)
  {
    reader->SetStep(time);
    reader->UpdatePiece(myRank, nbRanks, 0);

    if (external == ExternalDistributed::Enabled)
    {
      readerPart->SetStep(time);
      readerPart->UpdatePiece(0, 1, 0);
    }

    if (compositeType == VTK_MULTIBLOCK_DATA_SET)
    {
      harmonics->UpdateTimeStep(timeValues[time], myRank, nbRanks, 0);
      auto readPart = vtkMultiBlockDataSet::SafeDownCast(harmonics->GetOutputDataObject(0));
      auto readTotal = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutputDataObject(0));

      // Distributed Multi-block yields a vtkMultiPiece but the single part version does not,
      // So we need to decompose them to be able to compare.
      vtkMultiPieceDataSet* ugMP = vtkMultiPieceDataSet::SafeDownCast(readTotal->GetBlock(0));
      vtkUnstructuredGrid* ugBlock2 = vtkUnstructuredGrid::SafeDownCast(readPart->GetBlock(0));
      vtkMultiPieceDataSet* pdMP = vtkMultiPieceDataSet::SafeDownCast(readTotal->GetBlock(1));
      vtkUnstructuredGrid* ugBlock = vtkUnstructuredGrid::SafeDownCast(ugMP->GetPartition(myRank));
      vtkPolyData* pdBlock = vtkPolyData::SafeDownCast(pdMP->GetPartition(myRank));
      vtkPolyData* pdBlock2 = vtkPolyData::SafeDownCast(readPart->GetBlock(1));

      pdBlock2->GetFieldData()->RemoveArray("Time");
      ugBlock2->GetFieldData()->RemoveArray("Time");
      pdBlock->GetFieldData()->RemoveArray("Time");
      ugBlock->GetFieldData()->RemoveArray("Time");

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
      if (external == ExternalDistributed::Enabled)
      {
        auto part = vtkMultiBlockDataSet::SafeDownCast(readerPart->GetOutputDataObject(0));
        if (!part)
        {
          return false;
        }
        for (unsigned int block = 0; block < 2; ++block)
        {
          part->GetBlock(block)->GetFieldData()->RemoveArray("Time");
          if (!vtkTestUtilities::CompareDataObjects(
                readPart->GetBlock(block), part->GetBlock(block)))
          {
            vtkLog(ERROR, "External temporal composite part does not match source");
            return false;
          }
        }
      }
    }
    else
    {
      harmonics->UpdateTimeStep(timeValues[time], myRank, nbRanks, 0);
      auto readPart =
        vtkPartitionedDataSetCollection::SafeDownCast(harmonics->GetOutputDataObject(0));
      auto readTotal =
        vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutputDataObject(0));

      for (unsigned int block = 0; block < readPart->GetNumberOfPartitionedDataSets(); ++block)
      {
        readPart->GetPartitionedDataSet(block)->GetPartition(0)->GetFieldData()->RemoveArray(
          "Time");
        readTotal->GetPartitionedDataSet(block)->GetPartition(myRank)->GetFieldData()->RemoveArray(
          "Time");
      }

      if (!vtkTestUtilities::CompareDataObjects(readPart->GetPartitionedDataSet(0)->GetPartition(0),
            readTotal->GetPartitionedDataSet(0)->GetPartition(myRank)))
      {
        vtkLog(ERROR, "Original and read part do not match");
        return false;
      }
      if (!vtkTestUtilities::CompareDataObjects(readPart->GetPartitionedDataSet(1)->GetPartition(0),
            readTotal->GetPartitionedDataSet(1)->GetPartition(myRank)))
      {
        vtkLog(ERROR, "Original and read part do not match");
        return false;
      }
      if (external == ExternalDistributed::Enabled)
      {
        auto part =
          vtkPartitionedDataSetCollection::SafeDownCast(readerPart->GetOutputDataObject(0));
        if (!part)
        {
          return false;
        }
        for (unsigned int block = 0; block < readPart->GetNumberOfPartitionedDataSets(); ++block)
        {
          auto* leaf = part->GetPartitionedDataSet(block)->GetPartition(0);
          leaf->GetFieldData()->RemoveArray("Time");
          if (!vtkTestUtilities::CompareDataObjects(
                readPart->GetPartitionedDataSet(block)->GetPartition(0), leaf))
          {
            vtkLog(ERROR, "External temporal composite part does not match source");
            return false;
          }
        }
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool TestDistributedMultiBlockMissingBlocks(vtkMPIController* controller,
  const std::string& tempDir, const std::string& dataRoot, ExternalDistributed external)
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

  const std::string writtenFile =
    OutputPrefix(tempDir, external) + "distributed_missing_blocks.vtkhdf";
  vtkNew<vtkHDFWriter> writer;
  writer->SetUseExternalDistributed(external == ExternalDistributed::Enabled);
  writer->SetWriteAllTimeSteps(false);
  writer->SetFileName(writtenFile.c_str());
  writer->SetInputConnection(reader->GetOutputPort());
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
bool TestDistributedTemporalStaticMultiBlockMissingBlocks(vtkMPIController* controller,
  const std::string& tempDir, const std::string& dataRoot, ExternalDistributed external)
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

  forceStatic->Update();

  std::string writtenFile = OutputPrefix(tempDir, external) + fileExt.substr(1);
  vtkNew<vtkHDFWriter> writer;
  writer->SetUseExternalDistributed(external == ExternalDistributed::Enabled);
  writer->SetWriteAllTimeSteps(true);
  writer->SetFileName(writtenFile.c_str());
  writer->SetInputConnection(forceStatic->GetOutputPort());
  writer->Write();

  controller->Barrier();

  vtkNew<vtkHDFReader> readerHDF;
  readerHDF->SetFileName(writtenFile.c_str());
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
      vtkMultiPieceDataSet::SafeDownCast(outputData->GetBlock(0))->GetPartition(myRank))
                            ->GetMeshMTime();

    vtkLog(INFO, << "MeshMtime is " << outputMeshTime);
  }
  return true;
}

//------------------------------------------------------------------------------
bool TestDistributedPolyData(
  vtkMPIController* controller, const std::string& tempDir, ExternalDistributed external)
{
  return TestDistributedObject(controller, tempDir, VTK_POLY_DATA, external);
}

//------------------------------------------------------------------------------
bool TestDistributedUnstructuredGrid(
  vtkMPIController* controller, const std::string& tempDir, ExternalDistributed external)
{
  return TestDistributedObject(controller, tempDir, VTK_UNSTRUCTURED_GRID, external);
}

//------------------------------------------------------------------------------
bool TestDistributedMultiBlock(
  vtkMPIController* controller, const std::string& tempDir, ExternalDistributed external)
{
  return TestCompositeDistributedObject(controller, tempDir, VTK_MULTIBLOCK_DATA_SET, external);
}

//------------------------------------------------------------------------------
bool TestDistributedPartitionedDataSetCollection(
  vtkMPIController* controller, const std::string& tempDir, ExternalDistributed external)
{
  return TestCompositeDistributedObject(
    controller, tempDir, VTK_PARTITIONED_DATA_SET_COLLECTION, external);
}

//------------------------------------------------------------------------------
bool TestDistributedUnstructuredGridTemporal(vtkMPIController* controller,
  const std::string& tempDir, const std::string& dataRoot, ExternalDistributed external)
{
  return TestDistributedTemporal(controller, tempDir, dataRoot, false, false, false, external);
}

//------------------------------------------------------------------------------
bool TestDistributedUnstructuredGridTemporalStatic(vtkMPIController* controller,
  const std::string& tempDir, const std::string& dataRoot, ExternalDistributed external)
{
  return TestDistributedTemporal(controller, tempDir, dataRoot, false, true, false, external);
}

//------------------------------------------------------------------------------
bool TestDistributedUnstructuredGridTemporalNullPart(vtkMPIController* controller,
  const std::string& tempDir, const std::string& dataRoot, ExternalDistributed external)
{
  return TestDistributedTemporal(controller, tempDir, dataRoot, false, false, true, external);
}

//------------------------------------------------------------------------------
bool TestDistributedPolyDataTemporal(vtkMPIController* controller, const std::string& tempDir,
  const std::string& dataRoot, ExternalDistributed external)
{
  return TestDistributedTemporal(controller, tempDir, dataRoot, true, false, false, external);
}

//------------------------------------------------------------------------------
bool TestDistributedPolyDataTemporalStatic(vtkMPIController* controller, const std::string& tempDir,
  const std::string& dataRoot, ExternalDistributed external)
{
  return TestDistributedTemporal(controller, tempDir, dataRoot, true, true, false, external);
}

//------------------------------------------------------------------------------
bool Test4PartMultiBlock(vtkMPIController* controller, const std::string& tempDir,
  const std::string& dataRoot, ExternalDistributed external)
{
  // This test reads a multiblock with 4 partitions on 2 blocks of a multiblock on 3 MPI ranks.
  // Make sure that the writer handles properly unbalanced partition writing

  int myRank = controller->GetLocalProcessId();

  vtkNew<vtkHDFReader> baselineReader;
  baselineReader->SetPieceDistribution(vtkHDFReader::Block);
  const std::string filename = dataRoot + "/Data/vtkHDF/sph4_mb.vtkhdf";

  baselineReader->SetFileName(filename.c_str());
  baselineReader->UpdatePiece(myRank, 3, 0);
  vtkMultiBlockDataSet* mbBaseline =
    vtkMultiBlockDataSet::SafeDownCast(baselineReader->GetOutputDataObject(0));
  auto block0baseline = vtkPartitionedDataSet::SafeDownCast(mbBaseline->GetBlock(0));
  auto block1baseline = vtkPartitionedDataSet::SafeDownCast(mbBaseline->GetBlock(1));

  unsigned int expectedParts = 4;
  if (block0baseline->GetNumberOfPartitions() != expectedParts)
  {
    vtkErrorWithObjectMacro(nullptr,
      "Expected " << expectedParts << " partitions but got"
                  << block0baseline->GetNumberOfPartitions());
  }
  if (block1baseline->GetNumberOfPartitions() != expectedParts)
  {
    vtkErrorWithObjectMacro(nullptr,
      "Expected " << expectedParts << " partitions but got"
                  << block0baseline->GetNumberOfPartitions());
  }

  auto block0part = block0baseline->GetPartitionAsDataObject(myRank + 1);
  if (block0part->GetNumberOfElements(vtkDataObject::AttributeTypes::POINT) == 0)
  {
    vtkErrorWithObjectMacro(nullptr, "Expected non-nullptr partition #" << myRank + 1);
  }

  // Only block 0 has 2 parts. We read with block piece distribution, so this extra piece has id 1
  auto block0part1 = block0baseline->GetPartitionAsDataObject(1);
  if (myRank == 0 && block0part1->GetNumberOfElements(vtkDataObject::AttributeTypes::POINT) == 0)
  {
    vtkErrorWithObjectMacro(nullptr, "Expected non-nullptr partition #3 on rank 0");
  }

  std::string writtenFile = OutputPrefix(tempDir, external) + "sph4parts_mb.vtkhdf";
  vtkNew<vtkHDFWriter> writer;
  writer->SetUseExternalDistributed(external == ExternalDistributed::Enabled);
  writer->SetFileName(writtenFile.c_str());
  writer->SetInputConnection(baselineReader->GetOutputPort());
  writer->Write();

  controller->Barrier();

  vtkNew<vtkHDFReader> reader;
  reader->SetPieceDistribution(vtkHDFReader::Block);
  reader->SetFileName(writtenFile.c_str());
  reader->UpdatePiece(myRank, 3, 0);

  vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutputDataObject(0));

  if (!vtkTestUtilities::CompareDataObjects(mb, mbBaseline))
  {
    vtkErrorWithObjectMacro(nullptr, "Expected multiblocks to match");
  }
  return true;
}

//------------------------------------------------------------------------------
bool Test4PartMultiBlockTemporal(vtkMPIController* controller, const std::string& tempDir,
  const std::string& dataRoot, ExternalDistributed external)
{
  // This test reads a multiblock with 4 partitions on 2 blocks of a multiblock on 3 MPI ranks.
  // Make sure that the writer handles properly unbalanced partition writing

  int myRank = controller->GetLocalProcessId();

  vtkNew<vtkHDFReader> baselineReader;
  baselineReader->SetPieceDistribution(vtkHDFReader::Block);
  const std::string filename = dataRoot + "/Data/vtkHDF/sph4_mb.vtkhdf";

  baselineReader->SetFileName(filename.c_str());

  // Generate several time steps
  vtkNew<vtkGenerateTimeSteps> generateTimeSteps;
  const std::array timeValues{ 1.0, 3.0, 5.0 };
  for (const double& value : timeValues)
  {
    generateTimeSteps->AddTimeStepValue(value);
  }
  generateTimeSteps->SetInputConnection(baselineReader->GetOutputPort());

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
  warp->SetScaleFactor(0.1);
  warp->UpdatePiece(myRank, 3, 0);

  std::string writtenFile = OutputPrefix(tempDir, external) + "sph4parts_mb_temporal.vtkhdf";
  vtkNew<vtkHDFWriter> writer;
  writer->SetUseExternalDistributed(external == ExternalDistributed::Enabled);
  writer->SetFileName(writtenFile.c_str());
  writer->SetInputConnection(warp->GetOutputPort());
  writer->Write();

  controller->Barrier();

  vtkNew<vtkHDFReader> reader;
  reader->SetPieceDistribution(vtkHDFReader::Block);
  reader->SetFileName(writtenFile.c_str());

  for (size_t step = 0; step < timeValues.size(); step++)
  {
    warp->GetOutputInformation(0)->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeValues[step]);
    warp->Modified();
    warp->Update();

    reader->SetStep(step);
    reader->UpdatePiece(myRank, 3, 0);

    vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutputDataObject(0));
    vtkMultiBlockDataSet* mbBaseline =
      vtkMultiBlockDataSet::SafeDownCast(warp->GetOutputDataObject(0));

    if (!vtkTestUtilities::CompareDataObjects(mb, mbBaseline))
    {
      vtkErrorWithObjectMacro(nullptr, "Expected multiblocks to match for time step " << step);
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool TestDistributedTemporalPartitionedDataSetCollection(
  vtkMPIController* controller, const std::string& tempDir, ExternalDistributed external)
{
  return TestCompositeTemporalDistributedObject(
    controller, tempDir, VTK_PARTITIONED_DATA_SET_COLLECTION, external);
}

//------------------------------------------------------------------------------
bool TestDistributedTemporalMultiBlock(
  vtkMPIController* controller, const std::string& tempDir, ExternalDistributed external)
{
  return TestCompositeTemporalDistributedObject(
    controller, tempDir, VTK_MULTIBLOCK_DATA_SET, external);
}
}

int TestHDFWriterDistributed(int argc, char* argv[])
{
  // Initialize MPI Controller
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);

  std::string threadName = "rank #";
  threadName += vtk::to_string(controller->GetLocalProcessId());
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
  for (auto external : { ExternalDistributed::Disabled, ExternalDistributed::Enabled })
  {
    res &= ::TestDistributedArrayNames(controller, tempDir, external);
    res &= ::TestDistributedArrayNames(controller, tempDir, external, true);
    res &= ::TestDistributedPolyData(controller, tempDir, external);
    res &= ::TestDistributedUnstructuredGrid(controller, tempDir, external);
    res &= ::TestDistributedMultiBlock(controller, tempDir, external);
    res &= ::TestDistributedMultiBlockMissingBlocks(controller, tempDir, dataRoot, external);
    res &= ::TestDistributedPartitionedDataSetCollection(controller, tempDir, external);
    res &= ::TestDistributedUnstructuredGridTemporal(controller, tempDir, dataRoot, external);
    res &= ::TestDistributedUnstructuredGridTemporalStatic(controller, tempDir, dataRoot, external);
    res &=
      ::TestDistributedUnstructuredGridTemporalNullPart(controller, tempDir, dataRoot, external);
    res &= ::TestDistributedPolyDataTemporal(controller, tempDir, dataRoot, external);
    res &= ::TestDistributedPolyDataTemporalStatic(controller, tempDir, dataRoot, external);
    res &= ::TestDistributedTemporalMultiBlock(controller, tempDir, external);
    res &= ::TestDistributedTemporalStaticMultiBlockMissingBlocks(
      controller, tempDir, dataRoot, external);
    res &= ::Test4PartMultiBlock(controller, tempDir, dataRoot, external);
    res &= ::TestDistributedTemporalPartitionedDataSetCollection(controller, tempDir, external);
    res &= ::Test4PartMultiBlockTemporal(controller, tempDir, dataRoot, external);
  }
  controller->Finalize();
  return res ? EXIT_SUCCESS : EXIT_FAILURE;
}
