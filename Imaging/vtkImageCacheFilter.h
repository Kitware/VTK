/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCacheFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageCacheFilter - Caches multiple vtkImageData objects.
// .SECTION Description
// vtkImageCacheFilter keep a number of vtkImageDataObjects from previous updates
// to satisfy future updates without needing to update the input.  
// It does not change the data at all.
// It just makes the pipeline more efficient at the expense of using extra memory.



#ifndef __vtkImageCacheFilter_h
#define __vtkImageCacheFilter_h

#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageCacheFilter : public vtkImageToImageFilter
{
public:
  static vtkImageCacheFilter *New();
  vtkTypeRevisionMacro(vtkImageCacheFilter,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is the maximum number of images that can be retained in memory.
  // it defaults to 10.
  void SetCacheSize(int size);
  vtkGetMacro(CacheSize, int);
  
  // Description:
  // This is an internal method that you should not call.
  void UpdateData(vtkDataObject *outData);

protected:
  vtkImageCacheFilter();
  ~vtkImageCacheFilter();

  int CacheSize;
  
  vtkImageData **Data;
  // I do not have write access to UpdateTime.
  unsigned long *Times;
private:
  vtkImageCacheFilter(const vtkImageCacheFilter&);  // Not implemented.
  void operator=(const vtkImageCacheFilter&);  // Not implemented.
};



#endif



