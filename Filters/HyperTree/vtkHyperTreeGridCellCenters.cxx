/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridCellCenters.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridCellCenters.h"

#include "vtkAlgorithm.h"
#include "vtkBitArray.h"
#include "vtkCellArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"

vtkStandardNewMacro(vtkHyperTreeGridCellCenters);

//-----------------------------------------------------------------------------
vtkHyperTreeGridCellCenters::vtkHyperTreeGridCellCenters()
{
  this->Input = nullptr;
  this->Output = nullptr;

  this->InData = nullptr;
  this->OutData = nullptr;

  this->Points = nullptr;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridCellCenters::~vtkHyperTreeGridCellCenters() {}

//----------------------------------------------------------------------------
vtkTypeBool vtkHyperTreeGridCellCenters::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {

    return this->RequestData(request, inputVector, outputVector);
  }

  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  // execute information
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridCellCenters::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridCellCenters::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Input)
  {
    os << indent << "Input:\n";
    this->Input->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Input: ( none )\n";
  }

  if (this->Output)
  {
    os << indent << "Output:\n";
    this->Output->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Output: ( none )\n";
  }

  if (this->Points)
  {
    os << indent << "Points:\n";
    this->Points->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Points: ( none )\n";
  }
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridCellCenters::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the information objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Retrieve input and output
  this->Input = vtkHyperTreeGrid::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  this->Output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Initialize output cell data
  this->InData = this->Input->GetPointData();
  this->OutData = this->Output->GetPointData();
  this->OutData->CopyAllocate(this->InData);

  // General cell centers of hyper tree grid
  this->ProcessTrees();

  // Squeeze output data
  this->OutData->Squeeze();

  // Clean up
  this->Input = nullptr;
  this->Output = nullptr;
  this->InData = nullptr;
  this->OutData = nullptr;

  this->UpdateProgress(1.);

  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridCellCenters::ProcessTrees()
{
  // Create storage for corners of leaf cells
  this->Points = vtkPoints::New();

  // Retrieve material mask
  this->InMask = this->Input->HasMask() ? this->Input->GetMask() : nullptr;

  // Iterate over all hyper trees
  vtkIdType index;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  this->Input->InitializeTreeIterator(it);
  vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursor;
  while (it.GetNextTree(index))
  {
    // Initialize new geometric cursor at root of current tree
    this->Input->InitializeNonOrientedGeometryCursor(cursor, index);
    // Generate leaf cell centers recursively
    this->RecursivelyProcessTree(cursor);
  } // it

  // Set output geometry and topology if required
  this->Output->SetPoints(this->Points);
  if (this->VertexCells)
  {
    vtkIdType np = this->Points->GetNumberOfPoints();
    vtkCellArray* vertices = vtkCellArray::New();
    vertices->AllocateEstimate(np, 1);
    for (vtkIdType i = 0; i < np; ++i)
    {
      vertices->InsertNextCell(1, &i);
    } // i
    this->Output->SetVerts(vertices);
    vertices->Delete();
  } // this->VertexCells

  // Clean up
  this->Points->Delete();
  this->Points = nullptr;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridCellCenters::RecursivelyProcessTree(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  // Create cell center if cursor is at leaf
  if (cursor->IsLeaf())
  {
    // Cursor is at leaf, retrieve its global index
    vtkIdType id = cursor->GetGlobalNodeIndex();

    // If leaf is masked, skip it
    if (this->InMask && this->InMask->GetValue(id))
    {
      return;
    }

    // Retrieve cell center coordinates
    double pt[3];
    cursor->GetPoint(pt);

    // Insert next point
    vtkIdType outId = this->Points->InsertNextPoint(pt);

    // Copy cell center data from leaf data, when needed
    if (this->VertexCells)
    {
      this->OutData->CopyData(this->InData, id, outId);
    }
  }
  else
  {
    // Cursor is not at leaf, recurse to all children
    int numChildren = this->Input->GetNumberOfChildren();
    for (int child = 0; child < numChildren; ++child)
    {
      cursor->ToChild(child);
      // Recurse
      this->RecursivelyProcessTree(cursor);
      cursor->ToParent();
    } // child
  }   // else
}
