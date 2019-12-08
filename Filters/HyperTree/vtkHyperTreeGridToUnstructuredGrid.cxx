/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridToUnstructuredGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridToUnstructuredGrid.h"

#include "vtkBitArray.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkHyperTreeGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"

vtkStandardNewMacro(vtkHyperTreeGridToUnstructuredGrid);

//-----------------------------------------------------------------------------
vtkHyperTreeGridToUnstructuredGrid::vtkHyperTreeGridToUnstructuredGrid()
  : Points(nullptr)
  , Cells(nullptr)
  , Dimension(0)
  , Orientation(0)
  , Axes(nullptr)
{
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridToUnstructuredGrid::~vtkHyperTreeGridToUnstructuredGrid()
{
  // The class members are only used during process and are destroyed once
  // the process is finished to reduce stack size during recursive calls.
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridToUnstructuredGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridToUnstructuredGrid::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridToUnstructuredGrid::ProcessTrees(
  vtkHyperTreeGrid* input, vtkDataObject* outputDO)
{
  // Downcast output data object to hyper tree grid
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(outputDO);
  if (!output)
  {
    vtkErrorMacro("Incorrect type of output: " << outputDO->GetClassName());
    return 0;
  }

  // Set instance variables needed for this conversion
  this->Points = vtkPoints::New();
  this->Cells = vtkCellArray::New();
  this->Dimension = input->GetDimension();
  this->Orientation = input->GetOrientation();
  this->Axes = input->GetAxes();

  // Initialize output cell data
  this->InData = input->GetPointData();
  this->OutData = output->GetCellData();
  this->OutData->CopyAllocate(this->InData);

  // Iterate over all hyper trees
  vtkIdType index;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  input->InitializeTreeIterator(it);
  vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursor;
  while (it.GetNextTree(index))
  {
    // Initialize new geometric cursor at root of current tree
    input->InitializeNonOrientedGeometryCursor(cursor, index);

    // Convert hyper tree into unstructured mesh recursively
    this->RecursivelyProcessTree(cursor);
  } // it

  // Set output geometry and topology
  output->SetPoints(this->Points);
  switch (this->Dimension)
  {
    case 1:
      // 1D cells are lines
      output->SetCells(VTK_LINE, this->Cells);
      break;
    case 2:
      // 2D cells are quadrilaterals
      output->SetCells(VTK_PIXEL, this->Cells);
      break;
    case 3:
      // 3D cells are voxels (i.e. hexahedra with indexing order equal to that of cursors)
      output->SetCells(VTK_VOXEL, this->Cells);
      break;
    default:
      break;
  } // switch ( this->Dimension )

  this->Points->FastDelete();
  this->Cells->FastDelete();
  this->Points = nullptr;
  this->Cells = nullptr;

  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridToUnstructuredGrid::RecursivelyProcessTree(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  // If leaf is masked, skip it
  if (cursor->IsMasked())
  {
    return;
  }

  // Create unstructured output if cursor is at leaf
  if (cursor->IsLeaf())
  {
    // Cursor is at leaf, retrieve its global index
    vtkIdType id = cursor->GetGlobalNodeIndex();

    // Create cell
    this->AddCell(id, cursor->GetOrigin(), cursor->GetSize());
  } // if ( cursor->IsLeaf() )
  else
  {
    // Cursor is not at leaf, recurse to all children
    int numChildren = cursor->GetNumberOfChildren();
    for (int ichild = 0; ichild < numChildren; ++ichild)
    {
      cursor->ToChild(ichild);
      // Recurse
      this->RecursivelyProcessTree(cursor);
      cursor->ToParent();
    } // child
  }   // else
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridToUnstructuredGrid::AddCell(vtkIdType inId, double* origin, double* size)
{
  // Storage for point coordinates
  double pt[] = { 0., 0., 0. };

  // Storage for cell vertex IDs
  vtkIdType ids[8];

  // Storage for cell ID
  vtkIdType outId;

  // First cell vertex is always at origin of cursor
  // Add vertex #0 : (0,0)
  memcpy(pt, origin, 3 * sizeof(double));
  ids[0] = this->Points->InsertNextPoint(pt);

  // Create remaining 2^d - 1 vertices depending on dimension
  switch (this->Dimension)
  {
    case 1:
    {
      assert("pre: internal" && this->Orientation == this->Axes[0]);

      // In 1D there is only one other vertex
      pt[0] = origin[this->Orientation] + size[this->Orientation];
      ids[1] = this->Points->InsertNextPoint(pt);

      // Insert next line
      outId = this->Cells->InsertNextCell(2, ids);
      break;
    }
    case 2:
    {
      unsigned int axis1 = this->Axes[0];
      unsigned int axis2 = this->Axes[1];

      // Add vertex #1 : (1,0)
      pt[axis1] = origin[axis1] + size[axis1];
      pt[axis2] = origin[axis2];
      ids[1] = this->Points->InsertNextPoint(pt);

      // Add vertex #2 : (0,1)
      pt[axis1] = origin[axis1];
      pt[axis2] = origin[axis2] + size[axis2];
      ids[2] = this->Points->InsertNextPoint(pt);

      // Add vertex #3 : (1,1)
      pt[axis1] = origin[axis1] + size[axis1];
      pt[axis2] = origin[axis2] + size[axis2];
      ids[3] = this->Points->InsertNextPoint(pt);

      // Insert next quadrangle
      outId = this->Cells->InsertNextCell(4, ids);
      break;
    }
    case 3:
    {
      // z=0 plane
      pt[2] = origin[2];

      // Add vertex #1 : (1,0,0)
      pt[0] = origin[0] + size[0];
      pt[1] = origin[1];
      ids[1] = this->Points->InsertNextPoint(pt);

      // Add vertex #2 : (0,1,0)
      pt[0] = origin[0];
      pt[1] = origin[1] + size[1];
      ids[2] = this->Points->InsertNextPoint(pt);

      // Add vertex #3 : (1,1,0)
      pt[0] = origin[0] + size[0];
      pt[1] = origin[1] + size[1];
      ids[3] = this->Points->InsertNextPoint(pt);

      // z=1 plane
      pt[2] = origin[2] + size[2];

      // Add vertex #4 : (0,0,1)
      pt[0] = origin[0];
      pt[1] = origin[1];
      ids[4] = this->Points->InsertNextPoint(pt);

      // Add vertex #5 : (1,0,1)
      pt[0] = origin[0] + size[0];
      pt[1] = origin[1];
      ids[5] = this->Points->InsertNextPoint(pt);

      // Add vertex #6 : (0,1,1)
      pt[0] = origin[0];
      pt[1] = origin[1] + size[1];
      ids[6] = this->Points->InsertNextPoint(pt);

      // Add vertex #7 : (1,1,1)
      pt[0] = origin[0] + size[0];
      pt[1] = origin[1] + size[1];
      ids[7] = this->Points->InsertNextPoint(pt);

      // Insert next voxel
      outId = this->Cells->InsertNextCell(8, ids);
      break;
    }
    default:
    {
      return;
    }
  } // switch ( this->Dimension )

  // Copy output data from input
  this->OutData->CopyData(this->InData, inId, outId);
}
