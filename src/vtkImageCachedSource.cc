/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCachedSource.cc
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
#include "vtkImageCachedSource.hh"
#include "vtkImageSimpleCache.hh"

//----------------------------------------------------------------------------
vtkImageCachedSource::vtkImageCachedSource()
{
  this->Output = NULL;
  this->SetAxes5d(VTK_IMAGE_X_AXIS,
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
  vtkObject::PrintSelf(os,indent);
}
  


//----------------------------------------------------------------------------
// Description:
// This method can be used to intercept a generate call made to a cache.
// It allows a source to generate a larger region than was originally 
// specified.  The default method does not alter the specified region bounds.
void vtkImageCachedSource::InterceptCacheUpdate(vtkImageRegion *region)
{
  region = region;
}



//----------------------------------------------------------------------------
// Description:
// This method fills an empty region with data.
void vtkImageCachedSource::UpdateRegion(vtkImageRegion *region)
{
  // switch statment is to determine the overhead of this series of 
  // Update calls.
  // It does nothing and is not tested.
  switch (VTK_IMAGE_DIMENSIONS)
    {
    case 5:
      this->UpdateRegion5d(region);
      break;
    case 4:
      this->UpdateRegion4d(region);
      break;
    case 3:
      this->UpdateRegion3d(region);
      break;
    case 2:
      this->UpdateRegion2d(region);
      break;
    case 1:
      this->UpdateRegion1d(region);
      break;
    }
}

  
//----------------------------------------------------------------------------
// Description:
// This default GenerateRegion5d method treats the 5d image as a 
// set of 4d images.  It loops over these 4d images and calls GenerateRegion4d.
void vtkImageCachedSource::UpdateRegion5d(vtkImageRegion *region)
{
  int coordinate4;
  int *bounds;
  int min4, max4;
  
  bounds = region->GetBounds5d();
  min4 = bounds[8];
  max4 = bounds[9];
  
  for (coordinate4 = min4; coordinate4 <= max4; ++coordinate4)
    {
    region->SetDefaultCoordinate4(coordinate4);
    this->UpdateRegion4d(region);
    }
}


//----------------------------------------------------------------------------
// Description:
// This default GenerateRegion4d method treats the 4d image as a 
// set of volumes.  It loops over these volumes and calls GenerateRegion3d.
void vtkImageCachedSource::UpdateRegion4d(vtkImageRegion *region)
{
  int coordinate3;
  int *bounds;
  int min3, max3;
  
  bounds = region->GetBounds4d();
  min3 = bounds[6];
  max3 = bounds[7];
  
  for (coordinate3 = min3; coordinate3 <= max3; ++coordinate3)
    {
    region->SetDefaultCoordinate3(coordinate3);
    this->UpdateRegion3d(region);
    }
}


//----------------------------------------------------------------------------
// Description:
// This default UpdateRegion3d method treats the volume as a set of 
// images.  It loops over these images and calls UpdateRegion2d.
void vtkImageCachedSource::UpdateRegion3d(vtkImageRegion *region)
{
  int coordinate2;
  int *bounds;
  int min2, max2;
  
  bounds = region->GetBounds3d();
  min2 = bounds[4];
  max2 = bounds[5];
  
  for (coordinate2 = min2; coordinate2 <= max2; ++coordinate2)
    {
    region->SetDefaultCoordinate2(coordinate2);
    this->UpdateRegion2d(region);
    }
}


//----------------------------------------------------------------------------
// Description:
// This default UpdateRegion2d method treats the image as a set of lines.
// It loops over these lines and calls UpdateRegion1d.
void vtkImageCachedSource::UpdateRegion2d(vtkImageRegion *region)
{
  int coordinate1;
  int *bounds;
  int min1, max1;
  
  bounds = region->GetBounds2d();
  min1 = bounds[2];
  max1 = bounds[3];
  
  for (coordinate1 = min1; coordinate1 <= max1; ++coordinate1)
    {
    region->SetDefaultCoordinate1(coordinate1);
    this->UpdateRegion1d(region);
    }
}


//----------------------------------------------------------------------------
// Description:
// There is no default UpdateRegion1d.  This method just prints an error.
void vtkImageCachedSource::UpdateRegion1d(vtkImageRegion *region)
{
  region = region;
  vtkErrorMacro(<< "UpdateRegion1d: "
                << "Subclass did not provide a UpdateRegion method.");
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
void vtkImageCachedSource::SetMemoryLimit(long limit)
{
  this->CheckCache();
  this->Output->SetMemoryLimit(limit);
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
// Description:
// This method is used when the source is treating the data as 1d lines.
// It chooses the parallel axis.
void vtkImageCachedSource::SetAxes1d(int axis0)
{
  int axis1;
  
  // Choose the next axis (2d) as the smallest axis.
  // This is the most efficeint if the data is ordered this way.
  // Maybe the cache or input should be queried for its data order.
  axis1 = 0;
  while (axis1 == axis0)
    {
    ++axis1;
    }
  
  this->SetAxes2d(axis0, axis1);
}

//----------------------------------------------------------------------------
// Description:
// This method is used when the source is treating the data as 2d images.
// It chooses the parallel plane axes.
void vtkImageCachedSource::SetAxes2d(int axis0, int axis1)
{
  int axis2;
  
  // Choose the next axis (3d) as the smallest axis.
  // This is the most efficeint if the data is ordered this way.
  // Maybe the cache or input should be queried for its data order.
  axis2 = 0;
  while (axis2 == axis0 || axis2 == axis1)
    {
    ++axis2;
    }
  
  this->SetAxes3d(axis0, axis1, axis2);
}

//----------------------------------------------------------------------------
// Description:
// This method is used when the source is treating the data as 3d volumes.
// It chooses the basis of the volume.
void vtkImageCachedSource::SetAxes3d(int axis0, int axis1, int axis2)
{
  int axis3;
  
  // Choose the next axis (4d) as the smallest axis (only remaining axis).
  axis3 = 0;
  while (axis3 == axis0 || axis3 == axis1 || axis3 == axis2)
    {
    ++axis3;
    }
  
  this->SetAxes4d(axis0, axis1, axis2, axis3);
}

//----------------------------------------------------------------------------
// Description:
// This method is used when the source is treating the data as 4d images.
// It chooses the basis of the 4d space.
void vtkImageCachedSource::SetAxes4d(int axis0,int axis1,int axis2,int axis3)
{
  int axis4;
  
  // Choose the next axis (4d) as the smallest axis (only remaining axis).
  axis4 = 0;
  while (axis4 == axis0 || axis4 == axis1 || axis4 == axis2 || axis4 == axis3)
    {
    ++axis4;
    }
  
  this->SetAxes5d(axis0, axis1, axis2, axis3, axis4);
}

//----------------------------------------------------------------------------
// Description:
// This method is used when the source is treating the data as 5d "image".
// It chooses the axes that form the basis of the space.
void vtkImageCachedSource::SetAxes5d(int axis0, int axis1, int axis2, 
				     int axis3, int axis4)
{
  int axes[VTK_IMAGE_DIMENSIONS];
  
  axes[0] = axis0;
  axes[1] = axis1;
  axes[2] = axis2;
  axes[3] = axis3;
  axes[4] = axis4;
  
  this->SetAxes(axes);
}

//----------------------------------------------------------------------------
// Description:
// This method always gets called when any of the SetAxes methods are invoked.
// It is the general SetAxes method
void vtkImageCachedSource::SetAxes(int *axes)
{
  int idx;
  
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->Axes[idx] = axes[idx];
    }
  this->Modified();
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













