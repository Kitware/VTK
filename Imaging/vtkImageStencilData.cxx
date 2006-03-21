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
#include "vtkObjectFactory.h"

#include <math.h>

vtkCxxRevisionMacro(vtkImageStencilData, "1.15.6.1");
vtkStandardNewMacro(vtkImageStencilData);

//----------------------------------------------------------------------------
vtkImageStencilData::vtkImageStencilData()
{
  this->Spacing[0] = this->OldSpacing[0] = 1;
  this->Spacing[1] = this->OldSpacing[1] = 1;
  this->Spacing[2] = this->OldSpacing[2] = 1;

  this->Origin[0] = this->OldOrigin[0] = 0;
  this->Origin[1] = this->OldOrigin[1] = 0;
  this->Origin[2] = this->OldOrigin[2] = 0;

  this->NumberOfExtentEntries = 0;
  this->ExtentLists = NULL;
  this->ExtentListLengths = NULL;

  int extent[6] = {0, -1, 0, -1, 0, -1};
  memcpy(this->Extent, extent, 6*sizeof(int));
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

  os << indent << "OldSpacing: (" 
     << this->OldSpacing[0] << ", "
     << this->OldSpacing[1] << ", "
     << this->OldSpacing[2] << ")\n";

  os << indent << "OldOrigin: (" 
     << this->OldOrigin[0] << ", "
     << this->OldOrigin[1] << ", "
     << this->OldOrigin[2] << ")\n";
}

//----------------------------------------------------------------------------
void vtkImageStencilData::Initialize()
{
  if (this->ExtentLists)
    {
    int n = this->NumberOfExtentEntries;
    for (int i = 0; i < n; i++)
      {
      delete [] this->ExtentLists[i];
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
      delete [] this->ExtentLists[i];
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
      int m = this->ExtentListLengths[i];
      this->ExtentLists[i] = new int[m];
      for (int j = 0; j < m; j++)
        {
        this->ExtentLists[i][j] = s->ExtentLists[i][j];
        }
      }
    }
  memcpy(this->Extent, s->GetExtent(), 6*sizeof(int));
}

//----------------------------------------------------------------------------
// Override from vtkDataObject because we have to handle the Spacing
// and Origin as well as the UpdateExtent.
void vtkImageStencilData::PropagateUpdateExtent()
{
#if 0
  // If we need to update due to PipelineMTime, or the fact that our
  // data was released, then propagate the update extent to the source 
  // if there is one.
  if ( this->UpdateTime < this->PipelineMTime || this->DataReleased ||
       this->UpdateExtentIsOutsideOfTheExtent() || 
       this->SpacingOrOriginHasChanged() || 
       this->LastUpdateExtentWasOutsideOfTheExtent)
    {
    if (this->Source)
      {
      this->Source->PropagateUpdateExtent(this);
      }
    }
  
  // update the value of this ivar
  this->LastUpdateExtentWasOutsideOfTheExtent = 
    this->UpdateExtentIsOutsideOfTheExtent();
#else
  this->Superclass::PropagateUpdateExtent();
#endif
}

//----------------------------------------------------------------------------
// Override from vtkDataObject because we have to handle the Spacing
// and Origin as well as the UpdateExtent.
void vtkImageStencilData::TriggerAsynchronousUpdate()
{
#if 0
  // If we need to update due to PipelineMTime, or the fact that our
  // data was released, then propagate the trigger to the source
  // if there is one.
  if ( this->UpdateTime < this->PipelineMTime || this->DataReleased ||
       this->UpdateExtentIsOutsideOfTheExtent() || 
       this->SpacingOrOriginHasChanged())
    {
    if (this->Source)
      {
      this->Source->TriggerAsynchronousUpdate();
      }
    }
#else
  this->Superclass::TriggerAsynchronousUpdate();
#endif
}

//----------------------------------------------------------------------------
void vtkImageStencilData::UpdateData()
{
#if 0
  // If we need to update due to PipelineMTime, or the fact that our
  // data was released, then propagate the UpdateData to the source
  // if there is one.
  if (this->UpdateTime < this->PipelineMTime || this->DataReleased ||
      this->UpdateExtentIsOutsideOfTheExtent() ||
      this->SpacingOrOriginHasChanged())
    {
    if (this->Source)
      {
      this->Source->UpdateData(this);
      }
    }
#else
  this->Superclass::UpdateData();
#endif
}

//----------------------------------------------------------------------------
int vtkImageStencilData::SpacingOrOriginHasChanged()
{
  double *spacing = this->Spacing;
  double *origin = this->Origin;
  double *ospacing = this->OldSpacing;
  double *oorigin = this->OldOrigin;

  return (spacing[0] != ospacing[0] || origin[0] != oorigin[0] ||
          spacing[1] != ospacing[1] || origin[1] != oorigin[1] ||
          spacing[2] != ospacing[2] || origin[2] != oorigin[2]);
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
        delete [] this->ExtentLists[i];
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
        delete this->ExtentLists[i];
        this->ExtentLists[i] = NULL;
        }
      }
    }
}

//----------------------------------------------------------------------------
// Given the output x extent [rmin,rmax] and the current y, z indices,
// return the sub extents [r1,r2] for which transformation/interpolation
// are to be done.  The variable 'iter' should be initialized to zero
// before the first call.  The return value is zero if there are no
// extents remaining.
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
