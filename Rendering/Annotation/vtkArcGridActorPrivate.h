// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkArcGridActorPrivate
 * @brief   renders a concentric list of arcs on overlay.
 *
 * vtkArcGridActorPrivate is designed for use by vtkPolarAxes2D, to draw
 * the arcs between the axes.
 *
 * Starting at an orignal axis points, it draws arcs around the given center
 * until a given Angle.
 * The number of points for each axes is defined by Resolution.
 */

#ifndef vtkArcGridActorPrivate_h
#define vtkArcGridActorPrivate_h

#include "vtkActor2D.h"

#include "vtkNew.h" // for vtkNew

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkDoubleArray;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkPolyDataMapper2D;

class vtkArcGridActorPrivate : public vtkActor2D
{
public:
  vtkTypeMacro(vtkArcGridActorPrivate, vtkActor2D);
  static vtkArcGridActorPrivate* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the point positions used to draw arcs.
   * Arcs are drawn from StartPoints as portion of circles
   * centered at Center and with Resolution points.
   */
  vtkSetMacro(TicksStart, vtkPoints*);

  ///@{
  /**
   * Set/Get the angle of the arcs, in degree.
   * Default is 90.
   */
  vtkSetMacro(Angle, double);
  vtkGetMacro(Angle, double);
  ///@}

  ///@{
  /**
   * Set/Get the center of the circle in normalized viewport coordinates.
   * Default is {0.5, 0.5}
   */
  vtkSetVector2Macro(Center, double);
  vtkGetVector2Macro(Center, double);
  ///@}

  ///@{
  /**
   * Set/Get the resolution of the arcs, i.e. the number
   * of points per arc.
   * Default is 10.
   */
  vtkSetMacro(Resolution, int);
  vtkGetMacro(Resolution, int);
  ///@}

  /**
   * Build the arcs and render them as overlay.
   */
  int RenderOverlay(vtkViewport*) override;

protected:
  vtkArcGridActorPrivate();
  ~vtkArcGridActorPrivate() override;

private:
  void operator=(const vtkArcGridActorPrivate&) = delete;
  vtkArcGridActorPrivate(const vtkArcGridActorPrivate&) = delete;

  /**
   * Return true if it has data to draw.
   */
  bool HasData();

  /**
   * Build the arcs from TicksStart to TicksEnd around Center.
   * Each arc has a Resolution number of points.
   */
  void BuildGrid(vtkViewport* viewport);

  vtkNew<vtkPolyData> PolyData;
  vtkNew<vtkPolyDataMapper2D> GridMapper;

  vtkPoints* TicksStart = nullptr;

  double Center[2] = { 0.5, 0.5 };
  double Angle = 90;
  int Resolution = 10;
};

VTK_ABI_NAMESPACE_END
#endif
