// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkContextKeyEvent
 * @brief   data structure to represent key events.
 *
 *
 * Provides a convenient data structure to represent key events in the
 * vtkContextScene. Passed to vtkAbstractContextItem objects.
 */

#ifndef vtkContextKeyEvent_h
#define vtkContextKeyEvent_h

#include "vtkRenderingContext2DModule.h" // For export macro
#include "vtkVector.h"                   // For vtkVector2i
#include "vtkWeakPointer.h"              // For vtkWeakPointer

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderWindowInteractor;

class VTKRENDERINGCONTEXT2D_EXPORT vtkContextKeyEvent
{
public:
  vtkContextKeyEvent();

  /**
   * Set the interactor for the key event.
   */
  void SetInteractor(vtkRenderWindowInteractor* interactor);

  /**
   * Get the interactor for the key event. This can be null, and is provided
   * only for convenience.
   */
  vtkRenderWindowInteractor* GetInteractor() const;

  /**
   * Set the position of the mouse when the key was pressed.
   */
  void SetPosition(const vtkVector2i& position) { this->Position = position; }

  /**
   * Get the position of the mouse when the key was pressed.
   */
  vtkVector2i GetPosition() const { return this->Position; }

  char GetKeyCode() const;

protected:
  vtkWeakPointer<vtkRenderWindowInteractor> Interactor;
  vtkVector2i Position;
};

VTK_ABI_NAMESPACE_END
#endif // vtkContextKeyEvent_h
// VTK-HeaderTest-Exclude: vtkContextKeyEvent.h
