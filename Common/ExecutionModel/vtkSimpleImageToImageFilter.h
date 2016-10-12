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
/**
 * @class   vtkSimpleImageToImageFilter
 * @brief   Generic image filter with one input.
 *
 * vtkSimpleImageToImageFilter is a filter which aims to avoid much
 * of the complexity associated with vtkImageAlgorithm (i.e.
 * support for pieces, multi-threaded operation). If you need to write
 * a simple image-image filter which operates on the whole input, use
 * this as the superclass. The subclass has to provide only an execute
 * method which takes input and output as arguments. Memory allocation
 * is handled in vtkSimpleImageToImageFilter. Also, you are guaranteed to
 * have a valid input in the Execute(input, output) method. By default,
 * this filter
 * requests it's input's whole extent and copies the input's information
 * (spacing, whole extent etc...) to the output. If the output's setup
 * is different (for example, if it performs some sort of sub-sampling),
 * ExecuteInformation has to be overwritten. As an example of how this
 * can be done, you can look at vtkImageShrink3D::ExecuteInformation.
 * For a complete example which uses templates to support generic data
 * types, see vtkSimpleImageToImageFilter.
 *
 * @sa
 * vtkImageAlgorithm vtkSimpleImageFilterExample
*/

#ifndef vtkSimpleImageToImageFilter_h
#define vtkSimpleImageToImageFilter_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkSimpleImageToImageFilter : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkSimpleImageToImageFilter,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkSimpleImageToImageFilter();
  ~vtkSimpleImageToImageFilter() VTK_OVERRIDE;

  // These are called by the superclass.
  int RequestUpdateExtent (vtkInformation *,
                                   vtkInformationVector **,
                                   vtkInformationVector *) VTK_OVERRIDE;

  // You don't have to touch this unless you have a good reason.
  int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE;

  // In the simplest case, this is the only method you need to define.
  virtual void SimpleExecute(vtkImageData* input, vtkImageData* output) = 0;

private:
  vtkSimpleImageToImageFilter(const vtkSimpleImageToImageFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSimpleImageToImageFilter&) VTK_DELETE_FUNCTION;
};

#endif







