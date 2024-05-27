// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkPlotLine3D
 * @brief   Class for drawing an XYZ line plot given three columns from
 * a vtkTable.
 *
 *
 * This class draws points with a line between them given three column from a
 * vtkTable in a vtkChartXYZ.
 *
 * @sa
 * vtkPlotPoints3D
 * vtkPlotLine
 *
 */

#ifndef vtkPlotLine3D_h
#define vtkPlotLine3D_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkPlotPoints3D.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkPlotLine3D : public vtkPlotPoints3D
{
public:
  vtkTypeMacro(vtkPlotLine3D, vtkPlotPoints3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates a 3D Chart object.
   */
  static vtkPlotLine3D* New();

  /**
   * Paint event for the XYZ plot, called whenever the chart needs to be drawn.
   */
  bool Paint(vtkContext2D* painter) override;

protected:
  vtkPlotLine3D();
  ~vtkPlotLine3D() override;

private:
  vtkPlotLine3D(const vtkPlotLine3D&) = delete;
  void operator=(const vtkPlotLine3D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkPlotLine3D_h
