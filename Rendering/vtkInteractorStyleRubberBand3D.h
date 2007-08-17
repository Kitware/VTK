/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleRubberBand3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkInteractorStyleRubberBand3D - A rubber band interactor for a 3D view
//
// .SECTION Description
// vtkInteractorStyleRubberBand3D manages interaction in a 3D view.
// The style also allows draws a rubber band using the left button.
// All camera changes invoke InteractionBeginEvent when the button
// is pressed, InteractionEvent when the mouse (or wheel) is moved,
// and InteractionEndEvent when the button is released.  The bindings
// are as follows:
// Left mouse - Select (invokes a SelectionChangedEvent).
// Right mouse - Rotate.
// Shift + right mouse - Zoom.
// Middle mouse - Pan.
// Scroll wheel - Zoom.

#ifndef __vtkInteractorStyleRubberBand3D_h
#define __vtkInteractorStyleRubberBand3D_h

#include "vtkInteractorStyleTrackballCamera.h"

class vtkUnsignedCharArray;

class VTK_RENDERING_EXPORT vtkInteractorStyleRubberBand3D : public vtkInteractorStyleTrackballCamera
{
public:
  static vtkInteractorStyleRubberBand3D *New();
  vtkTypeRevisionMacro(vtkInteractorStyleRubberBand3D, vtkInteractorStyleTrackballCamera);
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
  // Selection types
  enum
    {
    SELECT_NORMAL = 0,
    SELECT_UNION = 1
    };

  // Description:
  // Current interaction state
  vtkGetMacro(Interaction, int);
  enum
    {
    NONE,
    PANNING,
    ZOOMING,
    ROTATING,
    SELECTING
    };
    
protected:
  vtkInteractorStyleRubberBand3D();
  ~vtkInteractorStyleRubberBand3D();
  
  // The interaction mode
  int Interaction;
  
  // Draws the selection rubber band
  void RedrawRubberBand();
  
  // The end position of the selection
  unsigned int StartPosition[2];
  
  // The start position of the selection
  unsigned int EndPosition[2];
  
  // The pixel array for the rubber band
  vtkUnsignedCharArray* PixelArray;
  
private:
  vtkInteractorStyleRubberBand3D(const vtkInteractorStyleRubberBand3D&); //Not implemented
  void operator=(const vtkInteractorStyleRubberBand3D&); // Not implemented
};

#endif
