// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkInteractorStyleUser
 * @brief   provides customizable interaction routines
 *
 *
 * The most common way to customize user interaction is to write a subclass
 * of vtkInteractorStyle: vtkInteractorStyleUser allows you to customize
 * the interaction to without subclassing vtkInteractorStyle.  This is
 * particularly useful for setting up custom interaction modes in
 * scripting languages such as Python.  This class allows you
 * to hook into the MouseMove, ButtonPress/Release, KeyPress/Release,
 * etc. events.  If you want to hook into just a single mouse button,
 * but leave the interaction modes for the others unchanged, you
 * must use e.g. SetMiddleButtonPressMethod() instead of the more
 * general SetButtonPressMethod().
 */

#ifndef vtkInteractorStyleUser_h
#define vtkInteractorStyleUser_h

#include "vtkInteractionStyleModule.h" // For export macro
#include "vtkInteractorStyle.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

// new motion flag
#define VTKIS_USERINTERACTION 8

VTK_ABI_NAMESPACE_BEGIN
class VTKINTERACTIONSTYLE_EXPORT VTK_MARSHALAUTO vtkInteractorStyleUser : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleUser* New();
  vtkTypeMacro(vtkInteractorStyleUser, vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the most recent mouse position during mouse motion.
   * In your user interaction method, you must use this to track
   * the mouse movement.  Do not use GetEventPosition(), which records
   * the last position where a mouse button was pressed.
   */
  vtkGetVector2Macro(LastPos, int);
  ///@}

  ///@{
  /**
   * Get the previous mouse position during mouse motion, or after
   * a key press.  This can be used to calculate the relative
   * displacement of the mouse.
   */
  vtkGetVector2Macro(OldPos, int);
  ///@}

  ///@{
  /**
   * Test whether modifiers were held down when mouse button or key
   * was pressed.
   */
  vtkGetMacro(ShiftKey, int);
  vtkGetMacro(CtrlKey, int);
  ///@}

  ///@{
  /**
   * Get the character for a Char event.
   */
  vtkGetMacro(Char, int);
  ///@}

  ///@{
  /**
   * Get the KeySym (in the same format as vtkRenderWindowInteractor KeySyms)
   * for a KeyPress or KeyRelease method.
   */
  vtkGetStringMacro(KeySym);
  ///@}

  ///@{
  /**
   * Get the mouse button that was last pressed inside the window
   * (returns zero when the button is released).
   */
  vtkGetMacro(Button, int);
  ///@}

  ///@{
  /**
   * Generic event bindings
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
   * Keyboard functions
   */
  void OnChar() override;
  void OnKeyPress() override;
  void OnKeyRelease() override;
  ///@}

  ///@{
  /**
   * These are more esoteric events, but are useful in some cases.
   */
  void OnExpose() override;
  void OnConfigure() override;
  void OnEnter() override;
  void OnLeave() override;
  ///@}

  void OnTimer() override;

protected:
  vtkInteractorStyleUser();
  ~vtkInteractorStyleUser() override;

  int LastPos[2];
  int OldPos[2];

  int ShiftKey;
  int CtrlKey;
  int Char;
  char* KeySym;
  int Button;

private:
  vtkInteractorStyleUser(const vtkInteractorStyleUser&) = delete;
  void operator=(const vtkInteractorStyleUser&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
