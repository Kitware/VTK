// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageHybridMedian2D
 * @brief   Median filter that preserves lines and
 * corners.
 *
 * vtkImageHybridMedian2D is a median filter that preserves thin lines and
 * corners.  It operates on a 5x5 pixel neighborhood.  It computes two values
 * initially: the median of the + neighbors and the median of the x
 * neighbors.  It then computes the median of these two values plus the center
 * pixel.  This result of this second median is the output pixel value.
 */

#ifndef vtkImageHybridMedian2D_h
#define vtkImageHybridMedian2D_h

#include "vtkImageSpatialAlgorithm.h"
#include "vtkImagingGeneralModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGGENERAL_EXPORT vtkImageHybridMedian2D : public vtkImageSpatialAlgorithm
{
public:
  static vtkImageHybridMedian2D* New();
  vtkTypeMacro(vtkImageHybridMedian2D, vtkImageSpatialAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkImageHybridMedian2D();
  ~vtkImageHybridMedian2D() override = default;

  void ThreadedRequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inData, vtkImageData** outData,
    int outExt[6], int id) override;

private:
  vtkImageHybridMedian2D(const vtkImageHybridMedian2D&) = delete;
  void operator=(const vtkImageHybridMedian2D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
