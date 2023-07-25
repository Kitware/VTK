// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkQImageToImageSource
 * @brief   Create image data from a QImage.
 *
 * vtkQImageToImageSource produces image data from a QImage.
 */

#ifndef vtkQImageToImageSource_h
#define vtkQImageToImageSource_h

#include "vtkImageAlgorithm.h"
#include "vtkRenderingQtModule.h" // For export macro

class QImage;

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGQT_EXPORT vtkQImageToImageSource : public vtkImageAlgorithm
{
public:
  static vtkQImageToImageSource* New();
  vtkTypeMacro(vtkQImageToImageSource, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set/Get QImage surface to be used.
   */
  void SetQImage(QImage* image)
  {
    this->QtImage = image;
    this->Modified();
  }
  const QImage* GetQImage() { return QtImage; }

protected:
  vtkQImageToImageSource();
  ~vtkQImageToImageSource() override = default;

  const QImage* QtImage;
  int DataExtent[6];

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation* vtkNotUsed(request),
    vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector) override;

private:
  vtkQImageToImageSource(const vtkQImageToImageSource&) = delete;
  void operator=(const vtkQImageToImageSource&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
