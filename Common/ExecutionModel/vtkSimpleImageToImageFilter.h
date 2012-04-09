/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimpleImageToImageFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSimpleImageToImageFilter - Generic image filter with one input.
// .SECTION Description
// vtkSimpleImageToImageFilter is a filter which aims to avoid much
// of the complexity associated with vtkImageAlgorithm (i.e.
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
// types, see vtkSimpleImageToImageFilter.

// .SECTION See also
// vtkImageAlgorithm vtkSimpleImageFilterExample

#ifndef __vtkSimpleImageToImageFilter_h
#define __vtkSimpleImageToImageFilter_h

#include "vtkImageAlgorithm.h"

class VTK_FILTERING_EXPORT vtkSimpleImageToImageFilter : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkSimpleImageToImageFilter,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSimpleImageToImageFilter();
  ~vtkSimpleImageToImageFilter();

  // These are called by the superclass.
  virtual int RequestUpdateExtent (vtkInformation *, 
                                   vtkInformationVector **, 
                                   vtkInformationVector *);

  // You don't have to touch this unless you have a good reason.
  virtual int RequestData(vtkInformation *, 
                          vtkInformationVector **, 
                          vtkInformationVector *);

  // In the simplest case, this is the only method you need to define.
  virtual void SimpleExecute(vtkImageData* input, vtkImageData* output) = 0;

private:
  vtkSimpleImageToImageFilter(const vtkSimpleImageToImageFilter&);  // Not implemented.
  void operator=(const vtkSimpleImageToImageFilter&);  // Not implemented.
};

#endif







