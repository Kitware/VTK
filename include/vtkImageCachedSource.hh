/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCachedSource.hh
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
// .NAME vtkImageCachedSource - Source of data for pipeline.
// .SECTION Description
// vtkImageCachedSource is a source with a vtkImageCache as an output object.
// There is a one to one relationship between caches and cached sources.
// If a cache is not explicitely set, a default cache object will be created,
// and default cache releases its data after every call to generate.
// Consumers of the source's data are connected to its cache object with
// a statment similar to: 
//  "consumer->SetInput(source->GetOuput());"
//  This convention was chosen to allow sources to have multiple outputs.
// Caches are resposible for collecting regions generated when the source
// splits the generation into pieces, and can also save data in between
// multiple region gerenates.
// The chain of events is: 
//  1: Consummer calls caches UpdateRegion method,
//  2: Cache can fill data imediately using cached data,
//     or can call sources UpdateRegion method.
//  4: Source asks cache to allocate the data (all at once or in pieces)
//     by calling the caches AllocateRegion method. 
//  5: Source fills the region/regions and returns. 
//  It is set up this way to keep all of the data managment in the cache.


#ifndef __vtkImageCachedSource_h
#define __vtkImageCachedSource_h

#include "vtkObject.hh"
#include "vtkImageSource.hh"
#include "vtkImageRegion.hh"
class vtkImageCache;


class vtkImageCachedSource : public vtkObject
{
public:
  vtkImageCachedSource();
  ~vtkImageCachedSource();
  char *GetClassName() {return "vtkImageCachedSource";};

  virtual void InterceptCacheUpdate(vtkImageRegion *region);
  virtual void UpdateRegion(vtkImageRegion *region); 
  vtkImageSource *GetOutput();
  // Description:
  // This method should return the bounding box of the largest region 
  // that can be generated.
  virtual void UpdateImageInformation(vtkImageRegion *region) = 0;
  virtual unsigned long GetPipelineMTime();

  virtual void SetCache(vtkImageCache *cache);
  vtkImageCache *GetCache();

  void SetReleaseDataFlag(int value);
  int  GetReleaseDataFlag();

  void SetDataType(int type);
  int  GetDataType();
  
  virtual void SetAxis1d(int axis0);
  virtual void SetAxes2d(int axis0, int axis1);
  virtual void SetAxes3d(int axis0, int axis1, int axis2);
  virtual void SetAxes4d(int axis0, int axis1, int axis2, int axis3);
  // Description:
  // Get the axes reordering of this filter.
  vtkGetVector4Macro(Axes,int);
  
  void DebugOn();
  void DebugOff();
  void SetMemoryLimit(long limit);

protected:
  vtkImageCache *Output;
  int Axes[VTK_IMAGE_DIMENSIONS];            // reorder the axes
  
  virtual void UpdateRegion4d(vtkImageRegion *region); 
  virtual void UpdateRegion3d(vtkImageRegion *region); 
  virtual void UpdateRegion2d(vtkImageRegion *region); 
  virtual void UpdateRegion1d(vtkImageRegion *region); 
  virtual void CheckCache();
};


// all necessary header files will be included automatically.
#include "vtkImageCache.hh"

#endif


