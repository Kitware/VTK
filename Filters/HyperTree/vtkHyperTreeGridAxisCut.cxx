/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridAxisCut.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridAxisCut.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkCellData.h"
#include "vtkHyperTreeGrid.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkHyperTreeGridAxisCut);

//-----------------------------------------------------------------------------
vtkHyperTreeGridAxisCut::vtkHyperTreeGridAxisCut()
{
  this->Points = 0;
  this->Cells = 0;
  this->Input = 0;
  this->Output = 0;

  this->PlanePosition = 0.0;
  this->PlaneNormalAxis = 0;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridAxisCut::ProcessTrees()
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
void vtkHyperTreeGridAxisCut::AddFace(vtkIdType inId, double* origin, double* size,
                                      double offset0, int axis0, int axis1, int axis2)
{
  vtkIdType ids[4];
  double pt[3];
  pt[0] = origin[0];
  pt[1] = origin[1];
  pt[2] = origin[2];
  pt[axis0] += size[axis0] * offset0;

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
void vtkHyperTreeGridAxisCut::RecursiveProcessTree(vtkHyperTreeSuperCursor* superCursor)
{
  // Terminate if the node does not touch the plane.
  if (superCursor->Origin[this->PlaneNormalAxis] > this->PlanePosition ||
      superCursor->Origin[this->PlaneNormalAxis]+superCursor->Size[this->PlaneNormalAxis]
        < this->PlanePosition)
    {
    return;
    }

  // If we are at a leaf, create the outer surfaces.
  if ( superCursor->GetCursor(0)->GetIsLeaf() )
    {
    vtkIdType inId = superCursor->GetCursor(0)->GetGlobalLeafIndex();
    double k = this->PlanePosition-superCursor->Origin[this->PlaneNormalAxis];
    k = k / superCursor->Size[this->PlaneNormalAxis];
    int axis1, axis2;
    switch (this->PlaneNormalAxis)
      {
      case 0:
        axis1 = 1; axis2 = 2;
        break;
      case 1:
        axis1 = 0; axis2 = 2;
        break;
      case 2:
        axis1 = 0; axis2 = 1;
        break;
      default:
        vtkErrorMacro("Bad Axis.");
        return;
      }
    this->AddFace(inId, superCursor->Origin, superCursor->Size,
                  k, this->PlaneNormalAxis, axis1, axis2);
    return;
    }

  // Not a leaf.  Recurse children to leaves.
  int numChildren = this->Input->GetNumberOfChildren();
  for ( int child = 0; child < numChildren; ++ child )
    {
    vtkHyperTreeSuperCursor newSuperCursor;
    this->Input->InitializeSuperCursorChild(superCursor,&newSuperCursor, child);
    this->RecursiveProcessTree( &newSuperCursor);
    }
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridAxisCut::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridAxisCut::RequestData( vtkInformation*,
                                          vtkInformationVector** inputVector,
                                          vtkInformationVector* outputVector )
{
  // Get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // Initialize
  this->Input = vtkHyperTreeGrid::SafeDownCast( inInfo->Get(vtkDataObject::DATA_OBJECT()) );
  this->Output= vtkPolyData::SafeDownCast( outInfo->Get(vtkDataObject::DATA_OBJECT()) );
  vtkCellData *outCD = this->Output->GetCellData();
  vtkCellData *inCD = this->Input->GetCellData();

  if ( this->Input->GetDimension() != 3 )
    {
    vtkErrorMacro("Axis cut only works with 3D trees.");
    return 0;
    }

  // Ensure that primal grid API is used for hyper trees
  this->Input->SetDualGridFlag( false );

  outCD->CopyAllocate( inCD );

  this->ProcessTrees();
  this->Input = 0;
  this->Output = 0;

  this->UpdateProgress ( 1. );

  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridAxisCut::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf( os, indent );

  this->Input->PrintSelf( os, indent.GetNextIndent() );
  this->Output->PrintSelf( os, indent.GetNextIndent() );
  this->Points->PrintSelf( os, indent.GetNextIndent() );
  this->Cells->PrintSelf( os, indent.GetNextIndent() );

  os << indent << "Plane Normal Axis : " << this->PlaneNormalAxis << endl;
  os << indent << "Plane Position : " << this->PlanePosition << endl;
}
