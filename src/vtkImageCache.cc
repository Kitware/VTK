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
// default it saves its data between requests.  But if the cache is created
// automatically by the vtkImageCachedSource, it does not.
vtkImageCache::vtkImageCache()
{
  this->Source = NULL;
  this->Data = NULL;
  this->Region = NULL;
  // default is to save data,
  // (But caches automatically created by sources set ReleaseDataFlag to 1)
  this->ReleaseDataFlag = 0;
  this->RequestMemoryLimit = 25000000;  // 5000 x 5000 image
}


//----------------------------------------------------------------------------
vtkImageCache::~vtkImageCache()
{
  if (this->Region)
    {
    this->Region->Delete();
    this->Region = NULL;
    }
  if (this->Data)
    {
    this->Data->Delete();
    this->Data = NULL;
    }
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
// This method returns the boundary of the largest region that can be
// requested.  It simply passes the message to its source.
void vtkImageCache::GetBoundary(int *offset, int *size)
{
  int idx;
  
  if (this->BoundaryTime.GetMTime() < this->GetPipelineMTime())
    {
    // pipeline has been modified, we have to get the boundary again.
    vtkDebugMacro(<< "GetBoundary: Pipeline modified, recompute boundary");
    if ( ! this->Source)
      {
      vtkErrorMacro(<< "GetBoundary: No source");
      return;
      }
    this->Source->GetBoundary(offset, size);
    
    // Save the boundary
    for (idx = 0; idx < 3; ++idx)
      {
      this->BoundaryOffset[idx] = offset[idx];
      this->BoundarySize[idx] = size[idx];      
      }
    this->BoundaryTime.Modified();

    return;
    }
  
  // No modifications have been made, so return our own copy.
  vtkDebugMacro(<< "GetBoundary: Using own copy of boundary");
  for (idx = 0; idx < 3; ++idx)
    {
    offset[idx] = this->BoundaryOffset[idx];
    size[idx] = this->BoundarySize[idx];
    }
}

//----------------------------------------------------------------------------
// Description:
// This Method sets the value of "ReleaseDataFlag" which turns cache on or off.
// When cache is off, memory is freed after a request has been satisfied.
void vtkImageCache::SetReleaseDataFlag(int value)
{
  if ( value == 1 && this->Data)
    {
    this->Data->Delete();
    this->Data = NULL;
    }

  if ((value && ! this->ReleaseDataFlag) || ( ! value && this->ReleaseDataFlag))
    {
    // value has changed
    this->Modified();
    this->ReleaseDataFlag = value;
    }
}


//----------------------------------------------------------------------------
// Description:
// This Method handles external requests for data.
// It returns a region contianing the region requested,
// or NULL if the memory could not be allocated (SplitFactor is set).
vtkImageRegion *vtkImageCache::RequestRegion(int offset[3], int size[3])
{
  long requestMemory;

  
  vtkDebugMacro(<< "RequestRegion: offset = (" 
		<< offset[0] << ", " << offset[1] << ", " << offset[2] 
		<< "), size = (" 
		<< size[0] << ", " << size[1] << ", " << size[2] << ")");

  // Check if request exceeds memory limit
  requestMemory = size[0] * size[1] * size[2];
  if ( requestMemory > this->RequestMemoryLimit)
    {
    this->SplitFactor = (requestMemory / this->RequestMemoryLimit) + 1;
    vtkDebugMacro(<< "RequestRegion: Reuest too large, SplitFactor= "
                  << this->SplitFactor);
    return NULL;
    }

  // Must have a source to generate the data
  if ( ! this->Source)
    {
    vtkErrorMacro(<< "RequestRegion: Can not GenerateData with no Source");
    // Tell the requestor that spliting the request will not help.
    this->SplitFactor = 0;
    return NULL;
    }

  // Pass request to subclass method to satisfy
  if (this->ReleaseDataFlag)
    {
    // Since SaveData is off, Data must be NULL.  Generate request.
    return this->RequestUnCachedRegion(offset, size);
    }
  else
    {
    // look to cached data to fill request
    return this->RequestCachedRegion(offset, size);
    }
}



//----------------------------------------------------------------------------
// Description:
// This method uses the source to generate a whole region.  
// It is called by RequestRegion when ReleaseDataFlag is on, or
// the Requested region is not in cache.  The method returns
// the region (or NULL if a something failed).  If "Data" is set the
// method first frees it.  "Data" is set to NULL before method returns.
// The subclass method which calls this function is resposible for
// getting the data from the tile and saving it if it want to.
vtkImageRegion *vtkImageCache::RequestUnCachedRegion(int offset[3],int size[3])
{
  vtkImageRegion *region;


  vtkDebugMacro(<< "RequestUnCachedRegion: offset = ..., size = ...");
  
  // Get rid of old data (Just in case)
  if (this->Data)
    {
    this->Data->Delete();
    this->Data = NULL;
    }
  
  // Create the data object for this request, but delay allocating the
  // memory for as long as possible.
  this->Data = new vtkImageData;
  this->Data->SetOffset(offset);
  this->Data->SetSize(size);

  // Create a region (data container) to satisfy GetRegion calls from the 
  // source. Saving this reduces creation and destruction of objects, and
  // the filter does not have to delete the region it obtained with GetRegion..
  this->Region = new vtkImageRegion;

  // Tell the filter to generate the data for this region
  this->Source->GenerateRegion(offset, size);

  // this->Data should be allocated by now.
  if ( ! this->Data->Allocated())
    {
    vtkWarningMacro(<< "RequestUnCachedRegion: Data should be allocated, "
                    << "but is not!");
    // why did the source fail?  Will splitting help or not?
    // Split Factor should be set by source or GetRegion method
    return NULL;
    }

  // Prepare region to be returned
  region = this->Region;
  this->Region = NULL;
  region->SetSize(size);
  region->SetOffset(offset);
  region->SetData(this->Data);

  // Delete (unregister) the data. (region has pointer/register by now)
  this->Data->UnRegister(this);
  this->Data = NULL;

  return region;
}



//----------------------------------------------------------------------------
// Description:
// This pure virtual method is used by a subclass to first look to cached 
// data to fill requests.  It can also return null if the request 
// fails for any reason.
vtkImageRegion *vtkImageCache::RequestCachedRegion(int offset[3], int size[3])
{
  // Avoid compiler warning messages
  offset = offset;
  size = size;
  vtkErrorMacro(<< "RequestCachedRegion method has not been defined.");
  // tell the requestor that splitting the request will not help.
  this->SplitFactor = 0;  

  return NULL;
}



//----------------------------------------------------------------------------
// Description:
// The caches source calls this method to obtain a region to fill in.
// The data may or may not be allocated before the method is called,
// but must be allocated before the method returns.
vtkImageRegion *vtkImageCache::GetRegion(int offset[3], int size[3])
{
  // Allocate memory at the last possible moment
  if ( ! this->Data->Allocated())
    if ( ! this->Data->Allocate())
      {
      // Output data could not be allocated.  Splitting will help.
      this->SplitFactor = 2;
      return NULL;
      }

  // Set up the region for the source
  this->Region->SetData(this->Data);
  this->Region->SetOffset(offset);
  this->Region->SetSize(size);

  return this->Region;
}
  










