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
// Description:
// Construct an instance of vtkImageSink fitler.
vtkImageCachedSource::vtkImageCachedSource()
{
  this->Cache = NULL;
}


//----------------------------------------------------------------------------
// Description:
// Destructor: Delete the cache as well. (should caches by reference counted?}
vtkImageCachedSource::~vtkImageCachedSource()
{
  if (this->Cache)
    {
    this->Cache->Delete();
    this->Cache = NULL;
    }
}


//----------------------------------------------------------------------------
// Description:
// Since this is a CachedSource this method should not be called 
// (for consistencey).  All requests for regions should be made through
// this sources cache object (use the GetOutput method)
vtkImageRegion *vtkImageCachedSource::RequestRegion(int offset[3], int size[3])
{
  vtkWarningMacro(<< "RequestRegion: This is a CachedSource. "
                  << "All requests should be made through the cache.");
  // make sure a cache exists
  this->CheckCache();
  // forward the message to the cache
  return this->Cache->RequestRegion(offset, size);
}

  


//----------------------------------------------------------------------------
// Description:
// Returns the cache object of the source.  
// All requests for data are made through this object.
vtkImageCache *vtkImageCachedSource::GetCache()
{
  this->CheckCache();
  
  return this->Cache;
}



//----------------------------------------------------------------------------
// Description:
// Returns an object which will satisfy requests for Regions.
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
  time2 = this->Cache->GetMTime();
  
  // Get the maximum of the two times
  time1 = (time1 > time2) ? time1 : time2;
  
  return time1;
}





//----------------------------------------------------------------------------
// Description:
// Limits the size of tile which can be returned. 
// The messaged is forwarded to the sources cache.
// If the source does not have a cache, a default cache is created.
void vtkImageCachedSource::SetRequestMemoryLimit(long limit)
{
  this->CheckCache();

  this->Cache->SetRequestMemoryLimit(limit);
}



//----------------------------------------------------------------------------
// Description:
// Use this method to specify a cache object for the filter.  
// If a cache is not explicitly set, a default cache will be created.
// Cache objects can not be changed, so this method must be called before
// any connections are made.
void vtkImageCachedSource::SetCache(vtkImageCache *cache)
{
  if (this->Cache)
    {
    // could this be handled more elegantly?
    vtkErrorMacro(<< "SetCache: A cache already exists for this source");
    return;
    }

  this->Cache = cache;
  cache->SetSource(this);
  this->Modified();

  if (this->GetDebug())
    cache->DebugOn();
}




//----------------------------------------------------------------------------
// Description:
// This method should get the output tile from the cache and fill the
// data in the region of interest (offset, size).
void vtkImageCachedSource::GenerateRegion(int outOffset[3], int outSize[3])
{
  vtkDebugMacro(<< "GenerateRegion: offset = ("
                << outOffset[0] << ", " << outOffset[1] << ", " << outOffset[2]
                << "), size = ("
                << outSize[0] << ", " << outSize[1] << ", " << outSize[2]
                << ")");

  vtkErrorMacro(<< "GenerateRegion has not been defined for this cached source");
}




//----------------------------------------------------------------------------
// Description:
// This method turns debugging on for both the source and its cache.
void vtkImageCachedSource::DebugOn()
{
  this->vtkObject::DebugOn();
  if ( this->Cache)
    this->Cache->DebugOn();
}




//----------------------------------------------------------------------------
// Description:
// This method turns debugging off for both the source and its cache.
void vtkImageCachedSource::DebugOff()
{
  this->vtkObject::DebugOff();
  if ( this->Cache)
    this->Cache->DebugOff();
}




//----------------------------------------------------------------------------
// Description:
// This method sets the value of the caches ReleaseDataFlag
void vtkImageCachedSource::SetReleaseDataFlag(int value)
{
  this->CheckCache();
  this->Cache->SetReleaseDataFlag(value);
}





//----------------------------------------------------------------------------
// Description:
// This method creates a cache if one has not been set.
// ReleaseDataFlag is turned on.
void vtkImageCachedSource::CheckCache()
{
  // create a default cache if one has not been set
  if ( ! this->Cache)
    {
    this->Cache = new vtkImageSimpleCache;
    this->Cache->ReleaseDataFlagOn();
    this->Cache->SetSource(this);
    if (this->GetDebug())
      this->Cache->DebugOn();
    this->Modified();
    }
}













