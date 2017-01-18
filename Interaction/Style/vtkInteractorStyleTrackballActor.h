/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleTrackballActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkInteractorStyleTrackballActor
 * @brief   manipulate objects in the scene independent of each other
 *
 * vtkInteractorStyleTrackballActor allows the user to interact with (rotate,
 * pan, etc.) objects in the scene indendent of each other.  In trackball
 * interaction, the magnitude of the mouse motion is proportional to the
 * actor motion associated with a particular mouse binding. For example,
 * small left-button motions cause small changes in the rotation of the
 * actor around its center point.
 *
 * The mouse bindings are as follows. For a 3-button mouse, the left button
 * is for rotation, the right button for zooming, the middle button for
 * panning, and ctrl + left button for spinning.  (With fewer mouse buttons,
 * ctrl + shift + left button is for zooming, and shift + left button is for
 * panning.)
 *
 * @sa
 * vtkInteractorStyleTrackballCamera vtkInteractorStyleJoystickActor
 * vtkInteractorStyleJoystickCamera
*/

#ifndef vtkInteractorStyleTrackballActor_h
#define vtkInteractorStyleTrackballActor_h

#include "vtkInteractionStyleModule.h" // For export macro
#include "vtkInteractorStyle.h"

class vtkCellPicker;

class VTKINTERACTIONSTYLE_EXPORT vtkInteractorStyleTrackballActor : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleTrackballActor *New();
  vtkTypeMacro(vtkInteractorStyleTrackballActor,vtkInteractorStyle);
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
  vtkInteractorStyleTrackballActor();
  ~vtkInteractorStyleTrackballActor() VTK_OVERRIDE;

  void FindPickedActor(int x, int y);

  void Prop3DTransform(vtkProp3D *prop3D,
                       double *boxCenter,
                       int NumRotation,
                       double **rotate,
                       double *scale);

  double MotionFactor;

  vtkProp3D *InteractionProp;
  vtkCellPicker *InteractionPicker;

private:
  vtkInteractorStyleTrackballActor(const vtkInteractorStyleTrackballActor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkInteractorStyleTrackballActor&) VTK_DELETE_FUNCTION;
};

#endif
