// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkPlotPoints3D
 * @brief   3D scatter plot.
 *
 *
 * 3D scatter plot.
 *
 * @sa
 * vtkPlotLine3D
 * vtkPlotPoints
 *
 */

#ifndef vtkPlotPoints3D_h
#define vtkPlotPoints3D_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkPlot3D.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkContext2D;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkPlotPoints3D : public vtkPlot3D
{
public:
  vtkTypeMacro(vtkPlotPoints3D, vtkPlot3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkPlotPoints3D* New();

  /**
   * Paint event for the XY plot, called whenever the chart needs to be drawn
   */
  bool Paint(vtkContext2D* painter) override;

protected:
  vtkPlotPoints3D();
  ~vtkPlotPoints3D() override;

  void ReleaseGraphicsCache() override;

  /**
   * The selected points.
   */
  vtkNew<vtkPoints> SelectedPoints;

  /**
   * The selected points.
   */
  vtkTimeStamp SelectedPointsBuildTime;

private:
  vtkPlotPoints3D(const vtkPlotPoints3D&) = delete;
  void operator=(const vtkPlotPoints3D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkPlotPoints3D_h
