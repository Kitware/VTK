/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCache.hh
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
// .NAME vtkImageCache - Caches are used by vtkImageCachedSource.
// .SECTION Description
// vtkImageCache is the super class of all filter caches.  
// If the source descides to generate a request in pieces, the caches 
// collects all of the pieces into a single vtkImageRegion object.
// The cache can also save vtkImageData objects between RequestRegion
// messages, to avoid regeneration of data.  Since requests for regions
// of an image can be any size or location, caching strategies can be
// numerous and complex.  Some predefined generic subclasses have been 
// defined, but specific applications may need to implement subclasses
// with their own stratgies tailored to the pattern of requests which
// will be made.


#ifndef __vtkImageCache_h
#define __vtkImageCache_h
#include "vtkImageSource.hh"
#include "vtkImageCachedSource.hh"
#include "vtkImageData.hh"
#include "vtkImageRegion.hh"


class vtkImageCache : public vtkImageSource
{
public:
  vtkImageCache();
  char *GetClassName() {return "vtkImageCache";};

  vtkImageRegion *RequestRegion(int offset[3], int size[3]);
  virtual vtkImageRegion *GetRegion(int offset[3], int size[3]);
  unsigned long int GetPipelineMTime();
  void GetBoundary(int *offset, int *size);

  // Description:
  // Set/Get the source associated with this cache
  vtkSetObjectMacro(Source,vtkImageCachedSource);
  vtkGetObjectMacro(Source,vtkImageCachedSource);

  void SetReleaseDataFlag(int value);
  // Description:
  // Turn the save data option on or off
  vtkGetMacro(ReleaseDataFlag,int);
  vtkBooleanMacro(ReleaseDataFlag,int);
  
  // Description:
  // Set/Get the MemoryLimit for region requests.  If a request is made that
  // exceeds this limit (number of pixels), the RequestRegion method
  // will return NULL.  I assume that a memory manager will query the memory
  // resources of a system, and set this value when the program starts.
  vtkSetMacro(RequestMemoryLimit,long);
  vtkGetMacro(RequestMemoryLimit,long);

protected:
  vtkImageCachedSource *Source;
  // Flag to tell the cache to save data or not.
  int ReleaseDataFlag;
  // Will every cache have a data object?
  vtkImageData *Data;
  // Allows one tile to be used for GetRegion and RequestRegion calls
  // "Region" is NULL before a request, is allocated when a request is made.
  // The same region is used for all GetRegion calls during a request.
  // RequestRegion returns the region and sets "Region" to NULL again.
  vtkImageRegion *Region;
  // Upperlimit on memory that can be returned by RequestRegion call
  long RequestMemoryLimit;

  // Cache the boundary, to avoid recomputing the boundary on each pass.
  int BoundaryOffset[3];
  int BoundarySize[3];
  vtkTimeStamp BoundaryTime;

  vtkImageRegion *RequestUnCachedRegion(int Offset[3], int Size[3]);
  virtual vtkImageRegion *RequestCachedRegion(int Offset[3], int Size[3]);
};

#endif


