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

#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUniformHyperTreeGrid.h"

#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"

vtkStandardNewMacro(vtkHyperTreeGridAxisCut);

//-----------------------------------------------------------------------------
vtkHyperTreeGridAxisCut::vtkHyperTreeGridAxisCut()
{
  // Defaut normal axis is Z
  this->PlaneNormalAxis = 0;

  // Defaut place intercept is 0
  this->PlanePosition = 0.;
  // JB La position reellement utilisee dans la coupe
  this->PlanePositionRealUse = 0.;

  // Default mask is empty
  this->OutMask = nullptr;

  // Output indices begin at 0
  this->CurrentId = 0;

  // JB Pour sortir un maillage de meme type que celui en entree
  this->AppropriateOutput = true;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridAxisCut::~vtkHyperTreeGridAxisCut()
{
  if (this->OutMask)
  {
    this->OutMask->Delete();
    this->OutMask = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridAxisCut::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PlaneNormalAxis : " << this->PlaneNormalAxis << endl;
  os << indent << "PlanePosition : " << this->PlanePosition << endl;
  os << indent << "OutMask: " << this->OutMask << endl;
  os << indent << "CurrentId: " << this->CurrentId << endl;
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridAxisCut::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridAxisCut::ProcessTrees(vtkHyperTreeGrid* input, vtkDataObject* outputDO)
{
  // Downcast output data object to hyper tree grid
  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(outputDO);
  if (!output)
  {
    vtkErrorMacro("Incorrect type of output: " << outputDO->GetClassName());
    return 0;
  }

  // This filter works only with 3D grids
  if (input->GetDimension() != 3)
  {
    vtkErrorMacro(<< "Bad input dimension:" << input->GetDimension());
    return 0;
  }

  output->Initialize();

  // Retrieve normal axis and intercept of cut plane
  int axis = this->PlaneNormalAxis;

  this->PlanePositionRealUse = this->PlanePosition;
  /* CORRECTIF pour les coupes sur axes
  Au minimum ici il faut modifier cette valeur afin
  de la deplacer un peu si necessaire
  si UHTG c'est rapide et facile
  sinon il faut trouver un HT concerne...
  */

  double inter = this->PlanePositionRealUse;

  // Set output grid sizes; must be 1 in the direction of cut plane normal
  unsigned int size[3];
  input->GetDimensions(size);
  size[axis] = 1;
  output->SetDimensions(size);

  vtkUniformHyperTreeGrid* inputUHTG = vtkUniformHyperTreeGrid::SafeDownCast(input);
  vtkUniformHyperTreeGrid* outputUHTG = vtkUniformHyperTreeGrid::SafeDownCast(outputDO);
  if (inputUHTG)
  {
    outputUHTG->CopyCoordinates(inputUHTG);
    outputUHTG->SetFixedCoordinates(axis, inter);
  }
  else
  {
    output->CopyCoordinates(input);
    output->SetFixedCoordinates(axis, inter);
  }

  // Other grid parameters are identical
  output->SetTransposedRootIndexing(input->GetTransposedRootIndexing());
  output->SetBranchFactor(input->GetBranchFactor());
  output->SetHasInterface(input->GetHasInterface());
  output->SetInterfaceNormalsName(input->GetInterfaceNormalsName());
  output->SetInterfaceInterceptsName(input->GetInterfaceInterceptsName());

  // Initialize output point data
  this->InData = input->GetPointData();
  this->OutData = output->GetPointData();
  this->OutData->CopyAllocate(this->InData);

  // Output indices begin at 0
  this->CurrentId = 0;

  // Create material mask bit array if one is present on input
  if (input->HasMask())
  {
    this->OutMask = vtkBitArray::New();
  }

  // Retrieve material mask
  this->InMask = this->OutMask ? input->GetMask() : nullptr;

  // Storage for root cell Cartesian coordinates
  unsigned int i, j, k;

  // Storage for material mask indices computed together with output grid
  vtkNew<vtkIdTypeArray> position;

  // Iterate over all input hyper trees
  vtkIdType inIndex;
  vtkIdType outIndex = 0;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  input->InitializeTreeIterator(it);
  vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> inCursor;
  vtkNew<vtkHyperTreeGridNonOrientedCursor> outCursor;
  while (it.GetNextTree(inIndex))
  {
    // Initialize new geometric cursor at root of current input tree
    input->InitializeNonOrientedGeometryCursor(inCursor, inIndex);

    // Retrieve geometric features of input cursor
    const double* origin = inCursor->GetOrigin();
    const double* _size = inCursor->GetSize();

    // Check whether root cell is intersected by plane
    if (origin[axis] < inter && (origin[axis] + _size[axis] >= inter))
    {
      // Root is intersected by plane, descend into current child
      input->GetLevelZeroCoordinatesFromIndex(inIndex, i, j, k);

      // Get root index into output hyper tree grid, depending on cut axes
      switch (axis)
      {
        case 0:
          output->GetIndexFromLevelZeroCoordinates(outIndex, 0, j, k);
          break;
        case 1:
          output->GetIndexFromLevelZeroCoordinates(outIndex, i, 0, k);
          break;
        case 2:
          output->GetIndexFromLevelZeroCoordinates(outIndex, i, j, 0);
          break;
        default:
          vtkErrorMacro("Incorrect orientation of output: " << axis);
          return 0;
      } // switch ( axis )

      // Initialize new cursor at root of current output tree
      output->InitializeNonOrientedCursor(outCursor, outIndex, true);

      // Cut tree recursively
      this->RecursivelyProcessTree(inCursor, outCursor);
    } // if origin
  }   // it

  // Squeeze and set output material mask if necessary
  if (this->OutMask)
  {
    this->OutMask->Squeeze();
    output->SetMask(this->OutMask);
    this->OutMask->FastDelete();
    this->OutMask = nullptr;
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridAxisCut::RecursivelyProcessTree(
  vtkHyperTreeGridNonOrientedGeometryCursor* inCursor, vtkHyperTreeGridNonOrientedCursor* outCursor)
{
  // Retrieve global index of input cursor
  vtkIdType inId = inCursor->GetGlobalNodeIndex();

  // Increase index count on output: postfix is intended
  vtkIdType outId = this->CurrentId++;

  // Retrieve output tree and set global index of output cursor
  vtkHyperTree* outTree = outCursor->GetTree();
  outTree->SetGlobalIndexFromLocal(outCursor->GetVertexId(), outId);

  // Update material mask if relevant
  if (this->InMask)
  {
    this->OutMask->InsertValue(outId, this->InMask->GetValue(inId));
  }

  // Copy output cell data from that of input cell
  this->OutData->CopyData(this->InData, inId, outId);

  // Descend further into input trees only if cursor is not at leaf
  if (!inCursor->IsLeaf())
  {
    // Cursor is not at leaf, subdivide output tree one level further
    outCursor->SubdivideLeaf();

    // Initialize output children index
    int outChild = 0;

    // If cursor is not at leaf, recurse to all children
    int numChildren = inCursor->GetNumberOfChildren();
    for (int inChild = 0; inChild < numChildren; ++inChild)
    {
      inCursor->ToChild(inChild);

      // Retrieve normal axis and intercept of plane
      int axis = this->PlaneNormalAxis;
      double inter = this->PlanePositionRealUse;

      // Retrieve geometric features of input cursor
      const double* origin = inCursor->GetOrigin();
      const double* size = inCursor->GetSize();

      // Check whether child is intersected by plane
      if (origin[axis] < inter && (origin[axis] + size[axis] >= inter))
      {
        // Child is intersected by plane, descend into current child
        outCursor->ToChild(outChild);

        // Recurse
        this->RecursivelyProcessTree(inCursor, outCursor);

        // Return to parent
        outCursor->ToParent();

        // Increment output children count
        ++outChild;
      }

      inCursor->ToParent();
    } // inChild
  }   // if ( ! cursor->IsLeaf() )
}
