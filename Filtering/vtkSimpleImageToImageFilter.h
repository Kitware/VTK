/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimpleImageToImageFilter.h
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
// .NAME vtkSimpleImageToImageFilter - Generic image filter with one input.
// .SECTION Description
// vtkSimpleImageToImageFilter is a filter which aims to avoid much
// of the complexity associated with vtkImageToImageFilter (i.e.
// support for pieces, multi-threaded operation). If you need to write
// a simple image-image filter which operates on the whole input, use
// this as the superclass. The subclass has to provide only an execute
// method which takes input and output as arguments. Memory allocation
// is handled in vtkSimpleImageToImageFilter. Also, you are guaranteed to
// have a valid input in the Execute(input, output) method. By default, 
// this filter
// requests it's input's whole extent and copies the input's information
// (spacing, whole extent etc...) to the output. If the output's setup
// is different (for example, if it performs some sort of sub-sampling),
// ExecuteInformation has to be overwritten. As an example of how this
// can be done, you can look at vtkImageShrink3D::ExecuteInformation.
// For a complete example which uses templates to support generic data
// types, see  vtkSimpleFilterExample.

// .SECTION See also
// vtkImageToImageFilter vtkSimpleImageFilterExample

#ifndef __vtkSimpleImageToImageFilter_h
#define __vtkSimpleImageToImageFilter_h

#include "vtkImageSource.h"

class VTK_FILTERING_EXPORT vtkSimpleImageToImageFilter : public vtkImageSource
{
public:

  vtkTypeRevisionMacro(vtkSimpleImageToImageFilter,vtkImageSource);

  // Description:
  // Set the Input of a filter. 
  virtual void SetInput(vtkImageData *input);
  vtkImageData *GetInput();
  
  
protected:
  vtkSimpleImageToImageFilter();
  ~vtkSimpleImageToImageFilter();

  // These are called by the superclass.
  // You might have to override ExecuteInformation
  virtual void ExecuteInformation();
  virtual void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);

  // You don't have to touch this unless you have a good reason.
  virtual void ExecuteData(vtkDataObject *output);
  // In the simplest case, this is the only method you need to define.
  virtual void SimpleExecute(vtkImageData* input, vtkImageData* output) = 0;
private:
  vtkSimpleImageToImageFilter(const vtkSimpleImageToImageFilter&);  // Not implemented.
  void operator=(const vtkSimpleImageToImageFilter&);  // Not implemented.
};

#endif







