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

#include "vtkInteractorStyle3D.h"

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRInteractorStyle : public vtkInteractorStyle3D
{
public:
  static vtkOpenVRInteractorStyle *New();
  vtkTypeMacro(vtkOpenVRInteractorStyle, vtkInteractorStyle3D);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
  * Override Middle Button to load next camera position
  */
  void OnMiddleButtonDown() VTK_OVERRIDE;
  void OnMiddleButtonUp() VTK_OVERRIDE;
  //@}

protected:
  vtkOpenVRInteractorStyle();
  ~vtkOpenVRInteractorStyle() VTK_OVERRIDE;

private:
  vtkOpenVRInteractorStyle(const vtkOpenVRInteractorStyle&) VTK_DELETE_FUNCTION;  // Not implemented.
  void operator=(const vtkOpenVRInteractorStyle&) VTK_DELETE_FUNCTION;  // Not implemented.
};

#endif
