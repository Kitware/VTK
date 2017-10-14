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
/**
 * @class   vtkInteractorStyleRubberBandPick
 * @brief   Like TrackBallCamera, but this can pick props underneath a rubber band selection rectangle.
 *
 *
 * This interactor style allows the user to draw a rectangle in the render
 * window by hitting 'r' and then using the left mouse button.
 * When the mouse button is released, the attached picker operates on the pixel
 * in the center of the selection rectangle. If the picker happens to be a
 * vtkAreaPicker it will operate on the entire selection rectangle.
 * When the 'p' key is hit the above pick operation occurs on a 1x1 rectangle.
 * In other respects it behaves the same as its parent class.
 *
 * @sa
 * vtkAreaPicker
*/

#ifndef vtkInteractorStyleRubberBandPick_h
#define vtkInteractorStyleRubberBandPick_h

#include "vtkInteractionStyleModule.h" // For export macro
#include "vtkInteractorStyleTrackballCamera.h"

class vtkUnsignedCharArray;

class VTKINTERACTIONSTYLE_EXPORT vtkInteractorStyleRubberBandPick : public vtkInteractorStyleTrackballCamera
{
public:
  static vtkInteractorStyleRubberBandPick *New();
  vtkTypeMacro(vtkInteractorStyleRubberBandPick, vtkInteractorStyleTrackballCamera);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void StartSelect();

  //@{
  /**
   * Event bindings
   */
  void OnMouseMove() override;
  void OnLeftButtonDown() override;
  void OnLeftButtonUp() override;
  void OnChar() override;
  //@}

protected:
  vtkInteractorStyleRubberBandPick();
  ~vtkInteractorStyleRubberBandPick() override;

  virtual void Pick();
  void RedrawRubberBand();

  int StartPosition[2];
  int EndPosition[2];

  int Moving;

  vtkUnsignedCharArray *PixelArray;

  int CurrentMode;

private:
  vtkInteractorStyleRubberBandPick(const vtkInteractorStyleRubberBandPick&) = delete;
  void operator=(const vtkInteractorStyleRubberBandPick&) = delete;
};

#endif
