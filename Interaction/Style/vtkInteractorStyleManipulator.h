// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInteractorStyleManipulator
 * @brief   interactive manipulation of the camera
 *
 * vtkInteractorStyleManipulator maintains a collection of vtkCameraManipulator instances. Each
 * manipulator is associated with a particular mouse button and modifier key combination. When the
 * user presses a mouse button, the interactor style looks for a manipulator that matches the button
 * and modifier keys and starts an interaction with that manipulator. As the user moves the mouse,
 * the interactor style forwards the mouse move events to the active manipulator. When the user
 * releases the mouse button, the interactor style ends the interaction with the active manipulator
 * and clears it.
 *
 * @sa vtkCameraManipulator
 */

#ifndef vtkInteractorStyleManipulator_h
#define vtkInteractorStyleManipulator_h

#include "vtkInteractorStyle.h"

#include "vtkInteractionStyleModule.h" // needed for export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkCollection;
class vtkCameraManipulator;
class VTKINTERACTIONSTYLE_EXPORT vtkInteractorStyleManipulator : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleManipulator* New();
  vtkTypeMacro(vtkInteractorStyleManipulator, vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove() override;
  void OnLeftButtonDown() override;
  void OnLeftButtonUp() override;
  void OnMiddleButtonDown() override;
  void OnMiddleButtonUp() override;
  void OnRightButtonDown() override;
  void OnRightButtonUp() override;
  void OnMouseWheelForward() override;
  void OnMouseWheelBackward() override;
  ///@}

  ///@{
  /**
   * Unlike mouse events, these are forwarded to all camera manipulators
   * since we don't have a mechanism to activate a manipulator by key presses
   * currently.
   */
  void OnKeyDown() override;
  void OnKeyUp() override;
  ///@}

  /**
   * Access to adding or removing manipulators.
   */
  void AddManipulator(vtkCameraManipulator* m);

  /**
   * Removes all manipulators.
   */
  void RemoveAllManipulators();

  ///@{
  /**
   * Get the collection of camera manipulators.
   */
  vtkGetObjectMacro(CameraManipulators, vtkCollection);
  ///@}

  ///@{
  /**
   * When enabled, mouse wheel  will zoom to the projected point under the cursor position.
   * There is no need to hold down Ctrl key to achieve this.
   */
  vtkSetMacro(MouseWheelZoomsToCursor, bool);
  vtkGetMacro(MouseWheelZoomsToCursor, bool);
  ///@}

  ///@{
  /**
   * Propagates the motion factor to the manipulators.
   * This simply sets an internal ivar.
   * It is propagated to a manipulator with vtkCameraManipulator::SetMouseMotionFactor
   * before the event is sent to it.
   * Also changing the MouseMotionFactor during interaction
   * i.e. after a button press but before a button up
   * has no effect until the next button press.
   */
  vtkSetMacro(MouseMotionFactor, double);
  vtkGetMacro(MouseMotionFactor, double);
  ///@}

  void Dolly() override;

  /**
   * Returns the chosen manipulator based on the modifiers.
   */
  vtkCameraManipulator* FindManipulator(int button, bool alt, bool control, bool shift);

protected:
  vtkInteractorStyleManipulator();
  ~vtkInteractorStyleManipulator() override;

private:
  vtkInteractorStyleManipulator(const vtkInteractorStyleManipulator&) = delete;
  void operator=(const vtkInteractorStyleManipulator&) = delete;

  bool MouseWheelZoomsToCursor = false;
  double MouseMotionFactor = 1.0;

  // The CameraInteractors also store there button and modifier.
  vtkCollection* CameraManipulators;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
  friend class vtkInternals;
};
VTK_ABI_NAMESPACE_END
#endif
