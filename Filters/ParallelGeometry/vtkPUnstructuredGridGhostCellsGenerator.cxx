/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPUnstructuredGridGhostCellsGenerator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPUnstructuredGridGhostCellsGenerator.h"

#include "vtkAppendFilter.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkExtractCells.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkKdTree.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkMath.h"
#include "vtkMergeCells.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <map>
#include <set>
#include <vector>

//----------------------------------------------------------------------------
// Helpers
namespace
{
// Class to hold asynchronous communication information
class CommDataInfo
{
public:
  CommDataInfo()
    : SendLen(-1)
    , RecvLen(-1)
    , CommStep(0)
  {
    this->SendBuffer = vtkCharArray::New();
    this->RecvBuffer = vtkCharArray::New();
  }

  ~CommDataInfo()
  {
    if (this->SendBuffer)
    {
      this->SendBuffer->Delete();
    }
    if (this->RecvBuffer)
    {
      this->RecvBuffer->Delete();
    }
  }

  vtkMPICommunicator::Request SendReqs[2];
  vtkMPICommunicator::Request RecvReqs[2];
  vtkCharArray* SendBuffer;
  vtkCharArray* RecvBuffer;
  vtkIdType SendLen;
  vtkIdType RecvLen;
  int CommStep;
  int RecvSize;

private:
  CommDataInfo(const CommDataInfo&) = delete;
  void operator=(const CommDataInfo&) = delete;
};
} // end anonymous namespace

struct vtkPUnstructuredGridGhostCellsGenerator::vtkInternals
{
  // SubController only has MPI processes which have cells
  vtkMPIController* SubController;

  // For global ids
  std::map<vtkIdType, vtkIdType> GlobalToLocalPointIdMap;
  std::map<int, std::vector<vtkIdType> > ProcessIdToSurfacePointIds;
  // Ids to send to a specific process. Only the ids of points in the
  // receive process's bounding box are sent.
  std::map<int, std::vector<vtkIdType> > SendIds;

  // For point coordinates
  std::map<int, std::vector<double> > ProcessIdToSurfacePoints;
  vtkSmartPointer<vtkIdTypeArray> LocalPointsMap; // from surface id to 3d grid id
  // Points to send to a specific process. Only the points in the
  // receive process's bounding box are sent.
  std::map<int, std::vector<double> > SendPoints;
  vtkSmartPointer<vtkDataArray> MyPoints;

  std::map<int, CommDataInfo> CommData;
  vtkUnstructuredGridBase* Input;
  vtkSmartPointer<vtkUnstructuredGrid> CurrentGrid;

  vtkIdTypeArray* InputGlobalPointIds;
  bool UseGlobalPointIds;

  // cells that need to be sent to a given proc
  std::map<int, std::set<vtkIdType> > CellsToSend;

  // cells that have been sent to a given proc over the entire time.
  // used to make sure we only send a cell once to a destination process.
  std::map<int, std::set<vtkIdType> > SentCells;

  // cells that have been received from a given proc over the entire time.
  // stores global cell id. used this to make sure that we don't send
  // a cell back to a process that already sent it to this rank
  std::map<int, std::set<vtkIdType> > ReceivedCells;

  // mapping from global cell id to local cell id.
  // only stores cells which have been received (aka are ghost cells)
  std::map<vtkIdType, vtkIdType> GlobalToLocalCellIdMap;

  // cells that were sent to a proc during the last round,
  // a "round" is receiving one layer of ghost cells
  std::map<int, std::set<vtkIdType> > SentCellsLastRound;

  // list of processes which are probably my neighbors. this
  // is based on overlapping local bounding boxes so it is
  // not guaranteed that they really are sharing an interprocess boundary
  std::vector<int> Neighbors;
};

namespace
{
static const int UGGCG_SIZE_EXCHANGE_TAG = 9000;
static const int UGGCG_DATA_EXCHANGE_TAG = 9001;
}

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkPUnstructuredGridGhostCellsGenerator);
vtkSetObjectImplementationMacro(
  vtkPUnstructuredGridGhostCellsGenerator, Controller, vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkPUnstructuredGridGhostCellsGenerator::vtkPUnstructuredGridGhostCellsGenerator()
{
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
vtkPUnstructuredGridGhostCellsGenerator::~vtkPUnstructuredGridGhostCellsGenerator()
{
  this->SetController(nullptr);

  delete this->Internals;
  this->Internals = nullptr;
}

//-----------------------------------------------------------------------------
void vtkPUnstructuredGridGhostCellsGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkPUnstructuredGridGhostCellsGenerator::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output. Input may just have the UnstructuredGridBase
  // interface, but output should be an unstructured grid.
  vtkUnstructuredGridBase* input =
    vtkUnstructuredGridBase::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!input)
  {
    vtkErrorMacro("No input data!");
    return 0;
  }

  if (!this->Controller)
  {
    this->Controller = vtkMultiProcessController::GetGlobalController();
  }

  int reqGhostLevel =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
  int maxGhostLevel = this->BuildIfRequired
    ? reqGhostLevel
    : std::max(reqGhostLevel, this->MinimumNumberOfGhostLevels);

  if (maxGhostLevel == 0 || this->Controller->GetNumberOfProcesses() == 1)
  {
    vtkDebugMacro("Don't need ghost cells or only have a single process. Nothing more to do.");
    output->ShallowCopy(input);
    return 1;
  }

  // if only a single process has cells then we can skip ghost cell computations but
  // otherwise we need to do it from scratch since the ghost information coming in
  // may be wrong (it was for the vtkFiltersParallelCxx-MPI-ParallelConnectivity4 test)
  int needsGhosts = input->GetNumberOfCells() > 0 ? 1 : 0;

  int globalNeedsGhosts = 0;
  this->Controller->AllReduce(&needsGhosts, &globalNeedsGhosts, 1, vtkCommunicator::SUM_OP);
  if (globalNeedsGhosts < 2)
  {
    vtkDebugMacro("At most one process has cells. Nothing more to do.");
    output->ShallowCopy(input);
    return 1;
  }

  // determine which processes have any non-ghost cells and then create a subcontroller
  // for just them to use
  int hasCells = input->GetNumberOfCells() > 0 ? 1 : 0;
  if (hasCells && input->GetCellGhostArray() && input->GetCellGhostArray()->GetRange()[0] != 0)
  {
    hasCells = 0; // all the cells are ghost cells which we don't care about anymore
  }

  vtkSmartPointer<vtkMPIController> subController;
  subController.TakeReference(
    vtkMPIController::SafeDownCast(this->Controller)->PartitionController(hasCells, 0));

  if (hasCells == 0 || subController->GetNumberOfProcesses() < 2)
  {
    vtkDebugMacro("No work to do since at most one process has data");
    output->ShallowCopy(input);
    return 1;
  }

  vtkNew<vtkUnstructuredGrid> cleanedInput;
  vtkUnsignedCharArray* cellGhostArray = input->GetCellGhostArray();
  if (cellGhostArray == nullptr || cellGhostArray->GetValueRange()[1] == 0)
  {
    // we either have no ghost cells or do but there are no ghost entities so we just need
    // to remove those arrays and can skip modifying the data set itself
    cleanedInput->ShallowCopy(input);
  }
  else
  {
    cleanedInput->DeepCopy(input);
    cleanedInput->RemoveGhostCells();
  }
  cleanedInput->GetPointData()->RemoveArray(vtkDataSetAttributes::GhostArrayName());
  cleanedInput->GetCellData()->RemoveArray(vtkDataSetAttributes::GhostArrayName());
  input = nullptr; // nullify input to make sure we don't use it after this

  delete this->Internals;
  this->Internals = new vtkPUnstructuredGridGhostCellsGenerator::vtkInternals();
  this->Internals->SubController = subController;

  this->Internals->Input = cleanedInput;

  vtkPointData* inputPD = cleanedInput->GetPointData();
  this->Internals->InputGlobalPointIds = vtkIdTypeArray::FastDownCast(inputPD->GetGlobalIds());

  if (!this->Internals->InputGlobalPointIds)
  {
    inputPD = cleanedInput->GetPointData();
    this->Internals->InputGlobalPointIds =
      vtkIdTypeArray::FastDownCast(inputPD->GetArray(this->GlobalPointIdsArrayName));
    inputPD->SetGlobalIds(this->Internals->InputGlobalPointIds);
  }

  if (!this->UseGlobalPointIds)
  {
    this->Internals->InputGlobalPointIds = nullptr;
  }
  else
  {
    int useGlobalPointIds = this->Internals->InputGlobalPointIds != nullptr ? 1 : 0;
    int allUseGlobalPointIds;
    this->Internals->SubController->AllReduce(
      &useGlobalPointIds, &allUseGlobalPointIds, 1, vtkCommunicator::MIN_OP);
    if (!allUseGlobalPointIds)
    {
      this->Internals->InputGlobalPointIds = nullptr;
    }
  }

  // ensure that global cell ids array is there if specified.
  // only need global cell ids when more than one ghost layer is needed
  if (maxGhostLevel > 1)
  {
    if (this->HasGlobalCellIds)
    {
      vtkCellData* inputCD = cleanedInput->GetCellData();
      if (!inputCD->GetGlobalIds())
      {
        vtkDataArray* globalCellIdsArray = inputCD->GetArray(this->GlobalCellIdsArrayName);
        if (globalCellIdsArray == nullptr)
        {
          this->SetHasGlobalCellIds(false);
        }
        else
        {
          inputCD->SetGlobalIds(globalCellIdsArray);
        }
      }
      else
      {
        // make sure GlobalCellIdsArrayName is correct
        this->SetGlobalCellIdsArrayName(inputCD->GetGlobalIds()->GetName());
      }
    }

    // ensure that everyone has the same value of HasGlobalCellIds
    int hasGlobalCellIds = this->HasGlobalCellIds != 0 ? 1 : 0;
    int allHasGlobalCellIds;
    this->Internals->SubController->AllReduce(
      &hasGlobalCellIds, &allHasGlobalCellIds, 1, vtkCommunicator::MIN_OP);
    if (!allHasGlobalCellIds)
    {
      this->HasGlobalCellIds = false;
    }
  }

  // add global cell ids if necessary
  if (!this->HasGlobalCellIds && maxGhostLevel > 1)
  {
    this->AddGlobalCellIds();
  }

  // obtain first level of ghost cells
  this->Internals->CurrentGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  vtkTimerLog::MarkStartEvent("GetFirstGhostCellLayer");
  this->GetFirstGhostLayer(maxGhostLevel, this->Internals->CurrentGrid);
  vtkTimerLog::MarkEndEvent("GetFirstGhostCellLayer");

  // add additional ghost layers one at a time
  vtkTimerLog::MarkStartEvent("Get Extra Ghost Cell Layers");
  for (int i = 1; i < maxGhostLevel; i++)
  {
    this->AddGhostLayer(i + 1, maxGhostLevel);
  }
  vtkTimerLog::MarkEndEvent("Get Extra Ghost Cell Layers");

  // remove global cell ids if they were added internally
  if (!this->HasGlobalCellIds && maxGhostLevel > 1)
  {
    this->RemoveGlobalCellIds();
  }

  // set the output
  output->ShallowCopy(this->Internals->CurrentGrid);
  output->GetInformation()->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), maxGhostLevel);

  // copy field data
  if (cleanedInput && cleanedInput->GetFieldData())
  {
    vtkNew<vtkFieldData> fd;
    fd->ShallowCopy(cleanedInput->GetFieldData());
    output->SetFieldData(fd);
  }

  vtkDebugMacro("Produced " << maxGhostLevel << " ghost levels.");

  delete this->Internals;
  this->Internals = nullptr;
  return 1;
}

//-----------------------------------------------------------------------------
// Get the first layer of ghost cells
void vtkPUnstructuredGridGhostCellsGenerator::GetFirstGhostLayer(
  int maxGhostLevel, vtkUnstructuredGrid* output)
{
  std::vector<double> allBounds;

  vtkTimerLog::MarkStartEvent("ExchangeBoundsAndDetermineNeighbors");
  this->ExchangeBoundsAndDetermineNeighbors(allBounds);
  vtkTimerLog::MarkEndEvent("ExchangeBoundsAndDetermineNeighbors");

  vtkTimerLog::MarkStartEvent("ExtractAndReduceSurfacePointsShareData");
  this->ExtractAndReduceSurfacePointsShareData(allBounds);
  vtkTimerLog::MarkEndEvent("ExtractAndReduceSurfacePointsShareData");

  allBounds.clear();
  this->UpdateProgress(1.0 / (3.0 * maxGhostLevel));

  vtkTimerLog::MarkStartEvent("ComputeSharedPoints");
  this->ComputeSharedPoints();
  vtkTimerLog::MarkEndEvent("ComputeSharedPoints");

  this->UpdateProgress(2.0 / (3.0 * maxGhostLevel));

  vtkTimerLog::MarkStartEvent("ExtractAndSendGhostCells");
  this->ExtractAndSendGhostCells(this->Internals->Input);
  vtkTimerLog::MarkEndEvent("ExtractAndSendGhostCells");

  this->UpdateProgress(2.5 / (3.0 * maxGhostLevel));

  // Shallow copy the input grid and initialize the ghost arrays
  vtkNew<vtkUnstructuredGrid> inputCopy;
  inputCopy->ShallowCopy(this->Internals->Input);
  inputCopy->AllocatePointGhostArray();
  inputCopy->AllocateCellGhostArray();

  vtkTimerLog::MarkStartEvent("ReceiveAndMergeGhostCells");
  this->ReceiveAndMergeGhostCells(1, maxGhostLevel, inputCopy.Get(), output);
  vtkTimerLog::MarkEndEvent("ReceiveAndMergeGhostCells");

  this->UpdateProgress(1.0 / maxGhostLevel);
}

//-----------------------------------------------------------------------------
// Step 0: Exchange bounds, and determine your neighbors
void vtkPUnstructuredGridGhostCellsGenerator::ExchangeBoundsAndDetermineNeighbors(
  std::vector<double>& allBounds)
{
  // vtkVLogF(vtkLogger::VERBOSITY_INFO, "Exchange Bounds to Determine Neighbors");

  // increase bounds by a certain percentage to deal with precision stuff
  double epsilon = 0.01;

  double bounds[6];
  this->Internals->Input->GetBounds(bounds);

  // Resize allBounds to fit the 6 tuple from each mpi rank
  allBounds.resize(this->Internals->SubController->GetNumberOfProcesses() * 6);

  // everyone shares bounds
  vtkTimerLog::MarkStartEvent("AllGather 6tuple Bounds");
  this->Internals->SubController->AllGather(bounds, &allBounds[0], 6);
  vtkTimerLog::MarkEndEvent("AllGather 6tuple Bounds");

  double xlength = bounds[1] - bounds[0];
  double ylength = bounds[3] - bounds[2];
  double zlength = bounds[5] - bounds[4];

  double xmin = bounds[0] - xlength * epsilon;
  double xmax = bounds[1] + xlength * epsilon;
  double ymin = bounds[2] - ylength * epsilon;
  double ymax = bounds[3] + ylength * epsilon;
  double zmin = bounds[4] - zlength * epsilon;
  double zmax = bounds[5] + zlength * epsilon;

  // go through bounds, and find the ones which intersect my bounds,
  // which are my possible neighbors
  int rank = this->Internals->SubController->GetLocalProcessId();
  vtkTimerLog::MarkStartEvent("Calculate Neighbors Based on Bounds");
  for (int p = 0; p < this->Internals->SubController->GetNumberOfProcesses(); p++)
  {
    if (p == rank)
    {
      continue;
    }

    double xlength2 = allBounds[p * 6 + 1] - allBounds[p * 6 + 0];
    double xmin2 = allBounds[p * 6 + 0] - xlength2 * epsilon;
    double xmax2 = allBounds[p * 6 + 1] + xlength2 * epsilon;

    if (xmin <= xmax2 && xmax >= xmin2)
    {
      double ylength2 = allBounds[p * 6 + 3] - allBounds[p * 6 + 2];
      double ymin2 = allBounds[p * 6 + 2] - ylength2 * epsilon;
      double ymax2 = allBounds[p * 6 + 3] + ylength2 * epsilon;
      if (ymin <= ymax2 && ymax >= ymin2)
      {
        double zlength2 = allBounds[p * 6 + 5] - allBounds[p * 6 + 4];
        double zmin2 = allBounds[p * 6 + 4] - zlength2 * epsilon;
        double zmax2 = allBounds[p * 6 + 5] + zlength2 * epsilon;
        if (zmin <= zmax2 && zmax >= zmin2)
        {
          // this proc is a neighbor
          this->Internals->Neighbors.push_back(p);
        }
      }
    }
  }
  vtkTimerLog::MarkEndEvent("Calculate Neighbors Based on Bounds");
}

//-----------------------------------------------------------------------------
// Step 1a: Extract surface geometry and send to my neighbors. Receive my
// neighbor's surface points
void vtkPUnstructuredGridGhostCellsGenerator::ExtractAndReduceSurfacePointsShareData(
  std::vector<double>& allBounds)
{
  // Extract boundary cells and points with the surface filter
  vtkTimerLog::MarkStartEvent("Get Local Partition Surface Points");
  vtkNew<vtkDataSetSurfaceFilter> surfaceFilter;
  surfaceFilter->SetInputData(this->Internals->Input);
  surfaceFilter->PassThroughPointIdsOn();
  surfaceFilter->Update();
  vtkPolyData* surface = surfaceFilter->GetOutput();
  vtkIdType nbSurfacePoints = surface->GetNumberOfPoints();
  double bounds[6];
  surface->GetBounds(bounds);
  double delta[3] = { .0001 * (bounds[1] - bounds[0]), .0001 * (bounds[3] - bounds[2]),
    .0001 * (bounds[5] - bounds[4]) };

  vtkIdTypeArray* surfaceOriginalPointIds = vtkArrayDownCast<vtkIdTypeArray>(
    surface->GetPointData()->GetArray(surfaceFilter->GetOriginalPointIdsName()));
  vtkTimerLog::MarkEndEvent("Get Local Partition Surface Points");

  vtkTimerLog::MarkStartEvent("Share Local Partition Surface Points With Potential Neighbors");
  std::vector<vtkMPICommunicator::Request> sendReqs(this->Internals->Neighbors.size() * 2);

  // reset CommStep
  std::map<int, CommDataInfo>::iterator comIter = this->Internals->CommData.begin();
  for (; comIter != this->Internals->CommData.end(); ++comIter)
  {
    comIter->second.CommStep = 0;
  }

  // we need sizesToSend to stick around for the noblocksends
  std::vector<int> sizesToSend(this->Internals->Neighbors.size());
  if (this->Internals->InputGlobalPointIds)
  {
    // get all sizes from neighbors
    // first set up the receives
    std::vector<int>::iterator iter = this->Internals->Neighbors.begin();
    for (; iter != this->Internals->Neighbors.end(); ++iter)
    {
      CommDataInfo& c = this->Internals->CommData[*iter];
      this->Internals->SubController->NoBlockReceive(
        &c.RecvSize, 1, *iter, UGGCG_SIZE_EXCHANGE_TAG, c.RecvReqs[0]);
    }

    // store the global point id arrays unique to each process (based on bounding box
    // of the receiving process) to send
    this->Internals->ProcessIdToSurfacePointIds.clear();

    for (iter = this->Internals->Neighbors.begin(); iter != this->Internals->Neighbors.end();
         ++iter)
    {
      std::vector<vtkIdType>& sendIds = this->Internals->SendIds[*iter];
      sendIds.clear();
      for (vtkIdType i = 0; i < nbSurfacePoints; i++)
      {
        double coord[3];
        surface->GetPoint(i, coord);
        if (vtkMath::PointIsWithinBounds(coord, &allBounds[*iter * 6], delta))
        {
          vtkIdType origPtId = surfaceOriginalPointIds->GetValue(i);
          vtkIdType globalPtId = this->Internals->InputGlobalPointIds->GetTuple1(origPtId);
          this->Internals->GlobalToLocalPointIdMap[globalPtId] = origPtId;
          sendIds.push_back(globalPtId);
        }
      }
    }

    // send surface point ids to each neighbor
    int reqidx = 0;
    for (iter = this->Internals->Neighbors.begin(); iter != this->Internals->Neighbors.end();
         ++iter)
    {
      std::vector<vtkIdType>& sendIds = this->Internals->SendIds[*iter];
      // send size of vector
      sizesToSend[reqidx] = static_cast<int>(sendIds.size());
      this->Internals->SubController->NoBlockSend(
        &sizesToSend[reqidx], 1, *iter, UGGCG_SIZE_EXCHANGE_TAG, sendReqs[2 * reqidx]);

      // send the vector
      this->Internals->SubController->NoBlockSend(
        &sendIds[0], sizesToSend[reqidx], *iter, UGGCG_DATA_EXCHANGE_TAG, sendReqs[2 * reqidx + 1]);
      reqidx++;
    }

    // loop until all sizes are received
    size_t counter = 0;
    size_t numNeighbors = this->Internals->Neighbors.size();
    while (counter != numNeighbors)
    {
      iter = this->Internals->Neighbors.begin();
      for (; iter != this->Internals->Neighbors.end(); ++iter)
      {
        CommDataInfo& c = this->Internals->CommData[*iter];
        if (!c.RecvReqs[0].Test() || c.CommStep != 0)
        {
          continue;
        }
        c.CommStep = 1;
        counter++;
      }
    }

    // create receive requests for the ids
    iter = this->Internals->Neighbors.begin();
    for (; iter != this->Internals->Neighbors.end(); ++iter)
    {
      CommDataInfo& c = this->Internals->CommData[*iter];
      this->Internals->ProcessIdToSurfacePointIds[*iter].resize(c.RecvSize);
      this->Internals->SubController->NoBlockReceive(
        &this->Internals->ProcessIdToSurfacePointIds[*iter][0], c.RecvSize, *iter,
        UGGCG_DATA_EXCHANGE_TAG, c.RecvReqs[1]);
    }

    // wait for receives
    counter = 0;
    while (counter != numNeighbors)
    {
      iter = this->Internals->Neighbors.begin();
      for (; iter != this->Internals->Neighbors.end(); ++iter)
      {
        CommDataInfo& c = this->Internals->CommData[*iter];
        if (!c.RecvReqs[1].Test() || c.CommStep != 1)
        {
          continue;
        }
        c.CommStep = 2;
        counter++;
      }
    }
    // should have all id data by now
  }
  else
  {
    // We can't use global ids, so we will process point coordinates instead
    // send surface points to all neighbors
    // could potentially just send points that are in a neighbor's bounding box
    this->Internals->ProcessIdToSurfacePoints.clear();
    this->Internals->SendPoints.clear();
    vtkPoints* surfacePoints = surface->GetPoints();
    this->Internals->LocalPointsMap = surfaceOriginalPointIds;

    std::vector<int>::iterator iter = this->Internals->Neighbors.begin();
    // get all sizes from neighbors
    // first set up the receives
    for (; iter != this->Internals->Neighbors.end(); ++iter)
    {
      CommDataInfo& c = this->Internals->CommData[*iter];
      this->Internals->SubController->NoBlockReceive(
        &c.RecvSize, 1, *iter, UGGCG_SIZE_EXCHANGE_TAG, c.RecvReqs[0]);
    }

    // keep my own points
    this->Internals->MyPoints = surfacePoints->GetData();

    // store the global point arrays unique to each process (based on bounding box
    // of the receiving process) to send
    for (iter = this->Internals->Neighbors.begin(); iter != this->Internals->Neighbors.end();
         ++iter)
    {
      std::vector<double>& sendPoints = this->Internals->SendPoints[*iter];
      sendPoints.clear();
      for (vtkIdType i = 0; i < nbSurfacePoints; i++)
      {
        double coord[3];
        surface->GetPoint(i, coord);
        if (vtkMath::PointIsWithinBounds(coord, &allBounds[*iter * 6], delta))
        {
          sendPoints.insert(sendPoints.end(), coord, coord + 3);
        }
      }
    }

    // now go through and send the data
    int reqidx = 0;
    for (iter = this->Internals->Neighbors.begin(); iter != this->Internals->Neighbors.end();
         ++iter)
    {
      // Send data length
      std::vector<double>& sendPoints = this->Internals->SendPoints[*iter];
      sizesToSend[reqidx] = static_cast<int>(sendPoints.size());
      this->Internals->SubController->NoBlockSend(
        &sizesToSend[reqidx], 1, *iter, UGGCG_SIZE_EXCHANGE_TAG, sendReqs[2 * reqidx]);

      // Send raw data
      this->Internals->SubController->NoBlockSend(&sendPoints[0], sizesToSend[reqidx], *iter,
        UGGCG_DATA_EXCHANGE_TAG, sendReqs[2 * reqidx + 1]);
      reqidx++;
    }

    // loop until all sizes are received
    size_t counter = 0;
    size_t numNeighbors = this->Internals->Neighbors.size();
    while (counter != numNeighbors)
    {
      for (iter = this->Internals->Neighbors.begin(); iter != this->Internals->Neighbors.end();
           ++iter)
      {
        CommDataInfo& c = this->Internals->CommData[*iter];
        if (!c.RecvReqs[0].Test() || c.CommStep != 0)
        {
          continue;
        }
        c.CommStep = 1;
        counter++;
      }
    }

    // create receive requests for point data
    for (iter = this->Internals->Neighbors.begin(); iter != this->Internals->Neighbors.end();
         ++iter)
    {
      CommDataInfo& c = this->Internals->CommData[*iter];
      std::vector<double>& incomingPoints = this->Internals->ProcessIdToSurfacePoints[*iter];
      incomingPoints.resize(c.RecvSize);
      this->Internals->SubController->NoBlockReceive(
        &incomingPoints[0], c.RecvSize, *iter, UGGCG_DATA_EXCHANGE_TAG, c.RecvReqs[1]);
    }

    // wait for receives of data
    counter = 0;
    while (counter != numNeighbors)
    {
      iter = this->Internals->Neighbors.begin();
      for (; iter != this->Internals->Neighbors.end(); ++iter)
      {
        CommDataInfo& c = this->Internals->CommData[*iter];
        if (!c.RecvReqs[1].Test() || c.CommStep != 1)
        {
          continue;
        }
        c.CommStep = 2;
        counter++;
      }
    }
  }
  // should have all point data by now
  // wait for all my sends to complete
  this->Internals->SubController->WaitAll(static_cast<int>(sendReqs.size()), &sendReqs[0]);
  vtkTimerLog::MarkEndEvent("Share Local Partition Surface Points With Potential Neighbors");
}

//---------------------------------------------------------------------------
// Step 2a: browse global ids/point coordinates of other ranks and check if some
// are duplicated locally.
// For each neighbor rank, save the ids of the cells adjacent to the surface
// points shared, those cells are the ghost cells we will send them.
void vtkPUnstructuredGridGhostCellsGenerator::ComputeSharedPoints()
{
  this->Internals->CellsToSend.clear();
  vtkNew<vtkIdList> cellIdsList;
  if (this->Internals->InputGlobalPointIds)
  {
    for (std::vector<int>::iterator iter = this->Internals->Neighbors.begin();
         iter != this->Internals->Neighbors.end(); ++iter)
    {
      std::vector<vtkIdType>& surfaceIds = this->Internals->ProcessIdToSurfacePointIds[*iter];
      for (std::vector<vtkIdType>::const_iterator id_iter = surfaceIds.begin();
           id_iter != surfaceIds.end(); ++id_iter)
      {
        vtkIdType localPointId = -1;
        // Check if this point exists locally from its global ids, if so
        // get its local id.
        vtkIdType gid = *id_iter;
        std::map<vtkIdType, vtkIdType>::iterator miter =
          this->Internals->GlobalToLocalPointIdMap.find(gid);
        if (miter != this->Internals->GlobalToLocalPointIdMap.end())
        {
          localPointId = miter->second;
          if (localPointId != -1)
          {
            // Current rank also has a copy of this global point
            // Get the cells connected to this point
            this->Internals->Input->GetPointCells(localPointId, cellIdsList.Get());
            vtkIdType nbIds = cellIdsList->GetNumberOfIds();
            // Add those cells to the list of cells to send to this rank
            for (vtkIdType k = 0; k < nbIds; k++)
            {
              this->Internals->CellsToSend[*iter].insert(cellIdsList->GetId(k));
              this->Internals->SentCellsLastRound[*iter].insert(cellIdsList->GetId(k));
              this->Internals->SentCells[*iter].insert(cellIdsList->GetId(k));
            }
          }
        }
      }
    }
  }
  else
  {
    // build kdtree of local surface points
    vtkNew<vtkKdTree> kdtree;
    vtkNew<vtkPoints> points;
    int myRank = this->Internals->SubController->GetLocalProcessId();
    points->SetData(this->Internals->MyPoints);
    kdtree->BuildLocatorFromPoints(points);
    double bounds[6];
    kdtree->GetBounds(bounds);
    double tolerance = 1.e-6 *
      sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
        (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
        (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));

    for (std::map<int, std::vector<double> >::iterator iter =
           this->Internals->ProcessIdToSurfacePoints.begin();
         iter != this->Internals->ProcessIdToSurfacePoints.end(); ++iter)
    {
      if (iter->first == myRank)
      {
        continue;
      }
      std::vector<double>& offProcSurfacePoints = iter->second;
      double dist2(0); // result will be distance squared
      for (size_t i = 0; i < offProcSurfacePoints.size(); i += 3)
      {
        vtkIdType id =
          kdtree->FindClosestPointWithinRadius(tolerance, &offProcSurfacePoints[i], dist2);
        if (id != -1)
        { // matching point...
          vtkIdType inputId = this->Internals->LocalPointsMap->GetValue(id);
          this->Internals->Input->GetPointCells(inputId, cellIdsList);
          // Add those cells to the list of cells to send to this rank
          for (vtkIdType k = 0; k < cellIdsList->GetNumberOfIds(); k++)
          {
            this->Internals->CellsToSend[iter->first].insert(cellIdsList->GetId(k));
            this->Internals->SentCellsLastRound[iter->first].insert(cellIdsList->GetId(k));
            this->Internals->SentCells[iter->first].insert(cellIdsList->GetId(k));
          }
        }
      }
    }
  }

  // Release memory of all reduced arrays
  this->Internals->ProcessIdToSurfacePointIds.clear();
  this->Internals->ProcessIdToSurfacePoints.clear();
  this->Internals->LocalPointsMap = nullptr;
  this->Internals->SendIds.clear();
  this->Internals->MyPoints = nullptr;
  // Now we know our neighbors and which points we have in common and the
  // ghost cells to share.
}

//-----------------------------------------------------------------------------
// Step 3: extract and send the ghost cells to the neighbor ranks
void vtkPUnstructuredGridGhostCellsGenerator::ExtractAndSendGhostCells(
  vtkUnstructuredGridBase* input)
{
  vtkNew<vtkIdList> cellIdsList;
  vtkNew<vtkExtractCells> extractCells;
  extractCells->SetInputData(input);

  for (std::vector<int>::iterator iter = this->Internals->Neighbors.begin();
       iter != this->Internals->Neighbors.end(); ++iter)
  {
    int toRank = *iter;
    CommDataInfo& c = this->Internals->CommData[toRank];
    std::map<int, std::set<vtkIdType> >::iterator miter = this->Internals->CellsToSend.find(toRank);
    if (miter == this->Internals->CellsToSend.end())
    { // no data to send
      c.SendLen = 0;
      this->Internals->SubController->NoBlockSend(
        &c.SendLen, 1, toRank, UGGCG_SIZE_EXCHANGE_TAG, c.SendReqs[0]);
      continue;
    }
    std::set<vtkIdType>& cellsToShare = miter->second;
    cellIdsList->SetNumberOfIds(static_cast<vtkIdType>(cellsToShare.size()));
    std::set<vtkIdType>::iterator sIter = cellsToShare.begin();
    for (vtkIdType i = 0; sIter != cellsToShare.end(); ++sIter, i++)
    {
      cellIdsList->SetId(i, *sIter);
    }
    extractCells->SetCellList(cellIdsList);
    extractCells->Update();
    vtkUnstructuredGrid* extractGrid = extractCells->GetOutput();

    // There might be case where the originalcellids needs to be removed
    // but there are definitely cases where it shouldn't.
    // So if you run into that case, think twice before you uncomment this
    // next line and look carefully at paraview issue #18470
    // extractGrid->GetCellData()->RemoveArray("vtkOriginalCellIds");

    // Send the extracted grid to the neighbor rank asynchronously
    if (vtkCommunicator::MarshalDataObject(extractGrid, c.SendBuffer))
    {
      c.SendLen = c.SendBuffer->GetNumberOfTuples();
      // Send data length
      this->Internals->SubController->NoBlockSend(
        &c.SendLen, 1, toRank, UGGCG_SIZE_EXCHANGE_TAG, c.SendReqs[0]);

      // Send raw data
      this->Internals->SubController->NoBlockSend((char*)c.SendBuffer->GetVoidPointer(0), c.SendLen,
        toRank, UGGCG_DATA_EXCHANGE_TAG, c.SendReqs[1]);
    }
  }
}

//-----------------------------------------------------------------------------
// Step 4: Receive the ghost cells from the neighbor ranks and merge them
// to the local grid.
// Argument output should be an empty unstructured grid.
void vtkPUnstructuredGridGhostCellsGenerator::ReceiveAndMergeGhostCells(int ghostLevel,
  int maxGhostLevel, vtkUnstructuredGridBase* currentGrid, vtkUnstructuredGrid* output)
{
  // reset CommStep
  assert(this->Internals->Neighbors.size() == this->Internals->CommData.size());
  for (std::map<int, CommDataInfo>::iterator comIter = this->Internals->CommData.begin();
       comIter != this->Internals->CommData.end(); ++comIter)
  {
    comIter->second.CommStep = 0;
  }

  // We need to compute a rough estimation of the total number of cells and
  // points for vtkMergeCells
  vtkIdType totalNbCells = currentGrid->GetNumberOfCells();
  vtkIdType totalNbPoints = currentGrid->GetNumberOfPoints();

  // Browse all neighbor ranks and receive the mesh that contains cells
  size_t nbNeighbors = this->Internals->Neighbors.size();
  std::vector<vtkUnstructuredGridBase*> neighborGrids;
  neighborGrids.reserve(nbNeighbors);

  // First create requests to receive the size of the mesh to receive
  std::vector<int>::iterator iter = this->Internals->Neighbors.begin();
  for (; iter != this->Internals->Neighbors.end(); ++iter)
  {
    int fromRank = *iter;
    CommDataInfo& c = this->Internals->CommData[fromRank];
    this->Internals->SubController->NoBlockReceive(
      &c.RecvLen, 1, fromRank, UGGCG_SIZE_EXCHANGE_TAG, c.RecvReqs[0]);
  }

  // Then, once the data length is received, create requests to receive the
  // mesh data
  size_t counter = 0;
  size_t nonEmptyNeighborCounter = 0; // some neighbors might not have data to send
  while (counter != nbNeighbors)
  {
    iter = this->Internals->Neighbors.begin();
    for (; iter != this->Internals->Neighbors.end(); ++iter)
    {
      int fromRank = *iter;
      CommDataInfo& c = this->Internals->CommData[fromRank];
      if (!c.RecvReqs[0].Test() || c.CommStep != 0)
      {
        continue;
      }
      if (c.RecvLen > 0)
      {
        c.CommStep = 1; // mark that this comm needs to receive the dataset
        c.RecvBuffer->SetNumberOfValues(c.RecvLen);
        this->Internals->SubController->NoBlockReceive((char*)c.RecvBuffer->GetVoidPointer(0),
          c.RecvLen, fromRank, UGGCG_DATA_EXCHANGE_TAG, c.RecvReqs[1]);
        nonEmptyNeighborCounter++;
      }
      else
      {
        c.CommStep = 2; // mark that this comm doesn't need to receive the dataset
      }
      counter++;
    }
  }

  // Browse all neighbor ranks and receive the mesh that contains cells
  // that are ghost cells for current rank.
  counter = 0;
  while (counter != nonEmptyNeighborCounter)
  {
    iter = this->Internals->Neighbors.begin();
    for (; iter != this->Internals->Neighbors.end(); ++iter)
    {
      int fromRank = *iter;
      CommDataInfo& c = this->Internals->CommData[fromRank];

      if (!c.RecvReqs[1].Test() || c.CommStep != 1)
      {
        continue;
      }

      c.CommStep = 2;
      vtkUnstructuredGrid* grid = vtkUnstructuredGrid::New();
      vtkCommunicator::UnMarshalDataObject(c.RecvBuffer, grid);
      // clear out some memory...
      c.RecvBuffer->SetNumberOfTuples(0);

      if (!grid->HasAnyGhostCells())
      {
        grid->AllocatePointGhostArray();
        grid->AllocateCellGhostArray();
      }

      // Flag the received grid elements as ghosts
      grid->GetPointGhostArray()->FillComponent(0, 1);
      grid->GetCellGhostArray()->FillComponent(0, 1);

      // record all cells that i received
      // only needed if we need to calculate more ghost layers
      if (ghostLevel < maxGhostLevel)
      {
        if (grid->GetCellData()->GetGlobalIds())
        {
          vtkIdTypeArray* cellids =
            vtkArrayDownCast<vtkIdTypeArray>(grid->GetCellData()->GetGlobalIds());
          for (vtkIdType i = 0; i < grid->GetNumberOfCells(); i++)
          {
            this->Internals->ReceivedCells[fromRank].insert(cellids->GetValue(i));
          }
        }
      }

      // Make sure the global point ids array is tagged accordingly
      if (this->Internals->InputGlobalPointIds && !grid->GetPointData()->GetGlobalIds())
      {
        grid->GetPointData()->SetGlobalIds(
          grid->GetPointData()->GetArray(this->Internals->InputGlobalPointIds->GetName()));
      }

      // Checking maxGhostLevel to see if global cell ids are needed.
      // If so, make sure the global cell ids array is tagged accordingly
      if (maxGhostLevel > 1)
      {
        if (!grid->GetCellData()->GetGlobalIds())
        {
          grid->GetCellData()->SetGlobalIds(
            grid->GetCellData()->GetArray(this->GlobalCellIdsArrayName));
        }
      }

      totalNbCells += grid->GetNumberOfCells();
      totalNbPoints += grid->GetNumberOfPoints();

      neighborGrids.push_back(grid);

      counter++;
    }
  }

  if (totalNbCells == 0)
  {
    output->ShallowCopy(currentGrid);
    return;
  }

  // Use MergeCells to merge currentGrid + new grids to the output grid
  vtkTimerLog::MarkStartEvent("MergeCells");
  vtkNew<vtkMergeCells> mergeCells;
  mergeCells->SetUnstructuredGrid(output);
  mergeCells->SetTotalNumberOfCells(totalNbCells);
  mergeCells->SetTotalNumberOfPoints(totalNbPoints);
  mergeCells->SetTotalNumberOfDataSets(1 + static_cast<int>(neighborGrids.size()));
  mergeCells->SetUseGlobalIds(this->Internals->InputGlobalPointIds != nullptr ? 1 : 0);
  mergeCells->SetPointMergeTolerance(0.0);
  mergeCells->SetUseGlobalCellIds(1);

  // Merge current grid first
  mergeCells->MergeDataSet(currentGrid);

  // Then merge ghost grid from neighbor ranks
  for (std::size_t i = 0; i < neighborGrids.size(); i++)
  {
    if (neighborGrids[i]->GetNumberOfCells())
    {
      mergeCells->MergeDataSet(neighborGrids[i]);
    }
    neighborGrids[i]->Delete();
  }

  // Finalize the merged output
  mergeCells->Finish();
  vtkTimerLog::MarkEndEvent("MergeCells");
  // for all ghost cells, store the global cell id to local cell id mapping.
  // we need this mapping later when determining if cells we want to send
  // have been received before. only needed if we are calculating more
  // ghost layers.
  if (ghostLevel < maxGhostLevel)
  {
    vtkDataArray* ghost = output->GetCellGhostArray();
    vtkDataArray* gids = output->GetCellData()->GetGlobalIds();
    for (vtkIdType lid = 0; lid < output->GetNumberOfCells(); lid++)
    {
      if (ghost->GetTuple1(lid) > 0)
      {
        vtkIdType gid = static_cast<vtkIdType>(gids->GetTuple1(lid));
        if (this->Internals->GlobalToLocalCellIdMap.find(gid) ==
          this->Internals->GlobalToLocalCellIdMap.end())
        {
          this->Internals->GlobalToLocalCellIdMap[gid] = lid;
        }
      }
    }
  }

  // wait here on the sends to make sure we don't corrupt the data before it's fully sent
  counter = 0;
  while (counter != nbNeighbors)
  {
    for (iter = this->Internals->Neighbors.begin(); iter != this->Internals->Neighbors.end();
         ++iter)
    {
      int toRank = *iter;
      CommDataInfo& c = this->Internals->CommData[toRank];
      std::map<int, std::set<vtkIdType> >::iterator miter =
        this->Internals->CellsToSend.find(toRank);
      if (miter == this->Internals->CellsToSend.end())
      {
        // this is a process that we don't send cells to so we only need to check
        // that the message with the buffer size doesn't get modified
        if (c.CommStep == 3 || !c.SendReqs[0].Test())
        {
          continue;
        }
        c.CommStep = 3;
        counter++;
      }
      else
      {
        if (c.CommStep == 3 || !c.SendReqs[1].Test())
        {
          continue;
        }
        c.CommStep = 3;
        counter++;
        // clear out some memory...
        if (c.SendBuffer)
        {
          c.SendBuffer->SetNumberOfTuples(0);
        }
      }
    }
  }
}

//-----------------------------------------------------------------------------
// Add another ghost layer. Assumes that at least one layer of ghost cells has
// already been created. Must be called after GetFirstGhostLayer.
void vtkPUnstructuredGridGhostCellsGenerator::AddGhostLayer(int ghostLevel, int maxGhostLevel)
{
  this->Internals->CellsToSend.clear();
  this->FindGhostCells();
  this->UpdateProgress((1.0 + ((ghostLevel - 1) * 3.0)) / (maxGhostLevel * 3.0));

  this->ExtractAndSendGhostCells(this->Internals->CurrentGrid);
  this->UpdateProgress((2.0 + ((ghostLevel - 1) * 3.0)) / (maxGhostLevel * 3.0));
  vtkSmartPointer<vtkUnstructuredGrid> outputGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  this->ReceiveAndMergeGhostCells(
    ghostLevel, maxGhostLevel, this->Internals->CurrentGrid, outputGrid);
  this->UpdateProgress((3.0 + ((ghostLevel - 1) * 3.0)) / (maxGhostLevel * 3.0));

  this->Internals->CurrentGrid = outputGrid;
}

//-----------------------------------------------------------------------------
// Find all cells that need to be sent as the next layer of ghost cells.
// Examine all cells that were sent the last round, find all cells which
// share points with those sent cells. These cells are the new ghost layers.
void vtkPUnstructuredGridGhostCellsGenerator::FindGhostCells()
{
  vtkNew<vtkIdList> pointIdsList;
  vtkNew<vtkIdList> cellIdsList;
  vtkNew<vtkIdList> pointId;
  pointId->SetNumberOfIds(1);

  std::map<int, std::set<vtkIdType> >::iterator iter = this->Internals->SentCellsLastRound.begin();
  for (; iter != this->Internals->SentCellsLastRound.end(); ++iter)
  {
    // keep track of points which we've already visited for this proc
    // since the topological lookup and insertion process is expensive
    std::set<vtkIdType> visitedPointIds;
    int toRank = iter->first;
    std::set<vtkIdType>& cellids = this->Internals->SentCellsLastRound[toRank];
    std::set<vtkIdType>& cellsToSend = this->Internals->CellsToSend[toRank];
    // iterate over all cells sent to toRank
    for (std::set<vtkIdType>::iterator cellIdIter = cellids.begin(); cellIdIter != cellids.end();
         ++cellIdIter)
    {
      this->Internals->CurrentGrid->GetCellPoints(*cellIdIter, pointIdsList);
      for (int j = 0; j < pointIdsList->GetNumberOfIds(); j++)
      {
        if (visitedPointIds.insert(pointIdsList->GetId(j)).second == true)
        {
          pointId->SetId(0, pointIdsList->GetId(j));
          this->Internals->CurrentGrid->GetCellNeighbors(*cellIdIter, pointId, cellIdsList);
          // add cells to CellsToSend
          for (int i = 0; i < cellIdsList->GetNumberOfIds(); i++)
          {
            vtkIdType neighborCellId = cellIdsList->GetId(i);
            cellsToSend.insert(neighborCellId);
          }
        }
      }
    }

    // remove all cells that were already sent
    std::set<vtkIdType>& cellIds = this->Internals->SentCells[toRank];
    std::set<vtkIdType>::iterator sIter = cellIds.begin();
    for (; sIter != cellIds.end(); ++sIter)
    {
      this->Internals->CellsToSend[toRank].erase(*sIter);
    }

    // remove all cells that have been received before
    std::set<vtkIdType>& rcellids = this->Internals->ReceivedCells[toRank];
    std::set<vtkIdType>::iterator rIter = rcellids.begin();
    for (; rIter != rcellids.end(); ++rIter)
    {
      vtkIdType lid = this->Internals->GlobalToLocalCellIdMap[*rIter];
      this->Internals->CellsToSend[toRank].erase(lid);
    }
  }

  // add all new cells to SentCells, and update SentCellsLastRound to these new
  // cells
  this->Internals->SentCellsLastRound.clear();
  iter = this->Internals->CellsToSend.begin();
  for (; iter != this->Internals->CellsToSend.end(); ++iter)
  {
    int toRank = iter->first;
    std::set<vtkIdType>& cellids = this->Internals->CellsToSend[toRank];
    for (std::set<vtkIdType>::iterator cellIdIter = cellids.begin(); cellIdIter != cellids.end();
         ++cellIdIter)
    {
      this->Internals->SentCells[toRank].insert(*cellIdIter);
      this->Internals->SentCellsLastRound[toRank].insert(*cellIdIter);
    }
  }
}

//-----------------------------------------------------------------------------
// Add global cell ids
void vtkPUnstructuredGridGhostCellsGenerator::AddGlobalCellIds()
{
  // first figure out what to name the array,
  // if the array name is already taken, keep adding 1's to the name
  vtkCellData* celldata = this->Internals->Input->GetCellData();
  while (celldata->GetArray(this->GlobalCellIdsArrayName) != nullptr)
  {
    std::string s = this->GlobalCellIdsArrayName;
    s = s + "1";
    this->SetGlobalCellIdsArrayName(s.c_str());
  }

  // do an all-to-all to share the number of cells everyone has
  vtkIdType numCells = this->Internals->Input->GetNumberOfCells();
  std::vector<vtkIdType> allNumCells(this->Internals->SubController->GetNumberOfProcesses());
  this->Internals->SubController->AllGather(&numCells, &allNumCells[0], 1);

  // the value of global cell ids starts at the number of cells that ranks
  // before you have
  vtkIdType idStart = 0;
  int rank = this->Internals->SubController->GetLocalProcessId();
  for (int r = 0; r < rank; r++)
  {
    idStart += allNumCells[r];
  }

  // create an array to hold global cell ids
  vtkSmartPointer<vtkIdTypeArray> globalCellIds = vtkSmartPointer<vtkIdTypeArray>::New();
  globalCellIds->SetName(this->GlobalCellIdsArrayName);
  globalCellIds->SetNumberOfComponents(1);
  globalCellIds->SetNumberOfTuples(this->Internals->Input->GetNumberOfCells());
  for (vtkIdType i = 0; i < this->Internals->Input->GetNumberOfCells(); i++)
  {
    globalCellIds->SetTuple1(i, i + idStart);
  }

  celldata->SetGlobalIds(globalCellIds);
}

//-----------------------------------------------------------------------------
// Remove global cell ids
void vtkPUnstructuredGridGhostCellsGenerator::RemoveGlobalCellIds()
{
  vtkCellData* celldata = this->Internals->CurrentGrid->GetCellData();
  if (celldata->HasArray(this->GlobalCellIdsArrayName))
  {
    celldata->RemoveArray(this->GlobalCellIdsArrayName);
  }
}
