/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebInteractionEvent.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkWebInteractionEvent
// .SECTION Description
//

#ifndef __vtkWebInteractionEvent_h
#define __vtkWebInteractionEvent_h

#include "vtkObject.h"
#include "vtkWebCoreModule.h" // needed for exports

class VTKWEBCORE_EXPORT vtkWebInteractionEvent : public vtkObject
{
public:
  static vtkWebInteractionEvent* New();
  vtkTypeMacro(vtkWebInteractionEvent, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  enum MouseButton
    {
    LEFT_BUTTON = 0x01,
    MIDDLE_BUTTON = 0x02,
    RIGHT_BUTTON = 0x04,
    };

  enum ModifierKeys
    {
    SHIFT_KEY = 0x01,
    CTRL_KEY = 0x02,
    ALT_KEY = 0x04,
    META_KEY = 0x08
    };

  // Description:
  // Set/Get the mouse buttons state.
  vtkSetMacro(Buttons, unsigned int);
  vtkGetMacro(Buttons, unsigned int);

  // Description:
  // Set/Get modifier state.
  vtkSetMacro(Modifiers, unsigned int);
  vtkGetMacro(Modifiers, unsigned int);

  // Description:
  // Set/Get the chart code.
  vtkSetMacro(KeyCode, char);
  vtkGetMacro(KeyCode, char);

  // Description:
  // Set/Get event position.
  vtkSetMacro(X, double);
  vtkGetMacro(X, double);
  vtkSetMacro(Y, double);
  vtkGetMacro(Y, double);
  vtkSetMacro(Scroll, double);
  vtkGetMacro(Scroll, double);

  // Handle double click
  vtkSetMacro(RepeatCount, int);
  vtkGetMacro(RepeatCount, int);
//BTX
protected:
  vtkWebInteractionEvent();
  ~vtkWebInteractionEvent();

  unsigned int Buttons;
  unsigned int Modifiers;
  char KeyCode;
  double X;
  double Y;
  double Scroll;
  int RepeatCount;

private:
  vtkWebInteractionEvent(const vtkWebInteractionEvent&); // Not implemented
  void operator=(const vtkWebInteractionEvent&); // Not implemented
//ETX
};

#endif
