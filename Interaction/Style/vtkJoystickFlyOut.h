// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkJoystickFlyOut
 * @brief   Rotates camera with xy mouse movement.
 *
 * vtkJoystickFlyOut allows the user to interactively
 * manipulate the camera, the viewpoint of the scene.
 */

#ifndef vtkJoystickFlyOut_h
#define vtkJoystickFlyOut_h

#include "vtkInteractionStyleModule.h" // needed for export macro
#include "vtkJoystickFly.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKINTERACTIONSTYLE_EXPORT VTK_MARSHALAUTO vtkJoystickFlyOut : public vtkJoystickFly
{
public:
  static vtkJoystickFlyOut* New();
  vtkTypeMacro(vtkJoystickFlyOut, vtkJoystickFly);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkJoystickFlyOut();
  ~vtkJoystickFlyOut() override;

private:
  vtkJoystickFlyOut(const vtkJoystickFlyOut&) = delete;
  void operator=(const vtkJoystickFlyOut&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
