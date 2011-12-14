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

#include <math.h>
#include <algorithm>

vtkStandardNewMacro(vtkImageStencilData);

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

  if (this->ExtentListLengths)
    {
    delete [] this->ExtentListLengths;
    }
  this->ExtentListLengths = NULL;

  if(this->Information)
    {
    int extent[6] = {0, -1, 0, -1, 0, -1};
    memcpy(this->Extent, extent, 6*sizeof(int));
    }
}

//----------------------------------------------------------------------------
void vtkImageStencilData::CopyInformationToPipeline(vtkInformation* request,
                                                    vtkInformation* input,
                                                    vtkInformation* output,
                                                    int forceCopy)
{
  // Let the superclass copy whatever it wants.
  this->Superclass::CopyInformationToPipeline(request, input, output, forceCopy);

  // Set default pipeline information during a request for information.
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    // Copy settings from the input if available.  Otherwise use our
    // current settings.

    if(input && input->Has(ORIGIN()))
      {
      output->CopyEntry(input, ORIGIN());
      }
    else if (!output->Has(ORIGIN()) || forceCopy)
      {
      // Set origin (only if it is not set).
      output->Set(ORIGIN(), this->GetOrigin(), 3);
      }

    if(input && input->Has(SPACING()))
      {
      output->CopyEntry(input, SPACING());
      }
    else if (!output->Has(SPACING()) || forceCopy)
      {
      // Set spacing (only if it is not set).
      output->Set(SPACING(), this->GetSpacing(), 3);
      }
    }
}

//----------------------------------------------------------------------------
void vtkImageStencilData::CopyInformationFromPipeline(vtkInformation* request)
{
  // Let the superclass copy whatever it wants.
  this->Superclass::CopyInformationFromPipeline(request);

  // Copy pipeline information to data information before the producer
  // executes.
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    this->CopyOriginAndSpacingFromPipeline();
    }
}

//----------------------------------------------------------------------------
void vtkImageStencilData::CopyOriginAndSpacingFromPipeline()
{
  // Copy origin and spacing from pipeline information to the internal
  // copies.
  vtkInformation* info = this->PipelineInformation;
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
void vtkImageStencilData::SetExtent(int* extent)
{
  memcpy(this->Extent, extent, 6*sizeof(int));
}

//----------------------------------------------------------------------------
void vtkImageStencilData::SetExtent(int x1, int x2, int y1, int y2, int z1, int z2)
{
  int ext[6];
  ext[0] = x1;
  ext[1] = x2;
  ext[2] = y1;
  ext[3] = y2;
  ext[4] = z1;
  ext[5] = z2;
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

  if (this->ExtentListLengths)
    {
    delete [] this->ExtentListLengths;
    }
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
int vtkImageStencilData::IsInside( int xIdx, int yIdx, int zIdx )
{

  // This can be faster than GetNextExtent if called on every voxel
  // (non-sequentially). If calling sequentially, the preferred way is to
  // use GetNextExtent and then loop over the returned [r1,r2] extents.

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
  for ( int iter = 0; iter < clistlen; )
    {
    if (clist[iter++] > xIdx)
      {
      ++iter;
      continue;
      }

    if (xIdx <= clist[iter++])
      {
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
//  Fills the stencil.  Extents must be set.
void vtkImageStencilData::Fill( void )
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
  int yMin = this->Extent[2];
  int yMax = this->Extent[3];
  int zMin = this->Extent[4];
  int incr = (yMax - yMin + 1)*(zIdx - zMin) + (yIdx - yMin);

  int &clistlen = this->ExtentListLengths[incr];
  int *&clist = this->ExtentLists[incr];

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
      int n = this->NumberOfExtentEntries;
      if (clist != &this->ExtentListLengths[n + 2*incr])
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

//----------------------------------------------------------------------------
void vtkImageStencilData::InsertAndMergeExtent(int r1, int r2,
                                               int yIdx, int zIdx)
{
  // calculate the index into the extent array
  int yMin = this->Extent[2];
  int yMax = this->Extent[3];
  int zMin = this->Extent[4];
  int incr = (yMax - yMin + 1)*(zIdx - zMin) + (yIdx - yMin);

  int &clistlen = this->ExtentListLengths[incr];
  int *&clist = this->ExtentLists[incr];

  if (clistlen == 0)
    {
    clist[clistlen] = r1;
    clist[clistlen + 1] = r2 + 1;
    clistlen += 2;
    return;
    }
  
  for (int k = 0; k < clistlen; k+=2)
    {
    if ((r1 >= clist[k] && r1 < clist[k+1]) || 
        (r2 >= clist[k] && r2 < clist[k+1]))
      {
      // An intersecting extent is already present. Merge with that one.
      if (r1 < clist[k])
        {
        clist[k] = r1;
        }
      else if (r2 >= clist[k+1])
        {
        clist[k+1] = r2+1;
        this->CollapseAdditionalIntersections(r2, k+2, clist, clistlen);
        }
      return;
      }
    else if (r1 < clist[k] && r2 >= clist[k+1])
      {
      clist[k]   = r1;
      clist[k+1] = r2+1;
      this->CollapseAdditionalIntersections(r2, k+2, clist, clistlen);
      return;
      }
    }

  // We will be inserting a unique extent...

  // check whether more space is needed
  // the allocated space is always the smallest power of two
  // that is not less than the number of stored items, therefore
  // we need to allocate space when clistlen is a power of two
  if (clistlen > 0 && (clistlen & (clistlen-1)) == 0)
    {
    int *newclist = new int[2*clistlen];
    for (int k = 0; k < clistlen; k++)
      {
      newclist[k] = clist[k];
      }
    int n = this->NumberOfExtentEntries;
    if (clist != &this->ExtentListLengths[n + incr])
      {
      delete [] clist;
      }
    clist = newclist;
    }

  // shift to make room for the insertion
  int insertIndex = clistlen;
  clistlen += 2;
  while (r1 < clist[insertIndex-1])
    {
    clist[insertIndex] = clist[insertIndex-2];
    clist[insertIndex+1] = clist[insertIndex-1];
    insertIndex -= 2;
    }

  clist[insertIndex] = r1;
  clist[insertIndex+1] = r2 + 1;
}


//----------------------------------------------------------------------------
void vtkImageStencilData::CollapseAdditionalIntersections(int r2, int idx,
                                                          int *clist,
                                                          int &clistlen)
{
  if (idx >= clistlen)
    {
    return;
    }

  int removeExtentStart = idx, removeExtentEnd = idx;
  // overlap with any of the remainder of the list?
  for (; idx < clistlen; idx+=2, removeExtentEnd+=2)
    {
    if (r2 < clist[idx])
      {
      if (idx == removeExtentStart)
        {
        // no additional overlap... thus no collapse
        return;
        }
      break;
      }
    else if (r2 < clist[idx+1])
      {
      clist[removeExtentStart - 1] = clist[idx+1];
      }
    }

  // collapse the list?
  int i;
  for (i = removeExtentEnd, idx = removeExtentStart; i < clistlen; i++, idx++)
    {
    clist[idx] = clist[i];
    }
  clistlen = idx;
}

//----------------------------------------------------------------------------
void vtkImageStencilData::RemoveExtent(int r1, int r2, int yIdx, int zIdx)
{
  int xMin = this->Extent[0];
  int xMax = this->Extent[1];
  int yMin = this->Extent[2];
  int yMax = this->Extent[3];
  int zMin = this->Extent[4];
  int zMax = this->Extent[5];

  if (zIdx < zMin || zIdx > zMax || yIdx < yMin || yIdx > yMax )
    {
    return;
    }
  
  // calculate the index into the extent array
  int incr = (yMax - yMin + 1)*(zIdx - zMin) + (yIdx - yMin);

  int &clistlen = this->ExtentListLengths[incr];
  int *&clist = this->ExtentLists[incr];

  if (clistlen == 0)
    { // nothing here.. nothing to remove
    return;
    }

  if (r1 <= xMin && r2 >= xMax)
    {
    // remove the whole row.
    clistlen = 0;
    int n = this->NumberOfExtentEntries;
    if (clist != &this->ExtentListLengths[n + 2*incr])
      {
      delete [] clist;
      clist = &this->ExtentListLengths[n + 2*incr];
      }
    return;
    }
  
  int length = clistlen;
  for (int k = 0; k < length; k += 2)
    {
    if (r1 <=  clist[k] && r2 >= (clist[k+1]-1))
      {  
      // Remove this entry;
      clistlen -= 2;

      if (clistlen == 0)
        {
        int n = this->NumberOfExtentEntries;
        if (clist != &this->ExtentListLengths[n + 2*incr])
          {
          delete [] clist;
          clist = &this->ExtentListLengths[n + 2*incr];
          }
        return;
        }

      int clistmaxlen = 2;
      while (clistlen > clistmaxlen)
        {
        clistmaxlen *= 2;
        }

      if (clistmaxlen == clistlen)
        {
        int n = this->NumberOfExtentEntries;
        int *newclist = &this->ExtentListLengths[n + 2*incr];
        if (clistmaxlen > 2)
          {
          newclist = new int[clistmaxlen];
          }
        for (int m = 0; m < k; m++)
          {
          newclist[m] = clist[m];
          }
        for (int m = k+2; m < length; m++)
          {
          newclist[m-2] = clist[m];
          }
        if (clist != &this->ExtentListLengths[n + 2*incr])
          {
          delete [] clist;
          }
        clist = newclist;
        }
      else
        {
        for (int m = k+2; m < length; m++)
          {
          clist[m-2] = clist[m];
          }
        }

      length = clistlen;
      if (k >= length)
        {
        return;
        }
      }
     
    if ((r1 >= clist[k] && r1 < clist[k+1]) || 
        (r2 >= clist[k] && r2 < clist[k+1]))
      {

      bool split = false;  
      int tmp = -1;    

      // An intersecting extent is already present. Merge with that one.
      if (r1 > clist[k])
        {
        tmp = clist[k+1];
        clist[k+1] = r1;
        split    = true;
        }
      if (split)
        {
        if (r2 < tmp-1)
          {
          // check whether more space is needed
          // the allocated space is always the smallest power of two
          // that is not less than the number of stored items, therefore
          // we need to allocate space when clistlen is a power of two
          if (clistlen > 0 && (clistlen & (clistlen-1)) == 0)
            {
            int *newclist = new int[2*clistlen];
            for (int m = 0; m < clistlen; m++)
              {
              newclist[m] = clist[m];
              }
            int n = this->NumberOfExtentEntries;
            if (clist != &this->ExtentListLengths[n + 2*incr])
              {
              delete [] clist;
              }
            clist = newclist;
            }
          clist[clistlen] = r2+1;
          clist[clistlen+1] = tmp;
          clistlen += 2;
          }
        }
      else
        {
        if (r2 < clist[k+1]-1)
          {
          clist[k] = r2+1;
          }
        }
      }
    }
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
void vtkImageStencilData::InternalAdd( vtkImageStencilData * stencil1 )
{
  int extent[6], extent1[6], extent2[6], r1, r2, idy, idz, iter=0;
  stencil1->GetExtent(extent1);
  this->GetExtent(extent2);
  
  extent[0] = (extent1[0] < extent2[0]) ? extent2[0] : extent1[0];
  extent[1] = (extent1[1] > extent2[1]) ? extent2[1] : extent1[1];
  extent[2] = (extent1[2] < extent2[2]) ? extent2[2] : extent1[2];
  extent[3] = (extent1[3] > extent2[3]) ? extent2[3] : extent1[3];
  extent[4] = (extent1[4] < extent2[4]) ? extent2[4] : extent1[4];
  extent[5] = (extent1[5] > extent2[5]) ? extent2[5] : extent1[5];

  bool modified = false;
  for (idz=extent[4]; idz<=extent[5]; idz++, iter=0)
    {
    for (idy = extent[2]; idy <= extent[3]; idy++, iter=0)
      {
      int moreSubExtents = 1;
      while( moreSubExtents )
        {
        moreSubExtents = stencil1->GetNextExtent( 
          r1, r2, extent[0], extent[1], idy, idz, iter);

        if (r1 <= r2 ) // sanity check 
          { 
          this->InsertAndMergeExtent(r1, r2, idy, idz); 
          modified = true;
          }
        }
      }
    }
  
  if (modified)
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkImageStencilData::Add( vtkImageStencilData * stencil1 )
{
  int extent[6], extent1[6], extent2[6], r1, r2, idy, idz, iter=0;
  stencil1->GetExtent(extent1);
  this->GetExtent(extent2);

  if (extent1[0] > extent1[1] || 
      extent1[2] > extent1[3] || 
      extent1[4] > extent1[5])
    {
    return;
    }

  if (vtkMath::ExtentIsWithinOtherExtent(extent1,extent2))
    {

    // Extents of stencil1 are entirely within the Self's extents. There
    // is no need to re-allocate the extent lists.

    this->InternalAdd(stencil1);
    return;
    }

  // Need to reallocate extent lists. 
  // 1. We will create a temporary stencil data.
  // 2. Copy Self into this temporary stencil. 
  // 3. Reallocate Self's extents to match the resized stencil. 
  // 4. Merge stencil data from both into the Self.

  // Find the smallest bounding box large enough to hold both stencils.
  extent[0] = (extent1[0] > extent2[0]) ? extent2[0] : extent1[0];
  extent[1] = (extent1[1] < extent2[1]) ? extent2[1] : extent1[1];
  extent[2] = (extent1[2] > extent2[2]) ? extent2[2] : extent1[2];
  extent[3] = (extent1[3] < extent2[3]) ? extent2[3] : extent1[3];
  extent[4] = (extent1[4] > extent2[4]) ? extent2[4] : extent1[4];
  extent[5] = (extent1[5] < extent2[5]) ? extent2[5] : extent1[5];

  vtkImageStencilData *tmp = vtkImageStencilData::New();
  tmp->DeepCopy(this);

  this->SetExtent(extent);
  this->AllocateExtents(); // Reallocate extents.

  for (idz=extent2[4]; idz<=extent2[5]; idz++, iter=0)
    {
    for (idy = extent2[2]; idy <= extent2[3]; idy++, iter=0)
      {
      int moreSubExtents = 1;
      while( moreSubExtents )
        {
        moreSubExtents = tmp->GetNextExtent( 
          r1, r2, extent[0], extent[1], idy, idz, iter);

        if (r1 <= r2 ) // sanity check 
          { 
          this->InsertAndMergeExtent(r1, r2, idy, idz); 
          }
        }
      }
    }
  
  tmp->Delete();

  for (idz=extent1[4]; idz<=extent1[5]; idz++, iter=0)
    {
    for (idy = extent1[2]; idy <= extent1[3]; idy++, iter=0)
      {
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
void vtkImageStencilData::Subtract( vtkImageStencilData * stencil1 )
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
      int moreSubExtents = 1;
      while( moreSubExtents )
        {
        moreSubExtents = stencil1->GetNextExtent( 
          r1, r2, extent[0], extent[1], idy, idz, iter);

        if (r1 <= r2 ) // sanity check 
          { 
          this->RemoveExtent(r1, r2, idy, idz); 
          }
        }
      }
    }

  this->Modified();
} 

//----------------------------------------------------------------------------
void vtkImageStencilData::Replace( vtkImageStencilData * stencil1 )
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
int vtkImageStencilData::Clip( int extent[6] )
{
  int currentExtent[6], idy, idz;
  this->Update();
  this->GetExtent( currentExtent );

  if (vtkMath::ExtentIsWithinOtherExtent( currentExtent, extent ))
    {
    // Nothing to do, we are already within the clipping extents.
    return 0;
    }

  bool removeXLeft  = (extent[0] > currentExtent[0]);
  bool removeXRight = (extent[1] < currentExtent[1]);  
  bool remove = false, removed = false;

  for (idz=currentExtent[4]; idz<=currentExtent[5]; idz++)
    {
    remove = (idz < extent[4] || idz > extent[5]);
    for (idy = currentExtent[2]; idy <= currentExtent[3]; idy++)
      {
      if (remove || idy < extent[2] || idy > extent[3])
        {
        // Remove everything at Y = idy, Z = idz.
        this->RemoveExtent( currentExtent[0],currentExtent[1], idy, idz );
        removed |= true;
        }
      else
        {
        if (removeXLeft)
          {
          // Clip on the left at Y = idy, Z = idz.
          this->RemoveExtent( currentExtent[0], extent[0]-1, idy, idz );
          removed |= true;
          }
        if (removeXRight)
          {
          // Clip on the right at Y = idy, Z = idz.
          this->RemoveExtent( extent[1]+1, currentExtent[1], idy, idz );
          removed |= true;
          }
        }
      }
    }

  return (removed ? 1 : 0);
}

//----------------------------------------------------------------------------
// tolerance for float-to-int conversion in stencil operations

#define VTK_STENCIL_TOL 7.62939453125e-06

//----------------------------------------------------------------------------
vtkImageStencilRaster::vtkImageStencilRaster(const int extent[2])
{
  int rsize = extent[1] - extent[0] + 1;

  // the "raster" is a sequence of pointer-pairs, where the first pointer
  // points to the first value in the raster line, and the second pointer
  // points to one location past the last vale in the raster line.  The
  // difference is the number of x values stored in the raster line.
  this->Raster = new double*[2*static_cast<size_t>(rsize)];

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
    size_t i = 2*static_cast<size_t>(this->UsedExtent[0] - this->Extent[0]);
    size_t imax = 2*static_cast<size_t>(this->UsedExtent[1] - this->Extent[0]);

    do
      {
      if (this->Raster[i])
        {
        delete [] this->Raster[i];
        }
      i += 2;
      }
    while (i <= imax);
    }
  delete [] this->Raster;
}

//----------------------------------------------------------------------------
void vtkImageStencilRaster::PrepareForNewData(const int allocateExtent[2])
{
  if (this->UsedExtent[1] >= this->UsedExtent[0])
    {
    // reset and re-use the allocated raster lines
    size_t i = 2*static_cast<size_t>(this->UsedExtent[0]-this->Extent[0]);
    size_t imax=2*static_cast<size_t>(this->UsedExtent[1]-this->Extent[0]);
    do
      {
      this->Raster[i+1] = this->Raster[i];
      i += 2;
      }
    while (i <= imax);
    }

  if (allocateExtent && allocateExtent[1] >= allocateExtent[1])
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
    size_t i = 2*static_cast<size_t>(ymin - this->Extent[0]);
    size_t imax = 2*static_cast<size_t>(ymax - this->Extent[0]);

    do
      {
      this->Raster[i] = 0;
      }
    while (++i <= imax);

    this->UsedExtent[0] = ymin;
    this->UsedExtent[1] = ymax;

    return;
    }

  if (ymin < this->UsedExtent[0])
    {
    size_t i = 2*static_cast<size_t>(ymin - this->Extent[0]);
    size_t imax = 2*static_cast<size_t>(this->UsedExtent[0]-this->Extent[0]-1);

    do
      {
      this->Raster[i] = 0;
      }
    while (++i <= imax);

    this->UsedExtent[0] = ymin;
    }

  if (ymax > this->UsedExtent[1])
    {
    size_t i = 2*static_cast<size_t>(this->UsedExtent[1]+1 - this->Extent[0]);
    size_t imax = 2*static_cast<size_t>(ymax - this->Extent[0]);

    do
      {
      this->Raster[i] = 0;
      }
    while (++i <= imax);

    this->UsedExtent[1] = ymax;
    }
}

//----------------------------------------------------------------------------
void vtkImageStencilRaster::InsertPoint(int y, double x)
{
  size_t pos = 2*static_cast<size_t>(y - this->Extent[0]);
  double* &rhead = this->Raster[pos];
  double* &rtail = this->Raster[pos+1];

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
    for (size_t i = 0; i < n; i++)
      {
      ptr[i] = rhead[i];
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
  const double pt1[2], const double pt2[2],
  bool inflection1, bool inflection2)
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
    bool tmp = inflection1;
    inflection1 = inflection2;
    inflection2 = tmp;
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

  double ymin = y1;
  double ymax = y2;

  // if an end is an inflection point, include a tolerance
  ymin -= inflection1*this->Tolerance;
  ymax += inflection2*this->Tolerance;

  // Integer y values for start and end of line
  int iy1, iy2;
  iy1 = this->Extent[0];
  iy2 = this->Extent[1];

  // Check for out of bounds
  if (ymax < iy1 || ymin >= iy2)
    {
    return;
    }

  // Guard against extentY
  if (ymin >= iy1)
    {
    iy1 = vtkMath::Floor(ymin) + 1;
    }
  if (ymax < iy2)
    {
    iy2 = vtkMath::Floor(ymax);
    }

  // Expand allocated extent if necessary
  if (iy1 < this->UsedExtent[0] ||
      iy2 > this->UsedExtent[1])
    {
    this->PrepareExtent(iy1, iy2);
    }

  // Precompute values for a Bresenham-like line algorithm
  double grad = (x2 - x1)/(y2 - y1);
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

    this->InsertPoint(y, x);
    }
}

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
      size_t pos = 2*static_cast<size_t>(idY - this->Extent[0]);
      double *rline = this->Raster[pos];
      double *rlineEnd = this->Raster[pos+1];

      if (rline == 0)
        {
        continue;
        }

      std::sort(rline, rlineEnd);

      int xy[2];
      xy[2-xj] = idY;

      int lastr = VTK_INT_MIN;

      size_t l = rlineEnd - rline;
      l = l - (l & 1); // force l to be an even number
      for (size_t k = 0; k < l; k += 2)
        {
        double x1 = rline[k] - this->Tolerance;
        double x2 = rline[k+1] + this->Tolerance;

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
        lastr = r2;

        for (int idX = r1; idX <= r2; idX++)
          {
          xy[xj-1] = idX;
          data->InsertNextExtent(zmin, zmax, xy[0], xy[1]);
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
      size_t pos = 2*static_cast<size_t>(idY - this->Extent[0]);
      double *rline = this->Raster[pos];
      double *rlineEnd = this->Raster[pos+1];

      if (rline == 0)
        {
        continue;
        }

      std::sort(rline, rlineEnd);

      int lastr = VTK_INT_MIN;

      // go through each raster line and fill the stencil
      size_t l = rlineEnd - rline;
      l = l - (l & 1); // force l to be an even number
      for (size_t k = 0; k < l; k += 2)
        {
        int yz[2];

        yz[yj-1] = idY;
        yz[2-yj] = zmin;

        double x1 = rline[k] - this->Tolerance;
        double x2 = rline[k+1] + this->Tolerance;

        if (x2 < xmin || x1 >= xmax)
          {
          continue;
          }

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
        lastr = r2;

        if (r2 >= r1)
          {
          data->InsertNextExtent(r1, r2, yz[0], yz[1]);
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
