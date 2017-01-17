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
/**
 * @class   vtkInteractorStyleRubberBandZoom
 * @brief   zoom in by amount indicated by rubber band box
 *
 * This interactor style allows the user to draw a rectangle in the render
 * window using the left mouse button.  When the mouse button is released,
 * the current camera zooms by an amount determined from the shorter side of
 * the drawn rectangle.
*/

#ifndef vtkInteractorStyleRubberBandZoom_h
#define vtkInteractorStyleRubberBandZoom_h

#include "vtkInteractionStyleModule.h" // For export macro
#include "vtkInteractorStyle.h"

class vtkUnsignedCharArray;

class VTKINTERACTIONSTYLE_EXPORT vtkInteractorStyleRubberBandZoom : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleRubberBandZoom *New();
  vtkTypeMacro(vtkInteractorStyleRubberBandZoom, vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Event bindings
   */
  void OnMouseMove() VTK_OVERRIDE;
  void OnLeftButtonDown() VTK_OVERRIDE;
  void OnLeftButtonUp() VTK_OVERRIDE;
  //@}

protected:
  vtkInteractorStyleRubberBandZoom();
  ~vtkInteractorStyleRubberBandZoom() VTK_OVERRIDE;

  void Zoom() VTK_OVERRIDE;

  int StartPosition[2];
  int EndPosition[2];

  int Moving;

  vtkUnsignedCharArray *PixelArray;

private:
  vtkInteractorStyleRubberBandZoom(const vtkInteractorStyleRubberBandZoom&) VTK_DELETE_FUNCTION;
  void operator=(const vtkInteractorStyleRubberBandZoom&) VTK_DELETE_FUNCTION;
};

#endif
