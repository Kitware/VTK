/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextScene.h

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

#ifndef __vtkContextMouseEvent_h
#define __vtkContextMouseEvent_h

#include "vtkVector.h" // Needed for vtkVector2f and vtkVector2i

class vtkContextMouseEvent
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
};
//ETX

#endif // __vtkContextMouseEvent_h
