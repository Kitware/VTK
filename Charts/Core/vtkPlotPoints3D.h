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

// .NAME vtkPlotPoints3D - 3D scatter plot.
//
// .SECTION Description
// 3D scatter plot.
//
// .SECTION See Also
// vtkPlotLine3D
// vtkPlotPoints
//

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

  // Description:
  // Paint event for the XY plot, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);

//BTX
protected:
  vtkPlotPoints3D();
  ~vtkPlotPoints3D();

  // Description:
  // The selected points.
  std::vector<vtkVector3f> SelectedPoints;

  // Description:
  // The selected points.
  vtkTimeStamp SelectedPointsBuildTime;

private:
  vtkPlotPoints3D(const vtkPlotPoints3D &); // Not implemented.
  void operator=(const vtkPlotPoints3D &); // Not implemented.
//ETX

};

#endif //vtkPlotPoints3D_h
