/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageStaticCache.h
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
// .NAME vtkImageStaticCache - Caches an ImageData object
// .SECTION Description
// vtkImageStaticCache is used to directly cache a vtkImageData object
// that has been passed to it. This should only be used in rare situations.

#ifndef __vtkImageStaticCache_h
#define __vtkImageStaticCache_h

#include "vtkImageCache.h"

class VTK_EXPORT vtkImageStaticCache : public vtkImageCache
{
public:
  vtkImageStaticCache();
  ~vtkImageStaticCache();
  static vtkImageStaticCache *New() {return new vtkImageStaticCache;};
  const char *GetClassName() {return "vtkImageStaticCache";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method updates the region specified by "UpdateExtent".  
  void Update();

  // Description:
  // This method updates the instance variables "WholeExtent", "Spacing", 
  // "Origin", "Bounds" etc.
  // It needs to be separate from "Update" because the image information
  // may be needed to compute the required UpdateExtent of the input
  // (see "vtkImageFilter").
  virtual void UpdateImageInformation();

  // Description:
  // Generates all the requested data and returns a vtkImageData.
  vtkImageData *UpdateAndReturnData();

  // Description:
  // This Method deletes any data in cache. For a static cache the data
  // cannot be released except by deleteing the instance or providing a new
  // CachedData
  void ReleaseData();

  // Description:
  // Make this a separate method to avoid another GetPipelineMTime call.
  virtual unsigned long GetPipelineMTime();


  // Description:
  // return the un filled data of the UpdateExtent in this cache.
  vtkImageData *GetData(); 
  
  // Description:
  // Set the vtkImageData for this cache to cache. 
  vtkSetObjectMacro(CachedData, vtkImageData);

  // Description:
  // Convenience method to get the range of the scalar data in the
  // current "UpdateExtent". Returns the (min/max) range.  The components
  // are lumped into one range.  If there are no scalars the method will 
  // return (0,1). Note: Update needs to be called first to create the scalars.
  void GetScalarRange(float range[2]);  
  
protected:
  vtkImageData *CachedData;
};

#endif


