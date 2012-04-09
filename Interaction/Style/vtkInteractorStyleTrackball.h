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
// .NAME vtkInteractorStyleTrackball - provides trackball motion control

// .SECTION Description
// vtkInteractorStyleTrackball is an implementation of vtkInteractorStyle
// that defines the trackball style. It is now deprecated and as such a
// subclass of vtkInteractorStyleSwitch

// .SECTION See Also
// vtkInteractorStyleSwitch vtkInteractorStyleTrackballActor vtkInteractorStyleJoystickCamera

#ifndef __vtkInteractorStyleTrackball_h
#define __vtkInteractorStyleTrackball_h

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
  vtkInteractorStyleTrackball(const vtkInteractorStyleTrackball&);  // Not implemented.
  void operator=(const vtkInteractorStyleTrackball&);  // Not implemented.
};

#endif
