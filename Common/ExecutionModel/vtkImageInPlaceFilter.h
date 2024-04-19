// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONEXECUTIONMODEL_EXPORT vtkImageInPlaceFilter : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkImageInPlaceFilter, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkImageInPlaceFilter();
  ~vtkImageInPlaceFilter() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  void CopyData(vtkImageData* in, vtkImageData* out, int* outExt);

private:
  vtkImageInPlaceFilter(const vtkImageInPlaceFilter&) = delete;
  void operator=(const vtkImageInPlaceFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
