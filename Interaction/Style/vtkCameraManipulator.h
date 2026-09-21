// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCameraManipulator
 * @brief   Abstraction of style away from button.
 *
 * vtkCameraManipulator is a superclass for actions inside an
 * interactor style and associated with a single button. An example
 * might be rubber-band bounding-box zoom. This abstraction allows a
 * camera manipulator to be assigned to any button.
 *
 * When the interactor style receives a mouse button press event, it finds a suitable manipulator
 * that was bound to the button and modifier keys. Then, it calls `StartInteraction` on the
 * manipulator, followed by `OnButtonDown`. As the mouse moves, it calls
 * `OnMouseMove` on the active manipulator. Finally, when the mouse button is released, it calls
 * `OnButtonUp` and then `EndInteraction` on the manipulator.
 *
 * @sa vtkInteractorStyleManipulator
 */

#ifndef vtkCameraManipulator_h
#define vtkCameraManipulator_h

#include "vtkObject.h"

#include "vtkInteractionStyleModule.h" // needed for export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderer;
class vtkRenderWindowInteractor;

class VTKINTERACTIONSTYLE_EXPORT VTK_MARSHALAUTO vtkCameraManipulator : public vtkObject
{
public:
  vtkTypeMacro(vtkCameraManipulator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * The vtkInterctorStyleManipulator will call these methods when the corresponding events occur.
   * The StartInteraction is called before the OnButtonDown and EndInteraction is
   * called after the OnButtonUp.
   */
  virtual void StartInteraction() = 0;
  virtual void EndInteraction() = 0;
  ///@}

  /**
   * These methods are called on the active manipulator when the corresponding events occur. The
   * manipulator can then perform the appropriate camera manipulation.
   */
  virtual void OnMouseMove(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* iren) = 0;
  virtual void OnButtonDown(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* iren) = 0;
  virtual void OnButtonUp(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* iren) = 0;

  ///@{
  /**
   * These methods are called on all registered manipulators, not just the
   * active one. Hence, these should just be used to record state and not
   * perform any interactions.
   */
  virtual void OnKeyUp(vtkRenderWindowInteractor* iren) = 0;
  virtual void OnKeyDown(vtkRenderWindowInteractor* iren) = 0;
  ///@}

  enum class MouseButtonType
  {
    Left = 1,
    Middle = 2,
    Right = 3
  };

  ///@{
  /**
   * These settings determine which button the
   * manipulator responds to. Button can be:
   * 1. MouseButtonType::Left
   * 2. MouseButtonType::Middle
   * 3. MouseButtonType::Right
   */
  vtkSetEnumMacro(MouseButton, MouseButtonType);
  vtkGetEnumMacro(MouseButton, MouseButtonType);
  ///@}

  ///@{
  /**
   * Set to true if this manipulator should respond when the `Alt`
   * modifier is used.
   */
  vtkSetMacro(Alt, bool);
  vtkGetMacro(Alt, bool);
  vtkBooleanMacro(Alt, bool);
  ///@}

  ///@{
  /**
   * Set to true if this manipulator should respond when the `Ctrl`
   * modifier is used.
   */
  vtkSetMacro(Control, bool);
  vtkGetMacro(Control, bool);
  vtkBooleanMacro(Control, bool);
  ///@}

  ///@{
  /**
   * Set to true if this manipulator should respond when the `Shift`
   * modifier is used.
   */
  vtkSetMacro(Shift, bool);
  vtkGetMacro(Shift, bool);
  vtkBooleanMacro(Shift, bool);
  ///@}

  ///@{
  /**
   * Set and get the center of rotation in world coordinates.
   * This is used by some manipulators as the center around which
   * the camera orbits, or rolls.
   */
  vtkSetVector3Macro(CenterOfRotation, double);
  vtkGetVector3Macro(CenterOfRotation, double);
  ///@}

  ///@{
  /**
   * Set and get the mouse motion factor.
   */
  vtkSetMacro(MouseMotionFactor, double);
  vtkGetMacro(MouseMotionFactor, double);
  ///@}

  ///@{
  /**
   * Set and get the manipulator name.
   */
  vtkSetStringMacro(ManipulatorName);
  vtkGetStringMacro(ManipulatorName);
  ///@}

protected:
  vtkCameraManipulator();
  ~vtkCameraManipulator() override;

  /**
   * Compute the center of rotation in display coordinates.
   * It transforsm the `CenterOfRotation` from world coordinates to display coordinates and saves it
   * in `CenterOfRotationInDisplayCoordinates`.
   */
  void ComputeCenterOfRotationInDisplayCoordinates(vtkRenderer* ren);

  // protected so subclasses can use it in their mouse move events.
  double CenterOfRotationInDisplayCoordinates[2] = { 0.0, 0.0 };

private:
  vtkCameraManipulator(const vtkCameraManipulator&) = delete;
  void operator=(const vtkCameraManipulator&) = delete;

  char* ManipulatorName = nullptr;

  MouseButtonType MouseButton = MouseButtonType::Left;

  bool Alt = false;
  bool Control = false;
  bool Shift = false;

  double CenterOfRotation[3] = { 0.0, 0.0, 0.0 };
  double MouseMotionFactor = 1.0;
};
VTK_ABI_NAMESPACE_END
#endif
