// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageTranslateExtent
 * @brief   Changes extent, nothing else.
 *
 * vtkImageTranslateExtent shift the whole extent, but does not
 * change the data.
 */

#ifndef vtkImageTranslateExtent_h
#define vtkImageTranslateExtent_h

#include "vtkImageAlgorithm.h"
#include "vtkImagingCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGCORE_EXPORT vtkImageTranslateExtent : public vtkImageAlgorithm
{
public:
  static vtkImageTranslateExtent* New();
  vtkTypeMacro(vtkImageTranslateExtent, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Delta to change "WholeExtent". -1 changes 0->10 to -1->9.
   */
  vtkSetVector3Macro(Translation, int);
  vtkGetVector3Macro(Translation, int);
  ///@}

protected:
  vtkImageTranslateExtent();
  ~vtkImageTranslateExtent() override = default;

  int Translation[3];

  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkImageTranslateExtent(const vtkImageTranslateExtent&) = delete;
  void operator=(const vtkImageTranslateExtent&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
