/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageStencilData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageStencilData.h"

#include "vtkImageStencilSource.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkDataSetAttributes.h"
#include "vtkDataArray.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"

#include <cmath>
#include <algorithm>

vtkStandardNewMacro(vtkImageStencilData);

//----------------------------------------------------------------------------
namespace {

// Compute a single index from yIdx and zIdx.
int vtkImageStencilDataIndex(const int extent[6], int yIdx, int zIdx)
{
  int yMin = extent[2];
  int yMax = extent[3];
  int zMin = extent[4];
  return (yMax - yMin + 1)*(zIdx - zMin) + (yIdx - yMin);
}

// Build up the stencil by appending the extent [r1,r2] to the end of
// "clist".  The extent list "clist" will be expanded and reallocated
// as necessary.  If this extent is adjacent to the extent that was
// previously added, then the extents will be joined. The parameter
// "clistsmall" is used as "clist" when only a single extent is stored
// in "clist", in order to reduce the need for dynamic memory allocation.
void vtkImageStencilDataInsertNextExtent(
  int r1, int r2, int *&clist, int &clistlen, int *clistsmall)
{
  if (clistlen > 0)
  {
    // this extent continues the previous extent
    if (r1 == clist[clistlen-1])
    {
      clist[clistlen-1] = r2 + 1;
      return;
    }
    // check if clistlen is a power of two
    else if ((clistlen & (clistlen-1)) == 0)
    {
      // the allocated space is always the smallest power of two
      // that is not less than the number of stored items, therefore
      // we need to allocate space when clistlen is a power of two
      int *newclist = new int[2*clistlen];
      for (int k = 0; k < clistlen; k++)
      {
        newclist[k] = clist[k];
      }
      if (clist != clistsmall)
      {
        delete [] clist;
      }
      clist = newclist;
    }
  }

  clist[clistlen] = r1;
  clist[clistlen + 1] = r2 + 1;
  clistlen += 2;
}

// Functor for logical "Or" operation.  The "notA" and "notB" provide
// hints that a "not" operation should be applied to the operand before
// the functor is called.
struct vtkImageStencilDataOrFunctor
{
  vtkImageStencilDataOrFunctor(bool notA, bool notB) :
    not1(notA), not2(notB) {}

  bool not1;
  bool not2;

  bool operator()(bool a, bool b) { return (a | b); }
};

// Functor for logical "And" operation.  The "notA" and "notB" provide
// hints that a "not" operation should be applied to the operand before
// the functor is called.
struct vtkImageStencilDataAndFunctor
{
  vtkImageStencilDataAndFunctor(bool notA, bool notB) :
    not1(notA), not2(notB) {}

  bool not1;
  bool not2;

  bool operator()(bool a, bool b) { return (a & b); }
};

// Combine extent lists "clist1" and "clist2" with "operation",
// and place the result in "clist".  The operation is done over
// the range [ext1, ext2].
template<typename F>
void vtkImageStencilDataBoolean(
  int *clist1, int clistlen1, int *clist2, int clistlen2,
  int *&clist, int &clistlen, int *clistsmall,
  F operation, int ext1, int ext2)
{
  // If "not" is set for operand 1 or 2 of the operation, then
  // we start in state "true" instead of the default of "false"
  bool state1 = operation.not1;
  bool state2 = operation.not2;

  // Look for sub-extents that precede the extent
  int i1 = 0;
  while (i1 < clistlen1 && clist1[i1] < ext1)
  {
    i1++;
    state1 ^= true;
  }
  int i2 = 0;
  while (i2 < clistlen2 && clist2[i2] < ext1)
  {
    i2++;
    state2 ^= true;
  }

  // Loop through all sub-extents within [ext1, ext2]
  int rnext = ext1;
  int rlast = ext2 + 1;
  while (rnext != rlast)
  {
    bool value = operation(state1, state2);
    int r = rnext;
    int t1 = rlast;
    int t2 = rlast;

    // Find the next position t1 in clist1, and t2 in clist2
    if (i1 < clistlen1 && clist1[i1] < t1)
    {
      t1 = clist1[i1];
    }
    if (i2 < clistlen2 && clist2[i2] < t2)
    {
      t2 = clist2[i2];
    }

    // Does t1 come first? Or t2? Or both?
    if (t1 <= t2)
    {
      state1 ^= true;
      i1++;
      rnext = t1;
    }
    if (t2 <= t1)
    {
      state2 ^= true;
      i2++;
      rnext = t2;
    }

    // If logical operation is true, then add this extent
    if (value)
    {
      vtkImageStencilDataInsertNextExtent(
        r, rnext-1, clist, clistlen, clistsmall);
    }
  }
}

// Clip the sub-extents in "clist" to the range [ext1,ext2]
void vtkImageStencilDataClipExtent(
  int ext1, int ext2, int *clist, int& clistlen)
{
  // Check what will be clipped at the leading edge
  int i = 0;
  while (i < clistlen && clist[i] < ext1)
  {
    i++;
  }
  if ((i & 1) != 0)
  {
    if (clist[i] == ext1)
    {
      i++;
    }
    else
    {
      // Adjust a clipped sub-extent
      clist[--i] = ext1;
    }
  }

  // Check what will be clipped at the trailing edge
  int j = clistlen;
  while (j > 0 && clist[j-1]-1 > ext2)
  {
    --j;
  }
  if ((j & 1) != 0)
  {
    if (clist[j-1]-1 == ext2)
    {
      j--;
    }
    else
    {
      // Adjust a clipped sub-extent
      clist[j++] = ext2+1;
    }
  }

  // Move the chosen sub-extents to the front of the list
  if (i > 0)
  {
    for (int k = i; k < j; k++)
    {
      clist[k - i] = clist[k];
    }
  }

  // Adjust the size of the list
  clistlen = j - i;
}

} // end anonymous namespace

//----------------------------------------------------------------------------
vtkImageStencilData::vtkImageStencilData()
{
  this->Spacing[0] = 1;
  this->Spacing[1] = 1;
  this->Spacing[2] = 1;

  this->Origin[0] = 0;
  this->Origin[1] = 0;
  this->Origin[2] = 0;

  this->NumberOfExtentEntries = 0;
  this->ExtentLists = NULL;
  this->ExtentListLengths = NULL;

  this->Extent[0] = 0;
  this->Extent[1] = -1;
  this->Extent[2] = 0;
  this->Extent[3] = -1;
  this->Extent[4] = 0;
  this->Extent[5] = -1;

  this->Information->Set(vtkDataObject::DATA_EXTENT_TYPE(), VTK_3D_EXTENT);
  this->Information->Set(vtkDataObject::DATA_EXTENT(), this->Extent, 6);
}

//----------------------------------------------------------------------------
vtkImageStencilData::~vtkImageStencilData()
{
  this->Initialize();
}

//----------------------------------------------------------------------------
void vtkImageStencilData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  int extent[6];
  this->GetExtent(extent);

  os << indent << "Extent: ("
     << extent[0] << ", "
     << extent[1] << ", "
     << extent[2] << ", "
     << extent[3] << ", "
     << extent[4] << ", "
     << extent[5] << ")\n";

  os << indent << "Spacing: ("
     << this->Spacing[0] << ", "
     << this->Spacing[1] << ", "
     << this->Spacing[2] << ")\n";

  os << indent << "Origin: ("
     << this->Origin[0] << ", "
     << this->Origin[1] << ", "
     << this->Origin[2] << ")\n";

}

//----------------------------------------------------------------------------
void vtkImageStencilData::Initialize()
{
  if (this->ExtentLists)
  {
    int n = this->NumberOfExtentEntries;
    for (int i = 0; i < n; i++)
    {
      if (this->ExtentLists[i] != &this->ExtentListLengths[n + 2*i])
      {
        delete [] this->ExtentLists[i];
      }
    }
    delete [] this->ExtentLists;
  }
  this->ExtentLists = NULL;
  this->NumberOfExtentEntries = 0;

  delete [] this->ExtentListLengths;
  this->ExtentListLengths = NULL;

  if(this->Information)
  {
    int extent[6] = {0, -1, 0, -1, 0, -1};
    memcpy(this->Extent, extent, 6*sizeof(int));
  }
}

//----------------------------------------------------------------------------
void vtkImageStencilData::CopyInformationFromPipeline(vtkInformation *info)
{
  // Let the superclass copy whatever it wants.
  this->Superclass::CopyInformationFromPipeline(info);

  // Copy pipeline information to data information before the producer
  // executes.
  this->CopyOriginAndSpacingFromPipeline(info);
}

//----------------------------------------------------------------------------
void vtkImageStencilData::CopyInformationToPipeline(vtkInformation* info)
{
  // Let the superclass copy information to the pipeline
  this->Superclass::CopyInformationToPipeline(info);

  // Copy the origin and spacing to the pipeline
  info->Set(vtkDataObject::SPACING(), this->Spacing, 3);
  info->Set(vtkDataObject::ORIGIN(), this->Origin, 3);
}

//----------------------------------------------------------------------------
void vtkImageStencilData::CopyOriginAndSpacingFromPipeline(vtkInformation* info)
{
  // Copy origin and spacing from pipeline information to the internal
  // copies.
  if(info->Has(SPACING()))
  {
    this->SetSpacing(info->Get(SPACING()));
  }
  if(info->Has(ORIGIN()))
  {
    this->SetOrigin(info->Get(ORIGIN()));
  }
}

//----------------------------------------------------------------------------
void vtkImageStencilData::SetExtent(int extent[6])
{
  for (int i = 0; i < 6; i++)
  {
    this->Extent[i] = extent[i];
  }
}

//----------------------------------------------------------------------------
void vtkImageStencilData::SetExtent(
  int x1, int x2, int y1, int y2, int z1, int z2)
{
  int ext[6] = { x1, x2, y1, y2, z1, z2 };
  this->SetExtent(ext);
}

//----------------------------------------------------------------------------
void vtkImageStencilData::ShallowCopy(vtkDataObject *o)
{
  vtkImageStencilData *s = vtkImageStencilData::SafeDownCast(o);

  if (s)
  {
    this->InternalImageStencilDataCopy(s);
  }

  vtkDataObject::ShallowCopy(o);
}

//----------------------------------------------------------------------------
void vtkImageStencilData::DeepCopy(vtkDataObject *o)
{
  vtkImageStencilData *s = vtkImageStencilData::SafeDownCast(o);

  if (s)
  {
    this->InternalImageStencilDataCopy(s);
  }

  vtkDataObject::DeepCopy(o);
}

//----------------------------------------------------------------------------
void vtkImageStencilData::InternalImageStencilDataCopy(vtkImageStencilData *s)
{
  // copy information that accompanies the data
  this->SetSpacing(s->Spacing);
  this->SetOrigin(s->Origin);

  // delete old data
  if (this->ExtentLists)
  {
    int n = this->NumberOfExtentEntries;
    for (int i = 0; i < n; i++)
    {
      if (this->ExtentLists[i] != &this->ExtentListLengths[n + 2*i])
      {
        delete [] this->ExtentLists[i];
      }
    }
    delete [] this->ExtentLists;
  }
  this->ExtentLists = NULL;
  this->NumberOfExtentEntries = 0;

  delete [] this->ExtentListLengths;
  this->ExtentListLengths = NULL;

  // copy new data
  if (s->NumberOfExtentEntries != 0)
  {
    this->NumberOfExtentEntries = s->NumberOfExtentEntries;
    int n = this->NumberOfExtentEntries;
    this->ExtentListLengths = new int[3*n];
    this->ExtentLists = new int *[n];
    for (int i = 0; i < n; i++)
    {
      this->ExtentListLengths[i] = s->ExtentListLengths[i];
      int m = this->ExtentListLengths[i];
      int clistmaxlen = 1;
      do { clistmaxlen *= 2; } while (m > clistmaxlen);
      this->ExtentLists[i] = &this->ExtentListLengths[n + 2*i];
      if (clistmaxlen > 2)
      {
        this->ExtentLists[i] = new int[clistmaxlen];
      }
      for (int j = 0; j < m; j++)
      {
        this->ExtentLists[i][j] = s->ExtentLists[i][j];
      }
    }
  }
  memcpy(this->Extent, s->GetExtent(), 6*sizeof(int));
}

//----------------------------------------------------------------------------
// Change the extent of the stencil while preserving any data that lies
// within the intersection of the new and old extents.  Any data that lies
// outside of the intersection will be zeroed.
void vtkImageStencilData::ChangeExtent(const int extent[6])
{
  int oldExtent[6];
  this->GetExtent(oldExtent);

  if (extent[2] != oldExtent[2] || extent[3] != oldExtent[3] ||
      extent[4] != oldExtent[4] || extent[5] != oldExtent[5])
  {
    // Save the current information
    int numberOfEntries = this->NumberOfExtentEntries;
    int *listLengths = this->ExtentListLengths;
    int **lists = this->ExtentLists;
    int *smallstore = &listLengths[numberOfEntries];

    // Clear the stencil
    this->NumberOfExtentEntries = 0;
    this->ExtentListLengths = 0;
    this->ExtentLists = 0;

    // Set the new extent and re-allocate
    this->SetExtent(
      extent[0], extent[1], extent[2], extent[3], extent[4], extent[5]);
    this->AllocateExtents();

    // Get the location for storing single extents
    int *ss = &this->ExtentListLengths[this->NumberOfExtentEntries];

    // Copy the information back again
    int k = 0;
    int zinc = extent[3] - extent[2] + 1;
    for (int idz = oldExtent[4]; idz <= oldExtent[5]; idz++)
    {
      for (int idy = oldExtent[2]; idy <= oldExtent[3]; idy++)
      {
        if (idy >= extent[2] && idy <= extent[3] &&
            idz >= extent[4] && idz <= extent[5])
        {
          if (extent[0] > oldExtent[0] || extent[1] < oldExtent[1])
          {
            vtkImageStencilDataClipExtent(
              extent[0], extent[1], lists[k], listLengths[k]);
          }

          int j = (idz - extent[4])*zinc + (idy - extent[2]);
          this->ExtentListLengths[j] = listLengths[k];
          if (lists[k] == &smallstore[2*k])
          {
            ss[2*j] = smallstore[2*k];
            ss[2*j + 1] = smallstore[2*k + 1];
            this->ExtentLists[j] = &ss[2*j];
          }
          else
          {
            this->ExtentLists[j] = lists[k];
          }
        }
        else // free out-of-bounds info that wasn't copied
        {
          if (lists[k] != &smallstore[2*k])
          {
            delete [] lists[k];
          }
        }
        k++;
      }
    }

    delete [] lists;
    delete [] listLengths;
  }
  else if (extent[0] > oldExtent[0] || extent[1] < oldExtent[1])
  {
    int *listLengths = this->ExtentListLengths;
    int **lists = this->ExtentLists;

    int k = 0;
    for (int idz = extent[4]; idz <= extent[5]; idz++)
    {
      for (int idy = extent[2]; idy <= extent[3]; idy++)
      {
        vtkImageStencilDataClipExtent(
          extent[0], extent[1], lists[k], listLengths[k]);
        k++;
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkImageStencilData::AllocateExtents()
{
  int extent[6];
  this->GetExtent(extent);
  int ySize = (extent[3] - extent[2] + 1);
  int zSize = (extent[5] - extent[4] + 1);

  int numEntries = ySize*zSize;
  if (numEntries != this->NumberOfExtentEntries)
  {
    if (this->NumberOfExtentEntries != 0)
    {
      int n = this->NumberOfExtentEntries;
      for (int i = 0; i < n; i++)
      {
        if (this->ExtentLists[i] != &this->ExtentListLengths[n + 2*i])
        {
          delete [] this->ExtentLists[i];
        }
      }
      delete [] this->ExtentLists;
      delete [] this->ExtentListLengths;
    }

    this->NumberOfExtentEntries = numEntries;
    this->ExtentLists = NULL;
    this->ExtentListLengths = NULL;

    if (numEntries)
    {
      // ExtentListLengths also holds space for the initial entries in
      // the ExtentLists array, which is why it has 3*numEntries values
      this->ExtentLists = new int *[numEntries];
      this->ExtentListLengths = new int[3*numEntries];
      for (int i = 0; i < numEntries; i++)
      {
        this->ExtentListLengths[i] = 0;
        this->ExtentLists[i] = &this->ExtentListLengths[numEntries + 2*i];
      }
    }
  }
  else
  {
    for (int i = 0; i < numEntries; i++)
    {
      if (this->ExtentLists[i] != &this->ExtentListLengths[numEntries + 2*i])
      {
        delete [] this->ExtentLists[i];
      }
      this->ExtentLists[i] = &this->ExtentListLengths[numEntries + 2*i];
      this->ExtentListLengths[i] = 0;
    }
  }
}

//----------------------------------------------------------------------------
// Given the total output x extent [xMin,xMax] and the current y, z indices,
// return each sub-extent [r1,r2] that lies within within the unclipped
// region in sequence.  A value of '0' is returned if no more sub-extents
// are available.  The variable 'iter' must be initialized to zero before
// the first call, unless you want the complementary sub-extents in which
// case you must initialize 'iter' to -1.  The variable 'iter' is used
// internally to keep track of which sub-extent should be returned next.
int vtkImageStencilData::GetNextExtent(int &r1, int &r2,
                                       int rmin, int rmax,
                                       int yIdx, int zIdx, int &iter)
{
  int yExt = this->Extent[3] - this->Extent[2] + 1;
  int zExt = this->Extent[5] - this->Extent[4] + 1;
  yIdx -= this->Extent[2];
  zIdx -= this->Extent[4];

  // initialize r1, r2 to defaults
  r1 = rmax + 1;
  r2 = rmax;

  if (yIdx < 0 || yIdx >= yExt || zIdx < 0 || zIdx >= zExt)
  { // out-of-bounds in y or z, use null extent
    // if iter was set to negative, reverse the result
    if (iter < 0)
    {
      iter = 0;
      r1 = rmin;
      r2 = rmax;
      return 1;
    }
    return 0;
  }

  // get the ExtentList and ExtentListLength for this yIdx,zIdx
  int incr = zIdx*yExt + yIdx;
  int *clist = this->ExtentLists[incr];
  int clistlen = this->ExtentListLengths[incr];

  if (iter <= 0)
  {
    int state = 1; // start outside
    if (iter < 0)  // unless iter is negative at start
    {
      iter = 0;
      state = -1;
    }

    r1 = VTK_INT_MIN;
    for ( ; iter < clistlen; iter++)
    {
      if (clist[iter] >= rmin)
      {
        if (state > 0)
        {
          r1 = clist[iter++];
        }
        break;
      }
      state = -state;
    }
    if (r1 == VTK_INT_MIN)
    {
      r1 = rmin;
      if (state > 0)
      {
        r1 = rmax + 1;
      }
    }
  }
  else
  {
    if (iter >= clistlen)
    {
      return 0;
    }
    r1 = clist[iter++];
    if (r1 < rmin)
    {
      r1 = rmin;
    }
  }

  if (r1 > rmax)
  {
    r1 = rmax + 1;
    return 0;
  }

  if (iter >= clistlen)
  {
    return 1;
  }

  r2 = clist[iter++] - 1;

  if (r2 > rmax)
  {
    r2 = rmax;
  }

  return 1;
}

//----------------------------------------------------------------------------
// Checks if an index is inside the stencil.
int vtkImageStencilData::IsInside(int xIdx, int yIdx, int zIdx)
{
  // IsInside provides an efficient way to check if a single index is within
  // the stencil.

  int yExt = this->Extent[3] - this->Extent[2] + 1;
  yIdx -= this->Extent[2];
  if (yIdx < 0 || yIdx >= yExt)
  {
    return 0; // out-of-bounds in y
  }

  int zExt = this->Extent[5] - this->Extent[4] + 1;
  zIdx -= this->Extent[4];
  if (zIdx < 0 || zIdx >= zExt)
  {
    return 0; // out-of-bounds in z
  }

  // get the ExtentList and ExtentListLength for this yIdx,zIdx
  int incr = zIdx*yExt + yIdx;
  int *clist = this->ExtentLists[incr];
  int clistlen = this->ExtentListLengths[incr];

  // Check now if we lie within any of the pairs for this (yIdx, zIdx)
  for (int iter = 0; iter < clistlen; )
  {
    if (clist[iter++] > xIdx)
    {
      ++iter;
      continue;
    }

    if (xIdx < clist[iter++])
    {
      return 1;
    }
  }

  return 0;
}

//----------------------------------------------------------------------------
//  Fills the stencil.  Extents must be set.
void vtkImageStencilData::Fill()
{
  int r1 = this->Extent[0];
  int r2 = this->Extent[1];

  int n = this->NumberOfExtentEntries;
  for (int i = 0; i < n; i++)
  {
    if (this->ExtentLists[i] != &this->ExtentListLengths[n + 2*i])
    {
      delete [] this->ExtentLists[i];
    }
    this->ExtentLists[i] = &this->ExtentListLengths[n + 2*i];
    this->ExtentLists[i][0] = r1;
    this->ExtentLists[i][1] = r2 + 1;
    this->ExtentListLengths[i] = 2;
  }
}

//----------------------------------------------------------------------------
// Insert a sub extent [r1,r2] on to the list for the x row at (yIdx,zIdx).
void vtkImageStencilData::InsertNextExtent(int r1, int r2, int yIdx, int zIdx)
{
  // calculate the index into the extent array
  int incr = vtkImageStencilDataIndex(this->Extent, yIdx, zIdx);

  vtkImageStencilDataInsertNextExtent(
    r1, r2, this->ExtentLists[incr], this->ExtentListLengths[incr],
    &this->ExtentListLengths[this->NumberOfExtentEntries + 2*incr]);
}

//----------------------------------------------------------------------------
void vtkImageStencilData::LogicalOperationExtent(
  int r1, int r2, int yIdx, int zIdx, Operation operation)
{
  // calculate the index into the extent array
  int incr = vtkImageStencilDataIndex(this->Extent, yIdx, zIdx);

  int &clistlen = this->ExtentListLengths[incr];
  int *&clist = this->ExtentLists[incr];
  int *clistsmall =
    &this->ExtentListLengths[this->NumberOfExtentEntries + 2*incr];

  int clistlen2 = 2;
  int clist2[2] = { r1, r2+1 };
  int clistsmall1[2];
  int clistlen1 = clistlen;
  int *clist1 = clist;
  if (clist == clistsmall)
  {
    clistsmall1[0] = clistsmall[0];
    clistsmall1[1] = clistsmall[1];
    clist1 = clistsmall1;
  }

  clist = clistsmall;
  clistlen = 0;

  if (operation == Merge)
  {
    vtkImageStencilDataBoolean(
      clist1, clistlen1, clist2, clistlen2,
      clist, clistlen, clistsmall,
      vtkImageStencilDataOrFunctor(false, false),
      this->Extent[0], this->Extent[1]);
  }
  else if (operation == Erase)
  {
    vtkImageStencilDataBoolean(
      clist1, clistlen1, clist2, clistlen2,
      clist, clistlen, clistsmall,
      vtkImageStencilDataAndFunctor(false, true),
      this->Extent[0], this->Extent[1]);
  }

  if (clist1 != clistsmall1)
  {
    delete [] clist1;
  }
}

//----------------------------------------------------------------------------
void vtkImageStencilData::InsertAndMergeExtent(
  int r1, int r2, int yIdx, int zIdx)
{
  this->LogicalOperationExtent(r1, r2, yIdx, zIdx, Merge);
}

//----------------------------------------------------------------------------
void vtkImageStencilData::RemoveExtent(
  int r1, int r2, int yIdx, int zIdx)
{
  this->LogicalOperationExtent(r1, r2, yIdx, zIdx, Erase);
}

//----------------------------------------------------------------------------
vtkImageStencilData* vtkImageStencilData::GetData(vtkInformation* info)
{
  return info? vtkImageStencilData::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkImageStencilData* vtkImageStencilData::GetData(vtkInformationVector* v,
                                                  int i)
{
  return vtkImageStencilData::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
void vtkImageStencilData::LogicalOperationInPlace(
  vtkImageStencilData *stencil, Operation operation)
{
  // Find the intersection of the two extents
  int extent[6];
  stencil->GetExtent(extent);
  for (int i = 0; i < 3; i++)
  {
    if (this->Extent[2*i] > extent[2*i])
    {
      extent[2*i] = this->Extent[2*i];
    }
    if (this->Extent[2*i + 1] < extent[2*i + 1])
    {
      extent[2*i + 1] = this->Extent[2*i + 1];
    }
    if (extent[2*i] > extent[2*i + 1])
    {
      extent[2*i] = this->Extent[2*i + 1] + 1;
      extent[2*i + 1] = this->Extent[2*i + 1];
    }
  }

  // Iterate over the intersected extent
  for (int idz = extent[4]; idz <= extent[5]; idz++)
  {
    for (int idy = extent[2]; idy <= extent[3]; idy++)
    {
      int incr = vtkImageStencilDataIndex(stencil->Extent, idy, idz);
      int clistlen2 = stencil->ExtentListLengths[incr];
      int *clist2 = stencil->ExtentLists[incr];

      incr = vtkImageStencilDataIndex(this->Extent, idy, idz);
      int &clistlen = this->ExtentListLengths[incr];
      int *&clist = this->ExtentLists[incr];
      int *clistsmall =
        &this->ExtentListLengths[this->NumberOfExtentEntries + 2*incr];

      int clistsmall1[2];
      int clistlen1 = clistlen;
      int *clist1 = clist;
      if (clist == clistsmall)
      {
        clistsmall1[0] = clistsmall[0];
        clistsmall1[1] = clistsmall[1];
        clist1 = clistsmall1;
      }

      clist = clistsmall;
      clistlen = 0;

      if (operation == Merge)
      {
        vtkImageStencilDataBoolean(
          clist1, clistlen1, clist2, clistlen2,
          clist, clistlen, clistsmall,
          vtkImageStencilDataOrFunctor(false, false),
          this->Extent[0], this->Extent[1]);
      }
      else if (operation == Erase)
      {
        vtkImageStencilDataBoolean(
          clist1, clistlen1, clist2, clistlen2,
          clist, clistlen, clistsmall,
          vtkImageStencilDataAndFunctor(false, true),
          this->Extent[0], this->Extent[1]);
      }

      if (clist1 != clistsmall1)
      {
        delete [] clist1;
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkImageStencilData::Add(vtkImageStencilData *stencil1)
{
  int extent[6], extent1[6], extent2[6];
  stencil1->GetExtent(extent1);
  this->GetExtent(extent2);

  if (extent1[0] > extent1[1] ||
      extent1[2] > extent1[3] ||
      extent1[4] > extent1[5])
  {
    return;
  }

  // Find the smallest bounding box large enough to hold both stencils.
  extent[0] = (extent1[0] > extent2[0]) ? extent2[0] : extent1[0];
  extent[1] = (extent1[1] < extent2[1]) ? extent2[1] : extent1[1];
  extent[2] = (extent1[2] > extent2[2]) ? extent2[2] : extent1[2];
  extent[3] = (extent1[3] < extent2[3]) ? extent2[3] : extent1[3];
  extent[4] = (extent1[4] > extent2[4]) ? extent2[4] : extent1[4];
  extent[5] = (extent1[5] < extent2[5]) ? extent2[5] : extent1[5];

  this->ChangeExtent(extent);

  this->LogicalOperationInPlace(stencil1, Merge);

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkImageStencilData::Subtract(vtkImageStencilData *stencil1)
{
  int extent1[6], extent2[6];
  stencil1->GetExtent(extent1);
  this->GetExtent(extent2);

  if ((extent1[0] > extent2[1]) || (extent1[1] < extent2[0]) ||
      (extent1[2] > extent2[3]) || (extent1[3] < extent2[2]) ||
      (extent1[4] > extent2[5]) || (extent1[5] < extent2[4]))
  {
    // The extents don't intersect.. No subtraction needed
    return;
  }

  this->LogicalOperationInPlace(stencil1, Erase);

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkImageStencilData::Replace(vtkImageStencilData *stencil1)
{
  int extent[6], extent1[6], extent2[6], r1, r2, idy, idz, iter=0;
  stencil1->GetExtent(extent1);
  this->GetExtent(extent2);

  if ((extent1[0] > extent2[1]) || (extent1[1] < extent2[0]) ||
      (extent1[2] > extent2[3]) || (extent1[3] < extent2[2]) ||
      (extent1[4] > extent2[5]) || (extent1[5] < extent2[4]))
  {
    // The extents don't intersect.. No subraction needed
    return;
  }

  // Find the smallest box intersection of the extents
  extent[0] = (extent1[0] < extent2[0]) ? extent2[0] : extent1[0];
  extent[1] = (extent1[1] > extent2[1]) ? extent2[1] : extent1[1];
  extent[2] = (extent1[2] < extent2[2]) ? extent2[2] : extent1[2];
  extent[3] = (extent1[3] > extent2[3]) ? extent2[3] : extent1[3];
  extent[4] = (extent1[4] < extent2[4]) ? extent2[4] : extent1[4];
  extent[5] = (extent1[5] > extent2[5]) ? extent2[5] : extent1[5];

  for (idz=extent[4]; idz<=extent[5]; idz++, iter=0)
  {
    for (idy = extent[2]; idy <= extent[3]; idy++, iter=0)
    {
      this->RemoveExtent(extent[0], extent[1], idy, idz);

      int moreSubExtents = 1;
      while( moreSubExtents )
      {
        moreSubExtents = stencil1->GetNextExtent(
          r1, r2, extent[0], extent[1], idy, idz, iter);

        if (r1 <= r2 ) // sanity check
        {
          this->InsertAndMergeExtent(r1, r2, idy, idz);
        }
      }
    }
  }

  this->Modified();
}

//----------------------------------------------------------------------------
int vtkImageStencilData::Clip(int extent[6])
{
  int currentExtent[6];
  this->GetExtent(currentExtent);

  if (vtkMath::ExtentIsWithinOtherExtent(currentExtent, extent))
  {
    // Nothing to do, we are already within the clipping extents.
    return 0;
  }

  // Get the data members that store the stencil
  int numberOfEntries = this->NumberOfExtentEntries;
  int *listLengths = this->ExtentListLengths;
  int **lists = this->ExtentLists;
  int *smallstore = &listLengths[numberOfEntries];

  // Perform the clip
  bool modified = false;
  int k = 0;
  for (int idz = currentExtent[4]; idz <= currentExtent[5]; idz++)
  {
    for (int idy = currentExtent[2]; idy <= currentExtent[3]; idy++)
    {
      if (idy >= extent[2] && idy <= extent[3] &&
          idz >= extent[4] && idz <= extent[5])
      {
        if (extent[0] > currentExtent[0] || extent[1] < currentExtent[1])
        {
          int l = listLengths[k];
          if (l > 0 &&
              (lists[k][0] < extent[0] || lists[k][l-1]-1 > extent[1]))
          {
            vtkImageStencilDataClipExtent(
              extent[0], extent[1], lists[k], l);
            listLengths[k] = l;
            modified = true;
          }
        }
      }
      else if (listLengths[k] > 0)
      {
        listLengths[k] = 0;
        if (lists[k] != &smallstore[2*k])
        {
          delete [] lists[k];
          lists[k] = &smallstore[2*k];
        }
        modified = true;
      }
      k++;
    }
  }

  return modified;
}

//----------------------------------------------------------------------------
// tolerance for float-to-int conversion in stencil operations, this value
// is exactly 0.5*2^-16 (in voxel units, not physical units)

#define VTK_STENCIL_TOL 7.62939453125e-06

//----------------------------------------------------------------------------
vtkImageStencilRaster::vtkImageStencilRaster(const int extent[2])
{
  int rsize = extent[1] - extent[0] + 1;

  // the "raster" is a sequence of pointer-pairs, where the first pointer
  // points to the first value in the raster line, and the second pointer
  // points to one location past the last vale in the raster line.  The
  // difference is the number of x values stored in the raster line.  To
  // allow for tolerance, we actually raster twice at slightly different
  // vertical positions, hence we need two sets of begin/end pointers.
  this->Raster = new double*[4*static_cast<size_t>(rsize)];

  // the extent is the range of y values (one line per y)
  this->Extent[0] = extent[0];
  this->Extent[1] = extent[1];

  // tolerance should be larger than expected roundoff errors
  this->Tolerance = VTK_STENCIL_TOL;

  // no extent is used initially
  this->UsedExtent[0] = 0;
  this->UsedExtent[1] = -1;
}

//----------------------------------------------------------------------------
vtkImageStencilRaster::~vtkImageStencilRaster()
{
  if (this->UsedExtent[1] >= this->UsedExtent[0])
  {
    size_t imin = static_cast<size_t>(this->UsedExtent[0] - this->Extent[0]);
    size_t imax = static_cast<size_t>(this->UsedExtent[1] - this->Extent[0]);
    for (size_t i = imin; i <= imax; i++)
    {
      delete [] this->Raster[4*i];
      delete [] this->Raster[4*i + 2];
    }
  }
  delete [] this->Raster;
}

//----------------------------------------------------------------------------
void vtkImageStencilRaster::PrepareForNewData(const int allocateExtent[2])
{
  if (this->UsedExtent[1] >= this->UsedExtent[0])
  {
    // reset and re-use the allocated raster lines
    size_t imin = static_cast<size_t>(this->UsedExtent[0]-this->Extent[0]);
    size_t imax = static_cast<size_t>(this->UsedExtent[1]-this->Extent[0]);
    for (size_t i = imin; i <= imax; i++)
    {
      this->Raster[4*i + 1] = this->Raster[4*i];
      this->Raster[4*i + 3] = this->Raster[4*i + 2];
    }
  }

  if (allocateExtent && allocateExtent[1] >= allocateExtent[0])
  {
    this->PrepareExtent(allocateExtent[0], allocateExtent[1]);
  }
}

//----------------------------------------------------------------------------
void vtkImageStencilRaster::PrepareExtent(int ymin, int ymax)
{
  // this does not do any allocation, it just initializes any
  // raster lines are not already part of the UsedExtent, and
  // then expands the UsedExtent to include [ymin, ymax]

  if (this->UsedExtent[1] < this->UsedExtent[0])
  {
    size_t imin = static_cast<size_t>(ymin - this->Extent[0]);
    size_t imax = static_cast<size_t>(ymax - this->Extent[0]);
    for (size_t i = imin; i <= imax; i++)
    {
      this->Raster[4*i] = 0;
      this->Raster[4*i + 1] = 0;
      this->Raster[4*i + 2] = 0;
      this->Raster[4*i + 3] = 0;
    }

    this->UsedExtent[0] = ymin;
    this->UsedExtent[1] = ymax;

    return;
  }

  if (ymin < this->UsedExtent[0])
  {
    size_t imin = static_cast<size_t>(ymin - this->Extent[0]);
    size_t imax = static_cast<size_t>(this->UsedExtent[0]-this->Extent[0]-1);
    for (size_t i = imin; i <= imax; i++)
    {
      this->Raster[4*i] = 0;
      this->Raster[4*i + 1] = 0;
      this->Raster[4*i + 2] = 0;
      this->Raster[4*i + 3] = 0;
    }

    this->UsedExtent[0] = ymin;
  }

  if (ymax > this->UsedExtent[1])
  {
    size_t imin = static_cast<size_t>(this->UsedExtent[1]+1 - this->Extent[0]);
    size_t imax = static_cast<size_t>(ymax - this->Extent[0]);
    for (size_t i = imin; i <= imax; i++)
    {
      this->Raster[4*i] = 0;
      this->Raster[4*i + 1] = 0;
      this->Raster[4*i + 2] = 0;
      this->Raster[4*i + 3] = 0;
    }

    this->UsedExtent[1] = ymax;
  }
}

//----------------------------------------------------------------------------
void vtkImageStencilRaster::InsertPoint(int y, double x, int i)
{
  size_t pos = static_cast<size_t>(y - this->Extent[0]);
  double* &rhead = this->Raster[4*pos + 2*i];
  double* &rtail = this->Raster[4*pos + 2*i + 1];

  // current size is the diff between the tail and the head
  size_t n = rtail - rhead;

  // no allocation on this raster line yet
  if (rhead == 0)
  {
    rhead = new double[2];
    rtail = rhead;
  }
  // grow whenever size reaches a power of two
  else if (n > 1 && (n & (n-1)) == 0)
  {
    double *ptr = new double[2*n];
    for (size_t j = 0; j < n; j++)
    {
      ptr[j] = rhead[j];
    }
    delete [] rhead;
    rhead = ptr;
    rtail = ptr + n;
  }

  // insert the value
  *rtail++ = x;
}

//----------------------------------------------------------------------------
void vtkImageStencilRaster::InsertLine(
  const double pt1[2], const double pt2[2])
{
  double x1 = pt1[0];
  double x2 = pt2[0];
  double y1 = pt1[1];
  double y2 = pt2[1];

  // swap end points if necessary
  if (y1 > y2)
  {
    x1 = pt2[0];
    x2 = pt1[0];
    y1 = pt2[1];
    y2 = pt1[1];
  }

  // find min and max of x values
  double xmin = x1;
  double xmax = x2;
  if (x1 > x2)
  {
    xmin = x2;
    xmax = x1;
  }

  // check for parallel to the x-axis
  if (y1 == y2)
  {
    return;
  }

  // compute dx/dy
  double grad = (x2 - x1)/(y2 - y1);

  // include tolerance for y endpoints
  double ymin[2] = { y1 - this->Tolerance, y1 + this->Tolerance };
  double ymax[2] = { y2 - this->Tolerance, y2 + this->Tolerance };

  // if tolerance is nonzero, then use a "double pattern" where the
  // raster is drawn into twice, once with the y values increased by the
  // tolerance, and again with the y values decreased by the tolerance
  int patternCount = (this->Tolerance > 0 ? 2 : 1);

  // consider both y+tol and y-tol
  for (int i = 0; i < patternCount; i++)
  {
    // Integer y values for start and end of line
    int iy1 = this->Extent[0];
    int iy2 = this->Extent[1];

    // Check for out of bounds
    if (ymax[i] < iy1 || ymin[i] >= iy2)
    {
      continue;
    }

    // Guard against extentY
    if (ymin[i] >= iy1)
    {
      iy1 = vtkMath::Floor(ymin[i]) + 1;
    }
    if (ymax[i] < iy2)
    {
      iy2 = vtkMath::Floor(ymax[i]);
    }

    // Expand allocated extent if necessary
    if (iy1 < this->UsedExtent[0] ||
        iy2 > this->UsedExtent[1])
    {
      this->PrepareExtent(iy1, iy2);
    }

    // Compute initial offset for a Bresenham-like line algorithm
    double delta = (iy1 - y1)*grad;

    // Go along y and place each x in the proper raster line
    for (int y = iy1; y <= iy2; y++)
    {
      double x = x1 + delta;
      // incrementing delta has less roundoff error than incrementing x,
      // since delta will typically be smaller than x
      delta += grad;

      // clamp x (because of tolerance, it might not be in range)
      x = ((x < xmax) ? x : xmax);
      x = ((x > xmin) ? x : xmin);

      this->InsertPoint(y, x, i);
    }
  }
}

#ifndef VTK_LEGACY_REMOVE
//----------------------------------------------------------------------------
void vtkImageStencilRaster::InsertLine(
  const double pt1[2], const double pt2[2], bool, bool)
{
  VTK_LEGACY_REPLACED_BODY(
    vtkImageStencilRaster::InsertLine(double[2], double[2], bool, bool),
    "VTK 6.2", vtkImageStencilRaster::InsertLine(double[2], double[2]));

  this->InsertLine(pt1, pt2);
}
#endif

//----------------------------------------------------------------------------
void vtkImageStencilRaster::FillStencilData(
  vtkImageStencilData *data, const int extent[6], int xj, int yj)
{
  if (xj != 0)
  {
    // slices are stacked in the x direction
    int xmin = extent[2*xj];
    int xmax = extent[2*xj+1];
    int ymin = this->UsedExtent[0];
    int ymax = this->UsedExtent[1];
    int zmin = extent[0];
    int zmax = extent[1];

    for (int idY = ymin; idY <= ymax; idY++)
    {
      size_t pos = static_cast<size_t>(idY - this->Extent[0]);
      double *rline[2] = { this->Raster[4*pos], this->Raster[4*pos + 2] };
      double *rlineEnd[2] = { this->Raster[4*pos+1], this->Raster[4*pos+3] };

      for (int i = 0; i < 2; i++)
      {
        if (rline[i])
        {
          // process in order from lowest to highest
          std::sort(rline[i], rlineEnd[i]);
          // force size to be divisible by two
          rlineEnd[i] -= (rlineEnd[i] - rline[i]) % 2;
        }
      }

      int xy[2];
      xy[2-xj] = idY;

      int lastr = VTK_INT_MIN;
      for (;;)
      {
        // find the span with the lowest lower bound
        double x1 = VTK_DOUBLE_MAX;
        int j = -1;
        for (int i = 0; i < 2; i++)
        {
          if (rline[i] != rlineEnd[i])
          {
            if (rline[i][0] < x1)
            {
              x1 = rline[i][0];
              j = i;
            }
          }
        }

        // done if no spans remain
        if (j < 0)
        {
          break;
        }

        // get upper bound for the span, then increment to next span
        double x2 = rline[j][1];
        rline[j] += 2;

        // increase the span by the tolerance
        x1 -= this->Tolerance;
        x2 += this->Tolerance;

        // make sure one of the ends is in bounds
        if (x2 < xmin || x1 >= xmax)
        {
          continue;
        }

        // clip the line segment with the bounds
        int r1 = xmin;
        int r2 = xmax;

        if (x1 >= xmin)
        {
          r1 = vtkMath::Floor(x1) + 1;
        }
        if (x2 < xmax)
        {
          r2 = vtkMath::Floor(x2);
        }

        // ensure no overlap occurs with previous
        if (r1 <= lastr)
        {
          r1 = lastr + 1;
        }
        if (r2 > lastr)
        {
          lastr = r2;

          for (int idX = r1; idX <= r2; idX++)
          {
            xy[xj-1] = idX;
            data->InsertNextExtent(zmin, zmax, xy[0], xy[1]);
          }
        }
      }
    }
  }
  else
  {
    // slices stacked in the y or z direction
    int zj = 3 - yj;
    int xmin = extent[0];
    int xmax = extent[1];
    int ymin = this->UsedExtent[0];
    int ymax = this->UsedExtent[1];
    int zmin = extent[2*zj];
    int zmax = extent[2*zj+1];

    // convert each raster line into extents for the stencil
    for (int idY = ymin; idY <= ymax; idY++)
    {
      size_t pos = static_cast<size_t>(idY - this->Extent[0]);
      double *rline[2] = { this->Raster[4*pos], this->Raster[4*pos + 2] };
      double *rlineEnd[2] = { this->Raster[4*pos+1], this->Raster[4*pos+3] };

      for (int i = 0; i < 2; i++)
      {
        if (rline[i])
        {
          // process in order from lowest to highest
          std::sort(rline[i], rlineEnd[i]);
          // force the size to be divisible by two
          rlineEnd[i] -= (rlineEnd[i] - rline[i]) % 2;
        }
      }

      int yz[2];
      yz[yj-1] = idY;
      yz[2-yj] = zmin;

      // go through each raster line and fill the stencil
      int lastr = VTK_INT_MIN;
      for (;;)
      {
        // find the span with the lowest lower bound
        double x1 = VTK_DOUBLE_MAX;
        int j = -1;
        for (int i = 0; i < 2; i++)
        {
          if (rline[i] != rlineEnd[i])
          {
            if (rline[i][0] < x1)
            {
              x1 = rline[i][0];
              j = i;
            }
          }
        }

        // done if no spans remain
        if (j < 0)
        {
          break;
        }

        // get upper bound for the span, then increment to next span
        double x2 = rline[j][1];
        rline[j] += 2;

        // increase the span by the tolerance
        x1 -= this->Tolerance;
        x2 += this->Tolerance;

        // verify that it lies at least partially within the bounds
        if (x2 < xmin || x1 >= xmax)
        {
          continue;
        }

        // convert from floating-point to integers
        int r1 = xmin;
        int r2 = xmax;

        if (x1 >= xmin)
        {
          r1 = vtkMath::Floor(x1) + 1;
        }
        if (x2 < xmax)
        {
          r2 = vtkMath::Floor(x2);
        }

        // ensure no overlap occurs between extents
        if (r1 <= lastr)
        {
          r1 = lastr + 1;
        }
        if (r2 > lastr)
        {
          lastr = r2;

          if (r2 >= r1)
          {
            data->InsertNextExtent(r1, r2, yz[0], yz[1]);
          }
        }
      }
    }

    // copy the result to all other slices
    if (zmin < zmax)
    {
      for (int idY = ymin; idY <= ymax; idY++)
      {
        int r1, r2;
        int yz[2];

        yz[yj-1] = idY;
        yz[2-yj] = zmin;

        int iter = 0;
        while (data->GetNextExtent(r1, r2, xmin, xmax, yz[0], yz[1], iter))
        {
          for (int idZ = zmin + 1; idZ <= zmax; idZ++)
          {
            yz[2-yj] = idZ;
            data->InsertNextExtent(r1, r2, yz[0], yz[1]);
          }
          yz[2-yj] = zmin;
        }
      }
    }
  }
}
