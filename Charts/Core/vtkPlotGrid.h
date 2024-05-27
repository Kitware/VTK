// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkPlotGrid
 * @brief   takes care of drawing the plot grid
 *
 *
 * The vtkPlotGrid is drawn in screen coordinates. It is usually one of the
 * first elements of a chart to be drawn, and will generally be obscured
 * by all other elements of the chart. It builds up its own plot locations
 * from the parameters of the x and y axis of the plot.
 */

#ifndef vtkPlotGrid_h
#define vtkPlotGrid_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkContextItem.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkContext2D;
class vtkPoints2D;
class vtkAxis;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkPlotGrid : public vtkContextItem
{
public:
  vtkTypeMacro(vtkPlotGrid, vtkContextItem);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates a 2D Chart object.
   */
  static vtkPlotGrid* New();

  /**
   * Set the X axis of the grid.
   */
  virtual void SetXAxis(vtkAxis* axis);

  /**
   * Set the X axis of the grid.
   */
  virtual void SetYAxis(vtkAxis* axis);

  /**
   * Paint event for the axis, called whenever the axis needs to be drawn
   */
  bool Paint(vtkContext2D* painter) override;

protected:
  vtkPlotGrid();
  ~vtkPlotGrid() override;

  ///@{
  /**
   * The vtkAxis objects are used to figure out where the grid lines should be
   * drawn.
   */
  vtkAxis* XAxis;
  vtkAxis* YAxis;
  ///@}

private:
  vtkPlotGrid(const vtkPlotGrid&) = delete;
  void operator=(const vtkPlotGrid&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkPlotGrid_h
