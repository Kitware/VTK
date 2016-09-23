/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleTrackball.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

class VTKINTERACTIONSTYLE_EXPORT vtkInteractorStyleTrackball : public vtkInteractorStyleSwitch
{
public:
  static vtkInteractorStyleTrackball *New();
  vtkTypeMacro(vtkInteractorStyleTrackball,vtkInteractorStyleSwitch);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkInteractorStyleTrackball();
  ~vtkInteractorStyleTrackball();

private:
  vtkInteractorStyleTrackball(const vtkInteractorStyleTrackball&) VTK_DELETE_FUNCTION;
  void operator=(const vtkInteractorStyleTrackball&) VTK_DELETE_FUNCTION;
};

#endif
