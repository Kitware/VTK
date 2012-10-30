/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDistributedDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkDistributedDataFilter.h"

#include "vtkBox.h"
#include "vtkBoxClipDataSet.h"
#include "vtkBSPCuts.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkClipDataSet.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataObjectTypes.h"
#include "vtkDataSetReader.h"
#include "vtkDataSetWriter.h"
#include "vtkExtractCells.h"
#include "vtkExtractUserDefinedPiece.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMergeCells.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPKdTree.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPointLocator.h"
#include "vtkSmartPointer.h"
#include "vtkSocketController.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkToolkits.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include "vtkMPIController.h"

#include <vector>


vtkStandardNewMacro(vtkDistributedDataFilter)

#define TEMP_ELEMENT_ID_NAME      "___D3___GlobalCellIds"
#define TEMP_INSIDE_BOX_FLAG      "___D3___WHERE"
#define TEMP_NODE_ID_NAME         "___D3___GlobalNodeIds"

#include <set>
#include <map>
#include <algorithm>

class vtkDistributedDataFilterSTLCloak
{
public:
  std::map<int, int> IntMap;
  std::multimap<int, int> IntMultiMap;
};

class vtkDistributedDataFilter::vtkInternals
{
public:
  std::vector<int> UserRegionAssignments;
};

//----------------------------------------------------------------------------
vtkDistributedDataFilter::vtkDistributedDataFilter()
{
  this->Kdtree = NULL;

  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->Target = NULL;
  this->Source = NULL;

  this->NumConvexSubRegions = 0;
  this->ConvexSubRegionBounds = NULL;

  this->GhostLevel = 0;

  this->RetainKdtree = 1;
  this->IncludeAllIntersectingCells = 0;
  this->ClipCells = 0;

  this->Timing = 0;

  this->UseMinimalMemory = 0;

  this->UserCuts = 0;
  this->Internals = new vtkDistributedDataFilter::vtkInternals();
}

//----------------------------------------------------------------------------
vtkDistributedDataFilter::~vtkDistributedDataFilter()
{
  if (this->Kdtree)
    {
    this->Kdtree->Delete();
    this->Kdtree = NULL;
    }

  this->SetController(NULL);

  if (this->Target)
    {
    delete [] this->Target;
    this->Target= NULL;
    }

  if (this->Source)
    {
    delete [] this->Source;
    this->Source= NULL;
    }

  if (this->ConvexSubRegionBounds)
    {
    delete [] this->ConvexSubRegionBounds;
    this->ConvexSubRegionBounds = NULL;
    }

  if (this->UserCuts)
    {
    this->UserCuts->Delete();
    this->UserCuts = NULL;
    }
  delete this->Internals;
  this->Internals = 0;
}

//----------------------------------------------------------------------------
void vtkDistributedDataFilter::SetCuts(vtkBSPCuts* cuts)
{
  if (cuts == this->UserCuts)
    {
    return;
    }
  if (this->UserCuts)
    {
    this->UserCuts->Delete();
    this->UserCuts = 0;
    }
  if (cuts)
    {
    cuts->Register(this);
    this->UserCuts = cuts;
    }
  // Delete the Kdtree so that it is regenerated next time.
  if (this->Kdtree)
    {
    this->Kdtree->SetCuts(cuts);
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkDistributedDataFilter::SetUserRegionAssignments(
  const int *map, int numRegions)
{
  std::vector<int> copy(this->Internals->UserRegionAssignments);
  this->Internals->UserRegionAssignments.resize(numRegions);
  for (int cc=0; cc < numRegions; cc++)
    {
    this->Internals->UserRegionAssignments[cc] = map[cc];
    }
  if (copy != this->Internals->UserRegionAssignments)
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------------
vtkIdTypeArray *vtkDistributedDataFilter::GetGlobalElementIdArray(vtkDataSet *set)
{
  vtkDataArray *da = set->GetCellData()->GetGlobalIds();
  return vtkIdTypeArray::SafeDownCast(da);
}

//----------------------------------------------------------------------------
vtkIdType *vtkDistributedDataFilter::GetGlobalElementIds(vtkDataSet *set)
{
  vtkIdTypeArray *ia = GetGlobalElementIdArray(set);
  if (!ia)
    {
    return NULL;
    }

  return ia->GetPointer(0);
}

//----------------------------------------------------------------------------
vtkIdTypeArray *vtkDistributedDataFilter::GetGlobalNodeIdArray(vtkDataSet *set)
{
  vtkDataArray *da = set->GetPointData()->GetGlobalIds();
  return vtkIdTypeArray::SafeDownCast(da);
}

//----------------------------------------------------------------------------
vtkIdType *vtkDistributedDataFilter::GetGlobalNodeIds(vtkDataSet *set)
{
  vtkIdTypeArray *ia = this->GetGlobalNodeIdArray(set);

  if (!ia)
    {
    return NULL;
    }

  return ia->GetPointer(0);
}

//----------------------------------------------------------------------------
void vtkDistributedDataFilter::SetController(vtkMultiProcessController *c)
{
  if (this->Kdtree)
    {
    this->Kdtree->SetController(c);
    }

  if ((c == NULL) || (c->GetNumberOfProcesses() == 0))
    {
    this->NumProcesses = 1;
    this->MyId = 0;
    }

  if (this->Controller == c)
    {
    return;
    }

  this->Modified();

  if (this->Controller != NULL)
    {
    this->Controller->UnRegister(this);
    this->Controller = NULL;
    }

  if (c == NULL)
    {
    return;
    }

  this->Controller = c;

  c->Register(this);
  this->NumProcesses = c->GetNumberOfProcesses();
  this->MyId    = c->GetLocalProcessId();
}

//----------------------------------------------------------------------------
void vtkDistributedDataFilter::SetBoundaryMode(int mode)
{
  switch (mode)
    {
    case vtkDistributedDataFilter::ASSIGN_TO_ONE_REGION:
      this->AssignBoundaryCellsToOneRegionOn();
      break;
    case vtkDistributedDataFilter::ASSIGN_TO_ALL_INTERSECTING_REGIONS:
      this->AssignBoundaryCellsToAllIntersectingRegionsOn();
      break;
    case vtkDistributedDataFilter::SPLIT_BOUNDARY_CELLS:
      this->DivideBoundaryCellsOn();
      break;
    }
}

//----------------------------------------------------------------------------
int vtkDistributedDataFilter::GetBoundaryMode()
{
  if (!this->IncludeAllIntersectingCells && !this->ClipCells)
    {
    return vtkDistributedDataFilter::ASSIGN_TO_ONE_REGION;
    }
  if (this->IncludeAllIntersectingCells && !this->ClipCells)
    {
    return vtkDistributedDataFilter::ASSIGN_TO_ALL_INTERSECTING_REGIONS;
    }
  if (this->IncludeAllIntersectingCells && this->ClipCells)
    {
    return vtkDistributedDataFilter::SPLIT_BOUNDARY_CELLS;
    }

  return -1;
}

//----------------------------------------------------------------------------
void vtkDistributedDataFilter::AssignBoundaryCellsToOneRegionOn()
{
  this->SetAssignBoundaryCellsToOneRegion(1);
}

//----------------------------------------------------------------------------
void vtkDistributedDataFilter::AssignBoundaryCellsToOneRegionOff()
{
  this->SetAssignBoundaryCellsToOneRegion(0);
}

//----------------------------------------------------------------------------
void vtkDistributedDataFilter::SetAssignBoundaryCellsToOneRegion(int val)
{
  if (val)
    {
    this->IncludeAllIntersectingCells = 0;
    this->ClipCells = 0;
    }
}

//----------------------------------------------------------------------------
void
vtkDistributedDataFilter::AssignBoundaryCellsToAllIntersectingRegionsOn()
{
  this->SetAssignBoundaryCellsToAllIntersectingRegions(1);
}

//----------------------------------------------------------------------------
void
vtkDistributedDataFilter::AssignBoundaryCellsToAllIntersectingRegionsOff()
{
  this->SetAssignBoundaryCellsToAllIntersectingRegions(0);
}

//----------------------------------------------------------------------------
void
vtkDistributedDataFilter::SetAssignBoundaryCellsToAllIntersectingRegions(int val)
{
  if (val)
    {
    this->IncludeAllIntersectingCells = 1;
    this->ClipCells = 0;
    }
}
void vtkDistributedDataFilter::DivideBoundaryCellsOn()
{
  this->SetDivideBoundaryCells(1);
}
void vtkDistributedDataFilter::DivideBoundaryCellsOff()
{
  this->SetDivideBoundaryCells(0);
}
void vtkDistributedDataFilter::SetDivideBoundaryCells(int val)
{
  if (val)
    {
    this->IncludeAllIntersectingCells = 1;
    this->ClipCells = 1;
    }
}


//============================================================================
// Execute

//-------------------------------------------------------------------------

int vtkDistributedDataFilter::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int piece, numPieces, ghostLevels;

  // We require preceding filters to refrain from creating ghost cells.

  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevels = 0;

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
              numPieces);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
              ghostLevels);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkDistributedDataFilter::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
               6);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::EXTENT_TRANSLATOR(),
               inInfo->Get(vtkStreamingDemandDrivenPipeline::EXTENT_TRANSLATOR()));
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);

  return 1;
}


//----------------------------------------------------------------------------
int vtkDistributedDataFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  this->GhostLevel = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  // get the input and output
  vtkDataSet *inputDS = vtkDataSet::GetData(inputVector[0], 0);
  vtkUnstructuredGrid *outputUG = vtkUnstructuredGrid::GetData(outInfo);
  if (inputDS && outputUG)
    {
    return this->RequestDataInternal(inputDS, outputUG);
    }

  vtkCompositeDataSet* inputCD =
    vtkCompositeDataSet::GetData(inputVector[0], 0);
  vtkMultiBlockDataSet* outputMB =
    vtkMultiBlockDataSet::GetData(outputVector, 0);
  if (!inputCD || !outputMB)
    {
    vtkErrorMacro("Input must either be a composite dataset or a vtkDataSet.");
    return 0;
    }

  outputMB->CopyStructure(inputCD);

  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(inputCD->NewIterator());
  // We want to traverse over empty nodes as well. This ensures that this
  // algorithm will work correctly in parallel.
  iter->SkipEmptyNodesOff();

  // Collect information about datatypes all the processes have at all the leaf
  // nodes. Ideally all processes will either have the same type or an empty
  // dataset. This assumes that all processes have the same composite structure.
  std::vector<int> leafTypes;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
    iter->GoToNextItem())
    {
    vtkDataObject* dObj = iter->GetCurrentDataObject();
    if (dObj)
      {
      leafTypes.push_back(dObj->GetDataObjectType());
      }
    else
      {
      leafTypes.push_back(-1);
      }
    }
  unsigned int numLeaves = static_cast<unsigned int>(leafTypes.size());

  int myId = this->Controller->GetLocalProcessId();
  int numProcs = this->Controller->GetNumberOfProcesses();
  if (numProcs > 1 && numLeaves > 0)
    {
    if (myId == 0)
      {
      for (int cc=1; cc < numProcs; cc++)
        {
        std::vector<int> receivedTypes;
        receivedTypes.resize(numLeaves, -1);
        if (!this->Controller->Receive(&receivedTypes[0],
            numLeaves, cc, 1020202))
          {
          vtkErrorMacro("Communication error.");
          return 0;
          }
        for (unsigned int kk=0; kk < numLeaves; kk++)
          {
          if (leafTypes[kk] == -1)
            {
            leafTypes[kk] = receivedTypes[kk];
            }
          if (receivedTypes[kk] != -1 && leafTypes[kk] != -1 &&
            receivedTypes[kk] != leafTypes[kk])
            {
            vtkWarningMacro("Data type mismatch on processes.");
            }
          }
        }
      for (int kk=1; kk < numProcs; kk++)
        {
        this->Controller->Send(&leafTypes[0], numLeaves, kk, 1020203);
        }
      }
    else
      {
      this->Controller->Send(&leafTypes[0], numLeaves, 0, 1020202);
      this->Controller->Receive(&leafTypes[0], numLeaves, 0, 1020203);
      }
    }

  unsigned int cc=0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
    iter->GoToNextItem(), cc++)
    {
    vtkSmartPointer<vtkDataSet> ds;
    ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (ds.GetPointer() == NULL)
      {
      if (leafTypes[cc] == -1)
        {
        // This is an empty block on all processes, just skip it.
        continue;
        }

      ds.TakeReference(vtkDataSet::SafeDownCast(
        vtkDataObjectTypes::NewDataObject(leafTypes[cc])));
      }
    vtkSmartPointer<vtkUnstructuredGrid> ug =
      vtkSmartPointer<vtkUnstructuredGrid>::New();
    if (!this->RequestDataInternal(ds, ug))
      {
      return 0;
      }
    if (ug->GetNumberOfPoints() >0)
      {
      outputMB->SetDataSet(iter, ug);
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkDistributedDataFilter::RequestDataInternal(vtkDataSet* input,
  vtkUnstructuredGrid* output)
{
  this->NextProgressStep = 0;
  int progressSteps = 5 + this->GhostLevel;
  if (this->ClipCells)
    {
    progressSteps++;
    }

  this->ProgressIncrement = 1.0 / (double)progressSteps;

  this->UpdateProgress(this->NextProgressStep++ * this->ProgressIncrement);
  this->SetProgressText("Begin data redistribution");

  if (this->NumProcesses == 1)
    {
    this->SingleProcessExecute(input, output);
    this->UpdateProgress(1.0);
    return 1;
    }

  // This method requires an MPI controller.

  int aok = 0;

  if (vtkMPIController::SafeDownCast(this->Controller))
    {
    aok = 1;
    }

  if (!aok)
    {
    vtkErrorMacro(<< "vtkDistributedDataFilter multiprocess requires MPI");
    return 1;
    }

  // Stage (0) - If any processes have 0 cell input data sets, then
  //   spread the input data sets around (quickly) before formal
  //   redistribution.

  vtkDataSet *splitInput = this->TestFixTooFewInputFiles(input);

  if (splitInput == NULL)
    {
    return 1;    // Fewer cells than processes - can't divide input
    }

  this->UpdateProgress(this->NextProgressStep++ * this->ProgressIncrement);
  this->SetProgressText("Compute spatial partitioning");

  // Stage (1) - use vtkPKdTree to...
  //   Create a load balanced spatial decomposition in parallel.
  //   Create a table assigning regions to processes.
  //
  // Note k-d tree will only be re-built if input or parameters
  // have changed on any of the processing nodes.

  int fail = this->PartitionDataAndAssignToProcesses(splitInput);

  if (fail)
    {
    if (splitInput != input)
      {
      splitInput->Delete();
      }
    vtkErrorMacro(<< "vtkDistributedDataFilter::Execute k-d tree failure");
    return 1;
    }

  this->UpdateProgress(this->NextProgressStep++ * this->ProgressIncrement);
  this->SetProgressText("Compute global data array bounds");

  // Let the vtkPKdTree class compile global bounds for all
  // data arrays.  These can be accessed by D3 user by getting
  // a handle to the vtkPKdTree object and querying it.

  this->Kdtree->CreateGlobalDataArrayBounds();

  this->UpdateProgress(this->NextProgressStep++ * this->ProgressIncrement);
  this->SetProgressText("Redistribute data");

  // Stage (2) - Redistribute data, so that each process gets a ugrid
  //   containing the cells in it's assigned spatial regions.  (Note
  //   that a side effect of merging the grids received from different
  //   processes is that the final grid has no duplicate points.)
  //
  // This call will delete splitInput if it's not this->GetInput().

  vtkUnstructuredGrid *redistributedInput = this->RedistributeDataSet(splitInput,
                                                                      input);

  if (redistributedInput == NULL)
    {
    this->Kdtree->Delete();
    this->Kdtree = NULL;

    vtkErrorMacro(<< "vtkDistributedDataFilter::Execute redistribute failure");
    return 1;
    }

  this->UpdateProgress(this->NextProgressStep++ * this->ProgressIncrement);

  // Stage (3) - Add ghost cells to my sub grid.

  vtkUnstructuredGrid *expandedGrid = redistributedInput;

  if (this->GhostLevel > 0)
    {
    // Create global nodes IDs if we don't have them

    if (this->GetGlobalNodeIdArray(redistributedInput) == NULL)
      {
      this->SetProgressText("Assign global point IDs");
      int rc = this->AssignGlobalNodeIds(redistributedInput);
      if (rc)
        {
        redistributedInput->Delete();
        this->Kdtree->Delete();
        this->Kdtree = NULL;
        vtkErrorMacro(<< "vtkDistributedDataFilter::Execute global node id creation");
        return 1;
        }
      }

    // redistributedInput will be deleted by AcquireGhostCells

    this->SetProgressText("Exchange ghost cells");
    expandedGrid = this->AcquireGhostCells(redistributedInput);
    }

  // Stage (4) - Clip cells to the spatial region boundaries

  if (this->ClipCells)
    {
    this->SetProgressText("Clip boundary cells");
    this->ClipGridCells(expandedGrid);
    this->UpdateProgress(this->NextProgressStep++ * this->ProgressIncrement);
    }

  // remove temporary arrays we created

  this->SetProgressText("Clean up and finish");

  vtkDataArray *da = expandedGrid->GetCellData()->GetArray(TEMP_ELEMENT_ID_NAME);

  if (da)
    {
    expandedGrid->GetCellData()->RemoveArray(TEMP_ELEMENT_ID_NAME);
    }

  da = expandedGrid->GetPointData()->GetArray(TEMP_NODE_ID_NAME);

  if (da)
    {
    expandedGrid->GetCellData()->RemoveArray(TEMP_NODE_ID_NAME);
    }

  output->ShallowCopy(expandedGrid);

  expandedGrid->Delete();

  if (!this->RetainKdtree)
    {
    this->Kdtree->Delete();
    this->Kdtree = NULL;
    }
  else
    {
    this->Kdtree->SetDataSet(NULL);
    }

  this->UpdateProgress(1);

  return 1;
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid *vtkDistributedDataFilter::RedistributeDataSet(
  vtkDataSet *set, vtkDataSet *input)
{
  // Create global cell ids before redistributing data.  These
  // will be necessary if we need ghost cells later on.

  vtkDataSet *inputPlus = set;

  if ((this->GhostLevel > 0) && (this->GetGlobalElementIdArray(set) == NULL))
    {
    if (set == input)
      {
      inputPlus = set->NewInstance();
      inputPlus->ShallowCopy(set);
      }

    this->AssignGlobalElementIds(inputPlus);
    }

  // next call deletes inputPlus at the earliest opportunity

  vtkUnstructuredGrid *finalGrid = this->MPIRedistribute(inputPlus, input);

  return finalGrid;
}

//----------------------------------------------------------------------------
int vtkDistributedDataFilter::PartitionDataAndAssignToProcesses(vtkDataSet *set)
{
  if (this->Kdtree == NULL)
    {
    this->Kdtree = vtkPKdTree::New();
    if (!this->UserCuts)
      {
      this->Kdtree->AssignRegionsContiguous();
      }
    this->Kdtree->SetTiming(this->GetTiming());
    }
  if (this->UserCuts)
    {
    this->Kdtree->SetCuts(this->UserCuts);
    }

  this->Kdtree->SetController(this->Controller);
  this->Kdtree->SetNumberOfRegionsOrMore(this->NumProcesses);
  this->Kdtree->SetMinCells(0);
  this->Kdtree->SetDataSet(set);

  // BuildLocator is smart enough to rebuild the k-d tree only if
  // the input geometry has changed, or the k-d tree build parameters
  // have changed.  It will reassign regions if the region assignment
  // scheme has changed.

  this->Kdtree->BuildLocator();

  int nregions = this->Kdtree->GetNumberOfRegions();

  if (nregions < this->NumProcesses)
    {
    if (nregions == 0)
      {
      vtkErrorMacro("Unable to build k-d tree structure");
      }
    else
      {
      vtkErrorMacro(<< "K-d tree must have at least one region per process.  "
                    << "Needed " << this->NumProcesses << ", has " << nregions);
      }
    this->Kdtree->Delete();
    this->Kdtree = NULL;
    return 1;
    }

  if (this->Internals->UserRegionAssignments.size() > 0)
    {
    if (
      static_cast<int>(this->Internals->UserRegionAssignments.size()) !=
      nregions)
      {
      vtkWarningMacro("Mismatch in number of user-defined regions and regions"
        " the in KdTree. Ignoring user-defined regions.");
      }
    else
      {
      this->Kdtree->AssignRegions(
        &this->Internals->UserRegionAssignments[0], nregions);
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkDistributedDataFilter::ClipGridCells(vtkUnstructuredGrid *grid)
{
  if (grid->GetNumberOfCells() == 0)
    {
    return 0;
    }

  // Global point IDs are meaningless after
  // clipping, since this tetrahedralizes the whole data set.
  // We remove that array.

  if (this->GetGlobalNodeIdArray(grid))
    {
    grid->GetPointData()->SetGlobalIds(0);
    }

  this->ClipCellsToSpatialRegion(grid);

  return 0;
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid *
  vtkDistributedDataFilter::AcquireGhostCells(vtkUnstructuredGrid *grid)
{
  if (this->GhostLevel < 1)
    {
    return grid;
    }

  // Create a search structure mapping global point IDs to local point IDs

  vtkIdType numPoints = grid->GetNumberOfPoints();

  vtkIdType *gnids = NULL;

  if (numPoints > 0)
    {
    gnids = this->GetGlobalNodeIds(grid);

    if (!gnids)
      {
      vtkWarningMacro(<< "Can't create ghost cells without global node IDs");
      return grid;
      }
    }

  vtkDistributedDataFilterSTLCloak globalToLocalMap;

  for (int localPtId = 0; localPtId < numPoints; localPtId++)
    {
    const int id = gnids[localPtId];
    globalToLocalMap.IntMap.insert(std::pair<const int, int>(id, localPtId));
    }

  vtkUnstructuredGrid *expandedGrid= NULL;

  if (this->IncludeAllIntersectingCells)
    {
    expandedGrid =
      this->AddGhostCellsDuplicateCellAssignment(grid, &globalToLocalMap);
    }
  else
    {
    expandedGrid =
      this->AddGhostCellsUniqueCellAssignment(grid, &globalToLocalMap);
    }

  return expandedGrid;
}

//----------------------------------------------------------------------------
void vtkDistributedDataFilter::SingleProcessExecute(vtkDataSet *input,
                                                    vtkUnstructuredGrid *output)
{
  vtkDebugMacro(<< "vtkDistributedDataFilter::SingleProcessExecute()");

  // we run the input through vtkMergeCells which will remove
  // duplicate points

  vtkDataSet* tmp = input->NewInstance();
  tmp->ShallowCopy(input);

  float tolerance = 0.0;

  if (this->RetainKdtree)
    {
    if (this->Kdtree == NULL)
      {
      this->Kdtree = vtkPKdTree::New();
      if (this->UserCuts)
        {
        this->Kdtree->SetCuts(this->UserCuts);
        }
      this->Kdtree->SetTiming(this->GetTiming());
      }

    this->Kdtree->SetDataSet(tmp);
    this->Kdtree->BuildLocator();
    tolerance = (float)this->Kdtree->GetFudgeFactor();
    this->Kdtree->CreateGlobalDataArrayBounds();
    }
  else if (this->Kdtree)
    {
    this->Kdtree->Delete();
    this->Kdtree = NULL;
    }

  vtkUnstructuredGrid *clean =
    vtkDistributedDataFilter::MergeGrids(&tmp, 1, DeleteYes,
                                         1, tolerance, 0);

  output->ShallowCopy(clean);
  clean->Delete();

  if (this->GhostLevel > 0)
    {
    // Add the vtkGhostLevels arrays.  We have the whole
    // data set, so all cells are level 0.

    vtkDistributedDataFilter::AddConstantUnsignedCharPointArray(
                              output, "vtkGhostLevels", 0);
    vtkDistributedDataFilter::AddConstantUnsignedCharCellArray(
                              output, "vtkGhostLevels", 0);
    }
}

//----------------------------------------------------------------------------
void vtkDistributedDataFilter::ComputeMyRegionBounds()
{
  if (this->ConvexSubRegionBounds)
    {
    delete [] this->ConvexSubRegionBounds;
    this->ConvexSubRegionBounds = NULL;
    }

  vtkIntArray *myRegions = vtkIntArray::New();

  this->Kdtree->GetRegionAssignmentList(this->MyId, myRegions);

  if (myRegions->GetNumberOfTuples() > 0)
    {
    this->NumConvexSubRegions =
      this->Kdtree->MinimalNumberOfConvexSubRegions(
        myRegions, &this->ConvexSubRegionBounds);
    }
  else
    {
    this->NumConvexSubRegions = 0;
    }

  myRegions->Delete();
}

//----------------------------------------------------------------------------
int vtkDistributedDataFilter::CheckFieldArrayTypes(vtkDataSet *set)
{
  int i;

  // problem - vtkIdType arrays are written out as int arrays
  // when marshalled with vtkDataWriter.  This is a problem
  // when receive the array and try to merge it with our own,
  // which is a vtkIdType

  vtkPointData *pd = set->GetPointData();
  vtkCellData *cd = set->GetCellData();

  int npointArrays = pd->GetNumberOfArrays();

  for (i=0; i<npointArrays; i++)
    {
    int arrayType = pd->GetArray(i)->GetDataType();

    if (arrayType == VTK_ID_TYPE)
      {
      return 1;
      }
    }

  int ncellArrays = cd->GetNumberOfArrays();

  for (i=0; i<ncellArrays; i++)
    {
    int arrayType = cd->GetArray(i)->GetDataType();

    if (arrayType == VTK_ID_TYPE)
      {
      return 1;
      }
    }

  return 0;
}

//-------------------------------------------------------------------------
// Quickly spread input data around if there are more processes than
// input data sets.
//-------------------------------------------------------------------------
struct vtkDistributedDataFilterProcInfo {
  vtkIdType had;
  int procId;
  vtkIdType has;
};
extern "C"
{
  int vtkDistributedDataFilterSortSize(const void *s1, const void *s2)
  {
    vtkDistributedDataFilterProcInfo *a, *b;

    a = (struct vtkDistributedDataFilterProcInfo *)s1;
    b = (struct vtkDistributedDataFilterProcInfo *)s2;

    if (a->has < b->has)
      {
      return 1;
      }
    else if (a->has == b->has)
      {
      return 0;
      }
    else
      {
      return -1;
      }
  }
}

//----------------------------------------------------------------------------
vtkDataSet *vtkDistributedDataFilter::TestFixTooFewInputFiles(vtkDataSet *input)
{
  vtkIdType i;
  int proc;
  int me = this->MyId;
  int nprocs = this->NumProcesses;

  vtkIdType numMyCells = input->GetNumberOfCells();

  // Find out how many input cells each process has.

  vtkIdTypeArray *inputSize = this->ExchangeCounts(numMyCells, 0x0001);
  vtkIdType *sizes = inputSize->GetPointer(0);

  int *nodeType = new int [nprocs];
  const int Producer = 1;
  const int Consumer = 2;
  int numConsumers = 0;
  vtkIdType numTotalCells = 0;

  for (proc = 0; proc < nprocs ; proc++)
    {
    numTotalCells += sizes[proc];
    if (sizes[proc] == 0)
      {
      numConsumers++;
      nodeType[proc] = Consumer;
      }
    else
      {
      nodeType[proc] = Producer;
      }
    }

  if (numConsumers == 0)
    {
    // Nothing to do.  Every process has input data.

    delete [] nodeType;
    inputSize->Delete();
    return input;
    }

  vtkIdType cellsPerNode = numTotalCells / nprocs;

  vtkIdList **sendCells = new vtkIdList * [ nprocs ];
  memset(sendCells, 0, sizeof(vtkIdList *) * nprocs);

  if (numConsumers == nprocs - 1)
    {
    // Simple and common case.
    // Only one process has data and divides it among the rest.

    inputSize->Delete();

    if (nodeType[me] == Producer)
      {
      if (numTotalCells < nprocs)
        {
        // If there are not enough cells to go around, just give one cell
        // to each process, duplicating as necessary.
        for (proc = 0; proc < nprocs; proc++)
          {
          sendCells[proc] = vtkIdList::New();
          sendCells[proc]->SetNumberOfIds(1);
          sendCells[proc]->SetId(0, proc%numTotalCells);
          }
        }
      else
        {
        vtkIdType sizeLast = numTotalCells - ((nprocs-1) * cellsPerNode);
        vtkIdType cellId = 0;

        for (proc=0; proc<nprocs; proc++)
          {
          vtkIdType ncells = ((proc == nprocs - 1) ? sizeLast :cellsPerNode);

          sendCells[proc] = vtkIdList::New();
          sendCells[proc]->SetNumberOfIds(ncells);

          for (i=0; i<ncells; i++)
            {
            sendCells[proc]->SetId(i, cellId++);
            }
          }
        }
      }
    }
  else
    {
    if (numTotalCells < nprocs)
      {
      for (proc = 0; nodeType[proc] != Producer; proc++)
        {
        // empty loop.
        }
      if (proc == me)
        {
        // Have one process give out its cells to consumers.
        vtkIdType numCells = inputSize->GetValue(me);
        i = 0;
        sendCells[me] = vtkIdList::New();
        sendCells[me]->SetNumberOfIds(1);
        sendCells[me]->SetId(0, i++);
        if (i >= numCells) i = 0;
        for (proc = 0; proc < nprocs; proc++)
          {
          if (nodeType[proc] == Consumer)
            {
            sendCells[proc] = vtkIdList::New();
            sendCells[proc]->SetNumberOfIds(1);
            sendCells[proc]->SetId(0, i++);
            if (i >= numCells) i = 0;
            }
          }
        }
      else if (nodeType[me] == Producer)
        {
        // All other producers keep their own cells.
        vtkIdType numCells = inputSize->GetValue(me);
        sendCells[me] = vtkIdList::New();
        sendCells[me]->SetNumberOfIds(numCells);
        for (i = 0; i < numCells; i++)
          {
          sendCells[me]->SetId(i, i);
          }
        }

      inputSize->Delete();
      }
    else
      {

      // The processes with data send it to processes without data.
      // This is not the most balanced decomposition, and it is not the
      // fastest.  It is somewhere in between.

      vtkIdType minCells = (vtkIdType)(.8 * (double)cellsPerNode);

      struct vtkDistributedDataFilterProcInfo *procInfo
        = new struct vtkDistributedDataFilterProcInfo [nprocs];

      for (proc = 0; proc < nprocs ; proc++)
        {
        procInfo[proc].had   = inputSize->GetValue(proc);
        procInfo[proc].procId = proc;
        procInfo[proc].has   = inputSize->GetValue(proc);
        }

      inputSize->Delete();

      qsort(procInfo, nprocs, sizeof(struct vtkDistributedDataFilterProcInfo),
            vtkDistributedDataFilterSortSize);

      struct vtkDistributedDataFilterProcInfo *nextProducer = procInfo;
      struct vtkDistributedDataFilterProcInfo *nextConsumer
        = procInfo + (nprocs - 1);

      vtkIdType numTransferCells = 0;

      int sanityCheck=0;
      int nprocsSquared = nprocs * nprocs;

      while (sanityCheck++ < nprocsSquared)
        {
        int c = nextConsumer->procId;

        if (nodeType[c] == Producer)
          {
          break;
          }

        vtkIdType cGetMin = minCells - nextConsumer->has;

        if (cGetMin < 1)
          {
          nextConsumer--;
          continue;
          }
        vtkIdType cGetMax = cellsPerNode - nextConsumer->has;

        int p = nextProducer->procId;

        vtkIdType pSendMax = nextProducer->has - minCells;

        if (pSendMax < 1)
          {
          nextProducer++;
          continue;
          }

        vtkIdType transferSize = (pSendMax < cGetMax) ? pSendMax : cGetMax;

        if (me == p)
          {
          vtkIdType startCellId = nextProducer->had - nextProducer->has;
          sendCells[c] = vtkIdList::New();
          sendCells[c]->SetNumberOfIds(transferSize);
          for (i=0; i<transferSize; i++)
            {
            sendCells[c]->SetId(i, startCellId++);
            }

          numTransferCells += transferSize;
          }

        nextProducer->has -= transferSize;
        nextConsumer->has += transferSize;

        continue;
        }

      delete [] procInfo;

      if (sanityCheck > nprocsSquared)
        {
        vtkErrorMacro(<< "TestFixTooFewInputFiles error");
        for (i=0; i<nprocs; i++)
          {
          if (sendCells[i])
            {
            sendCells[i]->Delete();
            }
          }
        delete [] sendCells;
        delete [] nodeType;
        sendCells = NULL;
        }
      else if (nodeType[me] == Producer)
        {
        vtkIdType keepCells = numMyCells - numTransferCells;
        vtkIdType startCellId = (vtkIdType)numTransferCells;
        sendCells[me] = vtkIdList::New();
        sendCells[me]->SetNumberOfIds(keepCells);
        for (i=0; i<keepCells; i++)
          {
          sendCells[me]->SetId(i, startCellId++);
          }
        }
      }
    }

  vtkUnstructuredGrid *newGrid = NULL;

  if (sendCells)
    {
    newGrid = this->ExchangeMergeSubGrids(
           sendCells, DeleteYes, input, DeleteNo,
           DuplicateCellsNo, GhostCellsNo, 0x0011);

    delete [] sendCells;
    delete [] nodeType;
    }

  return newGrid;
}

//============================================================================
// Communication routines - two versions:
//   *Lean version use minimal memory
//   *Fast versions use more memory, but are much faster

//-------------------------------------------------------------------------
void vtkDistributedDataFilter::SetUpPairWiseExchange()
{
  int iam = this->MyId;
  int nprocs = this->NumProcesses;

  if (this->Target)
    {
    delete [] this->Target;
    this->Target = NULL;
    }

  if (this->Source)
    {
    delete [] this->Source;
    this->Source = NULL;
    }

  if (nprocs == 1)
    {
    return;
    }

  this->Target = new int [nprocs - 1];
  this->Source = new int [nprocs - 1];

  for (int i=1; i< nprocs; i++)
    {
    this->Target[i-1] = (iam + i) % nprocs;
    this->Source[i-1] = (iam + nprocs - i) % nprocs;
    }
}

//-------------------------------------------------------------------------
void vtkDistributedDataFilter::FreeIntArrays(vtkIdTypeArray **ar)
{
  for (int i=0; i<this->NumProcesses; i++)
    {
    if (ar[i])
      {
      ar[i]->Delete();
      }
    }

  delete [] ar;
}

//-------------------------------------------------------------------------
void vtkDistributedDataFilter::FreeIdLists(vtkIdList**lists, int nlists)
{
  for (int i=0; i<nlists; i++)
    {
    if (lists[i])
      {
       lists[i]->Delete();
       lists[i] = NULL;
      }
    }
}

//-------------------------------------------------------------------------
vtkIdType vtkDistributedDataFilter::GetIdListSize(vtkIdList **lists, int nlists)
{
  vtkIdType numCells = 0;

  for (int i=0; i<nlists; i++)
    {
    if (lists[i])
      {
      numCells += lists[i]->GetNumberOfIds();
      }
    }

  return numCells;
}

//-------------------------------------------------------------------------
vtkUnstructuredGrid *
  vtkDistributedDataFilter::ExchangeMergeSubGrids(
               vtkIdList **cellIds, int deleteCellIds,
               vtkDataSet *myGrid, int deleteMyGrid,
               int filterOutDuplicateCells, int ghostCellFlag,
               int tag)
{
  int nprocs = this->NumProcesses;

  int *numLists = new int [nprocs];

  vtkIdList ***listOfLists = new vtkIdList ** [nprocs];

  for (int i=0; i<nprocs; i++)
    {
    if (cellIds[i] == NULL)
      {
      numLists[i] = 0;
      }
    else
      {
      numLists[i] = 1;
      }

    listOfLists[i] = &cellIds[i];
    }

  vtkUnstructuredGrid *grid = NULL;

  if (this->UseMinimalMemory)
    {
    grid = this->ExchangeMergeSubGridsLean(listOfLists, numLists, deleteCellIds,
             myGrid, deleteMyGrid, filterOutDuplicateCells, ghostCellFlag, tag);
    }
  else
    {
    grid = this->ExchangeMergeSubGridsFast(listOfLists, numLists, deleteCellIds,
             myGrid, deleteMyGrid, filterOutDuplicateCells, ghostCellFlag, tag);
    }

  delete [] numLists;
  delete [] listOfLists;

  return grid;
}

//-------------------------------------------------------------------------
vtkUnstructuredGrid *
  vtkDistributedDataFilter::ExchangeMergeSubGrids(
               vtkIdList ***cellIds, int *numLists, int deleteCellIds,
               vtkDataSet *myGrid, int deleteMyGrid,
               int filterOutDuplicateCells, int ghostCellFlag,
               int tag)
{
  vtkUnstructuredGrid *grid = NULL;

  if (this->UseMinimalMemory)
    {
    grid = this->ExchangeMergeSubGridsLean(cellIds, numLists, deleteCellIds,
             myGrid, deleteMyGrid, filterOutDuplicateCells, ghostCellFlag, tag);
    }
  else
    {
    grid = this->ExchangeMergeSubGridsFast(cellIds, numLists, deleteCellIds,
             myGrid, deleteMyGrid, filterOutDuplicateCells, ghostCellFlag, tag);
    }
  return grid;
}

//-------------------------------------------------------------------------
vtkIdTypeArray *vtkDistributedDataFilter::ExchangeCounts(vtkIdType myCount, int tag)
{
  vtkIdTypeArray *ia;

  if (this->UseMinimalMemory)
    {
    ia = this->ExchangeCountsLean(myCount, tag);
    }
  else
    {
    ia = this->ExchangeCountsFast(myCount, tag);
    }
  return ia;
}

//-------------------------------------------------------------------------
vtkFloatArray **vtkDistributedDataFilter::
  ExchangeFloatArrays(vtkFloatArray **myArray, int deleteSendArrays, int tag)
{
  vtkFloatArray **fa;

  if (this->UseMinimalMemory)
    {
    fa = this->ExchangeFloatArraysLean(myArray, deleteSendArrays, tag);
    }
  else
    {
    fa = this->ExchangeFloatArraysFast(myArray, deleteSendArrays, tag);
    }
  return fa;
}

//-------------------------------------------------------------------------
vtkIdTypeArray **vtkDistributedDataFilter::
  ExchangeIdArrays(vtkIdTypeArray **myArray, int deleteSendArrays, int tag)
{
  vtkIdTypeArray **ia;

  if (this->UseMinimalMemory)
    {
    ia = this->ExchangeIdArraysLean(myArray, deleteSendArrays, tag);
    }
  else
    {
    ia = this->ExchangeIdArraysFast(myArray, deleteSendArrays, tag);
    }
  return ia;
}

// ----------------------- Lean versions ----------------------------//
vtkIdTypeArray *vtkDistributedDataFilter::ExchangeCountsLean(vtkIdType myCount, int tag)
{
  vtkIdTypeArray *countArray = NULL;

  vtkIdType i;
  int nprocs = this->NumProcesses;

  vtkMPICommunicator::Request req;
  vtkMPIController *mpiContr = vtkMPIController::SafeDownCast(this->Controller);

  vtkIdType *counts = new vtkIdType [nprocs];
  counts[this->MyId] = myCount;

  if (!this->Source)
    {
    this->SetUpPairWiseExchange();
    }

  for (i = 0; i < this->NumProcesses - 1; i++)
    {
    int source = this->Source[i];
    int target = this->Target[i];
    mpiContr->NoBlockReceive(counts + source, 1, source, tag, req);
    mpiContr->Send(&myCount, 1, target, tag);
    req.Wait();
    }

  countArray = vtkIdTypeArray::New();
  countArray->SetArray(counts, nprocs, 0);

  return countArray;
}

//-------------------------------------------------------------------------
vtkFloatArray **
  vtkDistributedDataFilter::ExchangeFloatArraysLean(vtkFloatArray **myArray,
                                              int deleteSendArrays, int tag)
{
  vtkFloatArray **remoteArrays = NULL;

  int i;
  int nprocs = this->NumProcesses;
  int me = this->MyId;

  vtkMPICommunicator::Request req;
  vtkMPIController *mpiContr = vtkMPIController::SafeDownCast(this->Controller);

  int *recvSize = new int [nprocs];
  int *sendSize = new int [nprocs];

  if (!this->Source)
    {
    this->SetUpPairWiseExchange();
    }

  for (i= 0; i< nprocs; i++)
    {
    sendSize[i] = myArray[i] ? myArray[i]->GetNumberOfTuples() : 0;
    recvSize[i] = 0;
    }

  // Exchange sizes

  int nothers = nprocs - 1;

  for (i = 0; i < nothers; i++)
    {
    int source = this->Source[i];
    int target = this->Target[i];
    mpiContr->NoBlockReceive(recvSize + source, 1, source, tag, req);
    mpiContr->Send(sendSize + target, 1, target, tag);
    req.Wait();
    }

  // Exchange int arrays

  float **recvArrays = new float * [nprocs];
  memset(recvArrays, 0, sizeof(float *) * nprocs);

  if (sendSize[me] > 0)  // sent myself an array
    {
    recvSize[me] = sendSize[me];
    recvArrays[me] = new float [sendSize[me]];
    memcpy(recvArrays[me], myArray[me]->GetPointer(0), sendSize[me] * sizeof(float));
    }

  for (i = 0; i < nothers; i++)
    {
    int source = this->Source[i];
    int target = this->Target[i];
    recvArrays[source] = NULL;

    if (recvSize[source] > 0)
      {
      recvArrays[source] = new float [recvSize[source]];
      if (recvArrays[source] == NULL)
        {
        vtkErrorMacro(<<
          "vtkDistributedDataFilter::ExchangeIdArrays memory allocation");
        return NULL;
        }
      mpiContr->NoBlockReceive(recvArrays[source], recvSize[source], source, tag, req);
      }

    if (sendSize[target] > 0)
      {
      mpiContr->Send(myArray[target]->GetPointer(0), sendSize[target], target, tag);
      }

    if (myArray[target] && deleteSendArrays)
      {
      myArray[target]->Delete();
      }

    if (recvSize[source] > 0)
      {
      req.Wait();
      }
    }

  if (deleteSendArrays)
    {
    if (myArray[me])
      {
      myArray[me]->Delete();
      }
    delete [] myArray;
    }

  delete [] sendSize;

  remoteArrays = new vtkFloatArray * [nprocs];

  for (i=0; i<nprocs; i++)
    {
    if (recvSize[i] > 0)
      {
      remoteArrays[i] = vtkFloatArray::New();
      remoteArrays[i]->SetArray(recvArrays[i], recvSize[i], 0);
      }
    else
      {
      remoteArrays[i] = NULL;
      }
    }

  delete [] recvArrays;
  delete [] recvSize;

  return remoteArrays;
}

//-------------------------------------------------------------------------
vtkIdTypeArray **
  vtkDistributedDataFilter::ExchangeIdArraysLean(vtkIdTypeArray **myArray,
                                              int deleteSendArrays, int tag)
{
  vtkIdTypeArray **remoteArrays = NULL;

  int i;
  int nprocs = this->NumProcesses;
  int me = this->MyId;

  vtkMPICommunicator::Request req;
  vtkMPIController *mpiContr = vtkMPIController::SafeDownCast(this->Controller);

  vtkIdType *recvSize = new vtkIdType [nprocs];
  vtkIdType *sendSize = new vtkIdType [nprocs];

  if (!this->Source)
    {
    this->SetUpPairWiseExchange();
    }

  for (i= 0; i< nprocs; i++)
    {
    sendSize[i] = myArray[i] ? myArray[i]->GetNumberOfTuples() : 0;
    recvSize[i] = 0;
    }

  // Exchange sizes

  int nothers = nprocs - 1;

  for (i = 0; i < nothers; i++)
    {
    int source = this->Source[i];
    int target = this->Target[i];
    mpiContr->NoBlockReceive(recvSize + source, 1, source, tag, req);
    mpiContr->Send(sendSize + target, 1, target, tag);
    req.Wait();
    }

  // Exchange int arrays

  vtkIdType **recvArrays = new vtkIdType * [nprocs];
  memset(recvArrays, 0, sizeof(vtkIdType *) * nprocs);

  if (sendSize[me] > 0)  // sent myself an array
    {
    recvSize[me] = sendSize[me];
    recvArrays[me] = new vtkIdType [sendSize[me]];
    memcpy(recvArrays[me], myArray[me]->GetPointer(0), sendSize[me] * sizeof(vtkIdType));
    }

  for (i = 0; i < nothers; i++)
    {
    int source = this->Source[i];
    int target = this->Target[i];
    recvArrays[source] = NULL;

    if (recvSize[source] > 0)
      {
      recvArrays[source] = new vtkIdType [recvSize[source]];
      if (recvArrays[source] == NULL)
        {
        vtkErrorMacro(<<
          "vtkDistributedDataFilter::ExchangeIdArrays memory allocation");
        return NULL;
        }
      mpiContr->NoBlockReceive(recvArrays[source], recvSize[source], source, tag, req);
      }

    if (sendSize[target] > 0)
      {
      mpiContr->Send(myArray[target]->GetPointer(0), sendSize[target], target, tag);
      }

    if (myArray[target] && deleteSendArrays)
      {
      myArray[target]->Delete();
      }

    if (recvSize[source] > 0)
      {
      req.Wait();
      }
    }

  if (deleteSendArrays)
    {
    if (myArray[me])
      {
      myArray[me]->Delete();
      }
    delete [] myArray;
    }

  delete [] sendSize;

  remoteArrays = new vtkIdTypeArray * [nprocs];

  for (i=0; i<nprocs; i++)
    {
    if (recvSize[i] > 0)
      {
      remoteArrays[i] = vtkIdTypeArray::New();
      remoteArrays[i]->SetArray(recvArrays[i], recvSize[i], 0);
      }
    else
      {
      remoteArrays[i] = NULL;
      }
    }

  delete [] recvArrays;
  delete [] recvSize;

  return remoteArrays;
}

//-------------------------------------------------------------------------
vtkUnstructuredGrid *
  vtkDistributedDataFilter::ExchangeMergeSubGridsLean(
    vtkIdList ***cellIds, int *numLists, int deleteCellIds,
    vtkDataSet *myGrid, int deleteMyGrid,
    int filterOutDuplicateCells,   // flag if different processes may send same cells
    int vtkNotUsed(ghostCellFlag),   // flag if these cells are ghost cells
    int tag)
{
  vtkUnstructuredGrid *mergedGrid = NULL;
  int i;
  int packedGridSendSize=0, packedGridRecvSize=0;
  char *packedGridSend=NULL, *packedGridRecv=NULL;
  int recvBufSize=0;
  int numReceivedGrids = 0;

  int nprocs = this->NumProcesses;
  int iam = this->MyId;

  vtkMPIController *mpiContr = vtkMPIController::SafeDownCast(this->Controller);
  vtkMPICommunicator::Request req;

  vtkDataSet *tmpGrid = myGrid->NewInstance();
  tmpGrid->ShallowCopy(myGrid);

  vtkDataSet **grids = new vtkDataSet * [nprocs];

  if (numLists[iam] > 0)
    {
    // I was extracting/packing/sending/unpacking ugrids of zero cells,
    // and this caused corrupted data structures.  I don't know why, but
    // I am now being careful not to do that.

    vtkIdType numCells =
      vtkDistributedDataFilter::GetIdListSize(cellIds[iam], numLists[iam]);

    if (numCells > 0)
      {
      grids[numReceivedGrids++] =
        this->ExtractCells(cellIds[iam], numLists[iam], deleteCellIds, tmpGrid);
      }
    else if (deleteCellIds)
      {
      vtkDistributedDataFilter::FreeIdLists(cellIds[iam], numLists[iam]);
      }
    }

  if (this->Source == NULL)
    {
    this->SetUpPairWiseExchange();
    }

  int nothers = nprocs - 1;

  for (i=0; i<nothers; i++)
    {
    int target = this->Target[i];
    int source = this->Source[i];

    packedGridSendSize = 0;

    if (cellIds[target] && (numLists[target] > 0))
      {
      vtkIdType numCells = vtkDistributedDataFilter::GetIdListSize(
                      cellIds[target], numLists[target]);

      if (numCells > 0)
        {
        vtkUnstructuredGrid *sendGrid =
          this->ExtractCells(cellIds[target], numLists[target],
                                               deleteCellIds, tmpGrid);

        packedGridSend = this->MarshallDataSet(sendGrid, packedGridSendSize);
        sendGrid->Delete();
        }
      else if (deleteCellIds)
        {
        vtkDistributedDataFilter::FreeIdLists(cellIds[target], numLists[target]);
        }
      }

    // exchange size of packed grids

    mpiContr->NoBlockReceive(&packedGridRecvSize, 1, source, tag, req);
    mpiContr->Send(&packedGridSendSize, 1, target, tag);
    req.Wait();

    if (packedGridRecvSize > recvBufSize)
      {
      if (packedGridRecv)
        {
        delete [] packedGridRecv;
        }
      packedGridRecv = new char [packedGridRecvSize];
      if (!packedGridRecv)
        {
        vtkErrorMacro(<<
          "vtkDistributedDataFilter::ExchangeMergeSubGrids memory allocation");
        return NULL;
        }
      recvBufSize = packedGridRecvSize;
      }

    if (packedGridRecvSize > 0)
      {
      mpiContr->NoBlockReceive(packedGridRecv, packedGridRecvSize, source, tag, req);
      }

    if (packedGridSendSize > 0)
      {
      mpiContr->Send(packedGridSend, packedGridSendSize, target, tag);
      delete [] packedGridSend;
      }

    if (packedGridRecvSize > 0)
      {
      req.Wait();

      grids[numReceivedGrids++] =
        this->UnMarshallDataSet(packedGridRecv, packedGridRecvSize);
      }
    }

  tmpGrid->Delete();

  if (recvBufSize > 0)
    {
    delete [] packedGridRecv;
    packedGridRecv = NULL;
    }

  if (numReceivedGrids > 1)
    {
    // Merge received grids

    // this call will merge the grids and then delete them

    float tolerance = 0.0;

    if (this->Kdtree)
      {
      tolerance = (float)this->Kdtree->GetFudgeFactor();
      }

    mergedGrid =
      vtkDistributedDataFilter::MergeGrids(grids, numReceivedGrids, DeleteYes,
                                           1, tolerance, filterOutDuplicateCells);

    }
  else if (numReceivedGrids == 1)
    {
    mergedGrid = vtkUnstructuredGrid::SafeDownCast(grids[0]);
    }
  else
    {
    mergedGrid = this->ExtractZeroCellGrid(myGrid);
    }

  if (deleteMyGrid)
    {
    myGrid->Delete();
    }

  delete [] grids;

  return mergedGrid;
}

// ----------------------- Fast versions ----------------------------//
vtkIdTypeArray *vtkDistributedDataFilter::ExchangeCountsFast(vtkIdType myCount, int tag)
{
  vtkIdTypeArray *countArray = NULL;

  vtkIdType i;
  int nprocs = this->NumProcesses;
  int me = this->MyId;

  vtkMPICommunicator::Request *req = new vtkMPICommunicator::Request [nprocs];
  vtkMPIController *mpiContr = vtkMPIController::SafeDownCast(this->Controller);

  vtkIdType *counts = new vtkIdType [nprocs];
  counts[me] = myCount;

  for (i = 0; i < nprocs; i++)
    {
    if (i  == me)
      {
      continue;
      }
    mpiContr->NoBlockReceive(counts + i, 1, i, tag, req[i]);
    }

  mpiContr->Barrier();

  for (i = 0; i < nprocs; i++)
    {
    if (i  == me)
      {
      continue;
      }
    mpiContr->Send(&myCount, 1, i, tag);
    }

  countArray = vtkIdTypeArray::New();
  countArray->SetArray(counts, nprocs, 0);

  for (i = 0; i < nprocs; i++)
    {
    if (i  == me)
      {
      continue;
      }
    req[i].Wait();
    }

  delete [] req;

  return countArray;
}

//-------------------------------------------------------------------------
vtkFloatArray **
  vtkDistributedDataFilter::ExchangeFloatArraysFast(vtkFloatArray **myArray,
                                              int deleteSendArrays, int tag)
{
  vtkFloatArray **fa = NULL;
  int proc;
  int nprocs = this->NumProcesses;
  int iam = this->MyId;

  vtkMPIController *mpiContr = vtkMPIController::SafeDownCast(this->Controller);

  int *sendSize = new int [nprocs];
  int *recvSize = new int [nprocs];

  for (proc=0; proc < nprocs; proc++)
    {
    recvSize[proc] = sendSize[proc] = 0;

    if (proc == iam)
      {
      continue;
      }

    if (myArray[proc])
      {
      sendSize[proc] = myArray[proc]->GetNumberOfTuples();
      }
    }

  // Exchange sizes of arrays to send and receive

  vtkMPICommunicator::Request *reqBuf = new vtkMPICommunicator::Request [nprocs];

  for (proc=0; proc<nprocs; proc++)
    {
    if (proc == iam)
      {
      continue;
      }
    mpiContr->NoBlockReceive(recvSize + proc, 1, proc, tag, reqBuf[proc]);
    }

  mpiContr->Barrier();

  for (proc=0; proc<nprocs; proc++)
    {
    if (proc == iam)
      {
      continue;
      }
    mpiContr->Send(sendSize + proc, 1, proc, tag);
    }

  for (proc=0; proc<nprocs; proc++)
    {
    if (proc == iam)
      {
      continue;
      }
    reqBuf[proc].Wait();
    }

  // Allocate buffers and post receives

  float **recvBufs = new float * [nprocs];

  for (proc=0; proc < nprocs; proc++)
    {
    if (recvSize[proc] > 0)
      {
      recvBufs[proc] = new float [recvSize[proc]];
      mpiContr->NoBlockReceive(recvBufs[proc], recvSize[proc], proc, tag, reqBuf[proc]);
      }
    else
      {
      recvBufs[proc] = NULL;
      }
    }

  mpiContr->Barrier();

  // Send all arrays

  for (proc=0; proc < nprocs; proc++)
    {
    if (sendSize[proc] > 0)
      {
      mpiContr->Send(myArray[proc]->GetPointer(0), sendSize[proc], proc, tag);
      }
    }
  delete [] sendSize;

  // If I want to send an array to myself, place it in output now

  if (myArray[iam])
    {
    recvSize[iam] = myArray[iam]->GetNumberOfTuples();
    if (recvSize[iam] > 0)
      {
      recvBufs[iam] = new float [recvSize[iam]];
      memcpy(recvBufs[iam], myArray[iam]->GetPointer(0), recvSize[iam] * sizeof(float));
      }
    }

  if (deleteSendArrays)
    {
    for (proc=0; proc < nprocs; proc++)
      {
      if (myArray[proc])
        {
        myArray[proc]->Delete();
        }
      }
    delete [] myArray;
    }

  // Await incoming arrays

  fa = new vtkFloatArray * [nprocs];
  for (proc=0; proc < nprocs; proc++)
    {
    if (recvBufs[proc])
      {
      fa[proc] = vtkFloatArray::New();
      fa[proc]->SetArray(recvBufs[proc], recvSize[proc], 0);
      }
    else
      {
      fa[proc] = NULL;
      }
    }

  delete [] recvSize;

  for (proc=0; proc < nprocs; proc++)
    {
    if (proc == iam)
      {
      continue;
      }
    if (recvBufs[proc])
      {
      reqBuf[proc].Wait();
      }
    }

  delete [] reqBuf;
  delete [] recvBufs;

  return fa;
}

//-------------------------------------------------------------------------
vtkIdTypeArray **
  vtkDistributedDataFilter::ExchangeIdArraysFast(vtkIdTypeArray **myArray,
                                              int deleteSendArrays, int tag)
{
  vtkIdTypeArray **ia = NULL;
  int proc;
  int nprocs = this->NumProcesses;
  int iam = this->MyId;

  vtkMPIController *mpiContr = vtkMPIController::SafeDownCast(this->Controller);

  vtkIdType *sendSize = new vtkIdType [nprocs];
  vtkIdType *recvSize = new vtkIdType [nprocs];

  for (proc=0; proc < nprocs; proc++)
    {
    recvSize[proc] = sendSize[proc] = 0;

    if (proc == iam)
      {
      continue;
      }

    if (myArray[proc])
      {
      sendSize[proc] = myArray[proc]->GetNumberOfTuples();
      }
    }

  // Exchange sizes of arrays to send and receive

  vtkMPICommunicator::Request *reqBuf = new vtkMPICommunicator::Request [nprocs];

  for (proc=0; proc<nprocs; proc++)
    {
    if (proc == iam)
      {
      continue;
      }
    mpiContr->NoBlockReceive(recvSize + proc, 1, proc, tag, reqBuf[proc]);
    }

  mpiContr->Barrier();

  for (proc=0; proc<nprocs; proc++)
    {
    if (proc == iam)
      {
      continue;
      }
    mpiContr->Send(sendSize + proc, 1, proc, tag);
    }

  for (proc=0; proc<nprocs; proc++)
    {
    if (proc == iam)
      {
      continue;
      }
    reqBuf[proc].Wait();
    }

  // Allocate buffers and post receives

  vtkIdType **recvBufs = new vtkIdType * [nprocs];

  for (proc=0; proc < nprocs; proc++)
    {
    if (recvSize[proc] > 0)
      {
      recvBufs[proc] = new vtkIdType [recvSize[proc]];
      mpiContr->NoBlockReceive(recvBufs[proc], recvSize[proc], proc, tag, reqBuf[proc]);
      }
    else
      {
      recvBufs[proc] = NULL;
      }
    }

  mpiContr->Barrier();

  // Send all arrays

  for (proc=0; proc < nprocs; proc++)
    {
    if (sendSize[proc] > 0)
      {
      mpiContr->Send(myArray[proc]->GetPointer(0), sendSize[proc], proc, tag);
      }
    }
  delete [] sendSize;

  // If I want to send an array to myself, place it in output now

  if (myArray[iam])
    {
    recvSize[iam] = myArray[iam]->GetNumberOfTuples();
    if (recvSize[iam] > 0)
      {
      recvBufs[iam] = new vtkIdType [recvSize[iam]];
      memcpy(recvBufs[iam], myArray[iam]->GetPointer(0), recvSize[iam] * sizeof(vtkIdType));
      }
    }

  if (deleteSendArrays)
    {
    for (proc=0; proc < nprocs; proc++)
      {
      if (myArray[proc])
        {
        myArray[proc]->Delete();
        }
      }
    delete [] myArray;
    }

  // Await incoming arrays

  ia = new vtkIdTypeArray * [nprocs];
  for (proc=0; proc < nprocs; proc++)
    {
    if (recvBufs[proc])
      {
      ia[proc] = vtkIdTypeArray::New();
      ia[proc]->SetArray(recvBufs[proc], recvSize[proc], 0);
      }
    else
      {
      ia[proc] = NULL;
      }
    }

  delete [] recvSize;

  for (proc=0; proc < nprocs; proc++)
    {
    if (proc == iam)
      {
      continue;
      }
    if (recvBufs[proc])
      {
      reqBuf[proc].Wait();
      }
    }

  delete [] reqBuf;
  delete [] recvBufs;

  return ia;
}

//-------------------------------------------------------------------------
vtkUnstructuredGrid *
  vtkDistributedDataFilter::ExchangeMergeSubGridsFast(
    vtkIdList ***cellIds, int *numLists, int deleteCellIds,
    vtkDataSet *myGrid, int deleteMyGrid,
    int filterOutDuplicateCells,   // flag if different processes may send same cells
    int vtkNotUsed(ghostCellFlag),  // flag if these are ghost cells
    int tag)
{
  vtkUnstructuredGrid *mergedGrid = NULL;
  int proc;
  int nprocs = this->NumProcesses;
  int iam = this->MyId;

  vtkMPIController *mpiContr = vtkMPIController::SafeDownCast(this->Controller);

  vtkUnstructuredGrid **grids = new vtkUnstructuredGrid * [nprocs];
  char **sendBufs = new char * [nprocs];
  char **recvBufs = new char * [nprocs];
  int *sendSize = new int [nprocs];
  int *recvSize = new int [nprocs];

  // create & pack all sub grids

  vtkDataSet *tmpGrid = myGrid->NewInstance();
  tmpGrid->ShallowCopy(myGrid);

  for (proc=0; proc < nprocs; proc++)
    {
    recvSize[proc] = sendSize[proc] = 0;
    grids[proc] = NULL;
    sendBufs[proc] = recvBufs[proc] = NULL;

    if (numLists[proc] > 0)
      {
      vtkIdType numCells =
        vtkDistributedDataFilter::GetIdListSize(cellIds[proc], numLists[proc]);

      if (numCells > 0)
        {
        grids[proc] =
          vtkDistributedDataFilter::ExtractCells(cellIds[proc], numLists[proc],
                                          deleteCellIds, tmpGrid);

        if (proc != iam)
          {
          sendBufs[proc] = this->MarshallDataSet(grids[proc], sendSize[proc]);
          grids[proc]->Delete();
          grids[proc] = NULL;
          }
        }
      else if (deleteCellIds)
        {
        vtkDistributedDataFilter::FreeIdLists(cellIds[proc], numLists[proc]);
        }
      }
    }

  tmpGrid->Delete();

  // Exchange sizes of grids to send and receive

  vtkMPICommunicator::Request *reqBuf = new vtkMPICommunicator::Request [nprocs];

  for (proc=0; proc<nprocs; proc++)
    {
    if (proc == iam)
      {
      continue;
      }
    mpiContr->NoBlockReceive(recvSize + proc, 1, proc, tag, reqBuf[proc]);
    }

  mpiContr->Barrier();

  for (proc=0; proc<nprocs; proc++)
    {
    if (proc == iam)
      {
      continue;
      }
    mpiContr->Send(sendSize + proc, 1, proc, tag);
    }

  for (proc=0; proc<nprocs; proc++)
    {
    if (proc == iam)
      {
      continue;
      }
    reqBuf[proc].Wait();
    }

  // Allocate buffers and post receives

  int numReceives = 0;

  for (proc=0; proc < nprocs; proc++)
    {
    if (recvSize[proc] > 0)
      {
      recvBufs[proc] = new char [recvSize[proc]];
      mpiContr->NoBlockReceive(recvBufs[proc], recvSize[proc], proc, tag, reqBuf[proc]);
      numReceives++;
      }
    }

  mpiContr->Barrier();

  // Send all sub grids, then delete them

  for (proc=0; proc < nprocs; proc++)
    {
    if (sendSize[proc] > 0)
      {
      mpiContr->Send(sendBufs[proc], sendSize[proc], proc, tag);
      }
    }

  for (proc=0; proc < nprocs; proc++)
    {
    if (sendSize[proc] > 0)
      {
      delete [] sendBufs[proc];
      }
    }

  delete [] sendSize;
  delete [] sendBufs;

  // Await incoming sub grids, unpack them

  while (numReceives > 0)
    {
    for (proc=0; proc < nprocs; proc++)
      {
      if (recvBufs[proc] && (reqBuf[proc].Test() == 1))
        {
        grids[proc] = this->UnMarshallDataSet(recvBufs[proc], recvSize[proc]);
        delete [] recvBufs[proc];
        recvBufs[proc] = NULL;
        numReceives--;
        }
      }
    }

  delete [] reqBuf;
  delete [] recvBufs;
  delete [] recvSize;

  // Merge received grids

  float tolerance = 0.0;

  if (this->Kdtree)
    {
    tolerance = (float)this->Kdtree->GetFudgeFactor();
    }

  int numReceivedGrids = 0;

  vtkDataSet **ds = new vtkDataSet * [nprocs];

  for (proc=0; proc < nprocs; proc++)
    {
    if (grids[proc] != NULL)
      {
      ds[numReceivedGrids++] = static_cast<vtkDataSet *>(grids[proc]);
      }
    }

  delete [] grids;

  if (numReceivedGrids > 1)
    {
    // Normally, using this->GetGlobalNodeIds is the right thing.  However,
    // there is a bit of a bug here that this filter only works with ids
    // that are vtkIdType.  Otherwise, it will return NULL as the global ids.
    // That is bad because then the global node ids will be stripped in the
    // MergeGrids method, and the number of point arrays will not match,
    // causing a crash latter on.
    //int useGlobalNodeIds = (this->GetGlobalNodeIds(ds[0]) != NULL);
    int useGlobalNodeIds = (ds[0]->GetPointData()->GetGlobalIds() != NULL);

    // this call will merge the grids and then delete them
    mergedGrid =
      vtkDistributedDataFilter::MergeGrids(ds, numReceivedGrids, DeleteYes,
                                           useGlobalNodeIds, tolerance,
                                           filterOutDuplicateCells);

    }
  else if (numReceivedGrids == 1)
    {
    mergedGrid = vtkUnstructuredGrid::SafeDownCast(ds[0]);
    }
  else
    {
    mergedGrid = this->ExtractZeroCellGrid(myGrid);
    }

  if (deleteMyGrid)
    {
    myGrid->Delete();
    }

  delete [] ds;

  return mergedGrid;
}

//-------------------------------------------------------------------------
vtkUnstructuredGrid *vtkDistributedDataFilter::MPIRedistribute(vtkDataSet *in,
                                                               vtkDataSet *input)
{
  int proc;
  int nprocs = this->NumProcesses;

  // A cell belongs to a spatial region if it's centroid lies in that
  // region.  The kdtree object can create a list for each region of the
  // IDs of each cell I have read in that belong in that region.  If we
  // are building subgrids of all cells that intersect a region (a
  // superset of all cells that belong to a region) then the kdtree object
  // can build another set of lists of all cells that intersect each
  // region (but don't have their centroid in that region).

  if (this->IncludeAllIntersectingCells)
    {
    // TO DO:
    // We actually compute whether a cell intersects a spatial region.
    // This can be a lengthy calculation.  Perhaps it's good enough
    // to compute whether a cell's bounding box intersects the region.
    // Some of the cells we list will actually not be in the region, but
    // if we are clipping later, it doesn't matter.
    //
    // Is there any rendering algorithm that needs exactly all cells
    // which intersect the region, and no more?

    this->Kdtree->IncludeRegionBoundaryCellsOn();   // SLOW!!
    }

  this->Kdtree->CreateCellLists();  // required by GetCellIdsForProcess

  vtkIdList ***procCellLists = new vtkIdList ** [nprocs];
  int *numLists = new int [nprocs];

  for (proc = 0; proc < this->NumProcesses; proc++)
    {
    procCellLists[proc] = this->GetCellIdsForProcess(proc, numLists + proc);
    }

  int deleteDataSet = DeleteNo;

  if (in != input)
    {
    deleteDataSet = DeleteYes;
    }

  vtkUnstructuredGrid *myNewGrid =
    this->ExchangeMergeSubGrids(procCellLists, numLists, DeleteNo,
       in, deleteDataSet, DuplicateCellsNo, GhostCellsNo, 0x0012);

  for (proc = 0; proc < nprocs; proc++)
    {
    delete [] procCellLists[proc];
    }

  delete [] procCellLists;
  delete [] numLists;

  if (myNewGrid && (this->GhostLevel > 0))
    {
    vtkDistributedDataFilter::AddConstantUnsignedCharCellArray(
                            myNewGrid, "vtkGhostLevels", 0);
    vtkDistributedDataFilter::AddConstantUnsignedCharPointArray(
                            myNewGrid, "vtkGhostLevels", 0);
    }
  return myNewGrid;
}

//-------------------------------------------------------------------------
char *vtkDistributedDataFilter::MarshallDataSet(vtkUnstructuredGrid *extractedGrid, int &len)
{
  // taken from vtkCommunicator::WriteDataSet

  vtkUnstructuredGrid *copy;
  vtkDataSetWriter *writer = vtkDataSetWriter::New();

  copy = extractedGrid->NewInstance();
  copy->ShallowCopy(extractedGrid);

  // There is a problem with binary files with no data.
  if (copy->GetNumberOfCells() > 0)
    {
    writer->SetFileTypeToBinary();
    }
  writer->WriteToOutputStringOn();
  writer->SetInputData(copy);

  writer->Write();

  len = writer->GetOutputStringLength();

  char *packedFormat = writer->RegisterAndGetOutputString();

  writer->Delete();

  copy->Delete();

  return packedFormat;
}

//-------------------------------------------------------------------------
vtkUnstructuredGrid *vtkDistributedDataFilter::UnMarshallDataSet(char *buf, int size)
{
  // taken from vtkCommunicator::ReadDataSet

  vtkDataSetReader *reader = vtkDataSetReader::New();

  reader->ReadFromInputStringOn();

  vtkCharArray* mystring = vtkCharArray::New();

  mystring->SetArray(buf, size, 1);

  reader->SetInputArray(mystring);
  mystring->Delete();

  vtkDataSet *output = reader->GetOutput();
  reader->Update();

  vtkUnstructuredGrid *newGrid = vtkUnstructuredGrid::New();

  newGrid->ShallowCopy(output);

  reader->Delete();

  return newGrid;
}

//-------------------------------------------------------------------------
vtkUnstructuredGrid
  *vtkDistributedDataFilter::ExtractCells(vtkIdList *cells, int deleteCellLists,
                                vtkDataSet *in)
{
  vtkIdList *tempCellList = NULL;

  if (cells == NULL)
    {
    // We'll get a zero cell unstructured grid which matches the input grid
    tempCellList = vtkIdList::New();
    }
  else
    {
    tempCellList = cells;
    }

  vtkUnstructuredGrid *subGrid = vtkDistributedDataFilter::ExtractCells(
    &tempCellList, 1, deleteCellLists, in);

  if (tempCellList != cells)
    {
    tempCellList->Delete();
    }

  return subGrid;
}

//-------------------------------------------------------------------------
vtkUnstructuredGrid
  *vtkDistributedDataFilter::ExtractCells(vtkIdList **cells, int nlists,
                 int deleteCellLists, vtkDataSet *in)
{
  vtkDataSet* tmpInput = in->NewInstance();
  tmpInput->ShallowCopy(in);

  vtkExtractCells *extCells = vtkExtractCells::New();

  extCells->SetInputData(tmpInput);

  for (int i=0; i<nlists; i++)
    {
    if (cells[i])
      {
      extCells->AddCellList(cells[i]);

      if (deleteCellLists)
        {
        cells[i]->Delete();
        }
      }
    }

  extCells->Update();

  // If this process has no cells for these regions, a ugrid gets
  // created anyway with field array information

  vtkUnstructuredGrid *keepGrid = vtkUnstructuredGrid::New();
  keepGrid->ShallowCopy(extCells->GetOutput());

  extCells->Delete();

  tmpInput->Delete();

  return keepGrid;
}

//-------------------------------------------------------------------------
vtkUnstructuredGrid
  *vtkDistributedDataFilter::ExtractZeroCellGrid(vtkDataSet *in)
{
  vtkDataSet* tmpInput = in->NewInstance();
  tmpInput->ShallowCopy(in);

  vtkExtractCells *extCells = vtkExtractCells::New();

  extCells->SetInputData(tmpInput);

  extCells->Update();   // extract no cells

  vtkUnstructuredGrid *keepGrid = vtkUnstructuredGrid::New();
  keepGrid->ShallowCopy(extCells->GetOutput());

  extCells->Delete();

  tmpInput->Delete();

  return keepGrid;
}

//-------------------------------------------------------------------------

// To save on storage, we return actual pointers into the vtkKdTree's lists
// of cell IDs.  So don't free the memory they are pointing to.
// vtkKdTree::DeleteCellLists will delete them all when we're done.

vtkIdList **vtkDistributedDataFilter::GetCellIdsForProcess(int proc, int *nlists)
{
  *nlists = 0;

  vtkIntArray *regions = vtkIntArray::New();

  int nregions = this->Kdtree->GetRegionAssignmentList(proc, regions);

  if (nregions == 0)
    {
    return NULL;
    }

  *nlists = nregions;

  if (this->IncludeAllIntersectingCells)
    {
    *nlists *= 2;
    }

  vtkIdList **lists = new vtkIdList * [*nlists];

  int nextList = 0;

  for (int reg=0; reg < nregions; reg++)
    {
    lists[nextList++] = this->Kdtree->GetCellList(regions->GetValue(reg));

    if (this->IncludeAllIntersectingCells)
      {
      lists[nextList++] = this->Kdtree->GetBoundaryCellList(regions->GetValue(reg));
      }
    }

  regions->Delete();

  return lists;
}

//==========================================================================
// Code related to clipping cells to the spatial region

//-------------------------------------------------------------------------
static int insideBoxFunction(vtkIdType cellId, vtkUnstructuredGrid *grid, void *data)
{
  char *arrayName = (char *)data;

  vtkDataArray *da= grid->GetCellData()->GetArray(arrayName);
  vtkUnsignedCharArray *inside = vtkUnsignedCharArray::SafeDownCast(da);

  unsigned char where = inside->GetValue(cellId);

  return where;   // 1 if cell is inside spatial region, 0 otherwise
}

//-------------------------------------------------------------------------
void vtkDistributedDataFilter::AddConstantUnsignedCharPointArray(
  vtkUnstructuredGrid *grid, const char *arrayName, unsigned char val)
{
  vtkIdType npoints = grid->GetNumberOfPoints();

  unsigned char *vals = new unsigned char [npoints];

  memset(vals, val, npoints);

  vtkUnsignedCharArray *Array = vtkUnsignedCharArray::New();
  Array->SetName(arrayName);
  Array->SetArray(vals, npoints, 0);

  grid->GetPointData()->AddArray(Array);

  Array->Delete();
}

//-------------------------------------------------------------------------
void vtkDistributedDataFilter::AddConstantUnsignedCharCellArray(
  vtkUnstructuredGrid *grid, const char *arrayName, unsigned char val)
{
  vtkIdType ncells = grid->GetNumberOfCells();

  unsigned char *vals = new unsigned char [ncells];

  memset(vals, val, ncells);

  vtkUnsignedCharArray *Array = vtkUnsignedCharArray::New();
  Array->SetName(arrayName);
  Array->SetArray(vals, ncells, 0);

  grid->GetCellData()->AddArray(Array);

  Array->Delete();
}

//-------------------------------------------------------------------------
// this is here temporarily, until vtkBoxClipDataSet is fixed to
// be able to generate the clipped output
void vtkDistributedDataFilter::ClipWithVtkClipDataSet(
           vtkUnstructuredGrid *grid, double *bounds,
           vtkUnstructuredGrid **outside, vtkUnstructuredGrid **inside)
{
  vtkUnstructuredGrid *in;
  vtkUnstructuredGrid *out ;

  vtkClipDataSet *clipped = vtkClipDataSet::New();

  vtkBox *box = vtkBox::New();
  box->SetBounds(bounds);

  clipped->SetClipFunction(box);
  box->Delete();
  clipped->SetValue(0.0);
  clipped->InsideOutOn();

  clipped->SetInputData(grid);

  if (outside)
    {
    clipped->GenerateClippedOutputOn();
    }

  clipped->Update();

  if (outside)
    {
    out = clipped->GetClippedOutput();
    out->Register(this);
    *outside = out;
    }

  in = clipped->GetOutput();
  in->Register(this);
  *inside = in;


  clipped->Delete();
}

//-------------------------------------------------------------------------
// In general, vtkBoxClipDataSet is much faster and makes fewer errors.
void vtkDistributedDataFilter::ClipWithBoxClipDataSet(
           vtkUnstructuredGrid *grid, double *bounds,
           vtkUnstructuredGrid **outside, vtkUnstructuredGrid **inside)
{
  vtkUnstructuredGrid *in;
  vtkUnstructuredGrid *out ;

  vtkBoxClipDataSet *clipped = vtkBoxClipDataSet::New();

  clipped->SetBoxClip(bounds[0], bounds[1],
                      bounds[2], bounds[3], bounds[4], bounds[5]);

  clipped->SetInputData(grid);

  if (outside)
    {
    clipped->GenerateClippedOutputOn();
    }

  clipped->Update();

  if (outside)
    {
    out = clipped->GetClippedOutput();
    out->Register(this);
    *outside = out;
    }

  in = clipped->GetOutput();
  in->Register(this);
  *inside = in;

  clipped->Delete();
}

//-------------------------------------------------------------------------
void vtkDistributedDataFilter::ClipCellsToSpatialRegion(vtkUnstructuredGrid *grid)
{
  this->ComputeMyRegionBounds();

  if (this->NumConvexSubRegions > 1)
    {
    // here we would need to divide the grid into a separate grid for
    // each convex region, and then do the clipping

    vtkErrorMacro(<<
       "vtkDistributedDataFilter::ClipCellsToSpatialRegion - "
       "assigned regions do not form a single convex region");

    return ;
    }

  double *bounds = this->ConvexSubRegionBounds;

  if (this->GhostLevel > 0)
    {
    // We need cells outside the clip box as well.

    vtkUnstructuredGrid *outside;
    vtkUnstructuredGrid *inside;

#if 1
    this->ClipWithBoxClipDataSet(grid, bounds, &outside, &inside);
#else
    this->ClipWithVtkClipDataSet(grid, bounds, &outside, &inside);
#endif

    grid->Initialize();

    // Mark the outside cells with a 0, the inside cells with a 1.

    int arrayNameLen = static_cast<int>(strlen(TEMP_INSIDE_BOX_FLAG));
    char *arrayName = new char [arrayNameLen + 1];
    strcpy(arrayName, TEMP_INSIDE_BOX_FLAG);
    vtkDistributedDataFilter::AddConstantUnsignedCharCellArray(outside, arrayName, 0);
    vtkDistributedDataFilter::AddConstantUnsignedCharCellArray(inside, arrayName, 1);

    // Combine inside and outside into a single ugrid.

    vtkDataSet *grids[2];
    grids[0] = inside;
    grids[1] = outside;

    vtkUnstructuredGrid *combined =
      vtkDistributedDataFilter::MergeGrids(grids, 2,  DeleteYes, 0,
                                           (float)this->Kdtree->GetFudgeFactor(), 0);

    // Extract the piece inside the box (level 0) and the requested
    // number of levels of ghost cells.

    vtkExtractUserDefinedPiece *ep = vtkExtractUserDefinedPiece::New();

    ep->SetConstantData(arrayName, arrayNameLen + 1);
    ep->SetPieceFunction(insideBoxFunction);
    ep->CreateGhostCellsOn();

    ep->GetExecutive()->GetOutputInformation(0)->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
      this->GhostLevel);
    ep->SetInputData(combined);

    ep->Update();

    grid->ShallowCopy(ep->GetOutput());
    grid->GetCellData()->RemoveArray(arrayName);

    ep->Delete();
    combined->Delete();

    delete [] arrayName;
    }
  else
    {
    vtkUnstructuredGrid *inside;

#if 1
    this->ClipWithBoxClipDataSet(grid, bounds, NULL, &inside);
#else
    this->ClipWithVtkClipDataSet(grid, bounds, NULL, &inside);
#endif

    grid->ShallowCopy(inside);
    inside->Delete();
    }

  return;
}

//==========================================================================
// Code related to assigning global node IDs and cell IDs

//-------------------------------------------------------------------------
int vtkDistributedDataFilter::AssignGlobalNodeIds(vtkUnstructuredGrid *grid)
{
  int nprocs = this->NumProcesses;
  int pid;
  vtkIdType ptId;
  vtkIdType nGridPoints = grid->GetNumberOfPoints();

  vtkIdType *numPointsOutside = new vtkIdType [nprocs];
  memset(numPointsOutside, 0, sizeof(vtkIdType) * nprocs);

  vtkIdTypeArray *globalIds = vtkIdTypeArray::New();
  globalIds->SetNumberOfValues(nGridPoints);
  globalIds->SetName(TEMP_NODE_ID_NAME);

  // 1. Count the points in grid which lie within my assigned spatial region

  vtkIdType myNumPointsInside = 0;

  for (ptId = 0; ptId < nGridPoints; ptId++)
    {
    double *pt = grid->GetPoints()->GetPoint(ptId);

    if (this->InMySpatialRegion(pt[0], pt[1], pt[2]))
      {
      globalIds->SetValue(ptId, 0);  // flag it as mine
      myNumPointsInside++;
      }
    else
      {
      // Well, whose region is this point in?

      int regionId = this->Kdtree->GetRegionContainingPoint(pt[0],pt[1],pt[2]);

      pid = this->Kdtree->GetProcessAssignedToRegion(regionId);

      numPointsOutside[pid]++;

      pid += 1;
      pid *= -1;

      globalIds->SetValue(ptId, pid);  // a flag
      }
    }

  // 2. Gather and Broadcast this number of "Inside" points for each process.

  vtkIdTypeArray *numPointsInside = this->ExchangeCounts(myNumPointsInside, 0x0013);

  // 3. Assign global Ids to the points inside my spatial region

  vtkIdType firstId = 0;
  vtkIdType numGlobalIdsSoFar = 0;

  for (pid = 0; pid < nprocs; pid++)
    {
    if (pid < this->MyId)
      {
      firstId += numPointsInside->GetValue(pid);
      }
    numGlobalIdsSoFar += numPointsInside->GetValue(pid);
    }

  numPointsInside->Delete();

  for (ptId = 0; ptId < nGridPoints; ptId++)
    {
    if (globalIds->GetValue(ptId) == 0)
      {
      globalIds->SetValue(ptId, firstId++);
      }
    }

  // -----------------------------------------------------------------
  // All processes have assigned global IDs to the points in their grid
  // which lie within their assigned spatial region.
  // Now they have to get the IDs for the
  // points in their grid which lie outside their region, and which
  // are within the spatial region of another process.
  // -----------------------------------------------------------------

  // 4. For every other process, build a list of points I have
  // which are in the region of that process.  In practice, the
  // processes for which I need to request points IDs should be
  // a small subset of all the other processes.

  // question: if the vtkPointArray has type double, should we
  // send doubles instead of floats to insure we get the right
  // global ID back?

  vtkFloatArray **ptarrayOut = new vtkFloatArray * [nprocs];
  memset(ptarrayOut, 0, sizeof(vtkFloatArray *) * nprocs);

  vtkIdTypeArray **localIds     = new vtkIdTypeArray * [nprocs];
  memset(localIds, 0, sizeof(vtkIdTypeArray *) * nprocs);

  vtkIdType *next = new vtkIdType [nprocs];
  vtkIdType *next3 = new vtkIdType [nprocs];

  for (ptId = 0; ptId < nGridPoints; ptId++)
    {
    pid = globalIds->GetValue(ptId);

    if (pid >= 0)
      {
      continue;   // that's one of mine
      }

    pid *= -1;
    pid -= 1;

    if (ptarrayOut[pid] == NULL)
      {
      vtkIdType npoints = numPointsOutside[pid];

      ptarrayOut[pid] = vtkFloatArray::New();
      ptarrayOut[pid]->SetNumberOfValues(npoints * 3);

      localIds[pid] = vtkIdTypeArray::New();
      localIds[pid]->SetNumberOfValues(npoints);

      next[pid] = 0;
      next3[pid] = 0;
      }

    localIds[pid]->SetValue(next[pid]++, ptId);

    double *dp = grid->GetPoints()->GetPoint(ptId);

    ptarrayOut[pid]->SetValue(next3[pid]++, (float)dp[0]);
    ptarrayOut[pid]->SetValue(next3[pid]++, (float)dp[1]);
    ptarrayOut[pid]->SetValue(next3[pid]++, (float)dp[2]);
    }

  delete [] numPointsOutside;
  delete [] next;
  delete [] next3;

  // 5. Do pairwise exchanges of the points we want global IDs for,
  //    and delete outgoing point arrays.

  vtkFloatArray **ptarrayIn = this->ExchangeFloatArrays(ptarrayOut,
              DeleteYes, 0x0014);

  // 6. Find the global point IDs that have been requested of me,
  //    and delete incoming point arrays.  Count "missing points":
  //    the number of unique points I receive which are not in my
  //    grid (this may happen if IncludeAllIntersectingCells is OFF).

  vtkIdType myNumMissingPoints = 0;

  vtkIdTypeArray **idarrayOut =
    this->FindGlobalPointIds(ptarrayIn, globalIds, grid, myNumMissingPoints);

  vtkIdTypeArray *missingCount = this->ExchangeCounts(myNumMissingPoints, 0x0015);

  if (this->IncludeAllIntersectingCells == 1)
    {
    // Make sure all points were found

    int aok = 1;
    for (pid=0; pid<nprocs; pid++)
      {
      if (missingCount->GetValue(pid) > 0)
        {
         vtkErrorMacro(<<
          "vtkDistributedDataFilter::AssignGlobalNodeIds bad point");
        aok = 0;
        break;
        }
      }
    if (!aok)
      {
      this->FreeIntArrays(idarrayOut);
      this->FreeIntArrays(localIds);
      missingCount->Delete();
      globalIds->Delete();

      return 1;
      }
    }

  // 7. Do pairwise exchanges of the global point IDs, and delete the
  //    outgoing point ID arrays.

  vtkIdTypeArray **idarrayIn = this->ExchangeIdArrays(idarrayOut,
                    DeleteYes, 0x0016);

  // 8. It's possible (if IncludeAllIntersectingCells is OFF) that some
  //    processes had "missing points".  Process A has a point P in it's
  //    grid which lies in the spatial region of process B.  But P is not
  //    in process B's grid.  We need to assign global IDs to these points
  //    too.

  vtkIdType *missingId = new vtkIdType [nprocs];

  if (this->IncludeAllIntersectingCells == 0)
    {
    missingId[0] = numGlobalIdsSoFar;

    for (pid = 1; pid < nprocs; pid++)
      {
      int prev = pid - 1;
      missingId[pid] = missingId[prev] + missingCount->GetValue(prev);
      }
    }

  missingCount->Delete();

  // 9. Update my ugrid with these mutually agreed upon global point IDs

  for (pid = 0; pid < nprocs; pid++)
    {
    if (idarrayIn[pid] == NULL)
      {
      continue;
      }

    vtkIdType count = idarrayIn[pid]->GetNumberOfTuples();

    for (ptId = 0; ptId < count; ptId++)
      {
      vtkIdType myLocalId = localIds[pid]->GetValue(ptId);
      vtkIdType yourGlobalId = idarrayIn[pid]->GetValue(ptId);

      if (yourGlobalId >= 0)
        {
        globalIds->SetValue(myLocalId, yourGlobalId);
        }
      else
        {
        vtkIdType ptIdOffset = yourGlobalId * -1;
        ptIdOffset -= 1;

        globalIds->SetValue(myLocalId, missingId[pid] + ptIdOffset);
        }
      }
    localIds[pid]->Delete();
    idarrayIn[pid]->Delete();
    }

  delete [] localIds;
  delete [] idarrayIn;
  delete [] missingId;

  grid->GetPointData()->SetGlobalIds(globalIds);
  globalIds->Delete();

  return 0;
}

//-------------------------------------------------------------------------
// If grids were distributed with IncludeAllIntersectingCells OFF, it's
// possible there are points in my spatial region that are not in my
// grid.  They need global Ids, so I will keep track of how many such unique
// points I receive from other processes, and will assign them temporary
// IDs.  They will get permanent IDs later on.

vtkIdTypeArray **vtkDistributedDataFilter::FindGlobalPointIds(
     vtkFloatArray **ptarray, vtkIdTypeArray *ids, vtkUnstructuredGrid *grid,
     vtkIdType &numUniqueMissingPoints)
{
  int nprocs = this->NumProcesses;
  vtkIdTypeArray **gids = new vtkIdTypeArray * [nprocs];

  if (grid->GetNumberOfCells() == 0)
    {
    // There are no cells in my assigned region

    memset(gids, 0, sizeof(vtkIdTypeArray *) * nprocs);

    return gids;
    }

  vtkKdTree *kd = vtkKdTree::New();

  kd->BuildLocatorFromPoints(grid->GetPoints());

  int procId;
  vtkIdType ptId, localId;

  vtkPointLocator *pl = NULL;
  vtkPoints *missingPoints = NULL;

  if (this->IncludeAllIntersectingCells == 0)
    {
    this->ComputeMyRegionBounds();
    pl = vtkPointLocator::New();
    pl->SetTolerance(this->Kdtree->GetFudgeFactor());
    missingPoints = vtkPoints::New();
    pl->InitPointInsertion(missingPoints, this->ConvexSubRegionBounds);
    }

  for (procId = 0; procId < nprocs; procId++)
    {
    if ((ptarray[procId] == NULL) ||
        (ptarray[procId]->GetNumberOfTuples() == 0))
      {
      gids[procId] = NULL;
      if (ptarray[procId]) ptarray[procId]->Delete();
      continue;
      }

    gids[procId] = vtkIdTypeArray::New();

    vtkIdType npoints = ptarray[procId]->GetNumberOfTuples() / 3;

    gids[procId]->SetNumberOfValues(npoints);
    vtkIdType next = 0;

    float *pt = ptarray[procId]->GetPointer(0);

    for (ptId = 0; ptId < npoints; ptId++)
      {
      localId = kd->FindPoint((double)pt[0], (double)pt[1], (double)pt[2]);

      if (localId >= 0)
        {
        gids[procId]->SetValue(next++, ids->GetValue(localId));  // global Id
        }
      else
        {
        // This point is not in my grid

        if (this->IncludeAllIntersectingCells)
          {
          // This is an error
          gids[procId]->SetValue(next++, -1);
          numUniqueMissingPoints++;
          }
        else
          {
          // Flag these with a negative point ID.  We'll assign
          // them real point IDs later.

          vtkIdType nextId;
          double dpt[3];
          dpt[0] = pt[0]; dpt[1] = pt[1]; dpt[2] = pt[2];
          pl->InsertUniquePoint(dpt, nextId);

          nextId += 1;
          nextId *= -1;
          gids[procId]->SetValue(next++, nextId);
          }
        }
      pt += 3;
      }

    ptarray[procId]->Delete();
    }

  delete [] ptarray;

  kd->Delete();

  if (missingPoints)
    {
    numUniqueMissingPoints = missingPoints->GetNumberOfPoints();
    missingPoints->Delete();
    pl->Delete();
    }

  return gids;
}

//-------------------------------------------------------------------------
int vtkDistributedDataFilter::AssignGlobalElementIds(vtkDataSet *in)
{
  vtkIdType i;
  vtkIdType myNumCells = in->GetNumberOfCells();
  vtkIdTypeArray *numCells = this->ExchangeCounts(myNumCells, 0x0017);

  vtkIdTypeArray *globalCellIds = vtkIdTypeArray::New();
  globalCellIds->SetNumberOfValues(myNumCells);
  //DDM - do we need to mark this as the GID array?
  globalCellIds->SetName(TEMP_ELEMENT_ID_NAME);

  vtkIdType StartId = 0;

  for (i=0; i < this->MyId; i++)
    {
    StartId += numCells->GetValue(i);
    }

  numCells->Delete();

  for (i=0; i<myNumCells; i++)
    {
    globalCellIds->SetValue(i, StartId++);
    }

  in->GetCellData()->SetGlobalIds(globalCellIds);

  globalCellIds->Delete();

  return 0;
}

//========================================================================
// Code related to acquiring ghost cells

//-------------------------------------------------------------------------
int vtkDistributedDataFilter::InMySpatialRegion(float x, float y, float z)
{
  return this->InMySpatialRegion((double)x, (double)y, (double)z);
}
int vtkDistributedDataFilter::InMySpatialRegion(double x, double y, double z)
{
  this->ComputeMyRegionBounds();

  double *box = this->ConvexSubRegionBounds;

  if (!box)
    {
    return 0;
    }

  // To avoid ambiguity, a point on a boundary is assigned to
  // the region for which it is on the upper boundary.  Or
  // (in one dimension) the region between points A and B
  // contains all points p such that A < p <= B.

  if ( (x <= box[0]) || (x > box[1]) ||
       (y <= box[2]) || (y > box[3]) ||
       (z <= box[4]) || (z > box[5])   )
    {
      return 0;
    }

  return 1;
}

//-----------------------------------------------------------------------
int vtkDistributedDataFilter::StrictlyInsideMyBounds(float x, float y, float z)
{
  return this->StrictlyInsideMyBounds((double)x, (double)y, (double)z);
}

//-----------------------------------------------------------------------
int vtkDistributedDataFilter::StrictlyInsideMyBounds(double x, double y, double z)
{
  this->ComputeMyRegionBounds();

  double *box = this->ConvexSubRegionBounds;

  if (!box)
    {
    return 0;
    }

  if ( (x <= box[0]) || (x >= box[1]) ||
       (y <= box[2]) || (y >= box[3]) ||
       (z <= box[4]) || (z >= box[5])   )
    {
      return 0;
    }

  return 1;
}

//-----------------------------------------------------------------------
vtkIdTypeArray **vtkDistributedDataFilter::MakeProcessLists(
                                    vtkIdTypeArray **pointIds,
                                    vtkDistributedDataFilterSTLCloak *procs)
{
  // Build a list of pointId/processId pairs for each process that
  // sent me point IDs.  The process Ids are all those processes
  // that had the specified point in their ghost level zero grid.

  int nprocs = this->NumProcesses;

  std::multimap<int, int>::iterator mapIt;

  vtkIdTypeArray **processList = new vtkIdTypeArray * [nprocs];
  memset(processList, 0, sizeof (vtkIdTypeArray *) * nprocs);

  for (int i=0; i<nprocs; i++)
    {
    if (pointIds[i] == NULL)
      {
      continue;
      }

    vtkIdType size = pointIds[i]->GetNumberOfTuples();

    if (size > 0)
      {
      for (vtkIdType j=0; j<size; )
        {
        // These are all the points in my spatial region
        // for which process "i" needs ghost cells.

        vtkIdType gid = pointIds[i]->GetValue(j);
        vtkIdType ncells = pointIds[i]->GetValue(j+1);

        mapIt = procs->IntMultiMap.find(gid);

        while (mapIt != procs->IntMultiMap.end() && mapIt->first == gid)
          {
          int processId = mapIt->second;

          if (processId != i)
            {
            // Process "i" needs to know that process
            // "processId" also has cells using this point

            if (processList[i] == NULL)
              {
              processList[i] = vtkIdTypeArray::New();
              }
            processList[i]->InsertNextValue(gid);
            processList[i]->InsertNextValue(processId);
            }
          ++mapIt;
          }
        j += (2 + ncells);
        }
      }
    }

  return processList;
}

//-----------------------------------------------------------------------
vtkIdTypeArray *vtkDistributedDataFilter::AddPointAndCells(
                        vtkIdType gid, vtkIdType localId, vtkUnstructuredGrid *grid,
                        vtkIdType *gidCells, vtkIdTypeArray *ids)
{
  if (ids == NULL)
    {
    ids = vtkIdTypeArray::New();
    }

  ids->InsertNextValue(gid);

  vtkIdList *cellList = vtkIdList::New();

  grid->GetPointCells(localId, cellList);

  vtkIdType numCells = cellList->GetNumberOfIds();

  ids->InsertNextValue(numCells);

  for (vtkIdType j=0; j<numCells; j++)
    {
    vtkIdType globalCellId = gidCells[cellList->GetId(j)];
    ids->InsertNextValue(globalCellId);
    }

  cellList->Delete();

  return ids;
}

//-----------------------------------------------------------------------
vtkIdTypeArray **vtkDistributedDataFilter::GetGhostPointIds(
  int ghostLevel, vtkUnstructuredGrid *grid,
  int AddCellsIAlreadyHave)
{
  int nprocs = this->NumProcesses;
  int me = this->MyId;
  vtkIdType numPoints = grid->GetNumberOfPoints();

  vtkIdTypeArray **ghostPtIds = new vtkIdTypeArray * [nprocs];
  memset(ghostPtIds, 0, sizeof(vtkIdTypeArray *) * nprocs);

  if (numPoints < 1)
    {
    return ghostPtIds;
    }

  int processId = -1;
  int regionId = -1;

  vtkPKdTree *kd = this->Kdtree;

  vtkPoints *pts = grid->GetPoints();

  vtkIdType *gidsPoint = this->GetGlobalNodeIds(grid);
  vtkIdType *gidsCell = this->GetGlobalElementIds(grid);


  vtkDataArray *da = grid->GetPointData()->GetArray("vtkGhostLevels");
  vtkUnsignedCharArray *uca = vtkUnsignedCharArray::SafeDownCast(da);
  unsigned char *levels = uca->GetPointer(0);

  unsigned char level = (unsigned char)(ghostLevel - 1);

  for (vtkIdType i=0; i<numPoints; i++)
    {
    double *pt = pts->GetPoint(i);
    regionId = kd->GetRegionContainingPoint(pt[0], pt[1], pt[2]);
    processId = kd->GetProcessAssignedToRegion(regionId);

    if (ghostLevel == 1)
      {
      // I want all points that are outside my spatial region

      if (processId == me)
        {
        continue;
        }

      // Don't include points that are not part of any cell

      int used = vtkDistributedDataFilter::LocalPointIdIsUsed(grid, i);

      if (!used)
        {
        continue;
        }
      }
    else if (levels[i] != level)
      {
      continue; // I want all points having the correct ghost level
      }

    vtkIdType gid = gidsPoint[i];

    if (AddCellsIAlreadyHave)
      {
      // To speed up exchange of ghost cells and creation of
      // new ghost cell grid, we tell other
      // processes which cells we already have, so they don't
      // send them to us.

      ghostPtIds[processId] =
        vtkDistributedDataFilter::AddPointAndCells(gid, i, grid, gidsCell,
                                       ghostPtIds[processId]);
      }
    else
      {
      if (ghostPtIds[processId] == NULL)
        {
        ghostPtIds[processId] = vtkIdTypeArray::New();
        }
      ghostPtIds[processId]->InsertNextValue(gid);
      ghostPtIds[processId]->InsertNextValue(0);
      }
    }
  return ghostPtIds;
}

//-----------------------------------------------------------------------
int vtkDistributedDataFilter::LocalPointIdIsUsed(
                              vtkUnstructuredGrid *grid, int ptId)
{
  int used = 1;

  int numPoints = grid->GetNumberOfPoints();

  if ((ptId < 0) || (ptId >= numPoints))
    {
    used = 0;
    }
  else
    {
    vtkIdType id = (vtkIdType)ptId;
    vtkIdList *cellList = vtkIdList::New();

    grid->GetPointCells(id, cellList);

    if (cellList->GetNumberOfIds() == 0)
      {
      used = 0;
      }

    cellList->Delete();
    }

  return used;
}

//-----------------------------------------------------------------------
int vtkDistributedDataFilter::GlobalPointIdIsUsed(vtkUnstructuredGrid *grid,
                    int ptId, vtkDistributedDataFilterSTLCloak *globalToLocal)
{
  int used = 1;

  std::map<int, int>::iterator mapIt;

  mapIt = globalToLocal->IntMap.find(ptId);

  if (mapIt == globalToLocal->IntMap.end())
    {
    used = 0;
    }
  else
    {
    int id = mapIt->second;

    used = vtkDistributedDataFilter::LocalPointIdIsUsed(grid, id);
    }

  return used;
}

//-----------------------------------------------------------------------
vtkIdType vtkDistributedDataFilter::FindId(vtkIdTypeArray *ids, vtkIdType gid, vtkIdType startLoc)
{
  vtkIdType gidLoc = -1;

  if (ids == NULL)
    {
    return gidLoc;
    }

  vtkIdType numIds = ids->GetNumberOfTuples();

  while ((ids->GetValue(startLoc) != gid) && (startLoc < numIds))
    {
    vtkIdType ncells = ids->GetValue(++startLoc);
    startLoc += (ncells + 1);
    }

  if (startLoc < numIds)
    {
    gidLoc = startLoc;
    }

  return gidLoc;
}

//-----------------------------------------------------------------------
// We create an expanded grid with the required number of ghost
// cells.  This is for the case where IncludeAllIntersectingCells is OFF.
// This means that when the grid was redistributed, each cell was
// uniquely assigned to one process, the process owning the spatial
// region that the cell's centroid lies in.
vtkUnstructuredGrid *
vtkDistributedDataFilter::AddGhostCellsUniqueCellAssignment(
                               vtkUnstructuredGrid *myGrid,
                               vtkDistributedDataFilterSTLCloak *globalToLocalMap)
{
  int i,j,k;
  int ncells=0;
  int processId=0;
  int gid=0;
  vtkIdType size=0;

  int nprocs = this->NumProcesses;
  int me = this->MyId;

  int gl = 1;

  // For each ghost level, processes request and send ghost cells

  vtkUnstructuredGrid *newGhostCellGrid = NULL;
  vtkIdTypeArray **ghostPointIds = NULL;

  vtkDistributedDataFilterSTLCloak *insidePointMap =
    new vtkDistributedDataFilterSTLCloak;
  std::multimap<int, int>::iterator mapIt;

  while (gl <= this->GhostLevel)
    {
    // For ghost level 1, create a list for each process (not
    // including me) of all points I have in that process'
    // assigned region.  We use this list for two purposes:
    // (1) to build a list on each process of all other processes
    // that have cells containing points in our region, (2)
    // these are some of the points that we need ghost cells for.
    //
    // For ghost level above 1, create a list for each process
    // (including me) of all my points in that process' assigned
    // region for which I need ghost cells.

    if (gl == 1)
      {
      ghostPointIds = this->GetGhostPointIds(gl, myGrid, 0);
      }
    else
      {
      ghostPointIds = this->GetGhostPointIds(gl, newGhostCellGrid, 1);
      }

    // Exchange these lists.

    vtkIdTypeArray **insideIds =
      this->ExchangeIdArrays(ghostPointIds, DeleteNo,
                              0x0018);

    if (gl == 1)
      {
      // For every point in my region that was sent to me by another process,
      // I now know the identity of all processes having cells containing
      // that point.  Begin by building a mapping from point IDs to the IDs
      // of processes that sent me that point.

      for (i=0; i<nprocs; i++)
        {
        if (insideIds[i] == NULL)
          {
          continue;
          }

        size = insideIds[i]->GetNumberOfTuples();

        if (size > 0)
          {
          for (j=0; j<size; j+=2)
            {
            // map global point id to process ids
            const int id = (int)insideIds[i]->GetValue(j);
            insidePointMap->IntMultiMap.insert(std::pair<const int, int>(id, i));
            }
          }
        }
      }

    // Build a list of pointId/processId pairs for each process that
    // sent me point IDs.  To process P, for every point ID sent to me
    // by P, I send the ID of every other process (not including myself
    // and P) that has cells in it's ghost level 0 grid which use
    // this point.

    vtkIdTypeArray **processListSent
      = this->MakeProcessLists(insideIds, insidePointMap);

    // Exchange these new lists.

    vtkIdTypeArray **processList =
      this->ExchangeIdArrays(processListSent, DeleteYes,
                              0x0019);

    // I now know the identity of every process having cells containing
    // points I need ghost cells for.  Create a request to each process
    // for these cells.

    vtkIdTypeArray **ghostCellsPlease = new vtkIdTypeArray * [nprocs];
    for (i=0; i<nprocs; i++)
      {
      ghostCellsPlease[i] = vtkIdTypeArray::New();
      ghostCellsPlease[i]->SetNumberOfComponents(1);
      }

    for (i=0; i<nprocs; i++)
      {
      if (i == me)
        {
        continue;
        }

      if (ghostPointIds[i])       // points I have in your spatial region,
        {                         // maybe you have cells that use them?

        for (j=0; j<ghostPointIds[i]->GetNumberOfTuples(); j++)
          {
          ghostCellsPlease[i]->InsertNextValue(ghostPointIds[i]->GetValue(j));
          }
        }
      if (processList[i])         // other processes you say that also have
        {                         // cells using those points
        size = processList[i]->GetNumberOfTuples();
        vtkIdType *array = processList[i]->GetPointer(0);
        int nextLoc = 0;

        for (j=0; j < size; j += 2)
          {
          gid = array[j];
          processId = array[j+1];

          ghostCellsPlease[processId]->InsertNextValue(gid);

          if (gl > 1)
            {
            // add the list of cells I already have for this point

            int where =
              vtkDistributedDataFilter::FindId(ghostPointIds[i], gid, nextLoc);

            if (where < 0)
              {
              // error really, not sure what to do
              nextLoc = 0;
              ghostCellsPlease[processId]->InsertNextValue(0);
              continue;
              }

            ncells = ghostPointIds[i]->GetValue(where + 1);

            ghostCellsPlease[processId]->InsertNextValue(ncells);

            for (k=0; k <ncells; k++)
              {
              vtkIdType cellId = ghostPointIds[i]->GetValue(where + 2 + k);
              ghostCellsPlease[processId]->InsertNextValue(cellId);
              }

            nextLoc = where;
            }
          else
            {
            ghostCellsPlease[processId]->InsertNextValue(0);
            }
          }
        }
      if ((gl==1) && insideIds[i])   // points you have in my spatial region,
        {                            // which I may need ghost cells for
        for (j=0; j<insideIds[i]->GetNumberOfTuples();)
          {
          gid = insideIds[i]->GetValue(j);
          int used = vtkDistributedDataFilter::GlobalPointIdIsUsed(
                                  myGrid, gid, globalToLocalMap);
          if (used)
            {
            ghostCellsPlease[i]->InsertNextValue(gid);
            ghostCellsPlease[i]->InsertNextValue(0);
            }

          ncells = insideIds[i]->GetValue(j+1);
          j += (ncells + 2);
          }
        }
      }

    if (gl > 1)
      {
      if (ghostPointIds[me])   // these points are actually inside my region
        {
        size = ghostPointIds[me]->GetNumberOfTuples();

        for (i=0; i<size;)
          {
          gid = ghostPointIds[me]->GetValue(i);
          ncells = ghostPointIds[me]->GetValue(i+1);

          mapIt = insidePointMap->IntMultiMap.find(gid);

          if (mapIt != insidePointMap->IntMultiMap.end())
            {
            while (mapIt->first == gid)
              {
              processId = mapIt->second;
              ghostCellsPlease[processId]->InsertNextValue(gid);
              ghostCellsPlease[processId]->InsertNextValue(ncells);

              for (k=0; k<ncells; k++)
                {
                vtkIdType cellId = ghostPointIds[me]->GetValue(i+1+k);
                ghostCellsPlease[processId]->InsertNextValue(cellId);
                }

              ++mapIt;
              }
            }
          i += (ncells + 2);
          }
        }
      }

    this->FreeIntArrays(ghostPointIds);
    this->FreeIntArrays(insideIds);
    this->FreeIntArrays(processList);

    // Exchange these ghost cell requests.

    vtkIdTypeArray **ghostCellRequest
      = this->ExchangeIdArrays(ghostCellsPlease, DeleteYes,
                                0x001a);

    // Build a list of cell IDs satisfying each request received.
    // Delete request arrays.

    vtkIdList **sendCellList =
      this->BuildRequestedGrids(ghostCellRequest, myGrid, globalToLocalMap);

    // Build subgrids and exchange them

    vtkUnstructuredGrid *incomingGhostCells = this->ExchangeMergeSubGrids(
             sendCellList, DeleteYes, myGrid, DeleteNo, DuplicateCellsNo,
             GhostCellsYes, 0x001b);

    delete [] sendCellList;

    // Set ghost level of new cells, and merge into grid of other
    // ghost cells received.

    newGhostCellGrid = this->SetMergeGhostGrid(newGhostCellGrid,
                              incomingGhostCells, gl, globalToLocalMap);

    this->UpdateProgress(this->NextProgressStep++ * this->ProgressIncrement);

    gl++;
  }

  delete insidePointMap;

  vtkUnstructuredGrid *newGrid = NULL;

  if (newGhostCellGrid && (newGhostCellGrid->GetNumberOfCells() > 0))
    {
    vtkDataSet *grids[2];

    grids[0] = myGrid;
    grids[1] = newGhostCellGrid;

    int useGlobalNodeIds = (this->GetGlobalNodeIds(myGrid)?1:0);

    newGrid =
      vtkDistributedDataFilter::MergeGrids(grids, 2, DeleteYes, useGlobalNodeIds, 0, 0);
    }
  else
    {
    newGrid = myGrid;
    }

  return newGrid;
}

//-----------------------------------------------------------------------
// We create an expanded grid that contains the ghost cells we need.
// This is in the case where IncludeAllIntersectingCells is ON.  This
// is easier in some respects because we know if that if a point lies
// in a region owned by a particular process, that process has all
// cells which use that point.  So it is easy to find ghost cells.
// On the otherhand, because cells are not uniquely assigned to regions,
// we may get multiple processes sending us the same cell, so we
// need to filter these out.
vtkUnstructuredGrid *
vtkDistributedDataFilter::AddGhostCellsDuplicateCellAssignment(
                             vtkUnstructuredGrid *myGrid,
                             vtkDistributedDataFilterSTLCloak *globalToLocalMap)
{
  int i,j;

  int nprocs = this->NumProcesses;
  int me = this->MyId;

  int gl = 1;

  // For each ghost level, processes request and send ghost cells

  vtkUnstructuredGrid *newGhostCellGrid = NULL;
  vtkIdTypeArray **ghostPointIds = NULL;
  vtkIdTypeArray **extraGhostPointIds = NULL;

  std::map<int, int>::iterator mapIt;

  vtkPoints *pts = myGrid->GetPoints();

  while (gl <= this->GhostLevel)
    {
    // For ghost level 1, create a list for each process of points
    // in my grid which lie in that other process' spatial region.
    // This is normally all the points for which I need ghost cells,
    // with one EXCEPTION.  If a cell is axis-aligned, and a face of
    // the cell is on my upper boundary, then the vertices of this
    // face are in my spatial region, but I need their ghost cells.
    // I can detect this case when the process across the boundary
    // sends me a request for ghost cells of these points.
    //
    // For ghost level above 1, create a list for each process of
    // points in my ghost grid which are in that process' spatial
    // region and for which I need ghost cells.

    if (gl == 1)
      {
      ghostPointIds = this->GetGhostPointIds(gl, myGrid, 1);
      }
    else
      {
      ghostPointIds = this->GetGhostPointIds(gl, newGhostCellGrid, 1);
      }

    // Exchange these lists.

    vtkIdTypeArray **insideIds =
      this->ExchangeIdArrays(ghostPointIds, DeleteYes, 0x001c);

    // For ghost level 1, examine the points Ids I received from
    // other processes, to see if the exception described above
    // applies and I need ghost cells from them for those points.

    if (gl == 1)
      {
      vtkIdType *gidsCell = this->GetGlobalElementIds(myGrid);

      extraGhostPointIds = new vtkIdTypeArray * [nprocs];

      for (i=0; i<nprocs; i++)
        {
        extraGhostPointIds[i] = NULL;

        if (i == me)
          {
          continue;
          }

        if (insideIds[i] == NULL)
          {
          continue;
          }

        vtkIdType size = insideIds[i]->GetNumberOfTuples();

        for (j=0; j<size;)
          {
          vtkIdType gid = insideIds[i]->GetValue(j);
          vtkIdType ncells = insideIds[i]->GetValue(j+1);
          j += (ncells + 2);

          mapIt = globalToLocalMap->IntMap.find(gid);

          if (mapIt == globalToLocalMap->IntMap.end())
            {
            // This point must be right on my boundary, and
            // not connected to any cell intersecting my region.

            continue;
            }
          vtkIdType localId = mapIt->second;

          double *pt = pts->GetPoint(localId);

          int interior = this->StrictlyInsideMyBounds(pt[0], pt[1], pt[2]);

          if (!interior)
            {
            extraGhostPointIds[i] = this->AddPointAndCells(gid, localId,
                            myGrid, gidsCell, extraGhostPointIds[i]);
            }
          }
        }

      // Exchange these lists.

      vtkIdTypeArray **extraInsideIds =
        this->ExchangeIdArrays(extraGhostPointIds, DeleteYes, 0x001d);

      // Add the extra point ids to the previous list

      for (i=0; i<nprocs; i++)
        {
        if (i == me)
          {
          continue;
          }

        if (extraInsideIds[i])
          {
          vtkIdType size = extraInsideIds[i]->GetNumberOfTuples();

          if (insideIds[i] == NULL)
            {
            insideIds[i] = vtkIdTypeArray::New();
            }

          for (j=0; j<size; j++)
            {
            insideIds[i]->InsertNextValue(extraInsideIds[i]->GetValue(j));
            }
          }
        }
        this->FreeIntArrays(extraInsideIds);
      }

    // Build a list of cell IDs satisfying each request received.

    vtkIdList **sendCellList =
      this->BuildRequestedGrids(insideIds, myGrid, globalToLocalMap);

    // Build subgrids and exchange them

    vtkUnstructuredGrid *incomingGhostCells =
      this->ExchangeMergeSubGrids( sendCellList, DeleteYes, myGrid, DeleteNo,
                                   DuplicateCellsYes, GhostCellsYes, 0x001e);

    delete [] sendCellList;

    // Set ghost level of new cells, and merge into grid of other
    // ghost cells received.

    newGhostCellGrid = this->SetMergeGhostGrid(newGhostCellGrid,
                              incomingGhostCells, gl, globalToLocalMap);

    this->UpdateProgress(this->NextProgressStep++ * this->ProgressIncrement);

    gl++;
  }

  vtkUnstructuredGrid *newGrid = NULL;

  if (newGhostCellGrid && (newGhostCellGrid->GetNumberOfCells() > 0))
    {
    vtkDataSet *grids[2];

    grids[0] = myGrid;
    grids[1] = newGhostCellGrid;

    int useGlobalNodeIds = (this->GetGlobalNodeIds(myGrid)?1:0);
    newGrid =
      vtkDistributedDataFilter::MergeGrids(grids, 2, DeleteYes,
                                           useGlobalNodeIds, 0, 0);
    }
  else
    {
    newGrid = myGrid;
    }

  return newGrid;
}

//-----------------------------------------------------------------------
// For every process that sent me a list of point IDs, create a list
// of all the cells I have in my original grid containing those points.
// We omit cells the remote process already has.

vtkIdList **vtkDistributedDataFilter::BuildRequestedGrids(
                        vtkIdTypeArray **globalPtIds,
                        vtkUnstructuredGrid *grid,
                        vtkDistributedDataFilterSTLCloak *ptIdMap)
{
  vtkIdType id;
  int proc;
  int nprocs = this->NumProcesses;
  vtkIdType cellId;
  vtkIdType nelts;

  // for each process, create a list of the ids of cells I need
  // to send to it

  std::map<int, int>::iterator imap;

  vtkIdList *cellList = vtkIdList::New();

  vtkIdList **sendCells = new vtkIdList * [nprocs];

  for (proc = 0; proc < nprocs; proc++)
    {
    sendCells[proc] = vtkIdList::New();

    if (globalPtIds[proc] == NULL)
      {
      continue;
      }

    if ((nelts = globalPtIds[proc]->GetNumberOfTuples()) == 0)
      {
      globalPtIds[proc]->Delete();
      continue;
      }

    vtkIdType *ptarray = globalPtIds[proc]->GetPointer(0);

    std::set<vtkIdType> subGridCellIds;

    vtkIdType nYourCells = 0;

    for (id = 0; id < nelts; id += (nYourCells + 2))
      {
      vtkIdType ptId = ptarray[id];
      nYourCells = ptarray[id+1];

      imap = ptIdMap->IntMap.find(ptId);

      if (imap == ptIdMap->IntMap.end())
        {
        continue; // I don't have this point
        }

      vtkIdType myPtId = (vtkIdType)imap->second;   // convert to my local point Id

      grid->GetPointCells(myPtId, cellList);

      vtkIdType nMyCells = cellList->GetNumberOfIds();

      if (nMyCells == 0)
        {
        continue;
        }

      if (nYourCells > 0)
        {
        // We don't send cells the remote process tells us it already
        // has.  This is much faster than removing duplicate cells on
        // the receive side.

        vtkIdType *remoteCells = ptarray + id + 2;
        vtkIdType *gidCells = this->GetGlobalElementIds(grid);

        vtkDistributedDataFilter::RemoveRemoteCellsFromList(cellList,
                                     gidCells, remoteCells, nYourCells);
        }

      vtkIdType nSendCells = cellList->GetNumberOfIds();

      if (nSendCells == 0)
        {
        continue;
        }

      for (cellId = 0; cellId < nSendCells; cellId++)
        {
        subGridCellIds.insert(cellList->GetId(cellId));
        }
      }

    globalPtIds[proc]->Delete();

    vtkIdType numUniqueCellIds = subGridCellIds.size();

    if (numUniqueCellIds == 0)
      {
      continue;
      }

    sendCells[proc]->SetNumberOfIds(numUniqueCellIds);
    vtkIdType next = 0;

    std::set<vtkIdType>::iterator it;

    for (it = subGridCellIds.begin(); it != subGridCellIds.end(); ++it)
      {
      sendCells[proc]->SetId(next++, *it);
      }
    }

  delete [] globalPtIds;

  cellList->Delete();

  return sendCells;
}

//-----------------------------------------------------------------------
void vtkDistributedDataFilter::RemoveRemoteCellsFromList(
  vtkIdList *cellList, vtkIdType *gidCells, vtkIdType *remoteCells, vtkIdType nRemoteCells)
{
  vtkIdType id, nextId;
  vtkIdType id2;
  vtkIdType nLocalCells = cellList->GetNumberOfIds();

  // both lists should be very small, so we just do an n^2 lookup

  for (id = 0, nextId = 0; id < nLocalCells; id++)
    {
    vtkIdType localCellId  = cellList->GetId(id);
    vtkIdType globalCellId = gidCells[localCellId];

    int found = 0;

    for (id2 = 0; id2 < nRemoteCells; id2++)
      {
      if (remoteCells[id2] == globalCellId)
        {
        found = 1;
        break;
        }
      }

    if (!found)
      {
      cellList->SetId(nextId++, localCellId);
      }
    }

  cellList->SetNumberOfIds(nextId);
}

//-----------------------------------------------------------------------
// Set the ghost levels for the points and cells in the received cells.
// Merge the new ghost cells into the supplied grid, and return the new grid.
// Delete all grids except the new merged grid.

vtkUnstructuredGrid *vtkDistributedDataFilter::SetMergeGhostGrid(
                            vtkUnstructuredGrid *ghostCellGrid,
                            vtkUnstructuredGrid *incomingGhostCells,
                            int ghostLevel, vtkDistributedDataFilterSTLCloak *idMap)

{
  int i;

  if (incomingGhostCells->GetNumberOfCells() < 1)
    {
    return ghostCellGrid;
    }

  // Set the ghost level of all new cells, and set the ghost level of all
  // the points.  We know some points in the new grids actually have ghost
  // level one lower, because they were on the boundary of the previous
  // grid.  This is OK if ghostLevel is > 1.  When we merge, vtkMergeCells
  // will skip these points because they are already in the previous grid.
  // But if ghostLevel is 1, those boundary points were in our original
  // grid, and we need to use the global ID map to determine if the
  // point ghost levels should be set to 0.

  vtkDataArray *da = incomingGhostCells->GetCellData()->GetArray("vtkGhostLevels");
  vtkUnsignedCharArray *cellGL = vtkUnsignedCharArray::SafeDownCast(da);

  da  = incomingGhostCells->GetPointData()->GetArray("vtkGhostLevels");
  vtkUnsignedCharArray *ptGL = vtkUnsignedCharArray::SafeDownCast(da);

  unsigned char *ia = cellGL->GetPointer(0);

  for (i=0; i < incomingGhostCells->GetNumberOfCells(); i++)
    {
    ia[i] = (unsigned char)ghostLevel;
    }

  ia = ptGL->GetPointer(0);

  for (i=0; i < incomingGhostCells->GetNumberOfPoints(); i++)
    {
    ia[i] = (unsigned char)ghostLevel;
    }

  // now merge

  vtkUnstructuredGrid *mergedGrid = incomingGhostCells;

  if (ghostCellGrid && (ghostCellGrid->GetNumberOfCells() > 0))
    {
    vtkDataSet *sets[2];

    sets[0] = ghostCellGrid;     // both sets will be deleted by MergeGrids
    sets[1] = incomingGhostCells;

    int useGlobalNodeIds = (this->GetGlobalNodeIds(ghostCellGrid)?1:0);
    mergedGrid =
      vtkDistributedDataFilter::MergeGrids(sets, 2, DeleteYes, useGlobalNodeIds, 0.0, 0);
    }

  // If this is ghost level 1, mark any points from our original grid
  // as ghost level 0.

  if (ghostLevel == 1)
    {
    da = mergedGrid->GetPointData()->GetArray("vtkGhostLevels");
    ptGL = vtkUnsignedCharArray::SafeDownCast(da);

    vtkIdType *gidPoints = this->GetGlobalNodeIds(mergedGrid);
    int npoints = mergedGrid->GetNumberOfPoints();

    std::map<int, int>::iterator imap;

    for (i=0; i < npoints; i++)
      {
      imap = idMap->IntMap.find(gidPoints[i]);

      if (imap != idMap->IntMap.end())
        {
        ptGL->SetValue(i,0);   // found among my ghost level 0 cells
        }
      }
    }

  return mergedGrid;
}

//-----------------------------------------------------------------------
vtkUnstructuredGrid *vtkDistributedDataFilter::MergeGrids(
  vtkDataSet **sets, int nsets, int deleteDataSets,
  int useGlobalNodeIds, float pointMergeTolerance,
  int useGlobalCellIds)
{
  int i;

  if (nsets == 0)
    {
    return NULL;
    }

  vtkUnstructuredGrid *newGrid = vtkUnstructuredGrid::New();
  // Any global ids should be consistent, so make sure they are passed.
  newGrid->GetPointData()->CopyGlobalIdsOn();
  newGrid->GetCellData()->CopyGlobalIdsOn();

  vtkMergeCells *mc = vtkMergeCells::New();
  mc->SetUnstructuredGrid(newGrid);

  mc->SetTotalNumberOfDataSets(nsets);

  vtkIdType totalPoints = 0;
  vtkIdType totalCells = 0;

  for (i=0; i<nsets; i++)
    {
    totalPoints += sets[i]->GetNumberOfPoints();
    totalCells += sets[i]->GetNumberOfCells();
    // Only use global ids if they are available.
    useGlobalNodeIds = (   useGlobalNodeIds
                        && (sets[i]->GetPointData()->GetGlobalIds() != NULL) );
    useGlobalCellIds = (   useGlobalCellIds
                        && (sets[i]->GetCellData()->GetGlobalIds() != NULL) );
    }

  mc->SetTotalNumberOfPoints(totalPoints);
  mc->SetTotalNumberOfCells(totalCells);

  if (!useGlobalNodeIds)
    {
    mc->SetPointMergeTolerance(pointMergeTolerance);
    }
  mc->SetUseGlobalIds(useGlobalNodeIds);
  mc->SetUseGlobalCellIds(useGlobalCellIds);

  for (i=0; i<nsets; i++)
    {
    mc->MergeDataSet(sets[i]);

    if (deleteDataSets)
      {
      sets[i]->Delete();
      }
    }

  mc->Finish();
  mc->Delete();

  return newGrid;
}

//-------------------------------------------------------------------------
int vtkDistributedDataFilter::RequestDataObject(vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }

  vtkDataObject *input = vtkDataObject::GetData(inInfo);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (input)
    {
    vtkDataObject *output = vtkDataObject::GetData(outInfo);
    // If input is composite dataset, output is a vtkMultiBlockDataSet of
    // unstructrued grids.
    // If input is a dataset, output is an unstructured grid.
    if (!output ||
      (input->IsA("vtkCompositeDataSet") && !output->IsA("vtkMultiBlockDataSet")) ||
      (input->IsA("vtkDataSet") && !output->IsA("vtkUnstructuredGrid")))
      {
      vtkDataObject* newOutput = 0;
      if (input->IsA("vtkCompositeDataSet"))
        {
        newOutput = vtkMultiBlockDataSet::New();
        }
      else // if (input->IsA("vtkDataSet"))
        {
        newOutput = vtkUnstructuredGrid::New();
        }
      outInfo->Set(vtkDataObject::DATA_OBJECT(), newOutput);
      newOutput->Delete();
      }
    return 1;
    }

  return 0;
}

//-------------------------------------------------------------------------
int vtkDistributedDataFilter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//-------------------------------------------------------------------------
vtkPKdTree *vtkDistributedDataFilter::GetKdtree()
{
  if (this->Kdtree == NULL)
    {
    this->Kdtree = vtkPKdTree::New();
    this->Kdtree->AssignRegionsContiguous();
    this->Kdtree->SetTiming(this->GetTiming());
    }

  return this->Kdtree;
}

//-------------------------------------------------------------------------
void vtkDistributedDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Kdtree: " << this->Kdtree << endl;
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "NumProcesses: " << this->NumProcesses << endl;
  os << indent << "MyId: " << this->MyId << endl;
  os << indent << "Target: " << this->Target << endl;
  os << indent << "Source: " << this->Source << endl;
  os << indent << "RetainKdtree: " << this->RetainKdtree << endl;
  os << indent << "IncludeAllIntersectingCells: " << this->IncludeAllIntersectingCells << endl;
  os << indent << "ClipCells: " << this->ClipCells << endl;

  os << indent << "Timing: " << this->Timing << endl;
  os << indent << "UseMinimalMemory: " << this->UseMinimalMemory << endl;
}
