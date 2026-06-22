// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTrackballMoveActor
 * @brief   Moves an actor.
 *
 * vtkTrackballMoveActor allows the user to interactively move an actor in the scene.
 * This class relies on the vtkCameraManipulatorGUIHelper to provide the active source and its
 * bounds. When the user clicks and drags the mouse, the actor will move based on the mouse movement
 * relative to the center of the active source's bounds
 *
 * Add observers to the RequestActiveSourceBoundsEvent, RequestActiveSourcePositionEvent, and
 * ApplyActiveSourcePositionEvent events to provide the necessary information and handle the
 * movement.
 *
 * 1. RequestActiveSourceBoundsEvent: This event is triggered when the manipulator needs the bounds
 * of the active source. Fill in the bounds of the active source in the callData of the event (cast
 * to double[6]).
 * 2. RequestActiveSourcePositionEvent: This event is triggered when the manipulator needs the
 * position of the active source. Fill in the position of the active source in the callData of the
 * event (cast to double[3]).
 * 3. ApplyActiveSourcePositionEvent: This event is triggered when the manipulator has calculated
 * the new position for the active source and wants to apply it. The new position is provided in the
 * callData of the event (cast to double[3]). Update the position of the active source accordingly.
 */

#ifndef vtkTrackballMoveActor_h
#define vtkTrackballMoveActor_h

#include "vtkCameraManipulator.h"

#include "vtkCommand.h"                // for vtkCommand::UserEvent
#include "vtkInteractionStyleModule.h" // needed for export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKINTERACTIONSTYLE_EXPORT vtkTrackballMoveActor : public vtkCameraManipulator
{
public:
  static vtkTrackballMoveActor* New();
  vtkTypeMacro(vtkTrackballMoveActor, vtkCameraManipulator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum MoveActorEvents
  {
    RequestActiveSourceBoundsEvent = vtkCommand::UserEvent + 1,
    RequestActiveSourcePositionEvent = vtkCommand::UserEvent + 3,
    ApplyActiveSourcePositionEvent = vtkCommand::UserEvent + 2
  };

  ///@{
  /**
   * Unimplemented methods from vtkCameraManipulator.
   */
  void StartInteraction() override {};
  void EndInteraction() override {};
  void OnKeyDown(vtkRenderWindowInteractor* vtkNotUsed(rwi)) override {}
  void OnKeyUp(vtkRenderWindowInteractor* vtkNotUsed(rwi)) override {}
  void OnButtonUp(int vtkNotUsed(x), int vtkNotUsed(y), vtkRenderer* vtkNotUsed(ren),
    vtkRenderWindowInteractor* vtkNotUsed(rwi)) override
  {
  }
  ///@}

  ///@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* iren) override;
  void OnButtonDown(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* iren) override;
  ///@}

protected:
  vtkTrackballMoveActor();
  ~vtkTrackballMoveActor() override;

private:
  vtkTrackballMoveActor(const vtkTrackballMoveActor&) = delete;
  void operator=(const vtkTrackballMoveActor&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
