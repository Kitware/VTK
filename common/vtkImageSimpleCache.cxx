/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSimpleCache.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkImageSimpleCache.h"

//----------------------------------------------------------------------------
vtkImageSimpleCache::vtkImageSimpleCache()
{
  this->CachedData = NULL;
}


//----------------------------------------------------------------------------
vtkImageSimpleCache::~vtkImageSimpleCache()
{
  if (this->CachedData)
    {
    this->CachedData->Delete();
    this->CachedData = NULL;
    }
}


//----------------------------------------------------------------------------
void vtkImageSimpleCache::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageCache::PrintSelf(os,indent);

  os << indent << "GenerateTime: " << this->GenerateTime << "\n";

  if ( ! this->CachedData)
    {
    os << indent << "CachedData: None\n";
    }
  else
    {
    os << indent << "CachedData: \n";
    this->CachedData->PrintSelf(os, indent.GetNextIndent());
    }
}

//----------------------------------------------------------------------------
// This Method deletes any data in cache.
void vtkImageSimpleCache::ReleaseData()
{
  if (this->CachedData)
    {
    this->CachedData->Delete();
    this->CachedData = NULL;
    }
}

void vtkImageSimpleCache::AllocateData()
{
  int dim[3];
  if (!this->CachedData) 
    {
    this->CachedData = vtkImageData::New();
    }
  
  this->CachedData->SetExtent(this->UpdateExtent);
  this->CachedData->SetOrigin(this->GetOrigin());
  this->CachedData->SetSpacing(this->GetSpacing());
  this->GetDimensions(dim);
  this->CachedData->SetDimensions(dim);
  this->CachedData->SetScalarType(this->ScalarType);
  this->CachedData->
    SetNumberOfScalarComponents(this->NumberOfScalarComponents);
  this->CachedData->AllocateScalars();
}

vtkImageData *vtkImageSimpleCache::UpdateAndReturnData()
{
  this->Update(); 
  return this->CachedData;
}


//----------------------------------------------------------------------------
// This method updates the region specified by "UpdateExtent".  
void vtkImageSimpleCache::Update()
{
  int updateExtentSave[6];
  unsigned long pipelineMTime = this->GetPipelineMTime();
  int *cachedExtent;
  
  // update if mtime indicates to do so
  if (pipelineMTime > this->ExecuteTime)
    {
    // Make sure image information is upto date
    this->UpdateImageInformation();
    }
  
  this->ClipUpdateExtentWithWholeExtent();
    
  // Let the cache modify the update extent, but save the old one.
  this->GetUpdateExtent(updateExtentSave);
  this->Source->InterceptCacheUpdate();
  
  if (this->CachedData)
    {
    cachedExtent = this->CachedData->GetExtent();
    }
  
  // if cache doesn't have the necessary data.
  if (pipelineMTime > this->ExecuteTime || this->DataReleased ||
      !this->CachedData || 
      (cachedExtent[0] > this->UpdateExtent[0] ||
       cachedExtent[1] < this->UpdateExtent[1] ||
       cachedExtent[2] > this->UpdateExtent[2] ||
       cachedExtent[3] < this->UpdateExtent[3] ||
       cachedExtent[4] > this->UpdateExtent[4] ||
       cachedExtent[5] < this->UpdateExtent[5]))
    {
    if (this->Source)
      {
      vtkDebugMacro("Update: We have to update the source.");
      // release the old data and setup the new
      this->AllocateData();
      this->Source->InternalUpdate(this->CachedData);
      // save the time and extent of the update for test "cache has data?"
      this->ExecuteTime.Modified();
      this->DataReleased = 0;
      }
    }
  else
    {
    vtkDebugMacro("Update: UpdateRegion already in cache.");
    }
  
  // Restore the old update extent
  this->SetUpdateExtent(updateExtentSave);
}



//----------------------------------------------------------------------------
// return the un filled data of the UpdateExtent in this cache.
vtkImageData *vtkImageSimpleCache::GetData()
{
  if (! this->CachedData)
    {
    this->AllocateData();
    }
  
  return this->CachedData;
}

//----------------------------------------------------------------------------
template <class T>
static void vtkImageSimpleCacheComputeRange(vtkImageData *data, int *ext,
					    float *range, T *ptr)
{
  int idxR, idxY, idxZ;
  int maxY, maxZ;
  int incX, incY, incZ;
  int rowLength;
  T r0, r1;

  r0 = r1 = *ptr;
  
  // find the region to loop over
  rowLength = (ext[1] - ext[0]+1)*data->GetNumberOfScalarComponents();
  maxY = ext[3] - ext[2]; 
  maxZ = ext[5] - ext[4];
  
  // Get increments to march through data 
  data->GetContinuousIncrements(ext, incX, incY, incZ);

  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    for (idxY = 0; idxY <= maxY; idxY++)
      {
      for (idxR = 0; idxR < rowLength; idxR++)
	{
	// Pixel operation
	if (*ptr < r0)
	  {
	  r0 = *ptr;
	  }
	if (*ptr > r1)
	  {
	  r1 = *ptr;
	  }
	ptr++;
	}
      ptr += incY;
      }
    ptr += incZ;
    }
  
  range[0] = (float)r0;
  range[1] = (float)r1;  
}


//----------------------------------------------------------------------------
void vtkImageSimpleCache::GetScalarRange(float range[2])
{
  int ext[6], *dataExt, idx;
  void *ptr;

  // make sure we have data cached
  range[0] = 0.0;
  range[1] = 1.0;
  if (this->CachedData == NULL)
    {
    vtkWarningMacro("GetScalarRange: Extent is not in cache");
    }
  
  // make sure we have all the data (clip if not).
  this->ClipUpdateExtentWithWholeExtent();
  this->GetUpdateExtent(ext);
  dataExt = this->CachedData->GetExtent();
  for (idx = 0; idx < 3; ++idx)
    {
    if (ext[idx*2] < dataExt[idx*2])
      {
      vtkWarningMacro("GetScalarRange: All of the extent is not in cache");
      ext[idx*2] = dataExt[idx*2];
      }
    if (ext[idx*2+1] > dataExt[idx*2+1])
      {
      vtkWarningMacro("GetScalarRange: All of the extent is not in cache");
      ext[idx*2+1] = dataExt[idx*2+1];
      }
    }
  
  // templated compue range function for each type
  ptr = this->CachedData->GetScalarPointerForExtent(ext);
  switch (this->CachedData->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageSimpleCacheComputeRange(this->CachedData, ext, range,
				      (float *) ptr);
      break;
    case VTK_INT:
      vtkImageSimpleCacheComputeRange(this->CachedData, ext, range,
				      (int *) ptr);
      break;
    case VTK_SHORT:
      vtkImageSimpleCacheComputeRange(this->CachedData, ext, range,
				      (short *) ptr);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageSimpleCacheComputeRange(this->CachedData, ext, range,
				      (unsigned short *) ptr);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageSimpleCacheComputeRange(this->CachedData, ext, range,
				      (unsigned char *) ptr);
      break;
    default:
      vtkErrorMacro("GetScalarRange: Could not handle scalar type.");
    }
} 
  
  
  

