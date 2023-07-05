// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBar
 * @brief   Bar class for vtk
 *
 * None.
 */

#ifndef vtkBar_h
#define vtkBar_h

#include "vtkObject.h"
#include "vtkmyCommonModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKMYCOMMON_EXPORT vtkBar : public vtkObject
{
public:
  static vtkBar* New();
  vtkTypeMacro(vtkBar, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkBar() {}
  ~vtkBar() override {}

private:
  vtkBar(const vtkBar&) = delete;
  void operator=(const vtkBar&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
