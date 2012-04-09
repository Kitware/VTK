/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimpleImageFilterExample.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSimpleImageFilterExample - Simple example of an image-image filter.
// .SECTION Description
// This is an example of a simple image-image filter. It copies it's input
// to it's output (point by point). It shows how templates can be used
// to support various data types.
// .SECTION See also
// vtkSimpleImageToImageFilter

#ifndef __vtkSimpleImageFilterExample_h
#define __vtkSimpleImageFilterExample_h

#include "vtkSimpleImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkSimpleImageFilterExample : public vtkSimpleImageToImageFilter
{
public:
  static vtkSimpleImageFilterExample *New();
  vtkTypeMacro(vtkSimpleImageFilterExample,vtkSimpleImageToImageFilter);

protected:

  vtkSimpleImageFilterExample() {};
  ~vtkSimpleImageFilterExample() {};

  virtual void SimpleExecute(vtkImageData* input, vtkImageData* output);
private:
  vtkSimpleImageFilterExample(const vtkSimpleImageFilterExample&);  // Not implemented.
  void operator=(const vtkSimpleImageFilterExample&);  // Not implemented.
};

#endif







