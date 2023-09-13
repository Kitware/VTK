/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestReducePartitions.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <string>

#include "vtkClipDataSet.h"
#include "vtkCompositeDataSet.h"
#include "vtkDIYAggregateDataSetFilter.h"
#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPlane.h"
#include "vtkPlaneCollection.h"
#include "vtkPolyData.h"
#include "vtkProcessGroup.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRedistributeDataSetFilter.h"
#include "vtkRedistributeDataSetToSubCommFilter.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLMultiBlockDataReader.h"
#include "vtkXMLPartitionedDataSetReader.h"
#include "vtkXMLPartitionedDataSetWriter.h"
#include <vtkGroupDataSetsFilter.h>
#include <vtk_mpi.h>

vtkMPIController* Controller = nullptr;

void LogMessage(const std::string& msg)
{
  if (Controller->GetLocalProcessId() == 0)
  {
    std::cout << msg << std::endl;
    std::cout.flush();
  }
}

////////////////////////////////////////////////////////////////////////////////
vtkPartitionedDataSet* TestDataAggregation(vtkProcessGroup* subGroup)
{
  const int nProcs = Controller->GetNumberOfProcesses();
  const int myRank = Controller->GetLocalProcessId();

  const int nTargetProcs = subGroup->GetNumberOfProcessIds();

  // allocate the output dataset
  vtkSmartPointer<vtkPartitionedDataSet> outputPDS = vtkPartitionedDataSet::New();
  outputPDS->Initialize();

  // initialize the partitioned dataset
  vtkSmartPointer<vtkPartitionedDataSet> pds = vtkPartitionedDataSet::New();

  vtkNew<vtkXMLPartitionedDataSetReader> pdsReader;
  pdsReader->SetFileName("/Users/c.wetterer-nelson/projects/Efficient-InSitu-IO/block1-pds.vtpd");
  pdsReader->UpdatePiece(myRank, nProcs, 0);
  for (int r = 0; r < nProcs; ++r)
  {
    if (myRank == r)
    {
      std::cout << "rank " << r << " has " << pdsReader->GetOutput()->GetNumberOfElements(0)
                << "\n";
    }
  }
  pds->ShallowCopy(vtkPartitionedDataSet::SafeDownCast(pdsReader->GetOutput()));

  vtkNew<vtkRedistributeDataSetToSubCommFilter> rdsc;
  rdsc->SetSubGroup(subGroup);
  rdsc->SetInputData(pds);
  rdsc->SetController(Controller);

  double t1, t2, elapsedTime;
  t1 = MPI_Wtime();
  rdsc->Update();
  t2 = MPI_Wtime();
  elapsedTime = t2 - t1;
  for (int r = 0; r < nProcs; ++r)
  {
    if (myRank == r)
    {
      std::cout << "rank " << r << " has " << rdsc->GetOutput()->GetNumberOfElements(0) << "\n";
    }
  }
  LogMessage("elapsed time: " + std::to_string(elapsedTime));
  return vtkPartitionedDataSet::SafeDownCast(rdsc->GetOutputDataObject(0));
}

/// @brief  create a very load imbalanced unstructured grid from clipping an image data across
/// partitions
/// @param subGroup: the subcommunicator to redistribute the unstructured grid dataset to
/// @return vtkPartitionedDataSet with data redistributed onto the subGroup
vtkUnstructuredGrid* TestAggregateUnstructuredGrid(vtkProcessGroup* subGroup)
{
  const int nProcs = Controller->GetNumberOfProcesses();
  const int myRank = Controller->GetLocalProcessId();

  const int nTargetProcs = subGroup->GetNumberOfProcessIds();

  // create a wavelet source
  vtkNew<vtkRTAnalyticSource> waveletSource;
  waveletSource->SetWholeExtent(0, 58, 0, 56, 0, 50);
  waveletSource->UpdatePiece(myRank, nProcs, 0);
  // print the number of vertices on each partition after clipping
  for (int r = 0; r < nProcs; ++r)
  {
    if (myRank == r)
    {
      std::cout << "WAVELET: rank " << r << " has "
                << waveletSource->GetOutput()->GetNumberOfElements(0) << "\n";
    }
  }
  // clip the corner off the box
  vtkNew<vtkClipDataSet> clipFilter;
  clipFilter->SetInputConnection(waveletSource->GetOutputPort());
  vtkNew<vtkPlane> plane;
  plane->SetOrigin(10, 10, 10);
  plane->SetNormal(-1.0, -1.0, -1.0);
  clipFilter->SetClipFunction(plane);
  clipFilter->UpdatePiece(myRank, nProcs, 0);

  // print the number of vertices on each partition after clipping
  for (int r = 0; r < nProcs; ++r)
  {
    if (myRank == r)
    {
      std::cout << "CLIPPED: rank " << r << " has "
                << clipFilter->GetOutput()->GetNumberOfElements(0) << "\n";
    }
  }
  // redistribute to sub group
  vtkNew<vtkRedistributeDataSetToSubCommFilter> rdsc;
  rdsc->SetSubGroup(subGroup);
  rdsc->SetInputData(clipFilter->GetOutput(0));
  rdsc->SetController(Controller);

  double t1, t2, elapsedTime;
  t1 = MPI_Wtime();
  rdsc->Update();
  t2 = MPI_Wtime();
  elapsedTime = t2 - t1;
  LogMessage("elapsed time: " + std::to_string(elapsedTime));
  for (int r = 0; r < nProcs; ++r)
  {
    if (myRank == r)
    {
      std::cout << "REPARTITIONED: rank " << r << " has "
                << rdsc->GetOutput()->GetNumberOfElements(0) << "\n";
    }
  }
  return vtkUnstructuredGrid::SafeDownCast(rdsc->GetOutputDataObject(0));
}
/// @brief  create a very load imbalanced unstructured grid from clipping an image data across
/// partitions
/// @param subGroup: the subcommunicator to redistribute the unstructured grid dataset to
/// @return vtkPartitionedDataSet with data redistributed onto the subGroup
vtkPartitionedDataSetCollection* TestAggregatePartitionedDatasetCollection(
  vtkProcessGroup* subGroup)
{
  const int nProcs = Controller->GetNumberOfProcesses();
  const int myRank = Controller->GetLocalProcessId();

  const int nTargetProcs = subGroup->GetNumberOfProcessIds();

  // create a wavelet source
  vtkNew<vtkRTAnalyticSource> waveletSource;
  waveletSource->SetWholeExtent(0, 58, 0, 56, 0, 50);
  waveletSource->UpdatePiece(myRank, nProcs, 0);
  // print the number of vertices on each partition after clipping
  for (int r = 0; r < nProcs; ++r)
  {
    if (myRank == r)
    {
      std::cout << "WAVELET: rank " << r << " has "
                << waveletSource->GetOutput()->GetNumberOfElements(0) << "\n";
    }
  }
  // clip the corner off the box
  vtkNew<vtkClipDataSet> clipFilter;
  clipFilter->SetInputConnection(waveletSource->GetOutputPort());
  vtkNew<vtkPlane> plane;
  plane->SetOrigin(10, 10, 10);
  plane->SetNormal(-1.0, -1.0, -1.0);
  clipFilter->SetClipFunction(plane);

  vtkNew<vtkGroupDataSetsFilter> groupFilter;
  groupFilter->SetOutputTypeToPartitionedDataSetCollection();
  groupFilter->SetInputConnection(clipFilter->GetOutputPort(0));
  groupFilter->UpdatePiece(myRank, nProcs, 0);
  // print the number of vertices on each partition after clipping
  for (int r = 0; r < nProcs; ++r)
  {
    if (myRank == r)
    {
      std::cout << "CLIPPED: rank " << r << " has "
                << groupFilter->GetOutput()->GetNumberOfElements(0) << "\n";
    }
  }
  // redistribute to sub group
  vtkNew<vtkRedistributeDataSetToSubCommFilter> rdsc;
  rdsc->SetSubGroup(subGroup);
  rdsc->SetInputData(groupFilter->GetOutput(0));
  rdsc->SetController(Controller);

  double t1, t2, elapsedTime;
  t1 = MPI_Wtime();
  rdsc->Update();
  t2 = MPI_Wtime();
  elapsedTime = t2 - t1;
  LogMessage("elapsed time: " + std::to_string(elapsedTime));
  for (int r = 0; r < nProcs; ++r)
  {
    if (myRank == r)
    {
      std::cout << "REPARTITIONED: rank " << r << " has "
                << rdsc->GetOutput()->GetNumberOfElements(0) << "\n";
    }
  }
  return vtkPartitionedDataSetCollection::SafeDownCast(rdsc->GetOutputDataObject(0));
}

int TestReducePartitions(int argc, char* argv[])
{
  // STEP 0: Initialize
  Controller = vtkMPIController::New();
  Controller->Initialize(&argc, &argv, 0);
  assert("pre: Controller should not be nullptr" && (Controller != nullptr));
  vtkMultiProcessController::SetGlobalController(Controller);
  LogMessage("Finished MPI Initialization!");

  int Rank = Controller->GetLocalProcessId();
  int NumberOfProcessors = Controller->GetNumberOfProcesses();
  assert("pre: NumberOfProcessors >= 1" && (NumberOfProcessors >= 1));
  assert("pre: Rank is out-of-bounds" && (Rank >= 0));

  // STEP 1: Run test where the number of partitions is equal to the number of
  // processes
  Controller->Barrier();
  LogMessage("Testing with same number of partitions as processes...");

  // create a vtkProcessGroup to represent the writer node
  int nTargetProcs = 4;
  vtkNew<vtkProcessGroup> subGroup;
  subGroup->Initialize(Controller);
  subGroup->RemoveAllProcessIds();
  for (int i = 0; i < nTargetProcs; ++i)
  {
    subGroup->AddProcessId(i);
  }

  vtkSmartPointer<vtkMPIController> subController = Controller->CreateSubController(subGroup);

  // TestDataAggregation(subGroup);

  auto* ret = TestAggregatePartitionedDatasetCollection(subGroup);
  // vtkNew<vtkXMLPartitionedDataSetReader> pdsReader;
  // pdsReader->SetFileName("/Users/c.wetterer-nelson/projects/Efficient-InSitu-IO/block1-pds.vtpd");
  // pdsReader->Update();
  // vtkPartitionedDataSet* pds = vtkPartitionedDataSet::SafeDownCast(pdsReader->GetOutput());
  // // loop over the blocks in the mesh and write them to a partitionedDataSet
  // vtkNew<vtkXMLPartitionedDataSetWriter> pdsWriter;
  //   std::string fname = "/Users/c.wetterer-nelson/projects/Efficient-InSitu-IO/clippedBox." +
  //   std::string(pdsWriter->GetDefaultFileExtension()); LogMessage("Writing converted-multiblock
  //   section to file: " + fname); pdsWriter->SetController(Controller);
  //   pdsWriter->SetFileName(fname.c_str());
  //   pdsWriter->SetInputDataObject(ret);
  //   pdsWriter->Update();
  // write the partitionedDataSet to disk

  // cleanup
  Controller->Finalize();
  Controller->Delete();
  return EXIT_SUCCESS;
}
