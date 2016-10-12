/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotPoints3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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

class vtkContext2D;

class VTKCHARTSCORE_EXPORT vtkPlotPoints3D : public vtkPlot3D
{
public:
  vtkTypeMacro(vtkPlotPoints3D, vtkPlot3D);
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  static vtkPlotPoints3D * New();

  /**
   * Paint event for the XY plot, called whenever the chart needs to be drawn
   */
  virtual bool Paint(vtkContext2D *painter);

protected:
  vtkPlotPoints3D();
  ~vtkPlotPoints3D();

  /**
   * The selected points.
   */
  std::vector<vtkVector3f> SelectedPoints;

  /**
   * The selected points.
   */
  vtkTimeStamp SelectedPointsBuildTime;

private:
  vtkPlotPoints3D(const vtkPlotPoints3D &) VTK_DELETE_FUNCTION;
  void operator=(const vtkPlotPoints3D &) VTK_DELETE_FUNCTION;

};

#endif //vtkPlotPoints3D_h
