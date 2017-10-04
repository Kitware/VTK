/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageInPlaceFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageInPlaceFilter
 * @brief   Filter that operates in place.
 *
 * vtkImageInPlaceFilter is a filter super class that
 * operates directly on the input region.  The data is copied
 * if the requested region has different extent than the input region
 * or some other object is referencing the input region.
*/

#ifndef vtkImageInPlaceFilter_h
#define vtkImageInPlaceFilter_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkImageInPlaceFilter : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkImageInPlaceFilter,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkImageInPlaceFilter();
  ~vtkImageInPlaceFilter() override;

  int RequestData(vtkInformation *request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector) override;

  void CopyData(vtkImageData *in, vtkImageData *out, int* outExt);

private:
  vtkImageInPlaceFilter(const vtkImageInPlaceFilter&) = delete;
  void operator=(const vtkImageInPlaceFilter&) = delete;
};

#endif







