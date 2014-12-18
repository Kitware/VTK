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

// .NAME vtkContextKeyEvent - data structure to represent key events.
//
// .SECTION Description
// Provides a convenient data structure to represent key events in the
// vtkContextScene. Passed to vtkAbstractContextItem objects.

#ifndef vtkContextKeyEvent_h
#define vtkContextKeyEvent_h

#include "vtkRenderingContext2DModule.h" // For export macro
#include "vtkWeakPointer.h" // For vtkWeakPointer
#include "vtkVector.h" // For vtkVector2i

class vtkRenderWindowInteractor;

class VTKRENDERINGCONTEXT2D_EXPORT vtkContextKeyEvent
{
public:
  vtkContextKeyEvent();
  ~vtkContextKeyEvent();

  // Description:
  // Set the interactor for the key event.
  void SetInteractor(vtkRenderWindowInteractor *interactor);

  // Description:
  // Get the interactor for the key event. This can be null, and is provided
  // only for convenience.
  vtkRenderWindowInteractor* GetInteractor() const;

  // Description:
  // Set the position of the mouse when the key was pressed.
  void SetPosition(const vtkVector2i& position) { this->Position = position; }

  // Description:
  // Get the position of the mouse when the key was pressed.
  vtkVector2i GetPosition() const { return this->Position; }

  char GetKeyCode() const;

protected:
  vtkWeakPointer<vtkRenderWindowInteractor> Interactor;
  vtkVector2i Position;
};

#endif // vtkContextKeyEvent_h
// VTK-HeaderTest-Exclude: vtkContextKeyEvent.h
