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

  this->Information->Set(vtkDataObject::DATA_EXTENT_TYPE(), VTK_3D_EXTENT);
  this->Information->Set(vtkDataObject::DATA_EXTENT(), this->Extent, 6);

  int extent[6] = {0, -1, 0, -1, 0, -1};
  memcpy(this->Extent, extent, 6*sizeof(int));
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
      if (this->ExtentLists[i]) 
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
      if (this->ExtentLists[i])
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
    this->ExtentListLengths = new int[n];
    this->ExtentLists = new int *[n];
    for (int i = 0; i < n; i++)
      {
      this->ExtentListLengths[i] = s->ExtentListLengths[i];
      if (this->ExtentListLengths[i])
        {
        int m = this->ExtentListLengths[i];

        int clistmaxlen = 2;
        while (m > clistmaxlen)
          {
          clistmaxlen *= 2;
          }
        this->ExtentLists[i] = new int[clistmaxlen];
        for (int j = 0; j < m; j++)
          {
          this->ExtentLists[i][j] = s->ExtentLists[i][j];
          }
        }
      else
        {
        this->ExtentLists[i] = 0;
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
  int numEntries = (extent[3] - extent[2] + 1)*(extent[5] - extent[4] + 1);

  if (numEntries != this->NumberOfExtentEntries)
    {
    if (this->NumberOfExtentEntries != 0)
      {
      int n = this->NumberOfExtentEntries;
      for (int i = 0; i < n; i++)
        {
        if (this->ExtentLists[i] != 0)
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
      this->ExtentLists = new int *[numEntries];
      this->ExtentListLengths = new int[numEntries];
      for (int i = 0; i < numEntries; i++)
        {
        this->ExtentListLengths[i] = 0;
        this->ExtentLists[i] = NULL;
        }
      }
    }
  else
    {
    for (int i = 0; i < numEntries; i++)
      {
      if (this->ExtentListLengths[i] != 0)
        {
        this->ExtentListLengths[i] = 0;
        delete [] this->ExtentLists[i];
        this->ExtentLists[i] = NULL;
        }
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
  int extent[6];
  this->GetExtent(extent);
  int yExt = extent[3] - extent[2] + 1;
  int zExt = extent[5] - extent[4] + 1;
  yIdx -= extent[2];
  zIdx -= extent[4];

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
//  Fills the stencil.  Extents must be set.
void vtkImageStencilData::Fill( void )
{
  int extent[6];
  this->GetExtent(extent);

  int yExt = extent[3] - extent[2] + 1;
  for (int idz=extent[4]; idz<=extent[5]; idz++)
    {
    int zIdx = idz - extent[4];
    for (int idy = extent[2]; idy <= extent[3]; idy++)
      {
      int yIdx = idy - extent[2];

      // get the ExtentList and ExtentListLength for this yIdx,zIdx
      int incr = zIdx*yExt + yIdx;
      this->ExtentListLengths[incr] = 0;
      delete [] this->ExtentLists[incr];
      this->ExtentLists[incr] = NULL;

      int &clistlen = this->ExtentListLengths[incr];
      int *&clist = this->ExtentLists[incr];
    
      clist = new int[2];
      clist[clistlen++] = extent[0];
      clist[clistlen++] = extent[1] + 1;
      }
    }
}

//----------------------------------------------------------------------------
// Insert a sub extent [r1,r2] on to the list for the x row at (yIdx,zIdx).
void vtkImageStencilData::InsertNextExtent(int r1, int r2, int yIdx, int zIdx)
{
  // calculate the index into the extent array
  int extent[6];
  this->GetExtent(extent);
  int yExt = extent[3] - extent[2] + 1;
  int incr = (zIdx - extent[4])*yExt + (yIdx - extent[2]);

  int &clistlen = this->ExtentListLengths[incr];
  int *&clist = this->ExtentLists[incr];

  if (clistlen == 0)
    { // no space has been allocated yet
    clist = new int[2];
    }
  else
    { // check whether more space is needed
    // the allocated space is always the smallest power of two
    // that is not less than the number of stored items, therefore
    // we need to allocate space when clistlen is a power of two
    int clistmaxlen = 2;
    while (clistlen > clistmaxlen)
      {
      clistmaxlen *= 2;
      }
    if (clistmaxlen == clistlen)
      { // need to allocate more space
      clistmaxlen *= 2;
      int *newclist = new int[clistmaxlen];
      for (int k = 0; k < clistlen; k++)
        {
        newclist[k] = clist[k];
        }
      delete [] clist;
      clist = newclist;
      }
    }

  clist[clistlen++] = r1;
  clist[clistlen++] = r2 + 1;
}

//----------------------------------------------------------------------------
void vtkImageStencilData::InsertAndMergeExtent(int r1, int r2,
                                               int yIdx, int zIdx)
{
  // calculate the index into the extent array
  int extent[6];
  this->GetExtent(extent);
  int yExt = extent[3] - extent[2] + 1;
  int incr = (zIdx - extent[4])*yExt + (yIdx - extent[2]);

  int &clistlen = this->ExtentListLengths[incr];
  int *&clist = this->ExtentLists[incr];

  if (clistlen == 0)
    { // no space has been allocated yet
    clist = new int[2];
    clist[clistlen++] = r1;
    clist[clistlen++] = r2 + 1;
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
        clist[k]   = r1;
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
  int clistmaxlen = 2;
  while (clistlen > clistmaxlen)
    {
    clistmaxlen *= 2;
    }
  int insertIndex = clistlen, offset = 0;
  if (clistmaxlen == clistlen || r1 < clist[clistlen-1])
    { // need to allocate more space or rearrange
    if (clistmaxlen == clistlen)
      {
      clistmaxlen *= 2;
      }
    int *newclist = new int[clistmaxlen];
    for (int k = 0; k < clistlen; k+=2)
      {
      if (offset == 0 && r1 < clist[k])
        {
        insertIndex = k;
        offset = 2;
        }
      newclist[k+offset] = clist[k];
      newclist[k+1+offset] = clist[k+1];
      }
    delete [] clist;
    clist = newclist;
    }

  clist[insertIndex] = r1;
  clist[insertIndex+1] = r2 + 1;
  clistlen += 2;
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
  if (zIdx < this->Extent[4] || zIdx > this->Extent[5] || 
      yIdx < this->Extent[2] || yIdx > this->Extent[3] )
    {
    return;
    }
  
  // calculate the index into the extent array
  int extent[6];
  this->GetExtent(extent);
  int yExt = extent[3] - extent[2] + 1;
  int incr = (zIdx - extent[4])*yExt + (yIdx - extent[2]);

  int &clistlen = this->ExtentListLengths[incr];
  int *&clist = this->ExtentLists[incr];

  if (clistlen == 0)
    { // nothing here.. nothing to remove
    return;
    }

  if (r1 <= extent[0] && r2 >= extent[1])
    {
    // remove the whole row.
    clistlen = 0;
    delete [] clist;
    clist = NULL;
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
        delete [] clist;
        clist = NULL;
        return;
        }

      int clistmaxlen = 2;
      while (clistlen > clistmaxlen)
        {
        clistmaxlen *= 2;
        }
      
      if (clistmaxlen == clistlen)
        {
        int *newclist = new int[clistmaxlen];
        for (int m = 0;   m < k;    m++)   { newclist[m]   = clist[m]; }
        for (int m = k+2; m < length; m++) { newclist[m-2] = clist[m]; }
        delete [] clist;
        clist = newclist;
        }
      else
        {
        for (int m = k+2; m < length; m++) { clist[m-2] = clist[m]; }
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
          int clistmaxlen = 2;
          while (clistlen > clistmaxlen)
            {
            clistmaxlen *= 2;
            }
          if (clistmaxlen == clistlen)
            { // need to allocate more space
            clistmaxlen *= 2;
            int *newclist = new int[clistmaxlen];
            for (int m = 0; m < clistlen; m++)
              {
              newclist[m] = clist[m];
              }
            delete [] clist;
            clist = newclist;
            }
          clist[clistlen++] = r2+1;
          clist[clistlen++] = tmp;
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

