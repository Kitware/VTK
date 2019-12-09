/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridAxisClip.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridAxisClip.h"

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
#include "vtkQuadric.h"
#include "vtkUniformHyperTreeGrid.h"

#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"

#include <set>

vtkStandardNewMacro(vtkHyperTreeGridAxisClip);
vtkCxxSetObjectMacro(vtkHyperTreeGridAxisClip, Quadric, vtkQuadric);

//-----------------------------------------------------------------------------
vtkHyperTreeGridAxisClip::vtkHyperTreeGridAxisClip()
{
  // Defaut clipping mode is by plane
  this->ClipType = vtkHyperTreeGridAxisClip::PLANE;

  // Defaut normal axis is Z
  this->PlaneNormalAxis = 0;

  // Defaut place intercept is 0
  this->PlanePosition = 0.;

  // Default clipping box is a unit cube centered at origin
  this->Bounds[0] = -.5;
  this->Bounds[1] = .5;
  this->Bounds[2] = -.5;
  this->Bounds[3] = .5;
  this->Bounds[4] = -.5;
  this->Bounds[5] = .5;

  // Default quadric is a sphere with radius 1 centered at origin
  this->Quadric = vtkQuadric::New();
  this->Quadric->SetCoefficients(1., 1., 1., 0., 0., 0., 0., 0., 0., -1.);

  // Defaut inside/out flag is false
  this->InsideOut = 0;

  this->OutMask = nullptr;

  // Output indices begin at 0
  this->CurrentId = 0;

  // JB Pour sortir un maillage de meme type que celui en entree
  this->AppropriateOutput = true;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridAxisClip::~vtkHyperTreeGridAxisClip()
{
  if (this->OutMask)
  {
    this->OutMask->Delete();
    this->OutMask = nullptr;
  }

  if (this->Quadric)
  {
    this->Quadric->Delete();
    this->Quadric = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridAxisClip::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ClipType: " << this->ClipType << endl;
  os << indent << "PlaneNormalAxis: " << this->PlaneNormalAxis << endl;
  os << indent << "PlanePosition: " << this->PlanePosition << endl;
  os << indent << "Bounds: " << this->Bounds[0] << "-" << this->Bounds[1] << ", " << this->Bounds[2]
     << "-" << this->Bounds[3] << ", " << this->Bounds[4] << "-" << this->Bounds[5] << endl;
  os << indent << "InsideOut: " << this->InsideOut << endl;
  os << indent << "OutMask: " << this->OutMask << endl;
  os << indent << "CurrentId: " << this->CurrentId << endl;

  if (this->Quadric)
  {
    this->Quadric->PrintSelf(os, indent.GetNextIndent());
  }
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridAxisClip::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid");
  return 1;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridAxisClip::GetMinimumBounds(double bMin[3])
{
  bMin[0] = this->Bounds[0];
  bMin[1] = this->Bounds[2];
  bMin[2] = this->Bounds[4];
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridAxisClip::GetMaximumBounds(double bMax[3])
{
  bMax[0] = this->Bounds[1];
  bMax[1] = this->Bounds[3];
  bMax[2] = this->Bounds[5];
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridAxisClip::SetQuadricCoefficients(double q[10])
{
  if (!this->Quadric)
  {
    this->Quadric = vtkQuadric::New();
  }
  this->Quadric->SetCoefficients(q);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridAxisClip::GetQuadricCoefficients(double q[10])
{
  this->Quadric->GetCoefficients(q);
}

//-----------------------------------------------------------------------------
double* vtkHyperTreeGridAxisClip::GetQuadricCoefficients()
{
  return this->Quadric->GetCoefficients();
}

//----------------------------------------------------------------------------
vtkMTimeType vtkHyperTreeGridAxisClip::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();

  if (this->Quadric)
  {
    vtkMTimeType time = this->Quadric->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridAxisClip::IsClipped(vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  // Check clipping status depending on clip type
  switch (this->ClipType)
  {
    case vtkHyperTreeGridAxisClip::PLANE:
    {
      // Retrieve normal axis and intercept of plane
      int axis = this->PlaneNormalAxis;
      double inter = this->PlanePosition;

      // Retrieve geometric origin of input cursor
      const double* origin = cursor->GetOrigin();

      // Retrieve geometric size of input cursor
      const double* size = cursor->GetSize();

      // Check whether cursor is below hyperplane
      if (origin[axis] + size[axis] < inter)
      {
        return !this->InsideOut;
      }
      break;
    } // case PLANE
    case vtkHyperTreeGridAxisClip::BOX:
    {
      // Retrieve bounds of box
      double bMin[3], bMax[3];
      this->GetMinimumBounds(bMin);
      this->GetMaximumBounds(bMax);

      // Retrieve geometric origin and size of input cursor
      const double* cMin = cursor->GetOrigin();
      const double* size = cursor->GetSize();

      // Compute upper bounds of input cursor
      double cMax[3];
      cMax[0] = cMin[0] + size[0];
      cMax[1] = cMin[1] + size[1];
      cMax[2] = cMin[2] + size[2];

      if (((cMin[0] >= bMin[0] && cMin[0] <= bMax[0]) ||
            (cMax[0] >= bMin[0] && cMax[0] <= bMax[0])) &&
        ((cMin[1] >= bMin[1] && cMin[1] <= bMax[1]) ||
          (cMax[1] >= bMin[1] && cMax[1] <= bMax[1])) &&
        ((cMin[2] >= bMin[2] && cMin[2] <= bMax[2]) || (cMax[2] >= bMin[2] && cMax[2] <= bMax[2])))
      {
        return this->InsideOut;
      }
      return !this->InsideOut;
    } // case BOX
    case vtkHyperTreeGridAxisClip::QUADRIC:
    {
      // Retrieve geometric origin and size of input cursor
      const double* origin = cursor->GetOrigin();
      const double* size = cursor->GetSize();

      // Iterate over all vertices
      double nVert = 1 << cursor->GetDimension();
      for (int v = 0; v < nVert; ++v)
      {
        // Transform flat index into triple
        div_t d1 = div(v, 2);
        div_t d2 = div(d1.quot, 2);

        // Compute vertex coordinates
        double pt[3];
        pt[0] = origin[0] + d1.rem * size[0];
        pt[1] = origin[1] + d2.rem * size[1];
        pt[2] = origin[2] + d2.quot * size[2];

        // Evaluate quadric at current vertex
        double qv = this->Quadric->EvaluateFunction(pt);
        if (qv <= 0)
        {
          // Found negative value at this vertex, cell not clipped out
          return !this->InsideOut;
        }
      } // v
      break;
    } // case QUADRIC
  }   //  switch (this->ClipType)

  return this->InsideOut;
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridAxisClip::ProcessTrees(vtkHyperTreeGrid* input, vtkDataObject* outputDO)
{
  // Downcast output data object to hyper tree grid
  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(outputDO);
  if (!output)
  {
    vtkErrorMacro("Incorrect type of output: " << outputDO->GetClassName());
    return 0;
  }

  this->OutMask = vtkBitArray::New();

  // Retrieve input dimension
  unsigned int dimension = input->GetDimension();

  // This filter works only with 3D grids
  if (dimension == 2 && static_cast<unsigned int>(this->PlaneNormalAxis) == input->GetOrientation())
  {
    vtkErrorMacro(<< "In 2D axis clip direction cannot be normal to grid plane:"
                  << input->GetOrientation());
    return 0;
  }
  else if (dimension == 1 &&
    static_cast<unsigned int>(this->PlaneNormalAxis) == input->GetOrientation())
  {
    vtkErrorMacro(<< "In 1D axis clip direction cannot be that of grid axis:"
                  << input->GetOrientation());
    return 0;
  }

  // Set identical grid parameters
  output->Initialize();
  output->CopyEmptyStructure(input);

  // Initialize output point data
  this->InData = input->GetPointData();
  this->OutData = output->GetPointData();
  this->OutData->CopyAllocate(this->InData);

  // Output indices begin at 0
  this->CurrentId = 0;

  // Retrieve material mask
  this->InMask = input->HasMask() ? input->GetMask() : nullptr;

  // Storage for Cartesian indices
  unsigned int cart[3];

  // Storage for global indices of clipped out root cells
  std::set<vtkIdType> clipped;

  // First pass across tree roots: compute extent of output grid indices
  unsigned int inSize[3];
  input->GetCellDims(inSize);

  unsigned int minId[] = { 0, 0, 0 };
  unsigned int maxId[] = { 0, 0, 0 };
  unsigned int outSize[3];
  vtkIdType inIndex;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  input->InitializeTreeIterator(it);
  vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> inCursor;
  while (it.GetNextTree(inIndex))
  {
    // Initialize new geometric cursor at root of current input tree
    input->InitializeNonOrientedGeometryCursor(inCursor, inIndex);

    // Check whether root cell is intersected by plane
    if (!this->IsClipped(inCursor))
    {
      // Root is intersected by plane, compute its Cartesian coordinates
      input->GetLevelZeroCoordinatesFromIndex(inIndex, cart[0], cart[1], cart[2]);

      // Update per-coordinate grid extrema if needed
      for (unsigned int d = 0; d < 3; ++d)
      {
        if (cart[d] < minId[d])
        {
          minId[d] = cart[d];
        }
        else if (cart[d] > maxId[d])
        {
          maxId[d] = cart[d];
        }
      } // d
    }   // if (! this->IsClipped(inCursor))
    else
    {
      // This tree root is clipped out, keep track of its global index
      clipped.insert(inIndex);
    } // else
  }   // it
  // Set grid sizes
  outSize[0] = maxId[0] - minId[0] + 1;
  outSize[1] = maxId[1] - minId[1] + 1;
  outSize[2] = maxId[2] - minId[2] + 1;

  // Compute or copy output coordinates depending on output grid sizes
  vtkUniformHyperTreeGrid* inputUHTG = vtkUniformHyperTreeGrid::SafeDownCast(input);
  vtkUniformHyperTreeGrid* outputUHTG = vtkUniformHyperTreeGrid::SafeDownCast(outputDO);
  if (inputUHTG)
  {
    assert(outputUHTG);
    double origin[3];
    inputUHTG->GetOrigin(origin);
    double scale[3];
    inputUHTG->GetGridScale(scale);
    outputUHTG->GetGridScale(scale);
    for (unsigned int d = 0; d < 3; ++d)
    {
      if (inSize[d] != outSize[d])
      {
        origin[d] += scale[d] * minId[d];
      } // if (inSize[d] != outSize[d])
    }   // d
    outputUHTG->GetOrigin(origin);
  }
  else
  {
    vtkDataArray* inCoords[3];
    inCoords[0] = input->GetXCoordinates();
    inCoords[1] = input->GetYCoordinates();
    inCoords[2] = input->GetZCoordinates();
    vtkDataArray* outCoords[3];
    outCoords[0] = output->GetXCoordinates();
    outCoords[1] = output->GetYCoordinates();
    outCoords[2] = output->GetZCoordinates();
    for (unsigned int d = 0; d < 3; ++d)
    {
      if (inSize[d] != outSize[d])
      {
        // Coordinate extent along d-axis is clipped
        outCoords[d]->SetNumberOfTuples(outSize[d] + 1);
        for (unsigned int m = 0; m <= outSize[d]; ++m)
        {
          unsigned int n = m + minId[d];
          outCoords[d]->SetTuple1(m, inCoords[d]->GetTuple1(n));
        }
      } // if (inSize[d] != outSize[d])
      else
      {
        // Coordinate extent along d-axis is unchanged
        outCoords[d]->ShallowCopy(inCoords[d]);
      } // else
    }   // d
  }

  // Second pass across tree roots: now compute clipped grid recursively
  vtkIdType outIndex = 0;
  input->InitializeTreeIterator(it);
  vtkNew<vtkHyperTreeGridNonOrientedCursor> outCursor;
  while (it.GetNextTree(inIndex))
  {
    // Initialize new geometric cursor at root of current input tree
    input->InitializeNonOrientedGeometryCursor(inCursor, inIndex);

    // Descend only tree roots that have not already been determined to be clipped out
    if (clipped.find(inIndex) == clipped.end())
    {
      // Root is intersected by plane, descend into current child
      input->GetLevelZeroCoordinatesFromIndex(inIndex, cart[0], cart[1], cart[2]);

      // Get root index into output hyper tree grid
      output->GetIndexFromLevelZeroCoordinates(
        outIndex, cart[0] - minId[0], cart[1] - minId[1], cart[2] - minId[2]);

      // Initialize new cursor at root of current output tree
      output->InitializeNonOrientedCursor(outCursor, outIndex, true);

      // Clip tree recursively
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
void vtkHyperTreeGridAxisClip::RecursivelyProcessTree(
  vtkHyperTreeGridNonOrientedGeometryCursor* inCursor, vtkHyperTreeGridNonOrientedCursor* outCursor)
{
  // Retrieve global index of input cursor
  vtkIdType inId = inCursor->GetGlobalNodeIndex();

  // Increase index count on output: postfix is intended
  vtkIdType outId = this->CurrentId++;

  // Retrieve output tree and set global index of output cursor
  outCursor->SetGlobalIndexFromLocal(outId);

  // Copy output cell data from that of input cell
  this->OutData->CopyData(this->InData, inId, outId);

  // Flag to keep track of whether current input cell is clipped out
  bool clipped = this->IsClipped(inCursor);

  // Descend further into input trees only if cursor is neither a leaf nor clipped out
  if (!inCursor->IsLeaf() && !clipped)
  {
    // Cursor is not at leaf, subdivide output tree one level further
    outCursor->SubdivideLeaf();

    // Initialize output children index
    int outChild = 0;

    // If cursor is not at leaf, recurse to all children
    int numChildren = inCursor->GetNumberOfChildren();
    for (int inChild = 0; inChild < numChildren; ++inChild, ++outChild)
    {
      inCursor->ToChild(inChild);
      // Child is not clipped out, descend into current child
      outCursor->ToChild(outChild);
      // Recurse
      this->RecursivelyProcessTree(inCursor, outCursor);
      // Return to parent
      outCursor->ToParent();
      inCursor->ToParent();
    } // inChild
  }   // if (! cursor->IsLeaf() && ! clipped)
  else if (!clipped && this->InMask && this->InMask->GetValue(inId))
  {
    // Handle case of not clipped but nonetheless masked leaf cells
    clipped = true;
  } // else

  // Mask output cell if necessary
  this->OutMask->InsertTuple1(outId, clipped);
}
