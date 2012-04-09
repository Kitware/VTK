/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleRubberBandZoom.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInteractorStyleRubberBandZoom - zoom in by amount indicated by rubber band box
// .SECTION Description
// This interactor style allows the user to draw a rectangle in the render
// window using the left mouse button.  When the mouse button is released,
// the current camera zooms by an amount determined from the shorter side of
// the drawn rectangle.

#ifndef __vtkInteractorStyleRubberBandZoom_h
#define __vtkInteractorStyleRubberBandZoom_h

#include "vtkInteractionStyleModule.h" // For export macro
#include "vtkInteractorStyle.h"

class vtkUnsignedCharArray;

class VTKINTERACTIONSTYLE_EXPORT vtkInteractorStyleRubberBandZoom : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleRubberBandZoom *New();
  vtkTypeMacro(vtkInteractorStyleRubberBandZoom, vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Event bindings
  virtual void OnMouseMove();
  virtual void OnLeftButtonDown();
  virtual void OnLeftButtonUp();

protected:
  vtkInteractorStyleRubberBandZoom();
  ~vtkInteractorStyleRubberBandZoom();

  virtual void Zoom();

  int StartPosition[2];
  int EndPosition[2];

  int Moving;

  vtkUnsignedCharArray *PixelArray;

private:
  vtkInteractorStyleRubberBandZoom(const vtkInteractorStyleRubberBandZoom&);  // Not implemented
  void operator=(const vtkInteractorStyleRubberBandZoom&);  // Not implemented
};

#endif
