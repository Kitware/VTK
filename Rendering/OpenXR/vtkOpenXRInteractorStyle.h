/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenXRInteractorStyle.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenXRInteractorStyle
 * @brief   extended from vtkInteractorStyle3D to override command methods
 */

#ifndef vtkOpenXRInteractorStyle_h
#define vtkOpenXRInteractorStyle_h

#include "vtkRenderingOpenXRModule.h" // For export macro
#include "vtkVRInteractorStyle.h"

class VTKRENDERINGOPENXR_EXPORT vtkOpenXRInteractorStyle : public vtkVRInteractorStyle
{
public:
  static vtkOpenXRInteractorStyle* New();
  vtkTypeMacro(vtkOpenXRInteractorStyle, vtkVRInteractorStyle);

  /**
   * Setup default actions defined with an action path and a corresponding command.
   */
  void SetupActions(vtkRenderWindowInteractor* iren) override;

  /**
   * Creates a new ControlsHelper suitable for use with this class.
   */
  vtkVRControlsHelper* MakeControlsHelper() override { return nullptr; };

  // likely to be removed
  void LoadNextCameraPose(){};

protected:
  vtkOpenXRInteractorStyle() = default;
  ~vtkOpenXRInteractorStyle() override = default;

private:
  vtkOpenXRInteractorStyle(const vtkOpenXRInteractorStyle&) = delete;
  void operator=(const vtkOpenXRInteractorStyle&) = delete;
};

#endif
