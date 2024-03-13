// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInteractorStyleTrackball
 * @brief   provides trackball motion control
 *
 *
 * vtkInteractorStyleTrackball is an implementation of vtkInteractorStyle
 * that defines the trackball style. It is now deprecated and as such a
 * subclass of vtkInteractorStyleSwitch
 *
 * @sa
 * vtkInteractorStyleSwitch vtkInteractorStyleTrackballActor vtkInteractorStyleJoystickCamera
 */

#ifndef vtkInteractorStyleTrackball_h
#define vtkInteractorStyleTrackball_h

#include "vtkInteractionStyleModule.h" // For export macro
#include "vtkInteractorStyleSwitch.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKINTERACTIONSTYLE_EXPORT VTK_MARSHALAUTO vtkInteractorStyleTrackball
  : public vtkInteractorStyleSwitch
{
public:
  static vtkInteractorStyleTrackball* New();
  vtkTypeMacro(vtkInteractorStyleTrackball, vtkInteractorStyleSwitch);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkInteractorStyleTrackball();
  ~vtkInteractorStyleTrackball() override;

private:
  vtkInteractorStyleTrackball(const vtkInteractorStyleTrackball&) = delete;
  void operator=(const vtkInteractorStyleTrackball&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
