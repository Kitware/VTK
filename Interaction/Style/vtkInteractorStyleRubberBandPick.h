/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleRubberBandPick.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInteractorStyleRubberBandPick - Like TrackBallCamera, but this can pick props underneath a rubber band selection rectangle.
//
// .SECTION Description
// This interactor style allows the user to draw a rectangle in the render
// window by hitting 'r' and then using the left mouse button.
// When the mouse button is released, the attached picker operates on the pixel
// in the center of the selection rectangle. If the picker happens to be a
// vtkAreaPicker it will operate on the entire selection rectangle.
// When the 'p' key is hit the above pick operation occurs on a 1x1 rectangle.
// In other respects it behaves the same as its parent class.
//
// .SECTION See Also
// vtkAreaPicker

#ifndef __vtkInteractorStyleRubberBandPick_h
#define __vtkInteractorStyleRubberBandPick_h

#include "vtkInteractionStyleModule.h" // For export macro
#include "vtkInteractorStyleTrackballCamera.h"

class vtkUnsignedCharArray;

class VTKINTERACTIONSTYLE_EXPORT vtkInteractorStyleRubberBandPick : public vtkInteractorStyleTrackballCamera
{
public:
  static vtkInteractorStyleRubberBandPick *New();
  vtkTypeMacro(vtkInteractorStyleRubberBandPick, vtkInteractorStyleTrackballCamera);
  void PrintSelf(ostream& os, vtkIndent indent);

  void StartSelect();

  // Description:
  // Event bindings
  virtual void OnMouseMove();
  virtual void OnLeftButtonDown();
  virtual void OnLeftButtonUp();
  virtual void OnChar();

protected:
  vtkInteractorStyleRubberBandPick();
  ~vtkInteractorStyleRubberBandPick();

  virtual void Pick();
  void RedrawRubberBand();

  int StartPosition[2];
  int EndPosition[2];

  int Moving;

  vtkUnsignedCharArray *PixelArray;

  int CurrentMode;

private:
  vtkInteractorStyleRubberBandPick(const vtkInteractorStyleRubberBandPick&);  // Not implemented
  void operator=(const vtkInteractorStyleRubberBandPick&);  // Not implemented
};

#endif
