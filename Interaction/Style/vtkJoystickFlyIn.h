// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkJoystickFlyIn
 * @brief   Rotates camera with xy mouse movement.
 *
 * vtkJoystickFlyIn allows the user to interactively
 * manipulate the camera, the viewpoint of the scene.
 */

#ifndef vtkJoystickFlyIn_h
#define vtkJoystickFlyIn_h

#include "vtkInteractionStyleModule.h" // needed for export macro
#include "vtkJoystickFly.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKINTERACTIONSTYLE_EXPORT VTK_MARSHALAUTO vtkJoystickFlyIn : public vtkJoystickFly
{
public:
  static vtkJoystickFlyIn* New();
  vtkTypeMacro(vtkJoystickFlyIn, vtkJoystickFly);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkJoystickFlyIn();
  ~vtkJoystickFlyIn() override;

private:
  vtkJoystickFlyIn(const vtkJoystickFlyIn&) = delete;
  void operator=(const vtkJoystickFlyIn&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
