/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagePanCache.cxx
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
#include "vtkImagePanCache.h"




//----------------------------------------------------------------------------
// Description:
// This Method tries to use the cached data to handle requests.  If there
// is any overlap of cache and the requested region, the data is copied.
// The method defaults to "SimpleCache" behavior if request is contained
// completely in cache.
vtkImageRegion *vtkImagePanCache::RequestCachedRegion(int offset[3], 
						      int size[3])
{
  vtkImageData *newData;
  vtkImageRegion *region;
  int cacheStatus;  // 1=>no overlap, 2=>All in cache, 3=>Part in cache
  int overlapOffset[3];
  int overlapSize[3];  

  
  vtkDebugMacro(<< "RequestCachedRegion: offset = ..., size = ...");
  
  cacheStatus = this->GetCacheStatus(offset, size, overlapOffset, overlapSize);
  if (cacheStatus == 2)
    {
    // use the data in cache with no copying or generating
    region = new vtkImageRegion;
    region->SetData(this->Data);
    region->SetOffset(offset);
    region->SetSize(size);
    
    return region;
    }

  if (cacheStatus == 1)
    {
    // Case where cache is completely invalid
    if (this->Data)
      {
      this->Data->Delete();
      this->Data = NULL;
      }
    region = this->RequestUnCachedRegion(offset, size);
    // Save the new data as cache
    this->Data = region->GetData();
    this->Data->Register(this);
    // record that the data was generated at this time.
    this->GenerateTime.Modified();
    
    return region;
    }
  
  // Cache is partially valid
  
  // Create the new data object for this request
  newData = new vtkImageData;
  newData->SetOffset(offset);
  newData->SetSize(size);
  newData->SetSizeOfType(sizeof(float));
  if ( ! newData->Allocate())
    {
    newData->Delete();
    this->SplitFactor = 2;
    vtkDebugMacro(<< "RequestRegion: Request too large, "
                  << "Allocate failed SplitFactor= "
                  << this->SplitFactor);
    return NULL;
    }
  
  // Create a region (data container) to satisfy GetRegion calls from the 
  // source.  Saving this reduces creation and destruction of objects, and
  // the filter does not have to delete the region it obtained with GetRegion..
  this->Region = new vtkImageRegion;

  // Copy overlapping region of cache into new data object
  this->CopyOverlap(newData, overlapOffset, overlapSize);
  
  // Get rid of old cached data 
  this->Data->Delete();
  this->Data = newData;

  // Generate the remaining pieces
  // note: The order of these calls might impact performance.
  this->HandlePieces(0, overlapOffset, overlapSize);
  this->HandlePieces(1, overlapOffset, overlapSize);
  this->HandlePieces(2, overlapOffset, overlapSize);
  
  // Configure the region to return.
  region = this->Region;
  region->SetSize(size);
  region->SetOffset(offset);
  region->SetData(this->Data);
  this->Region = NULL;
  
  return region;
}


//----------------------------------------------------------------------------
// Description:
// this method returns 1 if there is no overlap, 2 if the newly 
// requested region lies entirely in the cache data, and 2 if there
// is a partial overlap.  It returns the overlap between the newly 
// requested region and The data in cache in the argument arrays 
// "overlapOffset" and "overlapSize".  
int vtkImagePanCache::GetCacheStatus(int *newOffset, int *newSize, 
				    int *overlapOffset, int *overlapSize)
{
  int *cacheOffset, *cacheSize;
  int idx0, idx1, temp1, temp2;
  int newContainedInCache = 1;
  
  // special case: no cache data
  if ( ! this->Data)
    {
    vtkDebugMacro(<< "GetCacheStatus: No data cached");
    return 1;
    }
  
  if (this->GenerateTime.GetMTime() <= this->GetPipelineMTime())
    {
    vtkDebugMacro(<< "GetCacheStatus: Invalid because pipeline modified");
    return 1;
    }
  
  // get the dimensions of the cache
  cacheOffset = this->Data->GetOffset();
  cacheSize = this->Data->GetSize();
  
  // determine the overlapping region
  for (idx0 = 0; idx0 < 3; ++idx0)
    {
    // offset = left edge (easy: take max)
    temp1 = newOffset[idx0];
    temp2 = cacheOffset[idx0];
    overlapOffset[idx0] = (temp1 < temp2) ? temp2: temp1;
    // size = right edge - left edge (harder: first find right edges)
    temp1 += newSize[idx0];
    temp2 += cacheSize[idx0];
    // min(right edges) - left edge
    overlapSize[idx0] = (temp1 < temp2) ? 
      temp1-overlapOffset[idx0] : temp2-overlapOffset[idx0];
    // check for no overlap
    if (overlapSize[idx0] <= 0)
      {
      // return imediately (but first set overlap... */
      for (idx1 = 0; idx1 < 3; ++idx1)
	{
	overlapOffset[idx1] = newOffset[idx1];
	overlapSize[idx1] = 0;
	}
      vtkDebugMacro(<< "GetCacheStatus: No overlap");
      return 1;
      }
    // update contained in flag
    if (overlapOffset[idx0] != newOffset[idx0] || 
	overlapSize[idx0] != newSize[idx0]) 
      newContainedInCache = 0;
    }
 
  if (newContainedInCache)
    {
    vtkDebugMacro(<<"GetCacheStatus: requested region lies entirely in cache");
    return 2;
    }
  else
    {
    vtkDebugMacro(<< "GetCacheStatus: a part of requested region is in cache");
    return 3;
    }
}

  

//----------------------------------------------------------------------------
// Description:
// This method copies data from cache into the newly requested region.
// It has a loop for bytes in a pixel which might not be the most efficient
// implementation.
void vtkImagePanCache::CopyOverlap(vtkImageData *newData, 
				   int *offset, int* size)
{
  int newInc0, newInc1, newInc2;
  unsigned char *newPtr, *newPtr0, *newPtr1, *newPtr2;
  int cacheInc0, cacheInc1, cacheInc2;
  unsigned char *cachePtr, *cachePtr0, *cachePtr1, *cachePtr2;
  int idx, idx0, idx1, idx2;
  int sizeOfType;
  
  vtkDebugMacro(<< "CopyOverlap: " << newData->GetClassName() 
                << "(" << newData << ")" << ", offset = ("
                << offset[0] << ", " << offset[1] << ", " << offset[2] 
                << "), size = ("
                << size[0] << ", " << size[1] << ", " << size[2] << ")");
  
  // copy the overlapping region into newData
  newData->GetInc(newInc0, newInc1, newInc2);
  this->Data->GetInc(cacheInc0, cacheInc1, cacheInc2);
  sizeOfType = newData->GetSizeOfType(); // assume same as this->Data
  
  newPtr2 = newData->GetPointer(offset);
  cachePtr2 = this->Data->GetPointer(offset);
  for (idx2 = 0; idx2 < size[2]; ++idx2)
    {
    newPtr1 = newPtr2;
    cachePtr1 = cachePtr2;
    for (idx1 = 0; idx1 < size[1]; ++idx1)
      {
      newPtr0 = newPtr1;
      cachePtr0 = cachePtr1;
      for (idx0 = 0; idx0 < size[0]; ++idx0)
	{
	
	// Copy a pixel
	newPtr = newPtr0;
	cachePtr = cachePtr0;
	for (idx = 0; idx < sizeOfType; ++idx)
	  {
	  *newPtr++ = *cachePtr++;
	  }
	
	newPtr0 += newInc0;
	cachePtr0 += cacheInc0;
	}
      newPtr1 += newInc1;
      cachePtr1 += cacheInc1;
      }
    newPtr2 += newInc2;
    cachePtr2 += cacheInc2;
    }
}
  



//----------------------------------------------------------------------------
// Description:
// This Method generates the remaining pieces of a data object in one direction.
// Offset and size discribe the region generated so far.  This method expands
// along one axis of this region to the offset and size of the data.
void vtkImagePanCache::HandlePieces(int axis, int *offset, int *size)
{
  int *dataOffset, *dataSize;
  int pieceOffset[3];
  int pieceSize[3];
  int idx;

  // get the size of the data (requested image)
  dataOffset = this->Data->GetOffset();
  dataSize = this->Data->GetSize();
  
  // expand below the already-generated region
  if (dataOffset[axis] < offset[axis])
    {
    for (idx = 0; idx < 3; ++idx)
      {
      pieceOffset[idx] = offset[idx];
      pieceSize[idx] = size[idx];
      }
    pieceOffset[axis] = dataOffset[axis];
    pieceSize[axis] = offset[axis] - dataOffset[axis];
    if (size[0] * size[1] * size[2]) // ignore if the piece has no area
      this->Source->GenerateRegion(pieceOffset, pieceSize);
    // change offset and size to include the newly generated data
    offset[axis] = dataOffset[axis];
    size[axis] += pieceSize[axis];  
    }
  
  // expand above the already-generated region
  if (dataSize[axis] > size[axis])
    {
    for (idx = 0; idx < 3; ++idx)
      {
      pieceOffset[idx] = offset[idx];
      pieceSize[idx] = size[idx];
      }
    pieceOffset[axis] = size[axis] + offset[axis];
    pieceSize[axis] = dataSize[axis] - size[axis];
    // ignore if the piece has no area
    if (pieceSize[0] > 0 && pieceSize[1] > 0 && pieceSize[2] > 0) 
      this->Source->GenerateRegion(pieceOffset, pieceSize);
    // change offset and size to include the newly generated data
    size[axis] = dataSize[axis];  
    }
}



















