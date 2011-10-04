/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScatterPlotMatrix.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkScatterPlotMatrix - container for a matrix of charts.
//
// .SECTION Description
// This class contains a matrix of charts. These charts will be of type
// vtkChartXY by default, but this can be overridden. The class will manage
// their layout and object lifetime.

#ifndef __vtkScatterPlotMatrix_h
#define __vtkScatterPlotMatrix_h

#include "vtkChartMatrix.h"
#include "vtkSmartPointer.h" // For ivars
#include "vtkNew.h" // For ivars

class vtkStringArray;
class vtkTable;
class vtkAxis;

class VTK_CHARTS_EXPORT vtkScatterPlotMatrix : public vtkChartMatrix
{
public:
  vtkTypeMacro(vtkScatterPlotMatrix, vtkChartMatrix);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a new object.
  static vtkScatterPlotMatrix *New();

  // Description:
  // Perform any updates to the item that may be necessary before rendering.
  virtual void Update();

  // Description:
  // Paint event for the chart matrix.
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Set the active plot, the one that will be displayed in the top-right.
  // This defaults to (0, n-2), the plot below the first histogram on the left.
  // \return false is the position specified is not valid.
  virtual bool SetActivePlot(const vtkVector2i& position);

  // Description:
  // Get the position of the active plot.
  virtual vtkVector2i GetActivePlot();

  // Description:
  // Set the input table for the scatter plot matrix. This will cause all
  // columns to be plotted against each other - a square scatter plot matrix.
  virtual void SetInput(vtkTable *table);

  // Description:
  // Set the visibility of the specified column.
  void SetColumnVisibility(const vtkStdString& name, bool visible);

  // Description:
  // Get the visibility of the specified column.
  bool GetColumnVisibility(const vtkStdString& name);

  // Description:
  // Set the visibility of all columns (true will make them all visible, false
  // will remove all visible columns).
  void SetColumnVisibilityAll(bool visible);

  // Description:
  // Get a list of the columns, and the order in which they are displayed.
  virtual vtkStringArray* GetVisibleColumns();

protected:
  vtkScatterPlotMatrix();
  ~vtkScatterPlotMatrix();

  // Description:
  // Internal helper to do the layout of the charts in the scatter plot matrix.
  void UpdateLayout();

  // Description:
  // Attach axis range listener so we can forward to dependent axes in matrix.
  void AttachAxisRangeListener(vtkAxis*);
  void AxisRangeForwarderCallback(vtkObject*, unsigned long, void*);

  class PIMPL;
  PIMPL *Private;

  // The position of the active plot (defaults to 0, 1).
  vtkVector2i ActivePlot;

  // Weakly owned input data for the scatter plot matrix.
  vtkSmartPointer<vtkTable> Input;
  // Strongly owned internal data for the column visibility.
  vtkNew<vtkStringArray> VisibleColumns;

private:
  vtkScatterPlotMatrix(const vtkScatterPlotMatrix &); // Not implemented.
  void operator=(const vtkScatterPlotMatrix &); // Not implemented.
};

#endif //__vtkScatterPlotMatrix_h
