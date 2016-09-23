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
/**
 * @class   vtkSimpleImageFilterExample
 * @brief   Simple example of an image-image filter.
 *
 * This is an example of a simple image-image filter. It copies it's input
 * to it's output (point by point). It shows how templates can be used
 * to support various data types.
 * @sa
 * vtkSimpleImageToImageFilter
*/

#ifndef vtkSimpleImageFilterExample_h
#define vtkSimpleImageFilterExample_h

#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkSimpleImageToImageFilter.h"

class VTKIMAGINGGENERAL_EXPORT vtkSimpleImageFilterExample : public vtkSimpleImageToImageFilter
{
public:
  static vtkSimpleImageFilterExample *New();
  vtkTypeMacro(vtkSimpleImageFilterExample,vtkSimpleImageToImageFilter);

protected:

  vtkSimpleImageFilterExample() {}
  ~vtkSimpleImageFilterExample() {}

  virtual void SimpleExecute(vtkImageData* input, vtkImageData* output);
private:
  vtkSimpleImageFilterExample(const vtkSimpleImageFilterExample&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSimpleImageFilterExample&) VTK_DELETE_FUNCTION;
};

#endif







// VTK-HeaderTest-Exclude: vtkSimpleImageFilterExample.h
