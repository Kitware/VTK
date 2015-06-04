/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextMouseEvent.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkContextMouseEvent - data structure to represent mouse events.
//
// .SECTION Description
// Provides a convenient data structure to represent mouse events in the
// vtkContextScene. Passed to vtkAbstractContextItem objects.

#ifndef vtkContextMouseEvent_h
#define vtkContextMouseEvent_h

#include "vtkRenderingContext2DModule.h" // For export macro
#include "vtkWin32Header.h" // For export macros.
#include "vtkVector.h"      // Needed for vtkVector2f and vtkVector2i

class vtkRenderWindowInteractor;

class VTKRENDERINGCONTEXT2D_EXPORT vtkContextMouseEvent
{
public:
  // Description:
  // Enumeration of mouse buttons.
  enum {
    NO_BUTTON = 0,
    LEFT_BUTTON = 1,
    MIDDLE_BUTTON = 2,
    RIGHT_BUTTON = 4
  };

  // Description:
  // Enumeration of modifier keys.
  enum {
    NO_MODIFIER = 0,
    ALT_MODIFIER = 1,
    SHIFT_MODIFIER = 2,
    CONTROL_MODIFIER = 4
  };

  vtkContextMouseEvent()
  {
  }

  // Description:
  // Set the interactor for the mouse event.
  void SetInteractor(vtkRenderWindowInteractor *interactor)
  {
    this->Interactor = interactor;
  }

  // Description:
  // Get the interactor for the mouse event. This can be null, and is provided
  // only for convenience.
  vtkRenderWindowInteractor* GetInteractor() const
  {
    return this->Interactor;
  }

  // Description:
  // Set/get the position of the mouse in the item's coordinates.
  void SetPos(const vtkVector2f &pos) { this->Pos = pos; }
  vtkVector2f GetPos() const { return this->Pos; }

  // Description:
  // Set/get the position of the mouse in scene coordinates.
  void SetScenePos(const vtkVector2f &pos) { this->ScenePos = pos; }
  vtkVector2f GetScenePos() const { return this->ScenePos; }

  // Description:
  // Set/get the position of the mouse in screen coordinates.
  void SetScreenPos(const vtkVector2i &pos) { this->ScreenPos = pos; }
  vtkVector2i GetScreenPos() const { return this->ScreenPos; }

  // Description:
  // Set/get the position of the mouse in the item's coordinates.
  void SetLastPos(const vtkVector2f &pos) { this->LastPos = pos; }
  vtkVector2f GetLastPos() const { return this->LastPos; }

  // Description:
  // Set/get the position of the mouse in scene coordinates.
  void SetLastScenePos(const vtkVector2f &pos) { this->LastScenePos = pos; }
  vtkVector2f GetLastScenePos() const { return this->LastScenePos; }

  // Description:
  // Set/get the position of the mouse in screen coordinates.
  void SetLastScreenPos(const vtkVector2i &pos) { this->LastScreenPos = pos; }
  vtkVector2i GetLastScreenPos() const { return this->LastScreenPos; }

  // Description:
  // Set/get the mouse button that caused the event, with possible values being
  // NO_BUTTON, LEFT_BUTTON, MIDDLE_BUTTON and RIGHT_BUTTON.
  void SetButton(int button) { this->Button = button; }
  int GetButton() const { return this->Button; }

  // Description:
  // Return the modifier keys, if any, ORed together. Valid modifier enum values
  // are NO_MODIFIER, ALT_MODIFIER, SHIFT_MODIFIER and/or CONTROL_MODIFIER.
  int GetModifiers() const;

protected:
  // Description:
  // Position of the mouse in item coordinate system.
  vtkVector2f Pos;

  // Description:
  // Position of the mouse the scene coordinate system.
  vtkVector2f ScenePos;

  // Description:
  // Position of the mouse in screen coordinates
  vtkVector2i ScreenPos;

  // Description:
  // `Pos' at the previous mouse event.
  vtkVector2f LastPos;

  // Description:
  // `ScenePos'at the previous mouse event.
  vtkVector2f LastScenePos;

  // Description:
  // `ScreenPos' at the previous mouse event.
  vtkVector2i LastScreenPos;

  // Description:
  // Mouse button that caused the event, using the anonymous enumeration.
  int Button;

protected:
  vtkRenderWindowInteractor *Interactor;
};

#endif // vtkContextMouseEvent_h
// VTK-HeaderTest-Exclude: vtkContextMouseEvent.h
