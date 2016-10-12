/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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

class vtkStdString;
class vtkContext2D;
class vtkPoints2D;
class vtkAxis;

class VTKCHARTSCORE_EXPORT vtkPlotGrid : public vtkContextItem
{
public:
  vtkTypeMacro(vtkPlotGrid, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  /**
   * Creates a 2D Chart object.
   */
  static vtkPlotGrid *New();

  /**
   * Set the X axis of the grid.
   */
  virtual void SetXAxis(vtkAxis *axis);

  /**
   * Set the X axis of the grid.
   */
  virtual void SetYAxis(vtkAxis *axis);

  /**
   * Paint event for the axis, called whenever the axis needs to be drawn
   */
  virtual bool Paint(vtkContext2D *painter);

protected:
  vtkPlotGrid();
  ~vtkPlotGrid();

  //@{
  /**
   * The vtkAxis objects are used to figure out where the grid lines should be
   * drawn.
   */
  vtkAxis *XAxis;
  vtkAxis *YAxis;
  //@}

private:
  vtkPlotGrid(const vtkPlotGrid &) VTK_DELETE_FUNCTION;
  void operator=(const vtkPlotGrid &) VTK_DELETE_FUNCTION;

};

#endif //vtkPlotGrid_h
