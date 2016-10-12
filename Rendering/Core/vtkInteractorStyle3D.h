/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyle3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkInteractorStyle3D
 * @brief   extends interaction to support 3D input
 *
 * vtkInteractorStyle3D allows the user to interact with (rotate,
 * pan, etc.) objects in the scene indendent of each other. It is designed
 * to use 3d positions and orientations instead of 2D.
 *
 * The following interactions are specified by default.
 *
 * A click and hold in 3D within the bounding box of a prop
 * will pick up that prop allowing you to translate and
 * orient that prop as desired with the 3D controller.
 *
 * Click/dragging two controllers and pulling them apart or
 * pushing them together will initial a scale gesture
 * that will scale the world larger or smaller.
 *
 * Click/dragging two controllers and translating them in the same
 * direction will translate the camera/world
 * pushing them together will initial a scale gesture
 * that will scale the world larger or smaller.
 *
 * If a controller is right clicked (push touchpad on Vive)
 * then it starts a fly motion where the camer moves in the
 * direction the controller is pointing. It moves at a speed
 * scaled by the position of your thumb on the trackpad.
 * Higher moves faster forward. Lower moves faster backwards.
 *
 * For the Vive left click is mapped to the trigger and right
 * click is mapped to pushing the trackpad down.
 *
 * @sa
 * vtkRenderWindowInteractor3D
*/

#ifndef vtkInteractorStyle3D_h
#define vtkInteractorStyle3D_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkInteractorStyle.h"

class vtkPropPicker3D;
class vtkProp3D;
class vtkMatrix3x3;
class vtkMatrix4x4;
class vtkTransform;

class VTKRENDERINGCORE_EXPORT vtkInteractorStyle3D : public vtkInteractorStyle
{
public:
  static vtkInteractorStyle3D *New();
  vtkTypeMacro(vtkInteractorStyle3D,vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  virtual void OnMouseMove();
  virtual void OnLeftButtonDown();
  virtual void OnLeftButtonUp();
  virtual void OnRightButtonDown();
  virtual void OnRightButtonUp();
  //@}

  //@{
  /**
   * Event bindings for gestures
   */
  virtual void OnPinch();
  virtual void OnPan();
  //@}

  // This method handles updating the prop based on changes in the devices
  // pose. We use rotate as the state to mean adjusting-the-actor-pose
  virtual void Rotate();

  // This method handles updating the camera based on changes in the devices
  // pose. We use Dolly as the state to mean moving the camera forward
  virtual void Dolly();

protected:
  vtkInteractorStyle3D();
  ~vtkInteractorStyle3D();

  void FindPickedActor(double x, double y, double z);

  void Prop3DTransform(vtkProp3D *prop3D,
                       double *boxCenter,
                       int NumRotation,
                       double **rotate,
                       double *scale);

  vtkPropPicker3D *InteractionPicker;
  vtkProp3D *InteractionProp;
  vtkMatrix3x3 *TempMatrix3;
  vtkMatrix4x4 *TempMatrix4;
  vtkTransform *TempTransform;
  double AppliedTranslation[3];

private:
  vtkInteractorStyle3D(const vtkInteractorStyle3D&) VTK_DELETE_FUNCTION;  // Not implemented.
  void operator=(const vtkInteractorStyle3D&) VTK_DELETE_FUNCTION;  // Not implemented.
};

#endif
