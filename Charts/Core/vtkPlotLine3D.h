/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotLine3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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

class VTKCHARTSCORE_EXPORT vtkPlotLine3D : public vtkPlotPoints3D
{
public:
  vtkTypeMacro(vtkPlotLine3D, vtkPlotPoints3D);
  void PrintSelf(ostream &os, vtkIndent indent) override;

  /**
   * Creates a 3D Chart object.
   */
  static vtkPlotLine3D *New();

  /**
   * Paint event for the XYZ plot, called whenever the chart needs to be drawn.
   */
  bool Paint(vtkContext2D *painter) override;

protected:
  vtkPlotLine3D();
  ~vtkPlotLine3D() override;

private:
  vtkPlotLine3D(const vtkPlotLine3D &) = delete;
  void operator=(const vtkPlotLine3D &) = delete;

};

#endif //vtkPlotLine3D_h
