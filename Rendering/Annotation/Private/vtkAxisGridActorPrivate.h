// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAxisGridActorPrivate
 * @brief   renders a 2D grid given pairs of point positions
 *
 * vtkAxisGridActorPrivate is designed for use by vtkLegendScaleActor to render
 * the wireframe for the grid plane. It takes the position of ticks on each of the
 * 4 axes of vtkLegendScaleActor and draws lines between them, taking into account
 * that parallel axes give ticks in opposite order.
 */

#ifndef vtkAxisGridActorPrivate_h
#define vtkAxisGridActorPrivate_h

#include "vtkActor2D.h"

#include "vtkNew.h" // for vtkNew

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkDoubleArray;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkPolyDataMapper2D;

class vtkAxisGridActorPrivate : public vtkActor2D
{
public:
  vtkTypeMacro(vtkAxisGridActorPrivate, vtkActor2D);
  static vtkAxisGridActorPrivate* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the point positions used to draw horizontal and vertical lines.
   * Lines will be drawn between the left & right positions for both axis.
   * For a given axis, the number of points must be the same for left and end positions.
   * Note that Left & Right as well as Top & Bottom point lists should give tick positions in
   * opposite order.
   */
  void SetHorizontalLinesLeftPoints(vtkPoints* points);
  void SetHorizontalLinesRightPoints(vtkPoints* points);
  void SetVerticalLinesTopPoints(vtkPoints* points);
  void SetVerticalLinesBottomPoints(vtkPoints* points);
  ///@}

  /**
   * Build grid and render
   */
  int RenderOpaqueGeometry(vtkViewport*) override;

protected:
  vtkAxisGridActorPrivate();
  ~vtkAxisGridActorPrivate() override;

private:
  vtkNew<vtkPolyData> PolyData;
  vtkNew<vtkPoints> PolyDataPoints;
  vtkNew<vtkCellArray> PolyDataLines;
  vtkNew<vtkPolyDataMapper2D> GridMapper;

  vtkPoints* XTicksStart = nullptr;
  vtkPoints* YTicksStart = nullptr;
  vtkPoints* XTicksEnd = nullptr;
  vtkPoints* YTicksEnd = nullptr;

  /**
   * Build the grid lines by filling the polydata points and lines
   * from specified ticks positions.
   */
  void BuildGrid();

  void operator=(const vtkAxisGridActorPrivate&) = delete;
  vtkAxisGridActorPrivate(const vtkAxisGridActorPrivate&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
