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

#include "vtkAppendFilter.h"
#include "vtkBSPCuts.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPKdTree.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

// Needed to let vtkPDistributedDataFilter be instantiated when available
vtkObjectFactoryNewMacro(vtkDistributedDataFilter);

//----------------------------------------------------------------------------
vtkDistributedDataFilter::vtkDistributedDataFilter()
{
  this->Kdtree = nullptr;

  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->Target = nullptr;
  this->Source = nullptr;

  this->NumConvexSubRegions = 0;
  this->ConvexSubRegionBounds = nullptr;

  this->MinimumGhostLevel = 0;
  this->GhostLevel = 0;

  this->RetainKdtree = 1;
  this->IncludeAllIntersectingCells = 0;
  this->ClipCells = 0;

  this->Timing = 0;

  this->UseMinimalMemory = 0;

  this->UserCuts = nullptr;
}

//----------------------------------------------------------------------------
vtkDistributedDataFilter::~vtkDistributedDataFilter()
{
  if (this->Kdtree)
  {
    this->Kdtree->Delete();
    this->Kdtree = nullptr;
  }

  this->SetController(nullptr);

  delete[] this->Target;
  this->Target = nullptr;

  delete[] this->Source;
  this->Source = nullptr;

  delete[] this->ConvexSubRegionBounds;
  this->ConvexSubRegionBounds = nullptr;

  if (this->UserCuts)
  {
    this->UserCuts->Delete();
    this->UserCuts = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkDistributedDataFilter::SetController(vtkMultiProcessController* c)
{
  if (this->Kdtree)
  {
    this->Kdtree->SetController(c);
  }

  if ((c == nullptr) || (c->GetNumberOfProcesses() == 0))
  {
    this->NumProcesses = 1;
    this->MyId = 0;
  }

  if (this->Controller == c)
  {
    return;
  }

  this->Modified();

  if (this->Controller != nullptr)
  {
    this->Controller->UnRegister(this);
    this->Controller = nullptr;
  }

  if (c == nullptr)
  {
    return;
  }

  this->Controller = c;

  c->Register(this);
  this->NumProcesses = c->GetNumberOfProcesses();
  this->MyId = c->GetLocalProcessId();
}

//-------------------------------------------------------------------------
vtkPKdTree* vtkDistributedDataFilter::GetKdtree()
{
  if (this->Kdtree == nullptr)
  {
    this->Kdtree = vtkPKdTree::New();
    this->Kdtree->AssignRegionsContiguous();
    this->Kdtree->SetTiming(this->GetTiming());
  }

  return this->Kdtree;
}

//----------------------------------------------------------------------------
void vtkDistributedDataFilter::SetBoundaryMode(int mode)
{
  int include_all, clip_cells;
  switch (mode)
  {
    case vtkDistributedDataFilter::ASSIGN_TO_ONE_REGION:
      include_all = 0;
      clip_cells = 0;
      break;
    case vtkDistributedDataFilter::ASSIGN_TO_ALL_INTERSECTING_REGIONS:
      include_all = 1;
      clip_cells = 0;
      break;
    case vtkDistributedDataFilter::SPLIT_BOUNDARY_CELLS:
    default:
      include_all = 1;
      clip_cells = 1;
      break;
  }

  if (this->IncludeAllIntersectingCells != include_all || this->ClipCells != clip_cells)
  {
    this->IncludeAllIntersectingCells = include_all;
    this->ClipCells = clip_cells;
    this->Modified();
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

//-------------------------------------------------------------------------

int vtkDistributedDataFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int piece, numPieces, ghostLevels;

  // We require preceding filters to refrain from creating ghost cells.

  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevels = 0;

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevels);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  return 1;
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
    this->UserCuts = nullptr;
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
void vtkDistributedDataFilter::SetUserRegionAssignments(const int* map, int numRegions)
{
  std::vector<int> copy(this->UserRegionAssignments);
  this->UserRegionAssignments.resize(numRegions);
  for (int cc = 0; cc < numRegions; cc++)
  {
    this->UserRegionAssignments[cc] = map[cc];
  }
  if (copy != this->UserRegionAssignments)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkDistributedDataFilter::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);

  return 1;
}

//----------------------------------------------------------------------------
int vtkDistributedDataFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output.
  vtkDataObject* input = vtkDataObject::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* outputUG =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkUnstructuredGrid::DATA_OBJECT()));
  vtkCompositeDataSet* outputCD =
    vtkCompositeDataSet::SafeDownCast(outInfo->Get(vtkCompositeDataSet::DATA_OBJECT()));

  if (!input)
  {
    vtkErrorMacro("No input data!");
    return 0;
  }

  if (outputCD)
  {
    outputCD->ShallowCopy(input);
  }
  else
  {
    // vtkAppendFilter always produces a vtkUnstructuredGrid, so use it
    // to convert the vtkPolyData to an unstructured grid.
    vtkNew<vtkAppendFilter> converter;
    converter->SetInputData(input);
    converter->MergePointsOff();
    converter->Update();
    outputUG->ShallowCopy(converter->GetOutput());
  }

  return 1;
}

//-------------------------------------------------------------------------
int vtkDistributedDataFilter::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  vtkDataObject* input = vtkDataObject::GetData(inInfo);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (input)
  {
    vtkDataObject* output = vtkDataObject::GetData(outInfo);
    // If input is composite dataset, output is a vtkMultiBlockDataSet of
    // unstructrued grids.
    // If input is a dataset, output is an unstructured grid.
    if (!output || (input->IsA("vtkCompositeDataSet") && !output->IsA("vtkMultiBlockDataSet")) ||
      (input->IsA("vtkDataSet") && !output->IsA("vtkUnstructuredGrid")))
    {
      vtkDataObject* newOutput = nullptr;
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
int vtkDistributedDataFilter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//-------------------------------------------------------------------------
void vtkDistributedDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
