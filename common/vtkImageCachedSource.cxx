/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCachedSource.cxx
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
#include "vtkImageCachedSource.h"
#include "vtkImageSimpleCache.h"

//----------------------------------------------------------------------------
vtkImageCachedSource::vtkImageCachedSource()
{
  this->Output = NULL;
  this->SetAxes(VTK_IMAGE_X_AXIS,
		VTK_IMAGE_Y_AXIS,
		VTK_IMAGE_Z_AXIS,
		VTK_IMAGE_TIME_AXIS,
		VTK_IMAGE_COMPONENT_AXIS);
}


//----------------------------------------------------------------------------
// Description:
// Destructor: Delete the cache as well. (should caches by reference counted?)
vtkImageCachedSource::~vtkImageCachedSource()
{
  if (this->Output)
    {
    this->Output->Delete();
    this->Output = NULL;
    }
}


//----------------------------------------------------------------------------
void vtkImageCachedSource::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  vtkObject::PrintSelf(os,indent);

  os << indent << "NumberOfAxes: " << this->NumberOfAxes << "\n";
  os << indent << "Axes: (" << vtkImageAxisNameMacro(this->Axes[0]);
  for (idx = 1; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    os << ", " << vtkImageAxisNameMacro(this->Axes[idx]);
    }
  os << ")\n";

  if (this->Output)
    {
    os << indent << "Cache:\n";
    this->Output->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "Cache: NULL \n";
    }
}
  


//----------------------------------------------------------------------------
// Description:
// This method can be used to intercept a generate call made to a cache.
// It allows a source to generate a larger region than was originally 
// specified.  The default method does not alter the specified region extent.
void vtkImageCachedSource::InterceptCacheUpdate(vtkImageRegion *region)
{
  region = region;
}


//----------------------------------------------------------------------------
// Description:
// This method loops over an axis and calls UpdatePointData
// with a lower dimensional region.  The recursion is terminated when 
// axesIdx is equal to NumberOfAxes.  At this point the subclasses
// UpdatePointData is called.
void vtkImageCachedSource::UpdatePointData(int axisIdx, vtkImageRegion *region)
{
  int coordinate;
  int extent[2*axisIdx];
  int min, max;

  
  // Terminate recursion?
  if (axisIdx == this->NumberOfAxes)
    {
    this->UpdatePointData(region);
    return;
    }
  
  region->GetExtent(extent, axisIdx);
  min = extent[axisIdx*2 - 2];
  max = extent[axisIdx*2 - 1];
  
  for (coordinate = min; coordinate <= max; ++coordinate)
    {
    // colapse one dimension.
    extent[axisIdx*2 - 2] = extent[axisIdx*2 - 1] = coordinate;
    region->SetExtent(extent, axisIdx);
    // Continue recursion.
    this->vtkImageCachedSource::UpdatePointData(axisIdx - 1, region);
    }
  // restore original extent
  extent[axisIdx*2 - 2] = min;
  extent[axisIdx*2 - 1] = max;
  region->SetExtent(extent, axisIdx);  
}

//----------------------------------------------------------------------------
// Description:
// This function can be defined in a subclass to generate the data
// for a region.
void vtkImageCachedSource::UpdatePointData(vtkImageRegion *region)
{
  region = region;
  vtkErrorMacro(<< "UpdatePointData: Method not defined.");
}



//----------------------------------------------------------------------------
// Description:
// Returns the cache object of the source.  If one does not exist, a default
// is created.
vtkImageCache *vtkImageCachedSource::GetCache()
{
  this->CheckCache();
  
  return this->Output;
}



//----------------------------------------------------------------------------
// Description:
// Returns an object which will generate data for Regions.
vtkImageSource *vtkImageCachedSource::GetOutput()
{
  return this->GetCache();
}




//----------------------------------------------------------------------------
// Description:
// Returns the maximum mtime of this source and every object which effects
// this sources output. 
unsigned long vtkImageCachedSource::GetPipelineMTime()
{
  unsigned long time1, time2;
  
  // Get this objects modified time
  time1 = this->GetMTime();
  // get the caches modified time (just in case cache did not originate call)
  this->CheckCache();
  time2 = this->Output->GetMTime();
  
  // Get the maximum of the two times
  time1 = (time1 > time2) ? time1 : time2;
  
  return time1;
}





//----------------------------------------------------------------------------
// Description:
// Limits the size of tile which can be returned. 
// The messaged is forwarded to the sources cache.
// If the source does not have a cache, a default cache is created.
void vtkImageCachedSource::SetOutputMemoryLimit(long limit)
{
  this->CheckCache();
  this->Output->SetOutputMemoryLimit(limit);
  this->Modified();
}



//----------------------------------------------------------------------------
// Description:
// Use this method to specify a cache object for the filter.  
// If a cache is not explicitly set, a default cache will be created.
// Cache objects can not be changed (yet), so this method must be called 
// before any connections are made.
void vtkImageCachedSource::SetCache(vtkImageCache *cache)
{
  if (this->Output)
    {
    // could this be handled more elegantly?
    vtkErrorMacro(<< "SetCache: A cache already exists for this source");
    return;
    }

  this->Output = cache;
  cache->SetSource(this);
  this->Modified();

  if (this->GetDebug())
    cache->DebugOn();
}





//----------------------------------------------------------------------------
void vtkImageCachedSource::SetAxes(int *axes, int dim)
{
  int allAxes[VTK_IMAGE_DIMENSIONS];
  int idx1, idx2;

  // Copy axes
  for (idx1 = 0; idx1 < dim; ++idx1)
    {
    allAxes[idx1] = axes[idx1];
    }
  this->NumberOfAxes = dim;
  
  // choose the rest of the axes
  // look through original axes to find ones not taken
  for (idx1 = 0; 
       idx1 < VTK_IMAGE_DIMENSIONS && dim < VTK_IMAGE_DIMENSIONS; 
       ++idx1)
    {
    for (idx2 = 0; idx2 < dim; ++idx2)
      {
      if (allAxes[idx2] == this->Axes[idx1])
	{
	break;
	}
      }
    if (idx2 == dim)
      {
      allAxes[dim] = this->Axes[idx1];
      ++dim;
      }
    }
  
  // Sanity check
  if (dim != VTK_IMAGE_DIMENSIONS)
    {
    vtkErrorMacro(<< "SetAxes: Could not complete unspecified axes.");
    return;
    }
  
  // Actuall set the axes.
  for (idx1 = 0; idx1 < VTK_IMAGE_DIMENSIONS; ++idx1)
    {
    this->Axes[idx1] = allAxes[idx1];
    }
  this->Modified();
}




//----------------------------------------------------------------------------
void vtkImageCachedSource::GetAxes(int *axes, int dim)
{
  int idx;

  // Copy axes
  for (idx = 0; idx < dim; ++idx)
    {
    axes[idx] = this->Axes[idx];
    }
}




//----------------------------------------------------------------------------
// Description:
// This method turns debugging on for both the source and its cache.
// This may not be the best setup.  Debug statements are begining to get
// a little bit too numerous.
void vtkImageCachedSource::DebugOn()
{
  this->vtkObject::DebugOn();
  if ( this->Output)
    this->Output->DebugOn();
}




//----------------------------------------------------------------------------
// Description:
// This method turns debugging off for both the source and its cache.
void vtkImageCachedSource::DebugOff()
{
  this->vtkObject::DebugOff();
  if ( this->Output)
    this->Output->DebugOff();
}




//----------------------------------------------------------------------------
// Description:
// This method sets the value of the caches ReleaseDataFlag.  When this flag
// is set, the cache releases its data after every generate.  When a default
// cache is created, this flag is automatically set.
void vtkImageCachedSource::SetReleaseDataFlag(int value)
{
  this->CheckCache();
  this->Output->SetReleaseDataFlag(value);
}



//----------------------------------------------------------------------------
// Description:
// This method gets the value of the caches ReleaseDataFlag.
int vtkImageCachedSource::GetReleaseDataFlag()
{
  this->CheckCache();
  return this->Output->GetReleaseDataFlag();
}





//----------------------------------------------------------------------------
// Description:
// This method sets the value of the caches DataType.
void vtkImageCachedSource::SetOutputDataType(int value)
{
  this->CheckCache();
  this->Output->SetDataType(value);
}



//----------------------------------------------------------------------------
// Description:
// This method returns the caches DataType.
int vtkImageCachedSource::GetOutputDataType()
{
  this->CheckCache();
  return this->Output->GetDataType();
}





//----------------------------------------------------------------------------
// Description:
// This private method creates a cache if one has not been set.
// ReleaseDataFlag is turned on.
void vtkImageCachedSource::CheckCache()
{
  // create a default cache if one has not been set
  if ( ! this->Output)
    {
    this->Output = new vtkImageSimpleCache;
    this->Output->ReleaseDataFlagOn();
    this->Output->SetSource(this);
    if (this->GetDebug())
      this->Output->DebugOn();
    this->Modified();
    }
}













