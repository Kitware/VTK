// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageContinuousDilate3D
 * @brief   Dilate implemented as a maximum.
 *
 * vtkImageContinuousDilate3D replaces a pixel with the maximum over
 * an ellipsoidal neighborhood.  If KernelSize of an axis is 1, no processing
 * is done on that axis.
 */

#ifndef vtkImageContinuousDilate3D_h
#define vtkImageContinuousDilate3D_h

#include "vtkImageSpatialAlgorithm.h"
#include "vtkImagingMorphologicalModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkImageEllipsoidSource;

class VTKIMAGINGMORPHOLOGICAL_EXPORT vtkImageContinuousDilate3D : public vtkImageSpatialAlgorithm
{
public:
  ///@{
  /**
   * Construct an instance of vtkImageContinuousDilate3D filter.
   * By default zero values are dilated.
   */
  static vtkImageContinuousDilate3D* New();
  vtkTypeMacro(vtkImageContinuousDilate3D, vtkImageSpatialAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * This method sets the size of the neighborhood.  It also sets the
   * default middle of the neighborhood and computes the elliptical foot print.
   */
  void SetKernelSize(int size0, int size1, int size2);

protected:
  vtkImageContinuousDilate3D();
  ~vtkImageContinuousDilate3D() override;

  vtkImageEllipsoidSource* Ellipse;

  void ThreadedRequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inData, vtkImageData** outData,
    int outExt[6], int id) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkImageContinuousDilate3D(const vtkImageContinuousDilate3D&) = delete;
  void operator=(const vtkImageContinuousDilate3D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
