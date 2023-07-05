// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBar2
 * @brief   Bar2 class for vtk
 *
 * None.
 */

#ifndef vtkBar2_h
#define vtkBar2_h

#include "vtkObject.h"
#include "vtkmyUnsortedModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKMYUNSORTED_EXPORT vtkBar2 : public vtkObject
{
public:
  static vtkBar2* New();
  vtkTypeMacro(vtkBar2, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkBar2() {}
  ~vtkBar2() override {}

private:
  vtkBar2(const vtkBar2&) = delete;
  void operator=(const vtkBar2&) = delete;
};
VTK_ABI_NAMESPACE_END

#endif
