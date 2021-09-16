/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenVRInteractorStyle.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenVRInteractorStyle
 * @brief   extended from vtkInteractorStyle3D to override command methods
 */

#ifndef vtkOpenVRInteractorStyle_h
#define vtkOpenVRInteractorStyle_h

#include "vtkRenderingOpenVRModule.h" // For export macro
#include "vtkVRInteractorStyle.h"

class vtkRenderWindowInteractor;
class vtkVRControlsHelper;

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRInteractorStyle : public vtkVRInteractorStyle
{
public:
  static vtkOpenVRInteractorStyle* New();
  vtkTypeMacro(vtkOpenVRInteractorStyle, vtkVRInteractorStyle);

  virtual void SetupActions(vtkRenderWindowInteractor* iren) override;
  virtual void LoadNextCameraPose() override;
  vtkVRControlsHelper* MakeControlsHelper() override;

protected:
  vtkOpenVRInteractorStyle() = default;
  ~vtkOpenVRInteractorStyle() override = default;

private:
  vtkOpenVRInteractorStyle(const vtkOpenVRInteractorStyle&) = delete;
  void operator=(const vtkOpenVRInteractorStyle&) = delete;
};

#endif
