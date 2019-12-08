/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHyperTreeGridToDualGrid.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridToDualGrid.h"

#include "vtkBitArray.h"
#include "vtkCellType.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedMooreSuperCursor.h"
#include "vtkHyperTreeGridOrientedGeometryCursor.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkHyperTreeGridToDualGrid);

static const unsigned int CornerNeighborCursorsTable3D0[8] = { 0, 1, 3, 4, 9, 10, 12, 13 };
static const unsigned int CornerNeighborCursorsTable3D1[8] = { 1, 2, 4, 5, 10, 11, 13, 14 };
static const unsigned int CornerNeighborCursorsTable3D2[8] = { 3, 4, 6, 7, 12, 13, 15, 16 };
static const unsigned int CornerNeighborCursorsTable3D3[8] = { 4, 5, 7, 8, 13, 14, 16, 17 };
static const unsigned int CornerNeighborCursorsTable3D4[8] = { 9, 10, 12, 13, 18, 19, 21, 22 };
static const unsigned int CornerNeighborCursorsTable3D5[8] = { 10, 11, 13, 14, 19, 20, 22, 23 };
static const unsigned int CornerNeighborCursorsTable3D6[8] = { 12, 13, 15, 16, 21, 22, 24, 25 };
static const unsigned int CornerNeighborCursorsTable3D7[8] = { 13, 14, 16, 17, 22, 23, 25, 26 };
static const unsigned int* CornerNeighborCursorsTable3D[8] = {
  CornerNeighborCursorsTable3D0,
  CornerNeighborCursorsTable3D1,
  CornerNeighborCursorsTable3D2,
  CornerNeighborCursorsTable3D3,
  CornerNeighborCursorsTable3D4,
  CornerNeighborCursorsTable3D5,
  CornerNeighborCursorsTable3D6,
  CornerNeighborCursorsTable3D7,
};

//-----------------------------------------------------------------------------
vtkHyperTreeGridToDualGrid::vtkHyperTreeGridToDualGrid()
{
  // Dual grid corners (primal grid leaf centers)
  this->Points = nullptr;
  this->Connectivity = nullptr;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridToDualGrid::~vtkHyperTreeGridToDualGrid()
{
  if (this->Points)
  {
    this->Points->Delete();
  }

  if (this->Connectivity)
  {
    this->Connectivity->Delete();
  }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridToDualGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Points: " << this->Points << endl;
  os << indent << "Connectivity: " << this->Connectivity << endl;
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridToDualGrid::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridToDualGrid::ProcessTrees(vtkHyperTreeGrid* input, vtkDataObject* outputDO)
{
  // Downcast output data object to hyper tree grid
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(outputDO);
  if (!output)
  {
    vtkErrorMacro("Incorrect type of output: " << outputDO->GetClassName());
    return 0;
  }

  // Check if we can break out early
  // TODO: this isn't right, we need to trust the pipeline tell us when
  // we need to reexecute, or upstream grid will change and output of this filter will be stale.
  if (this->Points)
  {
    return 1;
  }

  // TODO: we shouldn't be using persistent internal Points and Connectivity, just populate the
  // output.
  // Create arrays needed by dual mesh
  this->Points = vtkPoints::New();
  this->Connectivity = vtkIdTypeArray::New();

  // Primal cell centers are dual points
  // JB On ne peut pas se reduire a dimensionner le tableau de points au nombre
  // de Vertices, en effet, surtout si l'on veut conserver le mapping 1:1
  // entre les noeuds de l'HTG et ces points du maillage dual.
  // En effet, si l'on definit un GlobalIndex ou un IndexStart specifique
  // cette ecriture simpliste ne fonctionnait plus... tableau trop petit
  // car GetGlobalIndex retourne une valeur > this->GetNumberOfVertices().
  this->Points->SetNumberOfPoints(input->GetGlobalNodeIndexMax() + 1);

  // TODO: find out why we get some uninitialized point coords instead
  this->Points->GetData()->Fill(0.0);

  int numVerts = 1 << input->GetDimension();
  this->Connectivity->SetNumberOfComponents(numVerts);

  // Initialize grid depth
  unsigned int gridDepth = 0;

  // Compute and assign scales of all tree roots
  // double scale[3] = { 1., 1., 1. };

  // Check whether coordinate arrays match grid size
  // If coordinates array are complete, compute all tree scales
  // SEB: int* dims = input->GetDimensions();
  // SEB: if ( dims[0] == input->GetXCoordinates()->GetNumberOfTuples()
  // SEB:      && dims[1] == input->GetYCoordinates()->GetNumberOfTuples()
  // SEB:      && dims[2] == input->GetZCoordinates()->GetNumberOfTuples() )
  if (static_cast<int>(input->GetDimensions()[0]) ==
      input->GetXCoordinates()->GetNumberOfTuples() &&
    static_cast<int>(input->GetDimensions()[1]) == input->GetYCoordinates()->GetNumberOfTuples() &&
    static_cast<int>(input->GetDimensions()[2]) == input->GetZCoordinates()->GetNumberOfTuples())
  {
    gridDepth = input->GetNumberOfLevels();
  }

  // Compute and store reduction factors for speed
  double factor = 1.;
  for (unsigned short p = 0; p < gridDepth; ++p)
  {
    this->ReductionFactors[p] = .5 * factor;
    factor /= input->GetBranchFactor();
  } // p

  // Retrieve material mask
  vtkBitArray* mask = input->HasMask() ? input->GetMask() : nullptr;

  // Iterate over all hyper trees
  vtkIdType index;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  input->InitializeTreeIterator(it);
  vtkNew<vtkHyperTreeGridNonOrientedMooreSuperCursor> cursor;
  while (it.GetNextTree(index))
  {
    // Initialize new Moore cursor at root of current tree
    input->InitializeNonOrientedMooreSuperCursor(cursor, index);
    // Convert hyper tree into unstructured mesh recursively
    if (mask)
    {
      this->TraverseDualRecursively(cursor, mask, input);
    }
    else
    {
      this->TraverseDualRecursively(cursor, input);
    }
  } // it

  // Adjust dual points as needed to fit the primal boundary
  for (unsigned int d = 0; d < input->GetDimension(); ++d)
  {
    // Iterate over all adjustments for current dimension
    for (std::map<vtkIdType, double>::const_iterator _it = this->PointShifts[d].begin();
         _it != this->PointShifts[d].end(); ++_it)
    {
      double pt[3];

      assert(_it->first < input->GetNumberOfVertices());
      this->Points->GetPoint(_it->first, pt);

      pt[d] += _it->second;

      assert(_it->first < input->GetNumberOfVertices());
      this->Points->SetPoint(_it->first, pt);
    } // it
    this->PointShifts[d].clear();
  } // d
  this->PointShifted.clear();

  // now populate my output from the mesh internals made above
  output->SetPoints(this->Points);
  output->GetPointData()->ShallowCopy(input->GetPointData());

  int numPts = 1 << input->GetDimension();
  int type = VTK_VOXEL;
  if (numPts == 4)
  {
    type = VTK_PIXEL;
  }
  if (numPts == 2)
  {
    type = VTK_LINE;
  }
  output->Allocate();
  int numCells = this->Connectivity->GetNumberOfTuples();
  for (int cellIdx = 0; cellIdx < numCells; cellIdx++)
  {
    vtkIdType* ptr = this->Connectivity->GetPointer(0) + cellIdx * numPts;
    output->InsertNextCell(type, numPts, ptr);
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridToDualGrid::TraverseDualRecursively(
  vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor, vtkHyperTreeGrid* input)
{
  // TODO - code duplication is evil, fold this with other TraverseDual
  // Create cell corner if cursor is at leaf
  if (cursor->IsLeaf())
  {
    // Center is a leaf, create dual items depending on dimension
    switch (input->GetDimension())
    {
      case 1:
        this->GenerateDualCornerFromLeaf1D(cursor, input);
        break;
      case 2:
        this->GenerateDualCornerFromLeaf2D(cursor, input);
        break;
      case 3:
        this->GenerateDualCornerFromLeaf3D(cursor, input);
        break;
    } // switch ( this->Dimension )
  }   // if ( cursor->IsLeaf() )
  else
  {
    // Cursor is not at leaf, recurse to all children
    int numChildren = input->GetNumberOfChildren();
    for (int child = 0; child < numChildren; ++child)
    {
      cursor->ToChild(child);
      // Recurse
      this->TraverseDualRecursively(cursor, input);
      cursor->ToParent();
    } // child
  }   // else
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridToDualGrid::GenerateDualCornerFromLeaf1D(
  vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor, vtkHyperTreeGrid* input)
{
  // With d=1:
  //   (d-0)-faces are corners, neighbor cursors are 0 and 2
  //   (d-1)-faces do not exist
  //   (d-2)-faces do not exist

  // Retrieve neighbor (left/right) cursors
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorL =
    cursor->GetOrientedGeometryCursor(0);
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorR =
    cursor->GetOrientedGeometryCursor(2);

  // Retrieve cursor center coordinates
  double pt[3];
  cursor->GetPoint(pt);

  // Check across d-face neighbors whether point must be adjusted
  if (!cursorL->HasTree())
  {
    // Move to left corner
    pt[input->GetOrientation()] -= .5 * cursor->GetSize()[input->GetOrientation()];
    ;
  }
  if (!cursorR->HasTree())
  {
    // Move to right corner
    pt[input->GetOrientation()] += .5 * cursor->GetSize()[input->GetOrientation()];
    ;
  }

  // Retrieve global index of center cursor
  vtkIdType id = cursor->GetGlobalNodeIndex();

  // Insert dual point at center of leaf cell
  this->Points->SetPoint(id, pt);

  // Storage for edge vertex IDs: dual cell ownership to cursor with higher index
  vtkIdType ids[2];
  ids[0] = id;

  // Check whether a dual edge to left neighbor exists
  if (cursorL->HasTree() && cursorL->IsLeaf())
  {
    // If left neighbor is a leaf, always create an edge
    ids[1] = cursorL->GetGlobalNodeIndex();
    this->Connectivity->InsertNextTypedTuple(ids);
  }

  // Check whether a dual edge to right neighbor exists
  if ((cursorR->HasTree() && cursorR->IsLeaf()) && cursorR->GetLevel() != cursor->GetLevel())
  {
    // If right neighbor is a leaf, create an edge only if right cell at higher level
    ids[1] = cursorR->GetGlobalNodeIndex();
    this->Connectivity->InsertNextTypedTuple(ids);
  }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridToDualGrid::GenerateDualCornerFromLeaf2D(
  vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor, vtkHyperTreeGrid* input)
{
  // With d=2:
  //   (d-0)-faces are edges, neighbor cursors are 1, 3, 5, 7
  //   (d-1)-faces are corners, neighbor cursors are 0, 2, 6, 8
  //   (d-2)-faces do not exist

  // Retrieve (d-0)-neighbor (south/east/west/north) cursors
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorS =
    cursor->GetOrientedGeometryCursor(1);
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorW =
    cursor->GetOrientedGeometryCursor(3);
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorE =
    cursor->GetOrientedGeometryCursor(5);
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorN =
    cursor->GetOrientedGeometryCursor(7);

  // Retrieve (d-1)-neighbor (southwest/southeast/northwest/northeast) cursors
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorSW =
    cursor->GetOrientedGeometryCursor(0);
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorSE =
    cursor->GetOrientedGeometryCursor(2);
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorNW =
    cursor->GetOrientedGeometryCursor(6);
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorNE =
    cursor->GetOrientedGeometryCursor(8);

  // Retrieve 2D axes (east-west/south-north)
  unsigned int axisWE = input->GetOrientation() ? 0 : 1;
  unsigned int axisSN = input->GetOrientation() == 2 ? 1 : 2;

  // Retrieve cursor center coordinates
  double pt[3];
  cursor->GetPoint(pt);

  // Compute potential shifts
  double shift[2];
  shift[0] = .5 * cursor->GetSize()[axisWE];
  shift[1] = .5 * cursor->GetSize()[axisSN];

  // Check across edge neighbors whether point must be adjusted
  if (!cursorS->HasTree())
  {
    // Move to south edge
    pt[axisSN] -= shift[1];
  }
  if (!cursorW->HasTree())
  {
    // Move to west edge
    pt[axisWE] -= shift[0];
  }
  if (!cursorE->HasTree())
  {
    // Move to east edge
    pt[axisWE] += shift[0];
  }
  if (!cursorN->HasTree())
  {
    // Move to north edge
    pt[axisSN] += shift[1];
  }

  // Retrieve global index of center cursor
  vtkIdType id = cursor->GetGlobalNodeIndex();

  // Insert dual point at center of leaf cell
  this->Points->SetPoint(id, pt);

  // Storage for edge vertex IDs: dual cell ownership to cursor with higher index
  vtkIdType ids[4];
  ids[0] = id;

  // Retrieve current level to break corner ownership ties
  unsigned int level = cursor->GetLevel();

  // Check whether a dual cell around SW corner exists
  if (cursorSW->HasTree() && cursorSW->IsLeaf() && cursorS->HasTree() && cursorS->IsLeaf() &&
    cursorW->HasTree() && cursorW->IsLeaf())
  {
    // If SW, S, and W neighbors are leaves, always create a face
    ids[1] = cursorW->GetGlobalNodeIndex();
    ids[2] = cursorS->GetGlobalNodeIndex();
    ids[3] = cursorSW->GetGlobalNodeIndex();
    this->Connectivity->InsertNextTypedTuple(ids);
  }

  // Check whether a dual cell around SE corner exists
  if (cursorS->HasTree() && cursorS->IsLeaf() && cursorSE->HasTree() && cursorSE->IsLeaf() &&
    cursorE->HasTree() && cursorE->IsLeaf() && level != cursorE->GetLevel())
  {
    // If S, SE, and E neighbors are leaves, create a face if E at higher level
    ids[1] = cursorE->GetGlobalNodeIndex();
    ids[2] = cursorS->GetGlobalNodeIndex();
    ids[3] = cursorSE->GetGlobalNodeIndex();
    this->Connectivity->InsertNextTypedTuple(ids);
  }

  // Check whether a dual cell around NE corner exists
  if (cursorE->HasTree() && cursorE->IsLeaf() && cursorNE->HasTree() && cursorNE->IsLeaf() &&
    cursorN->HasTree() && cursorN->IsLeaf() && level != cursorE->GetLevel() &&
    level != cursorNE->GetLevel() && level != cursorN->GetLevel())
  {
    // If E, NE, and N neighbors are leaves, create a face if E, NE, N at higher level
    ids[1] = cursorE->GetGlobalNodeIndex();
    ids[2] = cursorN->GetGlobalNodeIndex();
    ids[3] = cursorNE->GetGlobalNodeIndex();
    this->Connectivity->InsertNextTypedTuple(ids);
  }

  // Check whether a dual cell around NW corner exists
  if (cursorW->HasTree() && cursorW->IsLeaf() && cursorN->HasTree() && cursorN->IsLeaf() &&
    cursorNW->HasTree() && cursorNW->IsLeaf() && level != cursorNW->GetLevel() &&
    level != cursorN->GetLevel())
  {
    // If W, N, and NW neighbors are leaves, create a face if NW and N at higher level
    ids[1] = cursorW->GetGlobalNodeIndex();
    ids[2] = cursorN->GetGlobalNodeIndex();
    ids[3] = cursorNW->GetGlobalNodeIndex();
    this->Connectivity->InsertNextTypedTuple(ids);
  }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridToDualGrid::GenerateDualCornerFromLeaf3D(
  vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor, vtkHyperTreeGrid* vtkNotUsed(input))
{
  // With d=3:
  //   (d-0)-faces are faces, neighbor cursors are 4, 10, 12, 14, 16, 22
  //   (d-1)-faces are edges, neighbor cursors are 1, 3, 5, 7, 9, 11, 15, 17, 19, 21, 23, 25
  //   (d-2)-faces are corners, neighbor cursors are 0, 2, 6, 8, 18, 20, 24, 26

  // Retrieve cursors
  std::vector<vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> > cursors;
  cursors.resize(27);
  for (unsigned int c = 0; c < 27; ++c)
  {
    cursors[c] = cursor->GetOrientedGeometryCursor(c);
  }

  // Retrieve cursor center coordinates
  double pt[3];
  cursor->GetPoint(pt);

  // Compute potential shifts
  double shift[3];
  shift[0] = .5 * cursor->GetSize()[0];
  shift[1] = .5 * cursor->GetSize()[1];
  shift[2] = .5 * cursor->GetSize()[2];

  // Index offset relative to center cursor (13)
  unsigned int offset = 1;

  // Check across face neighbors whether point must be adjusted
  for (unsigned int axis = 0; axis < 3; ++axis, offset *= 3)
  {
    if (!cursors[13 - offset]->HasTree())
    {
      // Move to negative side along axis
      pt[axis] -= shift[axis];
    }
    if (!cursors[13 + offset]->HasTree())
    {
      // Move to positive side along axis
      pt[axis] += shift[axis];
    }
  } // axis

  // Retrieve global index of center cursor
  vtkIdType id = cursor->GetGlobalNodeIndex();

  // Insert dual point at center of leaf cell
  this->Points->SetPoint(id, pt);

  // Storage for edge vertex IDs: dual cell ownership to cursor with higher index
  vtkIdType ids[8];

  // Retrieve current level to break corner ownership ties
  unsigned int level = cursor->GetLevel();

  // Iterate over leaf corners
  for (unsigned int c = 0; c < 8; ++c)
  {
    // Assume center cursor leaf owns the corner
    bool owner = true;

    // Iterate over every leaf touching the corner
    for (unsigned int l = 0; l < 8 && owner; ++l)
    {
      // Retrieve cursor index of touching leaf
      unsigned int index = CornerNeighborCursorsTable3D[c][l];

      // Compute whether corner is owned by another leaf
      if (index != 13)
      {
        if (!cursors[index]->HasTree() || !cursors[index]->IsLeaf() ||
          (cursors[index]->GetLevel() == level && index > 13))
        {
          // If neighbor leaf is out of bounds or has not been
          // refined to a leaf, this leaf does not own the corner
          // A level tie is broken in favor of the largest index
          owner = false;
        }
        else
        {
          // Collect the leaf indices for the dual cell
          ids[l] = cursors[index]->GetGlobalNodeIndex();
        }
      }
      else
      { // if ( index != 13 )
        // Collect the leaf indices for the dual cell
        ids[l] = cursors[index]->GetGlobalNodeIndex();
      } // else
    }   // l

    // If leaf owns the corner, create dual cell
    if (owner)
    {
      this->Connectivity->InsertNextTypedTuple(ids);
    } // if ( owner )
  }   // c
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridToDualGrid::TraverseDualRecursively(
  vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor, vtkBitArray* mask, vtkHyperTreeGrid* input)
{
  // TODO - code duplication is evil, fold this with other TraverseDual
  // Create cell corner if cursor is at leaf
  if (cursor->IsLeaf())
  {
    // Cursor is at leaf, retrieve its global index
    vtkIdType id = cursor->GetGlobalNodeIndex();

    // Center is a leaf, create dual items depending on dimension
    if (mask->GetValue(id))
    {
      switch (input->GetDimension())
      {
        case 2:
          this->ShiftDualCornerFromMaskedLeaf2D(cursor, mask, input);
          break;
        case 3:
          this->ShiftDualCornerFromMaskedLeaf3D(cursor, mask, input);
      } // switch ( this->Dimension )
    }
    else
    {
      switch (input->GetDimension())
      {
        case 1:
          this->GenerateDualCornerFromLeaf1D(cursor, input);
          break;
        case 2:
          this->GenerateDualCornerFromLeaf2D(cursor, mask, input);
          break;
        case 3:
          this->GenerateDualCornerFromLeaf3D(cursor, mask, input);
      } // switch ( this->Dimension )
    }   // else
  }     // if ( cursor->IsLeaf() )
  else
  {
    // Cursor is not at leaf, recurse to all children
    int numChildren = input->GetNumberOfChildren();
    for (int child = 0; child < numChildren; ++child)
    {
      cursor->ToChild(child);
      // Recurse
      this->TraverseDualRecursively(cursor, mask, input);
      cursor->ToParent();
    } // child
  }   // else
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridToDualGrid::ShiftDualCornerFromMaskedLeaf2D(
  vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor, vtkBitArray* mask, vtkHyperTreeGrid* input)
{
  // With d=2:
  //   (d-0)-faces are edges, neighbor cursors are 1, 3, 5, 7
  //   (d-1)-faces are corners, neighbor cursors are 0, 2, 6, 8
  //   (d-2)-faces do not exist

  // Retrieve (d-0)-neighbor (south/east/west/north) cursors
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorS =
    cursor->GetOrientedGeometryCursor(1);
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorW =
    cursor->GetOrientedGeometryCursor(3);
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorE =
    cursor->GetOrientedGeometryCursor(5);
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorN =
    cursor->GetOrientedGeometryCursor(7);

  // Retrieve (d-1)-neighbor (southwest/southeast/northwest/northeast) cursors
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorSW =
    cursor->GetOrientedGeometryCursor(0);
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorSE =
    cursor->GetOrientedGeometryCursor(2);
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorNW =
    cursor->GetOrientedGeometryCursor(6);
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorNE =
    cursor->GetOrientedGeometryCursor(8);

  // Retrieve global indices of non-center cursors

  // Retrieve 2D axes (east-west/south-north)
  unsigned int axisWE = input->GetOrientation() ? 0 : 1;
  unsigned int axisSN = input->GetOrientation() == 2 ? 1 : 2;

  // Retrieve current level to break corner ownership ties
  unsigned int level = cursor->GetLevel();

  // Check whether dual point across S edge must be adjusted
  if (cursorS->HasTree() && cursorS->IsLeaf() && cursorS->GetLevel() < level)
  {
    vtkIdType idS = cursorS->GetGlobalNodeIndex();
    if (!mask->GetValue(idS))
    {
      // Dual point must be adjusted
      this->PointShifted[idS] = true;
      this->PointShifts[axisSN][idS] =
        cursorS->GetTree()->GetScale(axisSN) * this->ReductionFactors[cursorS->GetLevel()];
    }
  }

  // Check whether dual point across W edge must be adjusted
  if (cursorW->HasTree() && cursorW->IsLeaf() && cursorW->GetLevel() < level)
  {
    vtkIdType idW = cursorW->GetGlobalNodeIndex();
    if (!mask->GetValue(idW))
    {
      // Dual point must be adjusted
      this->PointShifted[idW] = true;
      this->PointShifts[axisWE][idW] =
        cursorW->GetTree()->GetScale(axisWE) * this->ReductionFactors[cursorW->GetLevel()];
    }
  }

  // Check whether dual point across E face must be adjusted
  if (cursorE->HasTree() && cursorE->IsLeaf() && cursorE->GetLevel() < level)
  {
    vtkIdType idE = cursorE->GetGlobalNodeIndex();
    if (!mask->GetValue(idE))
    {
      // Dual point must be adjusted
      this->PointShifted[idE] = true;
      this->PointShifts[axisWE][idE] =
        -cursorE->GetTree()->GetScale(axisWE) * this->ReductionFactors[cursorE->GetLevel()];
    }
  }

  // Check whether dual point across N edge must be adjusted
  if (cursorN->HasTree() && cursorN->IsLeaf() && cursorN->GetLevel() < level)
  {
    vtkIdType idN = cursorN->GetGlobalNodeIndex();
    if (!mask->GetValue(idN))
    {
      // Dual point must be adjusted
      this->PointShifted[idN] = true;
      this->PointShifts[axisSN][idN] =
        -cursorN->GetTree()->GetScale(axisSN) * this->ReductionFactors[cursorN->GetLevel()];
    }
  }

  // Check whether dual point across SE corner must be adjusted
  if (cursorSE->HasTree() && cursorSE->IsLeaf() && cursorSE->GetLevel() < level)
  {
    vtkIdType idSE = cursorSE->GetGlobalNodeIndex();
    if (!mask->GetValue(idSE) && !this->PointShifted[idSE])
    {
      // Dual point must be adjusted
      double shift[3];
      cursorSE->GetTree()->GetScale(shift);
      double factor = this->ReductionFactors[cursorSE->GetLevel()];
      this->PointShifts[axisWE][idSE] = factor * shift[axisWE];
      this->PointShifts[axisSN][idSE] = factor * shift[axisSN];
    }
  }

  // Check whether dual point across SW corner must be adjusted
  if (cursorSW->HasTree() && cursorSW->IsLeaf() && cursorSW->GetLevel() < level)
  {
    vtkIdType idSW = cursorSW->GetGlobalNodeIndex();
    if (!mask->GetValue(idSW) && !this->PointShifted[idSW])
    {
      // Dual point must be adjusted
      double shift[3];
      cursorSW->GetTree()->GetScale(shift);
      double factor = this->ReductionFactors[cursorSW->GetLevel()];
      this->PointShifts[axisWE][idSW] = -factor * shift[axisWE];
      this->PointShifts[axisSN][idSW] = factor * shift[axisSN];
    }
  }

  // Check whether dual point across NW corner must be adjusted
  if (cursorNW->HasTree() && cursorNW->IsLeaf() && cursorNW->GetLevel() < level)
  {
    vtkIdType idNW = cursorNW->GetGlobalNodeIndex();
    if (!mask->GetValue(idNW) && !this->PointShifted[idNW])
    {
      // Dual point must be adjusted
      double shift[3];
      cursorNW->GetTree()->GetScale(shift);
      double factor = this->ReductionFactors[cursorNW->GetLevel()];
      this->PointShifts[axisWE][idNW] = factor * shift[axisWE];
      this->PointShifts[axisSN][idNW] = -factor * shift[axisSN];
    }
  }

  // Check whether dual point across NE corner must be adjusted
  if (cursorNE->HasTree() && cursorNE->IsLeaf() && cursorNE->GetLevel() < level)
  {
    vtkIdType idNE = cursorNE->GetGlobalNodeIndex();
    if (!mask->GetValue(idNE) && !this->PointShifted[idNE])
    {
      // Dual point must be adjusted
      double shift[3];
      cursorNE->GetTree()->GetScale(shift);
      double factor = this->ReductionFactors[cursorNE->GetLevel()];
      this->PointShifts[axisWE][idNE] = -factor * shift[axisWE];
      this->PointShifts[axisSN][idNE] = -factor * shift[axisSN];
    }
  }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridToDualGrid::ShiftDualCornerFromMaskedLeaf3D(
  vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor, vtkBitArray* mask,
  vtkHyperTreeGrid* vtkNotUsed(input))
{
  // With d=3:
  //   (d-0)-faces are faces, neighbor cursors are 4, 10, 12, 14, 16, 22
  //   (d-1)-faces are edges, neighbor cursors are 1, 3, 5, 7, 9, 11, 15, 17, 19, 21, 23, 25
  //   (d-2)-faces are corners, neighbor cursors are 0, 2, 6, 8, 18, 20, 24, 26

  // Retrieve current level to break corner ownership ties
  unsigned int level = cursor->GetLevel();

  // Check whether dual points across face neighbors must be adjusted
  int offset = 1;
  for (unsigned int axis = 0; axis < 3; ++axis, offset *= 3)
  {
    vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorM =
      cursor->GetOrientedGeometryCursor(13 - offset);
    if (cursorM->HasTree() && cursorM->IsLeaf() && cursorM->GetLevel() < level)
    {
      vtkIdType idM = cursorM->GetGlobalNodeIndex();
      if (!mask->GetValue(idM))
      {
        // Dual point must be adjusted
        this->PointShifted[idM] = true;
        this->PointShifts[axis][idM] =
          cursorM->GetTree()->GetScale(axis) * this->ReductionFactors[cursorM->GetLevel()];
      }
    }
    vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorP =
      cursor->GetOrientedGeometryCursor(13 + offset);
    if (cursorP->HasTree() && cursorP->IsLeaf() && cursorP->GetLevel() < level)
    {
      vtkIdType idP = cursorP->GetGlobalNodeIndex();
      if (!mask->GetValue(idP))
      {
        // Dual point must be adjusted
        this->PointShifted[idP] = true;
        this->PointShifts[axis][idP] =
          -cursorP->GetTree()->GetScale(axis) * this->ReductionFactors[cursorP->GetLevel()];
      }
    }
  } // axis

  // Check whether dual points across edge neighbors must be adjusted
  int i = 1;
  for (int axis1 = 0; axis1 < 2; ++axis1, i *= 3)
  {
    int j = 3 * i;
    for (int axis2 = axis1 + 1; axis2 < 3; ++axis2, j *= 3)
    {
      for (int o2 = -1; o2 < 2; o2 += 2)
      {
        for (int o1 = -1; o1 < 2; o1 += 2)
        {
          int index = 13 + o1 * (i * o2 + j);
          vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorE =
            cursor->GetOrientedGeometryCursor(static_cast<unsigned int>(index));
          if (cursorE->HasTree() && cursorE->IsLeaf() && cursorE->GetLevel() < level)
          {
            vtkIdType idE = cursorE->GetGlobalNodeIndex();
            if (!mask->GetValue(idE) && !this->PointShifted[idE])
            {
              // Dual point must be adjusted
              this->PointShifted[idE] = true;
              double shift[3];
              cursorE->GetTree()->GetScale(shift);
              double factor = this->ReductionFactors[cursorE->GetLevel()];
              this->PointShifts[axis1][idE] = -o1 * o2 * factor * shift[axis1];
              this->PointShifts[axis2][idE] = -o1 * factor * shift[axis2];
            }
          }
        } // o1
      }   // o2
    }     // axis2
  }       // axis1

  // Check whether dual points across corner neighbors must be adjusted
  for (int o3 = -1; o3 < 2; o3 += 2)
  {
    for (int o2 = -1; o2 < 2; o2 += 2)
    {
      offset = o2 * (o3 + 3) + 9;
      for (int o1 = -1; o1 < 2; o1 += 2)
      {
        int index = 13 + o1 * offset;
        vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorC =
          cursor->GetOrientedGeometryCursor(index);
        if (cursorC->HasTree() && cursorC->IsLeaf() && cursorC->GetLevel() < level)
        {
          vtkIdType idC = cursorC->GetGlobalNodeIndex();
          if (!mask->GetValue(idC) && !this->PointShifted[idC])
          {
            // Dual point must be adjusted
            this->PointShifted[idC] = true;
            double shift[3];
            cursorC->GetTree()->GetScale(shift);
            double factor = this->ReductionFactors[cursorC->GetLevel()];
            this->PointShifts[0][idC] = -o1 * o2 * o3 * factor * shift[0];
            this->PointShifts[1][idC] = -o1 * o2 * factor * shift[1];
            this->PointShifts[2][idC] = -o1 * factor * shift[2];
          }
        }
      } // o1
    }   // o2
  }     // o3
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridToDualGrid::GenerateDualCornerFromLeaf2D(
  vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor, vtkBitArray* mask, vtkHyperTreeGrid* input)
{
  // With d=2:
  //   (d-0)-faces are edges, neighbor cursors are 1, 3, 5, 7
  //   (d-1)-faces are corners, neighbor cursors are 0, 2, 6, 8
  //   (d-2)-faces do not exist

  const unsigned int dS = 1;
  const unsigned int dW = 3;
  const unsigned int dE = 5;
  const unsigned int dN = 7;
  const unsigned int dSW = 0;
  const unsigned int dSE = 2;
  const unsigned int dNW = 6;
  const unsigned int dNE = 8;

  // Retrieve 2D axes (east-west/south-north)
  unsigned int axisWE = input->GetOrientation() ? 0 : 1;
  unsigned int axisSN = input->GetOrientation() == 2 ? 1 : 2;

  // Retrieve cursor center coordinates
  double pt[3];
  cursor->GetPoint(pt);

  // Compute potential shifts
  double shift[2];
  shift[0] = .5 * cursor->GetSize()[axisWE];
  shift[1] = .5 * cursor->GetSize()[axisSN];

  // When a mask is present, edge as well as face shifts are possible
  bool shifted = false;

  // Check across edge neighbors whether point must be adjusted
  if (!cursor->HasTree(dS))
  {
    // Move to south edge
    pt[axisSN] -= shift[1];
    shifted = true;
  }
  else if (cursor->IsLeaf(dS) && mask->GetValue(cursor->GetGlobalNodeIndex(dS)))
  {
    // Move to south edge
    pt[axisSN] -= shift[1];
    shifted = true;
  }

  if (!cursor->HasTree(dW))
  {
    // Move to west edge
    pt[axisWE] -= shift[0];
    shifted = true;
  }
  else if (cursor->IsLeaf(dW) && mask->GetValue(cursor->GetGlobalNodeIndex(dW)))
  {
    // Move to west edge
    pt[axisWE] -= shift[0];
    shifted = true;
  }

  if (!cursor->HasTree(dE))
  {
    // Move to east edge
    pt[axisWE] += shift[0];
    shifted = true;
  }
  else if (cursor->IsLeaf(dE) && mask->GetValue(cursor->GetGlobalNodeIndex(dE)))
  {
    // Move to east edge
    pt[axisWE] += shift[0];
    shifted = true;
  }

  if (!cursor->HasTree(dN))
  {
    // Move to north edge
    pt[axisSN] += shift[1];
    shifted = true;
  }
  else if (cursor->IsLeaf(dN) && mask->GetValue(cursor->GetGlobalNodeIndex(dN)))
  {
    // Move to north edge
    pt[axisSN] += shift[1];
    shifted = true;
  }

  // Only when point was not moved to edge, check corner neighbors
  if (!shifted)
  {
    if (!cursor->HasTree(dSW))
    {
      // Move to southwest corner
      pt[axisWE] -= shift[0];
      pt[axisSN] -= shift[1];
    }
    else if (cursor->IsLeaf(dSW) && mask->GetValue(cursor->GetGlobalNodeIndex(dSW)))
    {
      // Move to southwest corner
      pt[axisWE] -= shift[0];
      pt[axisSN] -= shift[1];
    }

    if (!cursor->HasTree(dSE))
    {
      // Move to southeast corner
      pt[axisWE] += shift[0];
      pt[axisSN] -= shift[1];
    }
    else if (cursor->IsLeaf(dSE) && mask->GetValue(cursor->GetGlobalNodeIndex(dSE)))
    {
      // Move to southeast corner
      pt[axisWE] += shift[0];
      pt[axisSN] -= shift[1];
    }

    if (!cursor->HasTree(dNW))
    {
      // Move to northwest corner
      pt[axisWE] -= shift[0];
      pt[axisSN] += shift[1];
    }
    else if (cursor->IsLeaf(dNW) && mask->GetValue(cursor->GetGlobalNodeIndex(dNW)))
    {
      // Move to northwest corner
      pt[axisWE] -= shift[0];
      pt[axisSN] += shift[1];
    }

    if (!cursor->HasTree(dNE))
    {
      // Move to northeast corner
      pt[axisWE] += shift[0];
      pt[axisSN] += shift[1];
    }
    else if (cursor->IsLeaf(dNE) && mask->GetValue(cursor->GetGlobalNodeIndex(dNE)))
    {
      // Move to northeast corner
      pt[axisWE] += shift[0];
      pt[axisSN] += shift[1];
    }
  } // if ( ! shifted )

  // Retrieve global index of center cursor
  vtkIdType id = cursor->GetGlobalNodeIndex();

  // Insert dual point at center of leaf cell
  assert(id < input->GetNumberOfVertices());
  this->Points->SetPoint(id, pt);

  // If cell is masked, terminate recursion, no dual cell will be generated
  if (mask->GetValue(id))
  {
    return;
  }

  // Storage for edge vertex IDs: dual cell ownership to cursor with higher index
  vtkIdType ids[4];
  ids[0] = id;

  // Retrieve current level to break corner ownership ties
  unsigned int level = cursor->GetLevel();

  // Check whether a dual cell around SW corner exists
  if (cursor->HasTree(dSW) && cursor->HasTree(dS) && cursor->HasTree(dW) && cursor->IsLeaf(dSW) &&
    cursor->IsLeaf(dS) && cursor->IsLeaf(dW))
  {
    vtkIdType idSW, idS, idW;
    if (!mask->GetValue(idSW = cursor->GetGlobalNodeIndex(dSW)) &&
      !mask->GetValue(idS = cursor->GetGlobalNodeIndex(dS)) &&
      !mask->GetValue(idW = cursor->GetGlobalNodeIndex(dW)))
    {
      // If SW, S, and W neighbors are leaves, always create a face
      ids[1] = idW;
      ids[2] = idS;
      ids[3] = idSW;
      this->Connectivity->InsertNextTypedTuple(ids);
    }
  }

  // Check whether a dual cell around SE corner exists
  if (cursor->HasTree(dS) && cursor->HasTree(dSE) && cursor->HasTree(dE) && cursor->IsLeaf(dS) &&
    cursor->IsLeaf(dSE) && cursor->IsLeaf(dE))
  {
    vtkIdType idS, idSE, idE;
    if (!mask->GetValue(idS = cursor->GetGlobalNodeIndex(dS)) &&
      !mask->GetValue(idSE = cursor->GetGlobalNodeIndex(dSE)) &&
      !mask->GetValue(idE = cursor->GetGlobalNodeIndex(dE)) && level != cursor->GetLevel(dE))
    {
      // If S, SE, and E neighbors are leaves, create a face if E at higher level
      ids[1] = idE;
      ids[2] = idS;
      ids[3] = idSE;
      this->Connectivity->InsertNextTypedTuple(ids);
    }
  }

  // Check whether a dual cell around NE corner exists
  if (cursor->HasTree(dE) && cursor->HasTree(dNE) && cursor->HasTree(dN) && cursor->IsLeaf(dE) &&
    cursor->IsLeaf(dNE) && cursor->IsLeaf(dN))
  {
    vtkIdType idE, idNE, idN;
    if (!mask->GetValue(idE = cursor->GetGlobalNodeIndex(dE)) &&
      !mask->GetValue(idNE = cursor->GetGlobalNodeIndex(dNE)) &&
      !mask->GetValue(idN = cursor->GetGlobalNodeIndex(dN)) && level != cursor->GetLevel(dE) &&
      level != cursor->GetLevel(dNE) && level != cursor->GetLevel(dN))
    {
      // If E, NE, and N neighbors are leaves, create a face if E, NE, N at higher level
      ids[1] = idE;
      ids[2] = idN;
      ids[3] = idNE;
      this->Connectivity->InsertNextTypedTuple(ids);
    }
  }

  // Check whether a dual cell around NW corner exists
  if (cursor->HasTree(dW) && cursor->HasTree(dN) && cursor->HasTree(dNW) && cursor->IsLeaf(dW) &&
    cursor->IsLeaf(dN) && cursor->IsLeaf(dNW))
  {
    vtkIdType idW, idN, idNW;
    if (!mask->GetValue(idW = cursor->GetGlobalNodeIndex(dW)) &&
      !mask->GetValue(idN = cursor->GetGlobalNodeIndex(dN)) &&
      !mask->GetValue(idNW = cursor->GetGlobalNodeIndex(dNW)) && level != cursor->GetLevel(dNW) &&
      level != cursor->GetLevel(dN))
    {
      // If W, N, and NW neighbors are leaves, create a face if NW and N at higher level
      ids[1] = idW;
      ids[2] = idN;
      ids[3] = idNW;
      this->Connectivity->InsertNextTypedTuple(ids);
    }
  }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridToDualGrid::GenerateDualCornerFromLeaf3D(
  vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor, vtkBitArray* mask,
  vtkHyperTreeGrid* vtkNotUsed(input))
{
  // With d=3:
  //   (d-0)-faces are faces, neighbor cursors are 4, 10, 12, 14, 16, 22
  //   (d-1)-faces are edges, neighbor cursors are 1, 3, 5, 7, 9, 11, 15, 17, 19, 21, 23, 25
  //   (d-2)-faces are corners, neighbor cursors are 0, 2, 6, 8, 18, 20, 24, 26

  // Retrieve cursor center coordinates
  double pt[3];
  cursor->GetPoint(pt);

  // Compute potential shifts
  double shift[3];
  shift[0] = .5 * cursor->GetSize()[0];
  shift[1] = .5 * cursor->GetSize()[1];
  shift[2] = .5 * cursor->GetSize()[2];

  // When a mask is present, corner, edge, and face shifts are possible
  bool shifted = false;

  // Check across face neighbors whether point must be adjusted
  unsigned int offset = 1;
  for (unsigned int axis = 0; axis < 3; ++axis, offset *= 3)
  {
    vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorM =
      cursor->GetOrientedGeometryCursor(13 - offset);
    if (!cursorM->HasTree())
    {
      // Move to negative side along axis
      pt[axis] -= shift[axis];
      shifted = true;
    }
    else
    {
      vtkIdType idM = cursorM->GetGlobalNodeIndex();
      if (cursorM->IsLeaf() && mask->GetValue(idM))
      {
        // Move to negative side along axis
        pt[axis] -= shift[axis];
        shifted = true;
      }
    }
    vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorP =
      cursor->GetOrientedGeometryCursor(13 + offset);
    if (!cursorP->HasTree())
    {
      // Move to positive side along axis
      pt[axis] += shift[axis];
      shifted = true;
    }
    else
    {
      vtkIdType idP = cursorP->GetGlobalNodeIndex();
      if (cursorP->IsLeaf() && mask->GetValue(idP))
      {
        // Move to positive side along axis
        pt[axis] += shift[axis];
        shifted = true;
      }
    }
  } // axis

  // Only when point was not moved to face, check edge neighbors
  if (!shifted)
  {
    int i = 1;
    for (int axis1 = 0; axis1 < 2; ++axis1, i *= 3)
    {
      int j = 3 * i;
      for (int axis2 = axis1 + 1; axis2 < 3; ++axis2, j *= 3)
      {
        for (int o2 = -1; o2 < 2; o2 += 2)
        {
          for (int o1 = -1; o1 < 2; o1 += 2)
          {
            int index = 13 + o1 * (i * o2 + j);
            vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorE =
              cursor->GetOrientedGeometryCursor(static_cast<unsigned int>(index));
            if (!cursorE->HasTree())
            {
              // Move to corresponding edge
              pt[axis1] += o1 * o2 * shift[axis1];
              pt[axis2] += o1 * shift[axis2];
              shifted = true;
            }
            else
            {
              vtkIdType idE = cursorE->GetGlobalNodeIndex();
              if (cursorE->IsLeaf() && mask->GetValue(idE))
              {
                // Move to corresponding edge
                pt[axis1] += o1 * o2 * shift[axis1];
                pt[axis2] += o1 * shift[axis2];
                shifted = true;
              }
            }
          } // o1
        }   // o2
      }     // axis2
    }       // axis1
  }         // if ( ! shifted )

  // Only when point was neither moved to face nor to edge, check corners neighbors
  if (!shifted)
  {
    // Iterate over all 8 corners
    for (int o3 = -1; o3 < 2; o3 += 2)
    {
      for (int o2 = -1; o2 < 2; o2 += 2)
      {
        offset = o2 * (o3 + 3) + 9;
        for (int o1 = -1; o1 < 2; o1 += 2)
        {
          int index = 13 + o1 * static_cast<int>(offset);
          vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorC =
            cursor->GetOrientedGeometryCursor(static_cast<unsigned int>(index));
          if (!cursorC->HasTree())
          {
            // Move to corresponding corner
            pt[0] += o1 * o2 * o3 * shift[0];
            pt[1] += o1 * o2 * shift[1];
            pt[2] += o1 * shift[2];
          }
          else
          { // if cursor
            vtkIdType idC = cursorC->GetGlobalNodeIndex();
            if (cursorC->IsLeaf() && mask->GetValue(idC))
            {
              // Move to corresponding corner
              pt[0] += o1 * o2 * o3 * shift[0];
              pt[1] += o1 * o2 * shift[1];
              pt[2] += o1 * shift[2];
            }
          } // if cursor
        }   // o1
      }     // o2
    }       // o3
  }         // if ( ! shifted )

  // Retrieve global index of center cursor
  vtkIdType id = cursor->GetGlobalNodeIndex();

  // Insert dual point at center of leaf cell
  // assert ( id < input->GetGlobalNodeIndexMax() + 1 );
  this->Points->SetPoint(id, pt);

  // Storage for edge vertex IDs: dual cell ownership to cursor with higher index
  vtkIdType ids[8];

  // Retrieve current level to break corner ownership ties
  unsigned int level = cursor->GetLevel();

  // Iterate over leaf corners
  for (unsigned int c = 0; c < 8; ++c)
  {
    // Assume center cursor leaf owns the corner
    bool owner = true;
    unsigned int real_l = 0;

    // Iterate over every leaf touching the corner
    for (unsigned int l = 0; l < 8 && owner; ++l)
    {
      // Retrieve cursor index of touching leaf
      unsigned int index = CornerNeighborCursorsTable3D[c][l];

      // Compute whether corner is owned by another leaf
      if (index != 13)
      {
        if (!cursor->HasTree(index) || !cursor->IsLeaf(index) ||
          (cursor->GetLevel(index) == level && index > 13))
        {
          // If neighbor leaf is out of bounds or has not been
          // refined to a leaf, this leaf does not own the corner
          // A level tie is broken in favor of the largest index
          owner = false;
        }
        else
        {
          vtkIdType idglobal = cursor->GetGlobalNodeIndex(index);
          if (!mask->GetValue(idglobal))
          {
            // Collect the leaf indices for the dual cell
            ids[real_l++] = cursor->GetGlobalNodeIndex(index);
          }
          else
          {
            owner = false;
          }
        }
      }
      else
      { // if ( index != 13 )
        // Collect the leaf indices for the dual cell
        ids[real_l++] = id;
      } // else
    }   // l

    // If leaf owns the corner, create dual cell
    if (owner)
    {
      if (real_l != 8)
      {
        if (real_l == 0)
        {
          continue;
        }
        vtkIdType last = ids[real_l - 1];
        for (; real_l < 8; ++real_l)
        {
          ids[real_l] = last;
        }
      }
      this->Connectivity->InsertNextTypedTuple(ids);
    } // if ( owner )
  }   // c
}
