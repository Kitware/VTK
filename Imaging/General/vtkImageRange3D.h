// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageRange3D
 * @brief   Max - min of a circular neighborhood.
 *
 * vtkImageRange3D replaces a pixel with the maximum minus minimum over
 * an ellipsoidal neighborhood.  If KernelSize of an axis is 1, no processing
 * is done on that axis.
 */

#ifndef vtkImageRange3D_h
#define vtkImageRange3D_h

#include "vtkImageSpatialAlgorithm.h"
#include "vtkImagingGeneralModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkImageEllipsoidSource;

class VTKIMAGINGGENERAL_EXPORT vtkImageRange3D : public vtkImageSpatialAlgorithm
{
public:
  static vtkImageRange3D* New();
  vtkTypeMacro(vtkImageRange3D, vtkImageSpatialAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * This method sets the size of the neighborhood.  It also sets the
   * default middle of the neighborhood and computes the elliptical foot print.
   */
  void SetKernelSize(int size0, int size1, int size2);

protected:
  vtkImageRange3D();
  ~vtkImageRange3D() override;

  vtkImageEllipsoidSource* Ellipse;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  void ThreadedRequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inData, vtkImageData** outData,
    int outExt[6], int id) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkImageRange3D(const vtkImageRange3D&) = delete;
  void operator=(const vtkImageRange3D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
