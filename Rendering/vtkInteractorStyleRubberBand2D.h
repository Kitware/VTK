/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleRubberBand2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkInteractorStyleRubberBand2D - A rubber band interactor for a 2D view
//
// .SECTION Description
// vtkInteractorStyleRubberBand2D manages interaction in a 2D view.
// Camera rotation is not allowed with this interactor style.
// The style also allows draws a rubber band using the left button.
// All camera changes invoke InteractionBeginEvent when the button
// is pressed, InteractionEvent when the mouse (or wheel) is moved,
// and InteractionEndEvent when the button is released.  The bindings
// are as follows:
// Left mouse - Select (invokes a SelectionChangedEvent).
// Right mouse - Zoom.
// Middle mouse - Pan.
// Scroll wheel - Zoom.

#ifndef __vtkInteractorStyleRubberBand2D_h
#define __vtkInteractorStyleRubberBand2D_h

#include "vtkInteractorStyle.h"

class vtkUnsignedCharArray;

class VTK_RENDERING_EXPORT vtkInteractorStyleRubberBand2D : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleRubberBand2D *New();
  vtkTypeMacro(vtkInteractorStyleRubberBand2D, vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void OnLeftButtonDown();
  virtual void OnLeftButtonUp();
  virtual void OnMiddleButtonDown();
  virtual void OnMiddleButtonUp();
  virtual void OnRightButtonDown();
  virtual void OnRightButtonUp();
  virtual void OnMouseMove();
  virtual void OnMouseWheelForward();
  virtual void OnMouseWheelBackward();

  // Description:
  // Whether to invoke a render when the mouse moves.
  vtkSetMacro(RenderOnMouseMove, bool);
  vtkGetMacro(RenderOnMouseMove, bool);
  vtkBooleanMacro(RenderOnMouseMove, bool);

  //BTX
  // Description:
  // Selection types
  enum
    {
    SELECT_NORMAL = 0,
    SELECT_UNION = 1
    };
  //ETX

  // Description:
  // Current interaction state
  vtkGetMacro(Interaction, int);
  
  //BTX
  enum
    {
    NONE,
    PANNING,
    ZOOMING,
    SELECTING
    };
  //ETX
    
protected:
  vtkInteractorStyleRubberBand2D();
  ~vtkInteractorStyleRubberBand2D();
  
  // The interaction mode
  int Interaction;
  
  // Draws the selection rubber band
  void RedrawRubberBand();
  
  // The end position of the selection
  int StartPosition[2];
  
  // The start position of the selection
  int EndPosition[2];
  
  // The pixel array for the rubber band
  vtkUnsignedCharArray* PixelArray;

  // Whether to render when the mouse moves
  bool RenderOnMouseMove;
  
private:
  vtkInteractorStyleRubberBand2D(const vtkInteractorStyleRubberBand2D&); // Not implemented
  void operator=(const vtkInteractorStyleRubberBand2D&); // Not implemented
};

#endif
