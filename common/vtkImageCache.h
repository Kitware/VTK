/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCache.h
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
// .NAME vtkImageCache - Caches are used by vtkImageCachedSource.
// .SECTION Description
// vtkImageCache is the super class of all filter caches.  
// If the cached source descides to generate in pieces, the caches 
// collects all of the pieces into a single vtkImageRegion object.
// The cache can also save vtkImageData objects between UpdateRegion
// messages, to avoid regeneration of data.  Since regions
// can be any size or location, caching strategies can be
// numerous and complex.  Some predefined generic subclasses have been 
// defined, but specific applications may need to implement subclasses
// with their own stratgies tailored to the pattern of regions generated.


#ifndef __vtkImageCache_h
#define __vtkImageCache_h
#include "vtkImageSource.h"
#include "vtkImageCachedSource.h"
#include "vtkImageData.h"
#include "vtkImageRegion.h"


class vtkImageCache : public vtkImageSource
{
public:
  vtkImageCache();
  ~vtkImageCache();
  char *GetClassName() {return "vtkImageCache";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  void UpdateRegion(vtkImageRegion *region);
  virtual void AllocateRegion(vtkImageRegion *region);
  unsigned long int GetPipelineMTime();
  void UpdateImageInformation(vtkImageRegion *region);
  
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
  // Subclass implements this method to delete any cached data.
  virtual void ReleaseData() = 0;
  
  // Description:
  // Set/Get the OutputMemoryLimit for region.  If a UpdateRegion
  // exceeds this limit (number of pixels), the UpdateRegion method
  // will return NULL.  
  vtkSetMacro(OutputMemoryLimit,long);
  vtkGetMacro(OutputMemoryLimit,long);

  // Description:
  // Set the data type of the regions created by this cache.
  vtkSetMacro(ScalarType,int);
  vtkGetMacro(ScalarType,int);
  
protected:
  vtkImageCachedSource *Source;

  //  to tell the cache to save data or not.
  int ReleaseDataFlag;

  // A pointer to the data is kept during one generate.
  // This may not be needed.
  vtkImageData *Data;
  
  // Upperlimit on memory that can be allocated by UpdateRegion call
  long OutputMemoryLimit;

  // Cache the ImageExtent, to avoid recomputing the ImageExtent on each pass.
  int ImageExtent[VTK_IMAGE_EXTENT_DIMENSIONS];
  float AspectRatio[VTK_IMAGE_DIMENSIONS];
  vtkTimeStamp ImageInformationTime;

  // The cache manipulates (and returns) regions with this data type.
  int ScalarType;
  
  void GenerateUnCachedRegionData(vtkImageRegion *region);
  // Description:
  // This method is used by a subclass to first look to cached 
  // data.  It can also return null if the method
  // fails for any reason.
  virtual void GenerateCachedRegionData(vtkImageRegion *region) = 0;
};

#endif


