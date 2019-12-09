/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridContour.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridContour.h"

#include "vtkBitArray.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkContourHelper.h"
#include "vtkContourValues.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridNonOrientedMooreSuperCursor.h"
#include "vtkIdTypeArray.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLine.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkPixel.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVoxel.h"

static const unsigned int MooreCursors1D[2] = { 0, 2 };
static const unsigned int MooreCursors2D[8] = { 0, 1, 2, 3, 5, 6, 7, 8 };
static const unsigned int MooreCursors3D[26] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26 };
static const unsigned int* MooreCursors[3] = {
  MooreCursors1D,
  MooreCursors2D,
  MooreCursors3D,
};

vtkStandardNewMacro(vtkHyperTreeGridContour);

//-----------------------------------------------------------------------------
vtkHyperTreeGridContour::vtkHyperTreeGridContour()
{
  // Initialize storage for contour values
  this->ContourValues = vtkContourValues::New();

  // Initialize locator to null
  this->Locator = nullptr;

  // Initialize list of selected cells
  this->SelectedCells = nullptr;

  // Initialize per-cell quantities of interest
  this->CellSigns = nullptr;
  this->CellScalars = nullptr;

  // Initialize structures for isocontouring
  this->Helper = nullptr;
  this->Leaves = vtkIdList::New();
  this->Line = vtkLine::New();
  this->Pixel = vtkPixel::New();
  this->Voxel = vtkVoxel::New();

  // Output indices begin at 0
  this->CurrentId = 0;

  // Process active point scalars by default
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, vtkDataSetAttributes::SCALARS);

  // Input scalars point to null by default
  this->InScalars = nullptr;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridContour::~vtkHyperTreeGridContour()
{
  if (this->ContourValues)
  {
    this->ContourValues->Delete();
    this->ContourValues = nullptr;
  }

  if (this->Locator)
  {
    this->Locator->Delete();
    this->Locator = nullptr;
  }

  if (this->Line)
  {
    this->Line->Delete();
    this->Line = nullptr;
  }

  if (this->Pixel)
  {
    this->Pixel->Delete();
    this->Pixel = nullptr;
  }

  if (this->Voxel)
  {
    this->Voxel->Delete();
    this->Voxel = nullptr;
  }

  if (this->Leaves)
  {
    this->Leaves->Delete();
    this->Leaves = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridContour::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  this->ContourValues->PrintSelf(os, indent.GetNextIndent());

  os << indent << "CurrentId: " << this->CurrentId << endl;

  if (this->InScalars)
  {
    os << indent << "InScalars:\n";
    this->InScalars->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "InScalars: ( none )\n";
  }

  if (this->Locator)
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }

  if (this->Line)
  {
    os << indent << ": " << this->Line << "\n";
  }
  else
  {
    os << indent << ": (none)\n";
  }

  if (this->Pixel)
  {
    os << indent << ": " << this->Pixel << "\n";
  }
  else
  {
    os << indent << ": (none)\n";
  }

  if (this->Voxel)
  {
    os << indent << ": " << this->Voxel << "\n";
  }
  else
  {
    os << indent << ": (none)\n";
  }

  if (this->Leaves)
  {
    os << indent << ": " << this->Leaves << "\n";
  }
  else
  {
    os << indent << ": (none)\n";
  }
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridContour::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridContour::SetLocator(vtkIncrementalPointLocator* locator)
{
  // Check if proposed locator is identical to existing one
  if (this->Locator == locator)
  {
    return;
  }

  // Clean up existing locator instance variable
  if (this->Locator)
  {
    this->Locator->Delete();
    this->Locator = nullptr;
  }

  // Register proposed locator and assign it
  if (locator)
  {
    locator->Register(this);
  }
  this->Locator = locator;

  // Modify time
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridContour::CreateDefaultLocator()
{
  // If no locator instance variable create a merge point one
  if (!this->Locator)
  {
    this->Locator = vtkMergePoints::New();
    this->Locator->Register(this);
    this->Locator->Delete();
  }
}

//-----------------------------------------------------------------------------
vtkMTimeType vtkHyperTreeGridContour::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType time;

  if (this->ContourValues)
  {
    time = this->ContourValues->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  if (this->Locator)
  {
    time = this->Locator->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridContour::ProcessTrees(vtkHyperTreeGrid* input, vtkDataObject* outputDO)
{
  // Downcast output data object to polygonal data set
  vtkPolyData* output = vtkPolyData::SafeDownCast(outputDO);
  if (!output)
  {
    vtkErrorMacro("Incorrect type of output: " << outputDO->GetClassName());
    return 0;
  }

  // Retrieve scalar quantity of interest
  this->InScalars = this->GetInputArrayToProcess(0, input);
  if (!this->InScalars)
  {
    vtkWarningMacro(<< "No scalar data to contour");
    return 1;
  }

  // Initialize output point data
  this->InData = input->GetPointData();
  this->OutData = output->GetPointData();
  this->OutData->CopyAllocate(this->InData);

  // Output indices begin at 0
  this->CurrentId = 0;

  // Retrieve material mask
  this->InMask = input->HasMask() ? input->GetMask() : nullptr;

  // Retrive ghost cells
  this->InGhostArray = input->GetGhostCells();

  // Estimate output size as a multiple of 1024
  vtkIdType numCells = input->GetNumberOfVertices();
  vtkIdType numContours = this->ContourValues->GetNumberOfContours();
  vtkIdType estimatedSize = static_cast<vtkIdType>(pow(static_cast<double>(numCells), .75));
  estimatedSize *= numContours;
  estimatedSize = estimatedSize / 1024 * 1024;
  if (estimatedSize < 1024)
  {
    estimatedSize = 1024;
  }

  // Create storage for output points
  vtkPoints* newPts = vtkPoints::New();
  newPts->Allocate(estimatedSize, estimatedSize);

  // Create storage for output vertices
  vtkCellArray* newVerts = vtkCellArray::New();
  newVerts->AllocateExact(estimatedSize, estimatedSize);

  // Create storage for output lines
  vtkCellArray* newLines = vtkCellArray::New();
  newLines->AllocateExact(estimatedSize, estimatedSize);

  // Create storage for output polygons
  vtkCellArray* newPolys = vtkCellArray::New();
  newPolys->AllocateExact(estimatedSize, estimatedSize);

  // Create storage for output scalar values
  this->CellScalars = this->InScalars->NewInstance();
  this->CellScalars->SetNumberOfComponents(this->InScalars->GetNumberOfComponents());
  this->CellScalars->Allocate(this->CellScalars->GetNumberOfComponents() * 8);

  // Initialize point locator
  if (!this->Locator)
  {
    // Create default locator if needed
    this->CreateDefaultLocator();
  }
  this->Locator->InitPointInsertion(newPts, input->GetBounds(), estimatedSize);

  // Instantiate a contour helper for convenience, with triangle generation on
  this->Helper = new vtkContourHelper(this->Locator, newVerts, newLines, newPolys,
    input->GetPointData(), nullptr, output->GetPointData(), nullptr, estimatedSize, true);

  // Create storage to keep track of selected cells
  this->SelectedCells = vtkBitArray::New();
  this->SelectedCells->SetNumberOfTuples(numCells);

  // Initialize storage for signs and values
  this->CellSigns = (vtkBitArray**)malloc(numContours * sizeof(vtkBitArray*));
  this->Signs.resize(numContours, true);
  for (int c = 0; c < numContours; ++c)
  {
    this->CellSigns[c] = vtkBitArray::New();
    this->CellSigns[c]->SetNumberOfTuples(numCells);
  }

  // First pass across tree roots to evince cells intersected by contours
  vtkIdType index;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  input->InitializeTreeIterator(it);
  vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
  while (it.GetNextTree(index))
  {
    // Initialize new grid cursor at root of current input tree
    input->InitializeNonOrientedCursor(cursor, index);
    // Pre-process tree recursively
    this->RecursivelyPreProcessTree(cursor);
  } // it

  // Second pass across tree roots: now compute isocontours recursively
  input->InitializeTreeIterator(it);
  vtkNew<vtkHyperTreeGridNonOrientedMooreSuperCursor> supercursor;
  while (it.GetNextTree(index))
  {
    // Initialize new Moore cursor at root of current tree
    input->InitializeNonOrientedMooreSuperCursor(supercursor, index);
    // Compute contours recursively
    this->RecursivelyProcessTree(supercursor);
  } // it

  // Set output
  output->SetPoints(newPts);
  if (newVerts->GetNumberOfCells())
  {
    output->SetVerts(newVerts);
  }
  if (newLines->GetNumberOfCells())
  {
    output->SetLines(newLines);
  }
  if (newPolys->GetNumberOfCells())
  {
    output->SetPolys(newPolys);
  }
  newPolys->Delete();

  // Clean up
  this->SelectedCells->Delete();
  for (vtkIdType c = 0; c < this->GetNumberOfContours(); ++c)
  {
    if (this->CellSigns[c])
    {
      this->CellSigns[c]->Delete();
    }
  } // c
  free(this->CellSigns);
  delete this->Helper;
  this->CellScalars->Delete();
  newPts->Delete();
  newVerts->Delete();
  newLines->Delete();
  this->Locator->Initialize();

  // Squeeze output
  output->Squeeze();

  return 1;
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridContour::RecursivelyPreProcessTree(vtkHyperTreeGridNonOrientedCursor* cursor)
{
  // Retrieve global index of input cursor
  vtkIdType id = cursor->GetGlobalNodeIndex();

  if (this->InGhostArray && this->InGhostArray->GetTuple1(id))
  {
    return false;
  }

  // Retrieve number of contours
  vtkIdType numContours = this->ContourValues->GetNumberOfContours();

  // Descend further into input trees only if cursor is not a leaf
  bool selected = false;
  if (!cursor->IsLeaf())
  {
    // Cursor is not at leaf, recurse to all all children
    int numChildren = cursor->GetNumberOfChildren();
    for (int child = 0; child < numChildren; ++child)
    {
      // Create storage for signs relative to contour values
      std::vector<bool> signs(numContours);

      cursor->ToChild(child);

      // Recurse and keep track of whether this branch is selected
      selected |= this->RecursivelyPreProcessTree(cursor);

      // Check if branch not completely selected
      if (!selected)
      {
        // If not, update contour values
        for (int c = 0; c < numContours; ++c)
        {
          // Retrieve global index of child
          vtkIdType childId = cursor->GetGlobalNodeIndex();

          // Compute and store selection flags for current contour
          if (!child)
          {
            // Initialize sign array with sign of first child
            signs[c] = (this->CellSigns[c]->GetTuple1(childId) != 0.0);
          } // if ( ! child )
          else
          {
            // For subsequent children compare their sign with stored value
            if (signs[c] != (this->CellSigns[c]->GetTuple1(childId) != 0.0))
            {
              // A change of sign occurred, therefore cell must selected
              selected = true;
            }
          } // else
        }   // c
      }     // if( ! selected )

      cursor->ToParent();
    } // child
  }
  else if (!this->InGhostArray || !this->InGhostArray->GetTuple1(id))
  {
    // Cursor is at leaf, retrieve its active scalar value
    double val = this->InScalars->GetTuple1(id);

    // Iterate over all contours
    double* values = this->ContourValues->GetValues();
    for (int c = 0; c < numContours; ++c)
    {
      this->Signs[c] = val > values[c];
    }
  } // else

  // Update list of selected cells
  this->SelectedCells->SetTuple1(id, selected);

  // Set signs for all contours
  for (int c = 0; c < numContours; ++c)
  {
    // Parent cell has that of one of its children
    this->CellSigns[c]->SetTuple1(id, this->Signs[c]);
  }

  // Return whether current node was fully selected
  return selected;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridContour::RecursivelyProcessTree(
  vtkHyperTreeGridNonOrientedMooreSuperCursor* supercursor)
{
  // Retrieve global index of input cursor
  vtkIdType id = supercursor->GetGlobalNodeIndex();

  if (this->InGhostArray && this->InGhostArray->GetTuple1(id))
  {
    return;
  }
  // Retrieve dimensionality
  unsigned int dim = supercursor->GetDimension();

  // Descend further into input trees only if cursor is not a leaf
  if (!supercursor->IsLeaf())
  {
    // Cell is not selected until proven otherwise
    bool selected = false;

    // Iterate over contours
    for (vtkIdType c = 0; c < this->ContourValues->GetNumberOfContours() && !selected; ++c)
    {
      // Retrieve sign with respect to contour value at current cursor
      bool sign = (this->CellSigns[c]->GetTuple1(id) != 0.0);

      // Iterate over all cursors of Von Neumann neighborhood around center
      unsigned int nn = supercursor->GetNumberOfCursors() - 1;
      for (unsigned int neighbor = 0; neighbor < nn && !selected; ++neighbor)
      {
        // Retrieve global index of neighbor
        unsigned int icursorN = MooreCursors[dim - 1][neighbor];
        if (supercursor->HasTree(icursorN))
        {
          vtkIdType idN = supercursor->GetGlobalNodeIndex(icursorN);

          // Decide whether neighbor was selected or must be retained because of a sign change
          selected = this->SelectedCells->GetTuple1(idN) == 1 ||
            ((this->CellSigns[c]->GetTuple1(idN) != 0.0) != sign) ||
            (this->InGhostArray && this->InGhostArray->GetTuple1(idN));
        }
        else
        {
          selected = false;
        }
      } // neighbor
    }   // c

    if (selected)
    {
      // Node has at least one neighbor containing one contour, recurse to all children
      unsigned int numChildren = supercursor->GetNumberOfChildren();
      for (unsigned int child = 0; child < numChildren; ++child)
      {
        // Create child cursor from parent in input grid
        supercursor->ToChild(child);
        // Recurse
        this->RecursivelyProcessTree(supercursor);
        supercursor->ToParent();
      }
    }
  }
  else if ((!this->InMask || !this->InMask->GetTuple1(id)))
  {
    // Cell is not masked, iterate over its corners
    unsigned int numLeavesCorners = 1 << dim;
    for (unsigned int cornerIdx = 0; cornerIdx < numLeavesCorners; ++cornerIdx)
    {
      bool owner = true;
      this->Leaves->SetNumberOfIds(numLeavesCorners);

      // Iterate over every leaf touching the corner and check ownership
      for (unsigned int leafIdx = 0; leafIdx < numLeavesCorners && owner; ++leafIdx)
      {
        owner = supercursor->GetCornerCursors(cornerIdx, leafIdx, this->Leaves);
      } // leafIdx

      // If cell owns dual cell, compute contours thereof
      if (owner)
      {
        vtkIdType numContours = this->ContourValues->GetNumberOfContours();
        double* values = this->ContourValues->GetValues();

        // Generate contour topology depending on dimensionality
        vtkCell* cell = nullptr;
        switch (dim)
        {
          case 1:
            cell = this->Line;
            break;
          case 2:
            cell = this->Pixel;
            break;
          case 3:
            cell = this->Voxel;
        } // switch ( dim )

        // Iterate over cell corners
        double x[3];
        supercursor->GetPoint(x);
        for (unsigned int _cornerIdx = 0; _cornerIdx < numLeavesCorners; ++_cornerIdx)
        {
          // Get cursor corresponding to this corner
          vtkIdType cursorId = this->Leaves->GetId(_cornerIdx);

          // Retrieve neighbor coordinates and store them
          supercursor->GetPoint(cursorId, x);
          cell->Points->SetPoint(_cornerIdx, x);

          // Retrieve neighbor index and add to list of cell vertices
          vtkIdType idN = supercursor->GetGlobalNodeIndex(cursorId);
          cell->PointIds->SetId(_cornerIdx, idN);

          // Assign scalar value attached to this contour item
          this->CellScalars->SetTuple(_cornerIdx, this->InScalars->GetTuple(idN));
        } // cornerIdx
        // Compute cell isocontour for each isovalue
        for (int c = 0; c < numContours; ++c)
        {
          this->Helper->Contour(cell, values[c], this->CellScalars, this->CurrentId);
        } // c

        // Increment output cell counter
        ++this->CurrentId;
      } // if ( owner )
    }   // cornerIdx
  }     // else if ( ! this->InMask || this->InMask->GetTuple1( id ) )
}
