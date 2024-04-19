// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageMedian3D
 * @brief   Median Filter
 *
 * vtkImageMedian3D a Median filter that replaces each pixel with the
 * median value from a rectangular neighborhood around that pixel.
 * Neighborhoods can be no more than 3 dimensional.  Setting one
 * axis of the neighborhood kernelSize to 1 changes the filter
 * into a 2D median.
 */

#ifndef vtkImageMedian3D_h
#define vtkImageMedian3D_h

#include "vtkImageSpatialAlgorithm.h"
#include "vtkImagingGeneralModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGGENERAL_EXPORT vtkImageMedian3D : public vtkImageSpatialAlgorithm
{
public:
  static vtkImageMedian3D* New();
  vtkTypeMacro(vtkImageMedian3D, vtkImageSpatialAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * This method sets the size of the neighborhood.  It also sets the
   * default middle of the neighborhood
   */
  void SetKernelSize(int size0, int size1, int size2);

  ///@{
  /**
   * Return the number of elements in the median mask
   */
  vtkGetMacro(NumberOfElements, int);
  ///@}

protected:
  vtkImageMedian3D();
  ~vtkImageMedian3D() override;

  int NumberOfElements;

  void ThreadedRequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inData, vtkImageData** outData,
    int outExt[6], int id) override;

private:
  vtkImageMedian3D(const vtkImageMedian3D&) = delete;
  void operator=(const vtkImageMedian3D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
