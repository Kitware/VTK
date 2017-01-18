/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleJoystickActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkInteractorStyleJoystickActor
 * @brief   manipulate objects in the scene independently of one another
 *
 * The class vtkInteractorStyleJoystickActor allows the user to interact
 * with (rotate, zoom, etc.) separate objects in the scene independent of
 * each other.  The position of the mouse relative to the center of the
 * object determines the speed of the object's motion.  The mouse's velocity
 * detemines the acceleration of the object's motion, so the object will
 * continue moving even when the mouse is not moving.
 * For a 3-button mouse, the left button is for rotation, the right button
 * for zooming, the middle button for panning, and ctrl + left button for
 * spinning.  (With fewer mouse buttons, ctrl + shift + left button is
 * for zooming, and shift + left button is for panning.)
 * @sa
 * vtkInteractorStyleJoystickCamera vtkInteractorStyleTrackballActor
 * vtkInteractorStyleTrackballCamera
*/

#ifndef vtkInteractorStyleJoystickActor_h
#define vtkInteractorStyleJoystickActor_h

#include "vtkInteractionStyleModule.h" // For export macro
#include "vtkInteractorStyle.h"

class vtkCellPicker;

// motion flags

class VTKINTERACTIONSTYLE_EXPORT vtkInteractorStyleJoystickActor : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleJoystickActor *New();

  vtkTypeMacro(vtkInteractorStyleJoystickActor,vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove() VTK_OVERRIDE;
  void OnLeftButtonDown() VTK_OVERRIDE;
  void OnLeftButtonUp() VTK_OVERRIDE;
  void OnMiddleButtonDown() VTK_OVERRIDE;
  void OnMiddleButtonUp() VTK_OVERRIDE;
  void OnRightButtonDown() VTK_OVERRIDE;
  void OnRightButtonUp() VTK_OVERRIDE;
  //@}

  // These methods for the different interactions in different modes
  // are overridden in subclasses to perform the correct motion. Since
  // they might be called from OnTimer, they do not have mouse coord parameters
  // (use interactor's GetEventPosition and GetLastEventPosition)
  void Rotate() VTK_OVERRIDE;
  void Spin() VTK_OVERRIDE;
  void Pan() VTK_OVERRIDE;
  void Dolly() VTK_OVERRIDE;
  void UniformScale() VTK_OVERRIDE;

protected:
  vtkInteractorStyleJoystickActor();
  ~vtkInteractorStyleJoystickActor() VTK_OVERRIDE;

  void FindPickedActor(int x, int y);

  void Prop3DTransform(vtkProp3D *prop3D,
                       double *boxCenter,
                       int numRotation,
                       double **rotate,
                       double *scale);

  double MotionFactor;

  vtkProp3D *InteractionProp;
  vtkCellPicker *InteractionPicker;

private:
  vtkInteractorStyleJoystickActor(const vtkInteractorStyleJoystickActor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkInteractorStyleJoystickActor&) VTK_DELETE_FUNCTION;
};

#endif
