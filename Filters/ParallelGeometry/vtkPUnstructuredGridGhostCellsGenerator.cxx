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
#include "vtkCellData.h"

#include <algorithm>
#include <vector>
#include <map>
#include <set>

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
  std::map<int, CommDataInfo> CommData;
  vtkUnstructuredGridBase* Input;
  vtkSmartPointer <vtkUnstructuredGrid> CurGrid;

  vtkDataArray* InputGlobalPointIds;
  bool UseGlobalPointIds;

  // cells that need to be sent to a given proc
  std::map<int, std::set<vtkIdType> > CellsToSend;

  // cells that have been sent to a given proc over the entire time
  std::map<int, std::set<vtkIdType> > SentCells;

  // cells that were sent to a proc during the last round,
  // a "round" is receiving one layer of ghost cells
  std::map<int, std::set<vtkIdType> > SentCellsLastRound;
};

static const int UGGCG_SIZE_EXCHANGE_TAG = 9000;
static const int UGGCG_DATA_EXCHANGE_TAG = 9001;
static const char* UGGCG_GLOBAL_POINT_IDS = "GlobalNodeIds";
static const char* UGGCG_GLOBAL_CELL_IDS = "GlobalNodeIds";

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
  this->BuildIfRequired = true;
  this->MinimumNumberOfGhostLevels = 1;

  this->UseGlobalPointIds = true;
  this->GlobalPointIdsArrayName = NULL;
  this->SetGlobalPointIdsArrayName(UGGCG_GLOBAL_POINT_IDS);

  this->HasGlobalCellIds = false;
  this->GlobalCellIdsArrayName = NULL;
  this->SetGlobalCellIdsArrayName(UGGCG_GLOBAL_CELL_IDS);
}

//----------------------------------------------------------------------------
vtkPUnstructuredGridGhostCellsGenerator::~vtkPUnstructuredGridGhostCellsGenerator()
{
  this->SetController(NULL);
  this->SetGlobalPointIdsArrayName(NULL);
  this->SetGlobalCellIdsArrayName(NULL);

  delete this->Internals;
  this->Internals = 0;
}

//-----------------------------------------------------------------------------
void vtkPUnstructuredGridGhostCellsGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);

  os << indent << "UseGlobalPointIds:" << UseGlobalPointIds << endl;
  os << indent << "GlobalPointIdsArrayName:" << GlobalPointIdsArrayName << endl;
  os << indent << "HasGlobalCellIds:" << HasGlobalCellIds << endl;
  os << indent << "GlobalCellIdsArrayName:" << GlobalCellIdsArrayName << endl;
  os << indent << "BuildIfRequired:" << BuildIfRequired << endl;
  os << indent << "MinimumNumberOfGhostLevels: " << this->MinimumNumberOfGhostLevels << endl;
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

  if (!input)
  {
    vtkErrorMacro(<< "No input data!");
    return 0;
  }

  if (input->GetCellGhostArray())
  {
    vtkDebugMacro(<< "Ghost cells already exist in the input. Nothing more to do.");
    output->ShallowCopy(input);
    return 1;
  }

  if (!this->Controller)
  {
    this->Controller = vtkMultiProcessController::GetGlobalController();
  }
  this->NumRanks = this->Controller ? this->Controller->GetNumberOfProcesses() : 1;
  this->RankId = this->Controller ? this->Controller->GetLocalProcessId() : 0;

  int reqGhostLevel = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
  int maxGhostLevel = this->BuildIfRequired ?
    reqGhostLevel : std::max(reqGhostLevel, this->MinimumNumberOfGhostLevels);

  if (maxGhostLevel == 0 || !this->Controller || this->NumRanks == 1)
  {
    vtkDebugMacro(<< "Ghost levels are not requested. Nothing more to do.");
    output->ShallowCopy(input);
    return 1;
  }

  delete this->Internals;
  this->Internals = new vtkPUnstructuredGridGhostCellsGenerator::vtkInternals();
  this->Internals->Input = input;

  vtkPointData *inputPD = input->GetPointData();
  this->Internals->InputGlobalPointIds = inputPD->GetGlobalIds();
  vtkUnstructuredGridBase *inputGridCopy = NULL;

  if (!this->Internals->InputGlobalPointIds)
  {
    inputGridCopy = input->NewInstance();
    inputGridCopy->ShallowCopy(input);
    this->Internals->Input = inputGridCopy;
    inputPD = inputGridCopy->GetPointData();
    this->Internals->InputGlobalPointIds =
      inputPD->GetArray(this->GlobalPointIdsArrayName);
    inputPD->SetGlobalIds(this->Internals->InputGlobalPointIds);
  }

  if (!this->UseGlobalPointIds)
  {
    this->Internals->InputGlobalPointIds = NULL;
  }
  int useGlobalPointIds = this->Internals->InputGlobalPointIds != 0 ? 1 : 0;
  int allUseGlobalPointIds;
  this->Controller->AllReduce(&useGlobalPointIds, &allUseGlobalPointIds, 1, vtkCommunicator::MIN_OP);
  if (!allUseGlobalPointIds)
  {
    this->Internals->InputGlobalPointIds = NULL;
  }

  // ensure that global cell ids array is there if specified.
  // only need global cell ids when more than one ghost layer is needed
  if (maxGhostLevel > 1)
  {
    if (this->HasGlobalCellIds)
    {
      vtkCellData *inputCD = input->GetCellData();
      if (!inputCD->GetGlobalIds())
      {
        vtkDataArray *globalCellIdsArray = inputCD->GetArray(
                                                this->GlobalCellIdsArrayName);
        if (globalCellIdsArray == NULL)
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
    this->Controller->AllReduce(&hasGlobalCellIds, &allHasGlobalCellIds, 1,
      vtkCommunicator::MIN_OP);
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
  this->Internals->CurGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  this->GetFirstGhostLayer(maxGhostLevel, this->Internals->CurGrid);

  // add additional ghost layers one at a time
  for (int i=1; i<maxGhostLevel; i++)
  {
    this->Controller->Barrier();
    this->AddGhostLayer(i+1, maxGhostLevel);
  }

  // remove global cell ids if they were added internally
  if (!this->HasGlobalCellIds && maxGhostLevel > 1)
  {
    this->RemoveGlobalCellIds();
  }

  // set the output
  output->ShallowCopy(this->Internals->CurGrid);
  output->GetInformation()->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(),
    maxGhostLevel);

  this->Controller->Barrier();

  delete this->Internals;
  this->Internals = NULL;
  if (inputGridCopy)
  {
    inputGridCopy->Delete();
  }

  return 1;
}

//-----------------------------------------------------------------------------
//Get the first layer of ghost cells
void vtkPUnstructuredGridGhostCellsGenerator::GetFirstGhostLayer(
  int maxGhostLevel, vtkUnstructuredGrid *output)
{
  this->ExtractAndReduceSurfacePoints();
  this->UpdateProgress(1.0 / (3.0 * maxGhostLevel));

  this->ComputeSharedPoints();
  this->UpdateProgress(2.0 / (3.0 * maxGhostLevel));

  this->ExtractAndSendGhostCells(this->Internals->Input);
  this->UpdateProgress(2.5 / (3.0 * maxGhostLevel));

  // Shallow copy the input grid and initialize the ghost arrays
  vtkNew<vtkUnstructuredGrid> inputCopy;
  inputCopy->ShallowCopy(this->Internals->Input);
  inputCopy->AllocatePointGhostArray();
  inputCopy->AllocateCellGhostArray();
  this->ReceiveAndMergeGhostCells(maxGhostLevel, inputCopy.Get(), output);

  this->UpdateProgress(3.0 / (3.0 * maxGhostLevel));
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

  vtkPolyData *surface = surfaceFilter->GetOutput();
  vtkIdType nbSurfacePoints = surface->GetNumberOfPoints();
  vtkCellArray *surfaceCells = surface->GetPolys();
  surfaceCells->InitTraversal();
  vtkIdType npts, *pts;

  vtkIdTypeArray *surfaceOriginalPointIds = vtkArrayDownCast<vtkIdTypeArray>(
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
      static_cast<vtkIdType>(globalIdsOfSurfacePoints.size()),
      this->Internals->AllGlobalIdsOfSurfacePoints,
      this->Internals->AllSizes, this->Internals->AllOffsets);
  }
  else
  {
    // We can't use global ids, so we will process point coordinates instead
    vtkPoints *inputPoints = this->Internals->Input->GetPoints();
    vtkNew<vtkPoints> surfacePoints;
    surfacePoints->SetDataTypeToDouble();
    surfacePoints->Allocate(nbSurfacePoints);
    double bounds[6] = { 0., 1., 0., 1., 0., 1. };
    if (surface->GetPoints())
    {
      surface->GetPoints()->GetBounds(bounds);
    }
    this->Internals->LocalPoints->InitPointInsertion(
      surfacePoints.Get(), bounds);
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
          this->Internals->CellsToSend[i].insert(cellIdsList->GetId(k));
          this->Internals->SentCellsLastRound[i].insert(cellIdsList->GetId(k));
          this->Internals->SentCells[i].insert(cellIdsList->GetId(k));
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
void vtkPUnstructuredGridGhostCellsGenerator::ExtractAndSendGhostCells(
  vtkUnstructuredGridBase* input)
{
  vtkNew<vtkIdList> cellIdsList;
  vtkNew<vtkExtractCells> extractCells;
  extractCells->SetInputData(input);

  vtkMPICommunicator *com =
    vtkMPICommunicator::SafeDownCast(this->Controller->GetCommunicator());

  std::map<int, std::set<vtkIdType> >::iterator iter = this->Internals->CellsToSend.begin();
  for (; iter != this->Internals->CellsToSend.end(); ++iter)
  {
    int toRank = iter->first;
    std::set<vtkIdType>& cellsToShare = iter->second;
    cellIdsList->SetNumberOfIds(static_cast<vtkIdType>(cellsToShare.size()));
    std::set<vtkIdType>::iterator sIter = cellsToShare.begin();
    for (vtkIdType i = 0; sIter != cellsToShare.end(); ++sIter, i++)
    {
      cellIdsList->SetId(i, *sIter);
    }
    extractCells->SetCellList(cellIdsList.Get());
    extractCells->Update();
    vtkUnstructuredGrid *extractGrid = extractCells->GetOutput();

    // Send the extracted grid to the neighbor rank asynchronously
    CommDataInfo& c = this->Internals->CommData[toRank];
    if (vtkCommunicator::MarshalDataObject(extractGrid, c.SendBuffer))
    {
      c.SendLen = c.SendBuffer->GetNumberOfTuples();
      // Send data length
      com->NoBlockSend(&c.SendLen, 1, toRank, UGGCG_SIZE_EXCHANGE_TAG,
                       c.SendReqs[0]);

      // Send raw data
      com->NoBlockSend((char*)c.SendBuffer->GetVoidPointer(0), c.SendLen,
                       toRank, UGGCG_DATA_EXCHANGE_TAG, c.SendReqs[1]);
    }
  }
}

//-----------------------------------------------------------------------------
// Step 4: Receive the ghost cells from the neighbor ranks and merge them
// to the local grid.
// Argument output should be an empty unstructured grid.
void vtkPUnstructuredGridGhostCellsGenerator::ReceiveAndMergeGhostCells(
  int maxGhostLevel, vtkUnstructuredGridBase *curGrid,
  vtkUnstructuredGrid *output)
{
  // reset CommStep
  std::map<int, CommDataInfo >::iterator comIter =
    this->Internals->CommData.begin();
  for (; comIter != this->Internals->CommData.end(); ++comIter)
  {
    comIter->second.CommStep = 0;
  }

  vtkMPICommunicator *com =
    vtkMPICommunicator::SafeDownCast(this->Controller->GetCommunicator());

  // We need to compute a rough estimation of the total number of cells and
  // points for vtkMergeCells
  vtkIdType totalNbCells = curGrid->GetNumberOfCells();
  vtkIdType totalNbPoints = curGrid->GetNumberOfPoints();

  // Browse all neighbor ranks and receive the mesh that contains cells
  int nbNeighbors = static_cast<int>(this->Internals->CellsToSend.size());
  std::vector<vtkUnstructuredGridBase*> neighborGrids;
  neighborGrids.reserve(nbNeighbors);

  // First create requests to receive the size of the mesh to receive
  std::map<int, std::set<vtkIdType> >::iterator iter = this->Internals->CellsToSend.begin();
  for (; iter != this->Internals->CellsToSend.end(); ++iter)
  {
    vtkIdType fromRank = iter->first;
    CommDataInfo& c = this->Internals->CommData[fromRank];
    com->NoBlockReceive(
      &c.RecvLen, 1, fromRank, UGGCG_SIZE_EXCHANGE_TAG, c.RecvReqs[0]);
  }

  // Then, once the data length is received, create requests to receive the
  // mesh data
  int counter = 0;
  while (counter != nbNeighbors)
  {
    iter = this->Internals->CellsToSend.begin();
    for (; iter != this->Internals->CellsToSend.end(); ++iter)
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
    iter = this->Internals->CellsToSend.begin();
    for (; iter != this->Internals->CellsToSend.end(); ++iter)
    {
      vtkIdType fromRank = iter->first;
      CommDataInfo& c = this->Internals->CommData[fromRank];

      if (!c.RecvReqs[1].Test() || c.CommStep != 1)
      {
        continue;
      }

      c.CommStep = 2;
      vtkUnstructuredGrid *grid = vtkUnstructuredGrid::New();
      vtkCommunicator::UnMarshalDataObject(c.RecvBuffer, grid);

      // delete RecvBuffer, and allocate empty arrays
      // just in case they are used again.
      c.RecvBuffer->Delete();
      c.RecvBuffer = vtkCharArray::New();

      if (!grid->HasAnyGhostCells())
      {
        grid->AllocatePointGhostArray();
        grid->AllocateCellGhostArray();
      }

      // Flag the received grid elements as ghosts
      grid->GetPointGhostArray()->FillComponent(0, 1);
      grid->GetCellGhostArray()->FillComponent(0, 1);

      // Make sure the global point ids array is tagged accordingly
      if (this->Internals->InputGlobalPointIds &&
          !grid->GetPointData()->GetGlobalIds())
      {
        grid->GetPointData()->SetGlobalIds(grid->GetPointData()->GetArray(
          this->Internals->InputGlobalPointIds->GetName()));
      }

      // Checking maxGhostLevel to see if global cell ids are needed.
      // If so, make sure the global cell ids array is tagged accordingly
      if (maxGhostLevel > 1)
      {
          if (!grid->GetCellData()->GetGlobalIds())
          {
            grid->GetCellData()->SetGlobalIds(grid->GetCellData()->GetArray(
              this->GlobalCellIdsArrayName));
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
    output->ShallowCopy(curGrid);
    return;
  }

  // Use MergeCells to merge curGrid + new grids to the output grid
  vtkNew<vtkMergeCells> mergeCells;
  mergeCells->SetUnstructuredGrid(output);
  mergeCells->SetTotalNumberOfCells(totalNbCells);
  mergeCells->SetTotalNumberOfPoints(totalNbPoints);
  mergeCells->SetTotalNumberOfDataSets(
    1 + static_cast<int>(this->Internals->CellsToSend.size()));
  mergeCells->SetUseGlobalIds(this->Internals->InputGlobalPointIds != 0 ? 1:0);
  mergeCells->SetPointMergeTolerance(0.0);
  mergeCells->SetUseGlobalCellIds(1);

  // Merge current grid first
  mergeCells->MergeDataSet(curGrid);

  // Then merge ghost grid from neighbor ranks
  for (std::size_t i = 0; i < neighborGrids.size(); i++)
  {
    mergeCells->MergeDataSet(neighborGrids[i]);
    neighborGrids[i]->Delete();
  }

  // Finalize the merged output
  mergeCells->Finish();

}

//-----------------------------------------------------------------------------
// Add another ghost layer. Assumes that at least one layer of ghost cells has
// already been created. Must be called after GetFirstGhostLayer.
void vtkPUnstructuredGridGhostCellsGenerator::AddGhostLayer(int ghostLevel,
  int maxGhostLevel)
{
  this->Internals->CellsToSend.clear();

  this->FindGhostCells();
  this->UpdateProgress((1.0 + ((ghostLevel-1) * 3.0)) / (maxGhostLevel * 3.0));

  this->ExtractAndSendGhostCells(this->Internals->CurGrid);
  this->UpdateProgress((2.0 + ((ghostLevel-1) * 3.0)) / (maxGhostLevel * 3.0));

  vtkSmartPointer <vtkUnstructuredGrid> outputGrid =
    vtkSmartPointer<vtkUnstructuredGrid>::New();
  this->ReceiveAndMergeGhostCells(maxGhostLevel, this->Internals->CurGrid,
    outputGrid);
  this->UpdateProgress((3.0 + ((ghostLevel-1) * 3.0)) / (maxGhostLevel * 3.0));

  this->Internals->CurGrid = outputGrid;
}

//-----------------------------------------------------------------------------
// Find all cells that need to be sent as the next layer of ghost cells.
// Examine all cells that were sent the last round, find all cells which
// share points with those sent cells. These cells are the new ghost layers.
void vtkPUnstructuredGridGhostCellsGenerator::FindGhostCells()
{
  vtkNew<vtkIdList> pointIdsList;
  vtkNew<vtkIdList> cellIdsList;

  std::map<int, std::set<vtkIdType> >::iterator iter =
    this->Internals->SentCellsLastRound.begin();
  for (; iter != this->Internals->SentCellsLastRound.end(); ++iter)
  {
    int toRank = iter->first;
    std::set<vtkIdType>& cellids = this->Internals->SentCellsLastRound[toRank];

    // iterate over all cells sent to toRank
    std::set<vtkIdType>::iterator cellidIter = cellids.begin();
    for (; cellidIter != cellids.end(); ++cellidIter)
    {
      // iterate over each point in the cell
      vtkIdType cellid = *cellidIter;
      pointIdsList->Reset();
      this->Internals->CurGrid->GetCellPoints(cellid, pointIdsList.Get());
      for (int p=0; p<pointIdsList->GetNumberOfIds(); p++)
      {
        // get all cells which use this point
        vtkIdType pointid = pointIdsList->GetId(p);
        cellIdsList->Reset();
        this->Internals->CurGrid->GetPointCells(pointid, cellIdsList.Get());

        // add cells to CellsToSend
        for (int i=0; i<cellIdsList->GetNumberOfIds(); i++)
        {
          vtkIdType neighborCellId = cellIdsList->GetId(i);
          this->Internals->CellsToSend[toRank].insert(neighborCellId);
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
  }

  // add all new cells to SentCells, and update SentCellsLastRound to these new
  // cells
  this->Internals->SentCellsLastRound.clear();
  iter = this->Internals->CellsToSend.begin();
  for (; iter != this->Internals->CellsToSend.end(); ++iter)
  {
    int toRank = iter->first;
    std::set<vtkIdType>& cellids = this->Internals->CellsToSend[toRank];
    std::set<vtkIdType>::iterator cellidIter = cellids.begin();
    for (; cellidIter != cellids.end(); ++cellidIter)
    {
      this->Internals->SentCells[toRank].insert(*cellidIter);
      this->Internals->SentCellsLastRound[toRank].insert(*cellidIter);
    }
  }
}

//-----------------------------------------------------------------------------
// Add global cell ids
void vtkPUnstructuredGridGhostCellsGenerator::AddGlobalCellIds()
{
  // first figure out what to name the array,
  // if the array name is already taken, keep adding 1's to the name
  vtkCellData *celldata = this->Internals->Input->GetCellData();
  while (celldata->GetArray(this->GlobalCellIdsArrayName) != NULL)
  {
    std::string s = this->GlobalCellIdsArrayName;
    s = s + "1";
    this->SetGlobalCellIdsArrayName(s.c_str());
  }

  // do an all-to-all to share the number of cells everyone has
  vtkIdType numCells = this->Internals->Input->GetNumberOfCells();
  vtkIdType *allNumCells = new vtkIdType[this->NumRanks];
  this->Controller->AllGather(&numCells, allNumCells, 1);

  // the value of global cell ids starts at the number of cells that ranks
  // before you have
  vtkIdType idStart = 0;
  for (int rank = 0; rank < this->RankId; rank++)
  {
    idStart += allNumCells[rank];
  }

  // create an array to hold global cell ids
  vtkSmartPointer <vtkIdTypeArray> globalCellIds =
    vtkSmartPointer<vtkIdTypeArray>::New();
  globalCellIds->SetName(this->GlobalCellIdsArrayName);
  globalCellIds->SetNumberOfComponents(1);
  globalCellIds->SetNumberOfTuples(this->Internals->Input->GetNumberOfCells());
  for (vtkIdType i=0; i<this->Internals->Input->GetNumberOfCells(); i++)
  {
    globalCellIds->SetTuple1(i, i+idStart);
  }

  celldata->SetGlobalIds(globalCellIds);

  delete[] allNumCells;
}

//-----------------------------------------------------------------------------
// Remove global cell ids
void vtkPUnstructuredGridGhostCellsGenerator::RemoveGlobalCellIds()
{
  vtkCellData *celldata = this->Internals->CurGrid->GetCellData();
  if (celldata->HasArray(this->GlobalCellIdsArrayName))
  {
    celldata->RemoveArray(this->GlobalCellIdsArrayName);
  }
}
