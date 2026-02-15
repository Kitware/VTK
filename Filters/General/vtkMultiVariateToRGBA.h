// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMultiVariateToRGBA
 * @brief   generalized histograms up to 4 dimensions
 *
 * vtkMultiVariateToRGBA takes a 3 component RGB image as
 */

#ifndef vtkMultiVariateToRGBA_h
#define vtkMultiVariateToRGBA_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersGeneralModule.h" // For export macro

#include <functional> // For std::function
#include <vector>     // For modality groups

VTK_ABI_NAMESPACE_BEGIN
class vtkLookupTable;

class VTKFILTERSGENERAL_EXPORT vtkMultiVariateToRGBA : public vtkDataSetAlgorithm
{
public:
  static vtkMultiVariateToRGBA* New();
  vtkTypeMacro(vtkMultiVariateToRGBA, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetMacro(ColorWheelRotation, double);
  vtkGetMacro(ColorWheelRotation, double);

  void SetModalityGroups(std::vector<std::vector<int>>& modalityGroups);
  std::vector<std::vector<int>> GetModalityGroups() VTK_FUTURE_CONST;

protected:
  vtkMultiVariateToRGBA();
  ~vtkMultiVariateToRGBA() override;

  double ColorWheelRotation = 0;
  std::vector<std::vector<int>> ModalityGroups;
  std::function<double(double)> GlobalMin = nullptr;
  std::function<double(double)> GlobalMax = nullptr;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkMultiVariateToRGBA(const vtkMultiVariateToRGBA&) = delete;
  void operator=(const vtkMultiVariateToRGBA&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
