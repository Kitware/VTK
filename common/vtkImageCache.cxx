/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCache.cxx
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
#include "vtkImageCache.h"

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
  
  // Invalid data type
  // This will be changed when the filter gets an input or
  // the ScalarType is set explicitly
  this->ScalarType = VTK_VOID;
  
  // default is to save data,
  // (But caches automatically created by sources set ReleaseDataFlag to 1)
  this->ReleaseDataFlag = 0;
}


//----------------------------------------------------------------------------
vtkImageCache::~vtkImageCache()
{
}

//----------------------------------------------------------------------------
void vtkImageCache::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageSource::PrintSelf(os,indent);

  os << indent << "Source: (" << this->Source << ").\n";
  os << indent << "ReleaseDataFlag: " << this->ReleaseDataFlag << "\n";
  os << indent << "ScalarType: "<<vtkImageScalarTypeNameMacro(this->ScalarType)
     << "\n";
}
  
    



//----------------------------------------------------------------------------
// Description:
// This Method returns the MTime of the pipeline before this cache.
// It considers both the source and the cache.
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
  region->GetAxes(saveAxes);

  if (this->ImageInformationTime.GetMTime() < this->GetPipelineMTime())
    {
    // pipeline has been modified, we have to get the ImageInformation again.
    vtkDebugMacro(<< "UpdateImageInformation: Pipeline modified, "
                  << "recompute ImageInformation");
    if ( ! this->Source)
      {
      vtkErrorMacro(<< "UpdateImageInformation: No source");
      return;
      }

    // Translate region into the sources coordinate system. (save old)
    region->SetAxes(this->Source->GetAxes());
    // Get the ImageExtent
    this->Source->UpdateImageInformation(region);
    // Save the ImageExtent to satisfy later calls.
    // Choose some constant coordinate system.
    region->SetAxes(1, 2, 3, 4, 0);
    region->GetImageExtent(this->ImageExtent);
    region->GetAspectRatio(this->AspectRatio);
    this->ImageInformationTime.Modified();

    // Leave the region in the original (before this method) coordinate system.
    region->SetAxes(saveAxes);
    
    return;
    }
  
  // No modifications have been made, so return our own copy.
  vtkDebugMacro(<< "UpdateImageInformation: Using own copy of ImageInfo");
  // Image extent Are saved in some constant coordinate system.
  region->SetAxes(1, 2, 3, 4, 0);
  region->SetImageExtent(this->ImageExtent);
  region->SetAspectRatio(this->AspectRatio);

  // Leave the region in the original (before this method) coordinate system.
  region->SetAxes(saveAxes);
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
// If the method cannot complete, the region is not allocated.
// "region" should not have data when this method is called.
void vtkImageCache::UpdateRegion(vtkImageRegion *region)
{
  long memory;
  int saveAxes[VTK_IMAGE_DIMENSIONS];
  int saveExtent[VTK_IMAGE_EXTENT_DIMENSIONS];

  // First Update the Image information 
  this->UpdateImageInformation(region);

  // Save the extent to restore later.
  region->GetExtent(VTK_IMAGE_DIMENSIONS, saveExtent);
  
  // Translate region into the sources coordinate system. (save old)
  region->GetAxes(VTK_IMAGE_DIMENSIONS, saveAxes);
  region->SetAxes(VTK_IMAGE_DIMENSIONS, this->Source->GetAxes());

  // Set default data type.
  if (region->GetScalarType() == VTK_VOID)
    {
    region->SetScalarType(this->GetScalarType());
    }
  else
    {
    // Do I really want this restriction?
    if (region->GetScalarType() != this->GetScalarType())
      {
      vtkErrorMacro(<< "UpdateRegion: ScalarType does not match.");
      }
    }
  
  // Allow the source to modify the extent of the region  
  this->Source->InterceptCacheUpdate(region);
  
  // Check if extent exceeds memory limit
  memory = region->GetMemorySize();
  // If the extent has no "volume", restore coordinate system and return.
  if (memory <= 0)
    {
    region->SetAxes(VTK_IMAGE_DIMENSIONS, saveAxes);
    return;
    }
  
  // Must have a source to generate the data
  if ( ! this->Source)
    {
    vtkErrorMacro(<< "UpdateRegion: Can not generate data with no Source");
    region->SetAxes(saveAxes);
    return;
    }

  if (this->ReleaseDataFlag)
    {
    // Since SaveData is off, Data must be NULL.  Source should Generate.
    this->GenerateUnCachedRegionData(region);
    }
  else
    {
    // look to cached data for update (subclass will handle the update)
    this->GenerateCachedRegionData(region);
    }

  // Release the cached data if needed.
  if (this->ReleaseDataFlag)
    {
    this->ReleaseData();
    }
  
  // Leave the region in the original (before this method) coordinate system.
  region->SetAxes(VTK_IMAGE_DIMENSIONS, saveAxes);
  region->SetExtent(VTK_IMAGE_DIMENSIONS, saveExtent);
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
  int saveAxes[VTK_IMAGE_DIMENSIONS];
  vtkImageData *data;

  vtkDebugMacro(<< "GenerateUnCachedRegionData: ");
  
  // Save the axes just in case it is important to restore coordinate system.
  // (Update region probably does not care but ...)
  region->GetAxes(saveAxes);

  // Create the data object for this region, but delay allocating the
  // memory for as long as possible.
  // Note: This step is simply to save the requested extent of the region.
  data = new vtkImageData;
  region->SetAxes(VTK_IMAGE_DIMENSIONS, data->GetAxes());
  data->SetExtent(VTK_IMAGE_DIMENSIONS, region->GetExtent());
  data->SetScalarType(this->ScalarType);
  region->SetAxes(saveAxes);
  region->SetData(data);
  // Since region has registered data, we mst delet our reference
  data->Delete();
  
  // Tell the filter to generate the data for this region
  // IMPORTANT: Region is just to communicate extent, and does not
  // return any infomation!
  this->Source->UpdatePointData(VTK_IMAGE_DIMENSIONS, region);
}












