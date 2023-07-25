// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImagePadFilter
 * @brief   Super class for filters that fill in extra pixels.
 *
 * vtkImagePadFilter Changes the image extent of an image.  If the image
 * extent is larger than the input image extent, the extra pixels are
 * filled by an algorithm determined by the subclass.
 * The image extent of the output has to be specified.
 */

#ifndef vtkImagePadFilter_h
#define vtkImagePadFilter_h

#include "vtkImagingCoreModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGCORE_EXPORT vtkImagePadFilter : public vtkThreadedImageAlgorithm
{
public:
  static vtkImagePadFilter* New();
  vtkTypeMacro(vtkImagePadFilter, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * The image extent of the output has to be set explicitly.
   */
  void SetOutputWholeExtent(int extent[6]);
  void SetOutputWholeExtent(int minX, int maxX, int minY, int maxY, int minZ, int maxZ);
  void GetOutputWholeExtent(int extent[6]);
  int* GetOutputWholeExtent() VTK_SIZEHINT(6) { return this->OutputWholeExtent; }
  ///@}

  ///@{
  /**
   * Set/Get the number of output scalar components.
   */
  vtkSetMacro(OutputNumberOfScalarComponents, int);
  vtkGetMacro(OutputNumberOfScalarComponents, int);
  ///@}

protected:
  vtkImagePadFilter();
  ~vtkImagePadFilter() override = default;

  int OutputWholeExtent[6];
  int OutputNumberOfScalarComponents;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  virtual void ComputeInputUpdateExtent(int inExt[6], int outExt[6], int wholeExtent[6]);

private:
  vtkImagePadFilter(const vtkImagePadFilter&) = delete;
  void operator=(const vtkImagePadFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
