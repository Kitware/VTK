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

class vtkCamera;
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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove() VTK_OVERRIDE;
  void OnLeftButtonDown() VTK_OVERRIDE;
  void OnLeftButtonUp() VTK_OVERRIDE;
  void OnRightButtonDown() VTK_OVERRIDE;
  void OnRightButtonUp() VTK_OVERRIDE;
  void OnMiddleButtonDown() VTK_OVERRIDE;
  void OnMiddleButtonUp() VTK_OVERRIDE;
  void OnFourthButtonUp() VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Event bindings for gestures
   */
  void OnPinch() VTK_OVERRIDE;
  void OnPan() VTK_OVERRIDE;
  //@}

  // This method handles updating the prop based on changes in the devices
  // pose. We use rotate as the state to mean adjusting-the-actor-pose
  void Rotate() VTK_OVERRIDE;

  // This method handles updating the camera based on changes in the devices
  // pose. We use Dolly as the state to mean moving the camera forward
  void Dolly() VTK_OVERRIDE;

  // This method handles updating the clip plane for all mappers
  // in the renderer
  virtual void Clip();

  //@{
  /**
   * Interaction mode for adjusting a hardware clipping plane
   */
  virtual void StartClip();
  virtual void EndClip();
  //@}

    //@{
  /**
   * Set/Get the dolly motion factor used when flying in 3D.
   * Defaults to 2.0 to simulate 2 meters per second
   * of movement in physical space. The dolly speed is
   * adjusted by the touchpad position as well. The maximum
   * rate is twice this setting.
   */
  vtkSetMacro(DollyMotionFactor, double);
  vtkGetMacro(DollyMotionFactor, double);
  //@}

  /**
   * Set the distance for the camera. The distance
   * in VR represents the scaling from world
   * to physical space. So when we set it to a new
   * value we also adjust the HMD position to maintain
   * the same relative position.
   */
  void SetDistance(vtkCamera *cam, double distance);

protected:
  vtkInteractorStyle3D();
  ~vtkInteractorStyle3D() VTK_OVERRIDE;

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

  double DollyMotionFactor;

private:
  vtkInteractorStyle3D(const vtkInteractorStyle3D&) VTK_DELETE_FUNCTION;  // Not implemented.
  void operator=(const vtkInteractorStyle3D&) VTK_DELETE_FUNCTION;  // Not implemented.
};

#endif
