/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageInPlaceFilter.h
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
// .NAME vtkImageInPlaceFilter - Filter that operates in place.
// .SECTION Description
// vtkImageInPlaceFilter is a filter super class that 
// operates directly on the input region.  The data is copied
// if the requested region has different extent than the input region
// or some other object is referencing the input region.  

// .SECTION See Also
// vtkImageToImageFilter vtkImageMultipleInputFilter vtkImageTwoInputFilter
// vtkImageTwoOutputFilter


#ifndef __vtkImageInPlaceFilter_h
#define __vtkImageInPlaceFilter_h

#include "vtkImageToImageFilter.h"

class VTK_FILTERING_EXPORT vtkImageInPlaceFilter : public vtkImageToImageFilter
{
public:
  vtkTypeRevisionMacro(vtkImageInPlaceFilter,vtkImageToImageFilter);


protected:
  vtkImageInPlaceFilter() {};
  ~vtkImageInPlaceFilter() {};

  virtual void ExecuteData(vtkDataObject *out);
  void CopyData(vtkImageData *in, vtkImageData *out);
  
private:
  vtkImageInPlaceFilter(const vtkImageInPlaceFilter&);  // Not implemented.
  void operator=(const vtkImageInPlaceFilter&);  // Not implemented.
};

#endif







