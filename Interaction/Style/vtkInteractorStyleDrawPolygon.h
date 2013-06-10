/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleDrawPolygon.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInteractorStyleDrawPolygon - draw polygon during mouse move
// .SECTION Description
// This interactor style allows the user to draw a polygon in the render
// window using the left mouse button while mouse is moving.
// When the mouse button is released, a SelectionChangedEvent will be fired.

#ifndef __vtkInteractorStyleDrawPolygon_h
#define __vtkInteractorStyleDrawPolygon_h

#include "vtkInteractionStyleModule.h" // For export macro
#include "vtkInteractorStyle.h"

#include <vector>      // For returning Polygon Points
#include "vtkVector.h" // For Polygon Points

class vtkUnsignedCharArray;

class VTKINTERACTIONSTYLE_EXPORT vtkInteractorStyleDrawPolygon : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleDrawPolygon *New();
  vtkTypeMacro(vtkInteractorStyleDrawPolygon, vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Event bindings
  virtual void OnMouseMove();
  virtual void OnLeftButtonDown();
  virtual void OnLeftButtonUp();

  // Description:
  // Whether to draw polygon in screen pixels. Default is ON
  vtkSetMacro(DrawPolygonPixels, bool);
  vtkGetMacro(DrawPolygonPixels, bool);
  vtkBooleanMacro(DrawPolygonPixels, bool);

  // Description:
  // Get the current polygon points in display units
  std::vector<vtkVector2i> GetPolygonPoints();

protected:
  vtkInteractorStyleDrawPolygon();
  ~vtkInteractorStyleDrawPolygon();

  virtual void DrawPolygon();

  int StartPosition[2];
  int EndPosition[2];
  int Moving;

  bool DrawPolygonPixels;

  vtkUnsignedCharArray *PixelArray;

private:
  vtkInteractorStyleDrawPolygon(const vtkInteractorStyleDrawPolygon&);  // Not implemented
  void operator=(const vtkInteractorStyleDrawPolygon&);  // Not implemented

  class vtkInternal;
  vtkInternal* Internal;
};

#endif
