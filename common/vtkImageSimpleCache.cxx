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
    os << indent << "CachedData: None";
    }
  else
    {
    os << indent << "CachedData: \n";
    this->CachedData->PrintSelf(os, indent.GetNextIndent());
    }
}

//----------------------------------------------------------------------------
// Description:
// This Method allocates a region and generates the data.
void vtkImageSimpleCache::GenerateCachedRegionData(vtkImageRegion *region)
{
  // Check Data to see if the region already exists
  if (this->CachedData)
    {
    int saveAxes[VTK_IMAGE_DIMENSIONS];
    int *cacheExtent, regionExtent[VTK_IMAGE_EXTENT_DIMENSIONS];
    cacheExtent = this->CachedData->GetExtent();

    region->GetAxes(saveAxes);
    region->SetAxes(this->CachedData->GetAxes());
    region->GetExtent(regionExtent);
    region->SetAxes(saveAxes);
    // Is the new region contained in the cache?
    if (regionExtent[0] >= cacheExtent[0] &&
	regionExtent[1] <= cacheExtent[1] &&
	regionExtent[2] >= cacheExtent[2] &&
	regionExtent[3] <= cacheExtent[3] &&
	regionExtent[4] >= cacheExtent[4] &&
	regionExtent[5] <= cacheExtent[5] &&
	regionExtent[6] >= cacheExtent[6] &&
	regionExtent[7] <= cacheExtent[7] &&
	regionExtent[8] >= cacheExtent[8] &&
	regionExtent[9] <= cacheExtent[9])
      {
      // check the gtime of cache to see if it is more recent than mtime */
      if (this->GenerateTime.GetMTime() >= this->GetPipelineMTime())
	{
	/* use the cache data (automatically registered by region) */
	vtkDebugMacro(<< "GenerateCachedRegionData: " 
	              << "Using cache to fill region.");
	region->SetScalarType(this->ScalarType);
	region->SetData(this->CachedData);
	return;
	}
      }
    }
  
  // Region not entirely in cache
  // Get rid of the old cached data
  if (this->CachedData)
    {
    this->CachedData->Delete();
    this->CachedData = NULL;
    }
  
  // Generate a new region
  this->GenerateUnCachedRegionData(region);
  
  // Now the cached data should be OK.
  region->SetData(this->CachedData);
}


//----------------------------------------------------------------------------
// Description:
// This Method saves a region in cache.
void vtkImageSimpleCache::CacheRegion(vtkImageRegion *region)
{
  // Save the data in cache
  this->CachedData = region->GetData();
  this->CachedData->Register(this);
  // Mark when this data was generated
  this->GenerateTime.Modified();
}


//----------------------------------------------------------------------------
// Description:
// This Method deletes any data in cache.
void vtkImageSimpleCache::ReleaseData()
{
  if (this->CachedData)
    {
    this->CachedData->Delete();
    this->CachedData = NULL;
    }
}
