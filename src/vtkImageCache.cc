/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCache.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
#include "vtkImageCache.hh"

//----------------------------------------------------------------------------
// Description:
// Constructor:  By default caches ReleaseDataFlags are turned off. However,
// the vtkImageCachedSource method CheckCache, which create a default cache, 
// turns this flag on.  If a cache is created and set explicitely, by 
// default it saves its data between generates.  But if the cache is created
// automatically by the vtkImageCachedSource, it does not.
vtkImageCache::vtkImageCache()
{
  this->Source = NULL;
  this->Data = NULL;
  
  // Invalid data type
  // This will be changed when the filter gets an input or
  // the DataType is set explicitly
  this->DataType = VTK_IMAGE_VOID;
  
  // default is to save data,
  // (But caches automatically created by sources set ReleaseDataFlag to 1)
  this->ReleaseDataFlag = 0;
  this->MemoryLimit = 25000000;  // 5000 x 5000 image
}


//----------------------------------------------------------------------------
vtkImageCache::~vtkImageCache()
{
  if (this->Data)
    {
    this->Data->Delete();
    this->Data = NULL;
    }
}




//----------------------------------------------------------------------------
// Description:
// This Method returns the MTime of the pipeline before this cache.
// It considers both the source and the cache.  This method assumes
// that the pipeline does not changed until the next UpdateRegion call.
unsigned long int vtkImageCache::GetPipelineMTime()
{
  unsigned long int time1, time2;

  // This objects MTime
  time1 = this->GetMTime();
  if ( ! this->Source){
    vtkWarningMacro(<< "GetPipelineMTime: Source not set.");
    return time1;
  }

  // Pipeline mtime
  time2 = this->Source->GetPipelineMTime();
  
  // Return the larger of the two
  if (time2 > time1)
    time1 = time2;

  return time1;
}


//----------------------------------------------------------------------------
// Description:
// This method computes and returns the bounding box of the whole image.
// Any data associated with "region" is ignored.
void vtkImageCache::UpdateImageInformation(vtkImageRegion *region)
{
  int saveAxes[VTK_IMAGE_DIMENSIONS];

  // Save the old coordinate system
  region->GetAxes4d(saveAxes);

  if (this->ImageBoundsTime.GetMTime() < this->GetPipelineMTime())
    {
    // pipeline has been modified, we have to get the ImageBounds again.
    vtkDebugMacro(<< "UpdateImageInformation: Pipeline modified, "
                  << "recompute ImageBounds");
    if ( ! this->Source)
      {
      vtkErrorMacro(<< "UpdateImageInformation: No source");
      return;
      }

    // Translate region into the sources coordinate system. (save old)
    region->SetAxes4d(this->Source->GetAxes());
    // Get the ImageBounds
    this->Source->UpdateImageInformation(region);
    // Save the ImageBounds to satisfy later calls.
    // We do not have the method GetAbsoluteBounds(int *), so ...
    region->SetAxes4d(0, 1, 2, 3);
    region->GetImageBounds4d(this->ImageBounds);
    this->ImageBoundsTime.Modified();

    // Leave the region in the original (before this method) coordinate system.
    region->SetAxes4d(saveAxes);
    
    return;
    }
  
  // No modifications have been made, so return our own copy.
  vtkDebugMacro(<< "UpdateImageInformation: Using own copy of ImageBounds");
  // We do not have a method SetAbsoluteBounds(int *), so ...
  region->SetAxes4d(0, 1, 2, 3);
  region->SetImageBounds4d(this->ImageBounds);

  // Leave the region in the original (before this method) coordinate system.
  region->SetAxes4d(saveAxes);
}


//----------------------------------------------------------------------------
// Description:
// This Method sets the value of "ReleaseDataFlag" which turns cache on or off.
// When cache is off, memory is freed after a generate has been completed.
void vtkImageCache::SetReleaseDataFlag(int value)
{
  this->ReleaseDataFlag = value;

  if ( value == 1)
    {
    // Tell the subclass to delete data it has cached.
    this->ReleaseData();
    }
}


//----------------------------------------------------------------------------
// Description:
// This Method handles external calls to generate data.
// It Allocates and fills the passed region.
// If the method cannot complete, the region is not allocated and
// SplitFactor is set as a hint for splitting the region so the next call
// will suceed.  "region" should not have data when this method is called.
void vtkImageCache::UpdateRegion(vtkImageRegion *region)
{
  long memory;
  int saveAxes[VTK_IMAGE_DIMENSIONS];
  int saveBounds[VTK_IMAGE_BOUNDS_DIMENSIONS];

  vtkDebugMacro(<< "UpdateRegion: ");

  // Allow the source to modify the bounds of the region
  region->GetBounds4d(saveBounds);
  this->Source->InterceptCacheUpdate(region);
  
  // Translate region into the sources coordinate system. (save old)
  region->GetAxes4d(saveAxes);
  region->SetAxes4d(this->Source->GetAxes());
  
  // Check if bounds exceeds memory limit
  memory = region->GetVolume();
  if ( memory > this->MemoryLimit)
    {
    this->SplitFactor = (memory / this->MemoryLimit) + 1;
    vtkDebugMacro(<< "UpdateRegion: Reuest too large, SplitFactor= "
                  << this->SplitFactor);
    region->SetAxes4d(saveAxes);
    return;
    }
  
  // Set the correct DataType for the region.
  region->SetDataType(this->DataType);
  
  // If the region bounds has no "volume", allocate and return.
  if (memory <= 0)
    {
    this->AllocateRegion(region);
    region->SetAxes4d(saveAxes);
    return;
    }
  
  // Must have a source to generate the data
  if ( ! this->Source)
    {
    vtkErrorMacro(<< "UpdateRegion: Can not generate data with no Source");
    // Tell the consumer that spliting the region will not help.
    this->SplitFactor = 0;
    region->SetAxes4d(saveAxes);
    return;
    }

  if (this->ReleaseDataFlag)
    {
    // Since SaveData is off, Data must be NULL.  Source should Generate.
    this->GenerateUnCachedRegionData(region);
    }
  else
    {
    // look to cached data for generate (subclass will handle the generate)
    this->GenerateCachedRegionData(region);
    }

  // Leave the region in the original (before this method) coordinate system.
  region->SetAxes4d(saveAxes);
  region->SetBounds4d(saveBounds);
}



//----------------------------------------------------------------------------
// Description:
// This method uses the source to generate a whole region.  
// It is called by UpdateRegion when ReleaseDataFlag is on, or
// by the subclass GenerateUnCachedRegionData method when the region data
// is not in cache.  The method returns the region (or NULL if a something 
// failed).  The subclass method which calls this function is responsible for
// getting the data from the region and saving it if it wants to.
// outBBox is not modified or deleted.
void vtkImageCache::GenerateUnCachedRegionData(vtkImageRegion *region)
{
  vtkDebugMacro(<< "GenerateUnCachedRegionData: ");
  
  // Create the data object for this region, but delay allocating the
  // memory for as long as possible.
  this->Data = new vtkImageData;
  this->Data->SetBounds(region->GetAbsoluteBounds());
  this->Data->SetType(this->DataType);

  // Tell the filter to generate the data for this region
  this->Source->UpdateRegion(region);

  // this->Data should be allocated by now. 
  // (unless somthing unexpected happened).
  if ( ! this->Data->IsAllocated())
    {
    vtkWarningMacro(<< "GenerateUnCachedRegionData: Data should be allocated, "
                    << "but is not!");
    // Split Factor should have been set by source or GetRegion method
    this->Data->Delete();
    this->Data = NULL;
    return;
    }

  // Prepare region to be returned
  region->SetData(this->Data);

  // Delete (unregister) the data. (region has pointer/register by now)
  this->Data->UnRegister(this);
  this->Data = NULL;
}




//----------------------------------------------------------------------------
// Description:
// The caches source calls this method to allocate the region to fill in.
// The data may or may not be allocated before the method is called,
// but must be allocated before the method returns.  The region
// may or may not be the same region passed to GenerateRegion method.
void vtkImageCache::AllocateRegion(vtkImageRegion *region)
{
  // Allocate memory at the last possible moment
  if ( ! this->Data)
    {
    this->Data = new vtkImageData;
    this->Data->SetBounds(region->GetAbsoluteBounds());
    this->Data->SetType(this->DataType);
    }

  if ( ! this->Data->IsAllocated())
    if ( ! this->Data->Allocate())
      {
      // Output data could not be allocated.  Splitting will help.
      // Memory failure should not happen. MemoryLimits set wrong.
      vtkWarningMacro(<< "AllocateRegion: Failure. MemoryLimits too big.");
      this->SplitFactor = 2;
      return;
      }

  // Set up the region for the source
  region->SetData(this->Data);
}
  










