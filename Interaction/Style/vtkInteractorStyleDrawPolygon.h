// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInteractorStyleDrawPolygon
 * @brief   draw polygon during mouse move
 *
 * This interactor style allows the user to draw a polygon in the render
 * window using the left mouse button while mouse is moving.
 * When the mouse button is released, a SelectionChangedEvent will be fired.
 */

#ifndef vtkInteractorStyleDrawPolygon_h
#define vtkInteractorStyleDrawPolygon_h

#include "vtkInteractionStyleModule.h" // For export macro
#include "vtkInteractorStyle.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

#include "vtkVector.h" // For Polygon Points
#include <vector>      // For returning Polygon Points

VTK_ABI_NAMESPACE_BEGIN
class vtkUnsignedCharArray;

class VTKINTERACTIONSTYLE_EXPORT VTK_MARSHALAUTO vtkInteractorStyleDrawPolygon
  : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleDrawPolygon* New();
  vtkTypeMacro(vtkInteractorStyleDrawPolygon, vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Event bindings
   */
  void OnMouseMove() override;
  void OnLeftButtonDown() override;
  void OnLeftButtonUp() override;
  ///@}

  ///@{
  /**
   * Whether to draw polygon in screen pixels. Default is ON
   */
  vtkSetMacro(DrawPolygonPixels, bool);
  vtkGetMacro(DrawPolygonPixels, bool);
  vtkBooleanMacro(DrawPolygonPixels, bool);
  ///@}

  /**
   * Get the current polygon points in display units
   */
  std::vector<vtkVector2i> GetPolygonPoints();

protected:
  vtkInteractorStyleDrawPolygon();
  ~vtkInteractorStyleDrawPolygon() override;

  virtual void DrawPolygon();

  int StartPosition[2];
  int EndPosition[2];
  int Moving;

  bool DrawPolygonPixels;

  vtkUnsignedCharArray* PixelArray;

private:
  vtkInteractorStyleDrawPolygon(const vtkInteractorStyleDrawPolygon&) = delete;
  void operator=(const vtkInteractorStyleDrawPolygon&) = delete;

  class vtkInternal;
  vtkInternal* Internal;
};

VTK_ABI_NAMESPACE_END
#endif
