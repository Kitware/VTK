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

#include "vtkCellArray.h"
#include "vtkCharArray.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkExtractCells.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMergeCells.h"
#include "vtkMergePoints.h"
#include "vtkMPICommunicator.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include <map>
#include <set>
#include <vector>

//----------------------------------------------------------------------------
// Helpers
namespace
{
template<class T>
bool AllGatherV(vtkMultiProcessController* controller,
                const T* localV,
                vtkIdType localSize,
                std::vector<T>& globalV,
                std::vector<vtkIdType>& sizes,
                std::vector<vtkIdType>& offsets)
{
  int nbOfRanks = controller->GetNumberOfProcesses();
  sizes.resize(nbOfRanks);
  int ret = controller->AllGather(&localSize, &sizes[0], 1);
  if (ret == 0)
    {
    vtkErrorWithObjectMacro(controller, << "Communication error!");
    return false;
    }
  vtkIdType count = 0;
  offsets.resize(nbOfRanks);
  for (int i = 0; i < nbOfRanks; i++)
    {
    offsets[i] = count;
    count += sizes[i];
    }
  globalV.resize(count);
  if (count > 0)
    {
    controller->AllGatherV(localSize > 0 ? localV : 0,
      &globalV[0], localSize, &sizes[0], &offsets[0]);
    }
  return true;
}
}

//----------------------------------------------------------------------------
// Internal data structures

// Class to hold asynchronous communication information
class CommDataInfo
{
public:
  CommDataInfo() : SendLen(-1), RecvLen(-1), CommStep(0)
  {
    this->SendBuffer = vtkCharArray::New();
    this->RecvBuffer = vtkCharArray::New();
  }

  CommDataInfo(const CommDataInfo& c)
  {
    *this = c;
    if (this->SendBuffer) { this->SendBuffer->Register(0); }
    if (this->RecvBuffer) { this->RecvBuffer->Register(0); }
  }

  ~CommDataInfo()
  {
    if (this->SendBuffer) { this->SendBuffer->Delete(); }
    if (this->RecvBuffer) { this->RecvBuffer->Delete(); }
  }

  vtkMPICommunicator::Request SendReqs[2];
  vtkMPICommunicator::Request RecvReqs[2];
  vtkCharArray *SendBuffer;
  vtkCharArray *RecvBuffer;
  vtkIdType SendLen;
  vtkIdType RecvLen;
  int CommStep;
};

// Communication arrays
struct vtkPUnstructuredGridGhostCellsGenerator::vtkInternals
{
  // For global ids
  std::map<vtkIdType, vtkIdType> GlobalToLocalPointIdMap;
  std::vector<vtkIdType> AllGlobalIdsOfSurfacePoints;

  // For point coordinates
  vtkNew<vtkMergePoints> LocalPoints;
  std::vector<vtkIdType> LocalPointsMap;
  std::vector<double> AllPointsOfSurfacePoints;

  std::vector<vtkIdType> AllSizes;
  std::vector<vtkIdType> AllOffsets;
  std::map<int, std::set<vtkIdType> > NeighborRanksCells;
  std::map<int, CommDataInfo> CommData;
  vtkUnstructuredGridBase* Input;

  vtkDataArray* InputGlobalPointIds;
  bool UseGlobalPointIds;
};

static const int UGGCG_SIZE_EXCHANGE_TAG = 9000;
static const int UGGCG_DATA_EXCHANGE_TAG = 9001;
static const char* UGGCG_GLOBAL_POINT_IDS = "GlobalNodeIds";

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkPUnstructuredGridGhostCellsGenerator)
vtkSetObjectImplementationMacro(
  vtkPUnstructuredGridGhostCellsGenerator, Controller, vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkPUnstructuredGridGhostCellsGenerator::vtkPUnstructuredGridGhostCellsGenerator()
{
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->Internals = NULL;
  this->GlobalPointIdsArrayName = NULL;
  this->UseGlobalPointIds = true;
  this->SetGlobalPointIdsArrayName(UGGCG_GLOBAL_POINT_IDS);
}

//----------------------------------------------------------------------------
vtkPUnstructuredGridGhostCellsGenerator::~vtkPUnstructuredGridGhostCellsGenerator()
{
  this->SetController(NULL);
  this->SetGlobalPointIdsArrayName(NULL);

  delete this->Internals;
  this->Internals = 0;
}

//-----------------------------------------------------------------------------
void vtkPUnstructuredGridGhostCellsGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkPUnstructuredGridGhostCellsGenerator::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output. Input may just have the UnstructuredGridBase
  // interface, but output should be an unstructured grid.
  vtkUnstructuredGridBase *input = vtkUnstructuredGridBase::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int ghostLevels = 1;
  //ghostLevels = outInfo->Get(
  //  vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  if (!this->Controller)
    {
    this->Controller = vtkMultiProcessController::GetGlobalController();
    }
  output->Reset();

  this->NumRanks = this->Controller ? this->Controller->GetNumberOfProcesses() : 1;
  this->RankId = this->Controller ? this->Controller->GetLocalProcessId() : 0;
  if (ghostLevels == 0 || !this->Controller || this->NumRanks == 1)
    {
    // Ghost levels are not requested. Nothing to do but pass the dataset.
    if (this->RankId == 0)
      {
      vtkWarningMacro(<< "Ghost cells not requested or not needed.");
      }
    output->ShallowCopy(input);
    return 1;
    }

  delete this->Internals;
  this->Internals = new vtkPUnstructuredGridGhostCellsGenerator::vtkInternals();
  this->Internals->Input = input;

  vtkPointData *inputPD = input->GetPointData();
  this->Internals->InputGlobalPointIds = inputPD->GetGlobalIds();

  if (!this->Internals->InputGlobalPointIds)
    {
    this->Internals->InputGlobalPointIds =
      inputPD->GetArray(this->GlobalPointIdsArrayName);
    inputPD->SetGlobalIds(this->Internals->InputGlobalPointIds);
    }

  if (!this->UseGlobalPointIds)
    {
    this->Internals->InputGlobalPointIds = NULL;
    }

  this->ExtractAndReduceSurfacePoints();
  this->UpdateProgress(0.3);

  this->ComputeSharedPoints();
  this->UpdateProgress(0.6);

  this->ExtractAndSendGhostCells();
  this->UpdateProgress(0.8);

  this->ReceiveAndMergeGhostCells(output);
  this->UpdateProgress(1.0);

  output->GetInformation()->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 1);

  this->Controller->Barrier();

  delete this->Internals;
  this->Internals = 0;

  return 1;
}

//-----------------------------------------------------------------------------
// Step 1: Extract surface geometry and all reduce global ids of surface points
void vtkPUnstructuredGridGhostCellsGenerator::ExtractAndReduceSurfacePoints()
{
  // Extract boundary cells and points with the surface filter
  vtkNew<vtkDataSetSurfaceFilter> surfaceFilter;
  surfaceFilter->SetInputData(this->Internals->Input);
  surfaceFilter->PassThroughPointIdsOn();
  surfaceFilter->Update();

  vtkPolyData* surface = surfaceFilter->GetOutput();
  vtkIdType nbSurfacePoints = surface->GetNumberOfPoints();
  vtkCellArray* surfaceCells = surface->GetPolys();
  surfaceCells->InitTraversal();
  vtkIdType npts, *pts;

  vtkIdTypeArray* surfaceOriginalPointIds = vtkIdTypeArray::SafeDownCast(
    surface->GetPointData()->GetArray(surfaceFilter->GetOriginalPointIdsName()));

  if (this->Internals->InputGlobalPointIds)
    {
    std::vector<vtkIdType> globalIdsOfSurfacePoints;
    globalIdsOfSurfacePoints.reserve(nbSurfacePoints);

    // Browse surface cells and save global and local ids of cell points
    while (surfaceCells->GetNextCell(npts, pts))
      {
      for (vtkIdType i = 0; i < npts; i++)
        {
        vtkIdType origPtId = surfaceOriginalPointIds->GetValue(pts[i]);
        vtkIdType globalPtId = static_cast<vtkIdType>(
          this->Internals->InputGlobalPointIds->GetTuple1(origPtId));

        if (this->Internals->GlobalToLocalPointIdMap.find(globalPtId) ==
          this->Internals->GlobalToLocalPointIdMap.end())
          {
          this->Internals->GlobalToLocalPointIdMap[globalPtId] = origPtId;
          globalIdsOfSurfacePoints.push_back(globalPtId);
          }
        }
      }

    // Now reduce surface point global ids on ALL ranks
    ::AllGatherV(this->Controller, &globalIdsOfSurfacePoints[0],
      globalIdsOfSurfacePoints.size(),
      this->Internals->AllGlobalIdsOfSurfacePoints,
      this->Internals->AllSizes, this->Internals->AllOffsets);
    }
  else
    {
    // We can't use global ids, so we will process point coordinates instead
    vtkPoints* inputPoints = this->Internals->Input->GetPoints();
    vtkNew<vtkPoints> surfacePoints;
    surfacePoints->SetDataTypeToDouble();
    surfacePoints->Allocate(nbSurfacePoints);
    this->Internals->LocalPoints->InitPointInsertion(
      surfacePoints.Get(), surface->GetPoints()->GetBounds());
    this->Internals->LocalPointsMap.reserve(nbSurfacePoints);

    // Browse surface cells and push point coordinates to the locator
    while (surfaceCells->GetNextCell(npts, pts))
      {
      for (vtkIdType i = 0; i < npts; i++)
        {
        vtkIdType origPtId = surfaceOriginalPointIds->GetValue(pts[i]);
        double p[3];
        inputPoints->GetPoint(origPtId, p);
        vtkIdType sid;
        if (this->Internals->LocalPoints->InsertUniquePoint(p, sid))
          {
          // New point, save the id of the original grid point id associated
          // to this surface point
          if (static_cast<vtkIdType>(this->Internals->LocalPointsMap.size()) <= sid)
            {
            this->Internals->LocalPointsMap.resize(sid + 1);
            }
          this->Internals->LocalPointsMap[sid] = origPtId;
          }
        }
      }

    // Now reduce surface point coordinates on ALL ranks
    ::AllGatherV(this->Controller,
      (double*)surfacePoints->GetVoidPointer(0),
      surfacePoints->GetNumberOfPoints() * 3,
      this->Internals->AllPointsOfSurfacePoints,
      this->Internals->AllSizes, this->Internals->AllOffsets);
    }
}

//---------------------------------------------------------------------------
// Step 2: browse global ids/point coordinates of other ranks and check if some
// are duplicated locally.
// For each neighbor rank, save the ids of the cells adjacent to the surface
// points shared, those cells are the ghost cells we will send them.
void vtkPUnstructuredGridGhostCellsGenerator::ComputeSharedPoints()
{
  vtkNew<vtkIdList> cellIdsList;
  for (int i = 0; i < this->NumRanks; i++)
    {
    if (i == this->RankId)
      {
      continue;
      }
    for (vtkIdType j = 0, idx = this->Internals->AllOffsets[i];
         j < this->Internals->AllSizes[i]; j++, idx++)
      {
      vtkIdType localPointId = -1;
      if (this->Internals->InputGlobalPointIds)
        {
        // Check if this point exists locally from its global ids, if so
        // get its local id.
        vtkIdType gid = this->Internals->AllGlobalIdsOfSurfacePoints[idx];
        std::map<vtkIdType, vtkIdType>::iterator iter =
          this->Internals->GlobalToLocalPointIdMap.find(gid);
        if (iter != this->Internals->GlobalToLocalPointIdMap.end())
          {
          localPointId = iter->second;
          }
        }
      else
        {
        // Check if this point exists locally from its coordinates, if so
        // get its local id.
        double *p = &this->Internals->AllPointsOfSurfacePoints[idx];
        localPointId = this->Internals->LocalPoints->IsInsertedPoint(p);

        if (localPointId != -1)
          {
          localPointId = this->Internals->LocalPointsMap[localPointId];
          }
        idx += 2; // jump to next coordinates
        j += 2;
        }

      if (localPointId != -1)
        {
        // Current rank also has a copy of this global point
        cellIdsList->Reset();
        // Get the cells connected to this point
        this->Internals->Input->GetPointCells(localPointId, cellIdsList.Get());
        vtkIdType nbIds = cellIdsList->GetNumberOfIds();
        // Add those cells to the list of cells to send to this rank
        for (vtkIdType k = 0; k < nbIds; k++)
          {
          this->Internals->NeighborRanksCells[i].insert(cellIdsList->GetId(k));
          }
        }
      }
    }

  // Release memory of all reduced arrays
  this->Internals->AllGlobalIdsOfSurfacePoints.resize(0);
  this->Internals->AllPointsOfSurfacePoints.resize(0);
  this->Internals->AllSizes.resize(0);
  this->Internals->AllOffsets.resize(0);
  // Now we know our neighbors and which points we have in common and the
  // ghost cells to share.
}

//-----------------------------------------------------------------------------
// Step 3: extract and send the ghost cells to the neighbor ranks
void vtkPUnstructuredGridGhostCellsGenerator::ExtractAndSendGhostCells()
{
  vtkNew<vtkIdList> cellIdsList;
  vtkNew<vtkExtractCells> extractCells;
  extractCells->SetInputData(this->Internals->Input);
  std::map<int, std::set<vtkIdType> >::iterator iter =
    this->Internals->NeighborRanksCells.begin();

  vtkMPICommunicator* com =
    vtkMPICommunicator::SafeDownCast(this->Controller->GetCommunicator());

  // Browse all neighbor ranks and extract the cells connected to the points we share
  for (; iter != this->Internals->NeighborRanksCells.end(); ++iter)
    {
    int toRank = iter->first;
    std::set<vtkIdType>& cellsToShare = iter->second;
    cellIdsList->SetNumberOfIds(cellsToShare.size());
    std::set<vtkIdType>::iterator sIter = cellsToShare.begin();
    for (vtkIdType i = 0; sIter != cellsToShare.end(); ++sIter, i++)
      {
      cellIdsList->SetId(i, *sIter);
      }
    extractCells->SetCellList(cellIdsList.Get());
    extractCells->Update();
    vtkUnstructuredGrid* extractGrid = extractCells->GetOutput();

    // Send the extracted grid to the neighbor rank asynchronously
    CommDataInfo& c = this->Internals->CommData[toRank];
    if (vtkCommunicator::MarshalDataObject(extractGrid, c.SendBuffer))
      {
      c.SendLen = c.SendBuffer->GetNumberOfTuples();
      // Send data length
      com->NoBlockSend(&c.SendLen, 1, toRank, UGGCG_SIZE_EXCHANGE_TAG, c.SendReqs[0]);
      // Send raw data
      com->NoBlockSend((char*)c.SendBuffer->GetVoidPointer(0), c.SendLen, toRank,
        UGGCG_DATA_EXCHANGE_TAG, c.SendReqs[1]);
      }
    }
}

//-----------------------------------------------------------------------------
// Step 4: receive the ghost cells from the neighbor ranks and merge them
// to the local grid.
void vtkPUnstructuredGridGhostCellsGenerator::ReceiveAndMergeGhostCells(
  vtkUnstructuredGrid *output)
{
  // We need to compute a rough estimation of the total number of cells and
  // points for vtkMergeCells
  vtkIdType totalNbCells = this->Internals->Input->GetNumberOfCells();
  vtkIdType totalNbPoints = this->Internals->Input->GetNumberOfPoints();

  vtkMPICommunicator* com =
    vtkMPICommunicator::SafeDownCast(this->Controller->GetCommunicator());

  // Browse all neighbor ranks and receive the mesh that contains cells
  int nbNeighbors = static_cast<int>(this->Internals->NeighborRanksCells.size());
  std::vector<vtkUnstructuredGridBase*> neighborGrids;
  neighborGrids.reserve(nbNeighbors);

  // First create requests to receive the size of the mesh to receive
  std::map<int, std::set<vtkIdType> >::iterator iter;
  for (iter = this->Internals->NeighborRanksCells.begin();
    iter != this->Internals->NeighborRanksCells.end(); ++iter)
    {
    vtkIdType fromRank = iter->first;
    CommDataInfo& c = this->Internals->CommData[fromRank];
    com->NoBlockReceive(
      &c.RecvLen, 1, fromRank, UGGCG_SIZE_EXCHANGE_TAG, c.RecvReqs[0]);
    }

  // Then, once the data length is received, create requests to receive the mesh data
  int counter = 0;
  while (counter != nbNeighbors)
    {
    for (iter = this->Internals->NeighborRanksCells.begin();
      iter != this->Internals->NeighborRanksCells.end(); ++iter)
      {
      vtkIdType fromRank = iter->first;
      CommDataInfo& c = this->Internals->CommData[fromRank];
      if (!c.RecvReqs[0].Test() || c.CommStep != 0)
        {
        continue;
        }
      c.CommStep = 1;
      c.RecvBuffer->SetNumberOfValues(c.RecvLen);
      com->NoBlockReceive(
        (char*)c.RecvBuffer->GetVoidPointer(0), c.RecvLen, fromRank,
        UGGCG_DATA_EXCHANGE_TAG, c.RecvReqs[1]);
      counter++;
      }
    }

  // Browse all neighbor ranks and receive the mesh that contains cells
  // that are ghost cells for current rank.
  counter = 0;
  while (counter != nbNeighbors)
    {
    for (iter = this->Internals->NeighborRanksCells.begin();
    iter != this->Internals->NeighborRanksCells.end(); ++iter)
      {
      vtkIdType fromRank = iter->first;
      CommDataInfo& c = this->Internals->CommData[fromRank];

      if (!c.RecvReqs[1].Test() || c.CommStep != 1)
        {
        continue;
        }

      c.CommStep = 2;
      vtkUnstructuredGrid* grid = vtkUnstructuredGrid::New();
      vtkCommunicator::UnMarshalDataObject(c.RecvBuffer, grid);
      c.RecvBuffer->Delete();
      c.RecvBuffer = NULL;

      // Flag the received grid elements as ghosts
      grid->AllocatePointGhostArray();
      grid->AllocateCellGhostArray();
      grid->GetPointGhostArray()->FillComponent(0, 1);
      grid->GetCellGhostArray()->FillComponent(0, 1);

      // Make sure the global point ids array is tagged accordingly
      if (this->Internals->InputGlobalPointIds &&
          !grid->GetPointData()->GetGlobalIds())
        {
        grid->GetPointData()->SetGlobalIds(grid->GetPointData()->GetArray(
          this->Internals->InputGlobalPointIds->GetName()));
        }

      totalNbCells += grid->GetNumberOfCells();
      totalNbPoints += grid->GetNumberOfPoints();

      neighborGrids.push_back(grid);

      counter++;
      }
    }

  // Shallow copy the input grid and initialize the ghost arrays
  vtkNew<vtkUnstructuredGrid> inputCopy;
  inputCopy->ShallowCopy(this->Internals->Input);
  inputCopy->AllocatePointGhostArray();
  inputCopy->AllocateCellGhostArray();

  // MergeCells merge input + grids that contains ghost cells to the output grid
  vtkNew<vtkMergeCells> mergeCells;
  mergeCells->SetUnstructuredGrid(output);
  mergeCells->SetTotalNumberOfCells(totalNbCells);
  mergeCells->SetTotalNumberOfPoints(totalNbPoints);
  mergeCells->SetTotalNumberOfDataSets(
    1 + static_cast<int>(this->Internals->NeighborRanksCells.size()));
  mergeCells->SetUseGlobalIds(this->Internals->InputGlobalPointIds != 0 ? 1 : 0);
  mergeCells->SetPointMergeTolerance(0.0);

  // Merge input grid first
  mergeCells->MergeDataSet(inputCopy.Get());

  // Then merge ghost grid from neighbor rank
  for (std::size_t i = 0; i < neighborGrids.size(); i++)
    {
    mergeCells->MergeDataSet(neighborGrids[i]);
    neighborGrids[i]->Delete();
    }

  // Finalize the merged output
  mergeCells->Finish();
}
