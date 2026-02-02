// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageMultiVariateToRGBA
 * @brief   generalized histograms up to 4 dimensions
 *
 * vtkImageMultiVariateToRGBA takes a 3 component RGB image as
 */

#ifndef vtkImageMultiVariateToRGBA_h
#define vtkImageMultiVariateToRGBA_h

#include "vtkImageAlgorithm.h"
#include "vtkImagingColorModule.h" // For export macro

#include <vector> // For modality groups

VTK_ABI_NAMESPACE_BEGIN
class vtkLookupTable;

class VTKIMAGINGCOLOR_EXPORT vtkImageMultiVariateToRGBA : public vtkImageAlgorithm
{
public:
  static vtkImageMultiVariateToRGBA* New();
  vtkTypeMacro(vtkImageMultiVariateToRGBA, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetMacro(ColorWheelRotation, double);
  vtkGetMacro(ColorWheelRotation, double);

  void SetModalityGroups(std::vector<std::vector<int>>& modalityGroups);
  std::vector<std::vector<int>> GetModalityGroups() VTK_FUTURE_CONST;

protected:
  vtkImageMultiVariateToRGBA();
  ~vtkImageMultiVariateToRGBA() override;

  double ColorWheelRotation = 0;
  std::vector<std::vector<int>> ModalityGroups;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkImageMultiVariateToRGBA(const vtkImageMultiVariateToRGBA&) = delete;
  void operator=(const vtkImageMultiVariateToRGBA&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
