/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSimpleCache.cc
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
#include "vtkImageSimpleCache.hh"

//----------------------------------------------------------------------------
// Description:
// This Method handles external requests for data.
// It returns a tile contianing the Region requested,
// or NULL if the memory could not be allocated.
vtkImageRegion *vtkImageSimpleCache::RequestCachedRegion(int offset[3], int size[3])
{
  vtkImageRegion *tile;
  
  // Check Data to see if the tile is already exists
  if (this->Data)
    {
    int *dataOffset, *dataSize;
    dataOffset = this->Data->GetOffset();
    dataSize = this->Data->GetSize();
    // Is the new request contained in the data?
    if (offset[0] >= dataOffset[0] 
	&& offset[1] >= dataOffset[1] 
	&& offset[2] >= dataOffset[2])
      {
      if (offset[0] + size[0] <= dataOffset[0] + dataSize[0]
	  && offset[1] + size[1] <= dataOffset[1] + dataSize[1]
	  && offset[2] + size[2] <= dataOffset[2] + dataSize[2])
	{
	// check the gtime of data to see if it is more recent than mtime */
	if (this->GenerateTime.GetMTime() >= this->GetPipelineMTime())
	  {
	  /* use the cache data (automatically registered by tile) */
	  vtkDebugMacro(<< "RequestRegion: Using cache to satisfy request.");
	  tile = new vtkImageRegion;
	  tile->SetOffset(offset);
	  tile->SetSize(size);
	  tile->SetData(this->Data);
	  return tile;
	}
      }
    }
  }

  // Get rid of the old cached data
  if (this->Data)
    {
    this->Data->Delete();
    this->Data = NULL;
    }
  // Generate a new region
  tile = this->RequestUnCachedRegion(offset, size);
  // Save the data in cache
  this->Data = tile->GetData();
  this->Data->Register(this);
  // Mark when this data was generated
  this->GenerateTime.Modified();
  
  return tile;
}



























