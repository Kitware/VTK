/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridGeometry.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridGeometry.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkCellData.h"
#include "vtkHyperTreeGrid.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkHyperTreeGridGeometry);

vtkHyperTreeGridGeometry::vtkHyperTreeGridGeometry()
{
  this->Points = 0;
  this->Cells = 0;
  this->Input = 0;
  this->Output = 0;
}



//-----------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::ProcessTrees()
{
  // TODO: MTime on generation of this table.
  this->Input->GenerateSuperCursorTraversalTable();

  // Primal corner points
  this->Points = vtkPoints::New();
  this->Cells = vtkCellArray::New();

  // Iterate over all hyper trees
  int *gridSize = this->Input->GetGridSize();
  for ( int i = 0; i < gridSize[0]; ++ i )
    {
    for ( int j = 0; j < gridSize[1]; ++ j )
      {
      for ( int k = 0; k < gridSize[2]; ++ k )
        {
        // Storage for super cursors
        vtkHyperTreeSuperCursor superCursor;
        // Initialize center cursor
        this->Input->InitializeSuperCursor(&superCursor, i, j, k);
        // Traverse and populate dual recursively
        this->RecursiveProcessTree( &superCursor);
        } // k
      } // j
    } // i

  if (this->Input->GetDimension() == 1)
    {
    this->Output->SetLines(this->Cells);
    }
  else
    {
    this->Output->SetPolys(this->Cells);
    }

  this->Output->SetPoints(this->Points);
  this->Points->Delete();
  this->Points = 0;
  this->Cells->Delete();
  this->Cells = 0;
}


//----------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::AddFace(vtkIdType inId, double* origin, double* size,
                                       int offset0, int axis0, int axis1, int axis2)
{
  vtkIdType ids[4];
  double pt[3];
  pt[0] = origin[0];
  pt[1] = origin[1];
  pt[2] = origin[2];
  if (offset0)
    {
    pt[axis0] += size[axis0];
    }
  ids[0] = this->Points->InsertNextPoint(pt);
  pt[axis1] += size[axis1];
  ids[1] = this->Points->InsertNextPoint(pt);
  pt[axis2] += size[axis2];
  ids[2] = this->Points->InsertNextPoint(pt);
  pt[axis1] = origin[axis1];
  ids[3] = this->Points->InsertNextPoint(pt);

  vtkIdType outId = this->Cells->InsertNextCell(4, ids);
  this->Output->GetCellData()->CopyData(this->Input->GetCellData(),inId,outId);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::RecursiveProcessTree( vtkHyperTreeSuperCursor* superCursor)
{
  // Terminate if the middle cells is not on the boundary.
  // Only 3d cells have internal faces to skip.
  int dim = this->Input->GetDimension();
  if (dim == 3 && 
      superCursor->GetCursor(-1)->GetTree() &&
      superCursor->GetCursor(1)->GetTree() &&
      superCursor->GetCursor(-3)->GetTree() &&
      superCursor->GetCursor(3)->GetTree() &&
      superCursor->GetCursor(-9)->GetTree() &&
      superCursor->GetCursor(9)->GetTree() )
    {
    return;
    }

  // If we are at a leaf, create the outer surfaces.
  if ( superCursor->GetCursor(0)->GetIsLeaf() )
    {
    vtkIdType inId = superCursor->GetCursor(0)->GetGlobalLeafIndex();
    if (dim == 1)
      { // 1 dimensional trees are a special case (probably never used).
      vtkIdType ids[2];
      ids[0] = this->Points->InsertNextPoint(superCursor->Origin);
      double pt[3];
      pt[0] = superCursor->Origin[0] + superCursor->Size[0];
      pt[1] = superCursor->Origin[1];
      pt[2] = superCursor->Origin[2];
      ids[1] = this->Points->InsertNextPoint(pt);
      this->Cells->InsertNextCell(2, ids);
      }
    else if (dim == 2)
      {
      this->AddFace(inId, superCursor->Origin, superCursor->Size, 0, 2,0,1);
      }
    else if (dim == 3)
      { // check the 6 faces of the leaf (for boundaries)
      if (superCursor->GetCursor(-1)->GetTree() == 0)
        {
        this->AddFace(inId, superCursor->Origin, superCursor->Size, 0, 0,1,2);
        }
      if (superCursor->GetCursor(1)->GetTree() == 0)
        {
        this->AddFace(inId, superCursor->Origin, superCursor->Size, 1, 0,1,2);
        }
      if (superCursor->GetCursor(-3)->GetTree() == 0)
        {
        this->AddFace(inId, superCursor->Origin, superCursor->Size, 0, 1,0,2);
        }
      if (superCursor->GetCursor(3)->GetTree() == 0)
        {
        this->AddFace(inId, superCursor->Origin, superCursor->Size, 1, 1,0,2);
        }
      if (superCursor->GetCursor(-9)->GetTree() == 0)
        {
        this->AddFace(inId, superCursor->Origin, superCursor->Size, 0, 2,0,1);
        }
      if (superCursor->GetCursor(9)->GetTree() == 0)
        {
        this->AddFace(inId, superCursor->Origin, superCursor->Size, 1, 2,0,1);
        }
      }
    return;
    }

  // Not a leaf.  Recurse children to leaves.
  vtkHyperTreeSuperCursor newSuperCursor;
  int numChildren = this->Input->GetNumberOfChildren();
  int child;
  for ( child = 0; child < numChildren; ++ child)
    {
    vtkHyperTreeSuperCursor newSuperCursor;
    this->Input->InitializeSuperCursorChild(superCursor,&newSuperCursor, child);
    this->RecursiveProcessTree( &newSuperCursor);
    }
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridGeometry::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}


//----------------------------------------------------------------------------
int vtkHyperTreeGridGeometry::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // Initialize
  this->Input = vtkHyperTreeGrid::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  this->Output= vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkCellData *outCD = this->Output->GetCellData();
  vtkCellData *inCD = this->Input->GetCellData();

  outCD->CopyAllocate(inCD);

  this->ProcessTrees();
  this->Input = 0;
  this->Output = 0;

  this->UpdateProgress (1.0);

  return 1;
}


void vtkHyperTreeGridGeometry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
