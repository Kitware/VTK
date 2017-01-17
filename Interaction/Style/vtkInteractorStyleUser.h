/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleUser.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkInteractorStyleUser
 * @brief   provides customizable interaction routines
 *
 *
 * The most common way to customize user interaction is to write a subclass
 * of vtkInteractorStyle: vtkInteractorStyleUser allows you to customize
 * the interaction to without subclassing vtkInteractorStyle.  This is
 * particularly useful for setting up custom interaction modes in
 * scripting languages such as Tcl and Python.  This class allows you
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

// new motion flag
#define VTKIS_USERINTERACTION 8

class VTKINTERACTIONSTYLE_EXPORT vtkInteractorStyleUser : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleUser *New();
  vtkTypeMacro(vtkInteractorStyleUser,vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get the most recent mouse position during mouse motion.
   * In your user interaction method, you must use this to track
   * the mouse movement.  Do not use GetEventPosition(), which records
   * the last position where a mouse button was pressed.
   */
  vtkGetVector2Macro(LastPos,int);
  //@}

  //@{
  /**
   * Get the previous mouse position during mouse motion, or after
   * a key press.  This can be used to calculate the relative
   * displacement of the mouse.
   */
  vtkGetVector2Macro(OldPos,int);
  //@}

  //@{
  /**
   * Test whether modifiers were held down when mouse button or key
   * was pressed
   */
  vtkGetMacro(ShiftKey,int);
  vtkGetMacro(CtrlKey,int);
  //@}

  //@{
  /**
   * Get the character for a Char event.
   */
  vtkGetMacro(Char,int);
  //@}

  //@{
  /**
   * Get the KeySym (in the same format as Tk KeySyms) for a
   * KeyPress or KeyRelease method.
   */
  vtkGetStringMacro(KeySym);
  //@}

  //@{
  /**
   * Get the mouse button that was last pressed inside the window
   * (returns zero when the button is released).
   */
  vtkGetMacro(Button,int);
  //@}

  //@{
  /**
   * Generic event bindings
   */
  void OnMouseMove() VTK_OVERRIDE;
  void OnLeftButtonDown() VTK_OVERRIDE;
  void OnLeftButtonUp() VTK_OVERRIDE;
  void OnMiddleButtonDown() VTK_OVERRIDE;
  void OnMiddleButtonUp() VTK_OVERRIDE;
  void OnRightButtonDown() VTK_OVERRIDE;
  void OnRightButtonUp() VTK_OVERRIDE;
  void OnMouseWheelForward() VTK_OVERRIDE;
  void OnMouseWheelBackward() VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Keyboard functions
   */
  void OnChar() VTK_OVERRIDE;
  void OnKeyPress() VTK_OVERRIDE;
  void OnKeyRelease() VTK_OVERRIDE;
  //@}

  //@{
  /**
   * These are more esoteric events, but are useful in some cases.
   */
  void OnExpose() VTK_OVERRIDE;
  void OnConfigure() VTK_OVERRIDE;
  void OnEnter() VTK_OVERRIDE;
  void OnLeave() VTK_OVERRIDE;
  //@}

  void OnTimer() VTK_OVERRIDE;

protected:

  vtkInteractorStyleUser();
  ~vtkInteractorStyleUser() VTK_OVERRIDE;

  int LastPos[2];
  int OldPos[2];

  int ShiftKey;
  int CtrlKey;
  int Char;
  char *KeySym;
  int Button;

private:
  vtkInteractorStyleUser(const vtkInteractorStyleUser&) VTK_DELETE_FUNCTION;
  void operator=(const vtkInteractorStyleUser&) VTK_DELETE_FUNCTION;
};

#endif
