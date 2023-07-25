// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCompositeCutter
 * @brief   Cut composite data sets with user-specified implicit function
 *
 * Loop over each data set in the composite input and apply vtkCutter
 * @sa
 * vtkCutter
 */

#ifndef vtkCompositeCutter_h
#define vtkCompositeCutter_h

#include "vtkCutter.h"
#include "vtkFiltersCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkCompositeCutter : public vtkCutter
{
public:
  vtkTypeMacro(vtkCompositeCutter, vtkCutter);

  static vtkCompositeCutter* New();

  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkCompositeCutter(vtkImplicitFunction* cf = nullptr);
  ~vtkCompositeCutter() override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkCompositeCutter(const vtkCompositeCutter&) = delete;
  void operator=(const vtkCompositeCutter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
