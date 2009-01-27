/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartViewBase.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkQtChartViewBase - Wraps a vtkQtChartArea into a VTK view.
//
// .SECTION Description
// vtkQtChartViewBase is a vtkView which wraps an instance of vtkQtChartArea.
// This view expects vtkQtChartTableRepresentation instances as its representation.
//
// .SECTION See Also
// vtkQtChartTableRepresentation

#ifndef __vtkQtChartViewBase_h
#define __vtkQtChartViewBase_h

#include "QVTKWin32Header.h"
#include "vtkView.h"

class vtkQtChartArea;
class vtkQtChartAxis;
class vtkQtChartLegend;
class vtkQtChartMouseSelection;
class vtkQtChartSeriesModelCollection;
class vtkQtChartWidget;
class vtkTable;

class QVTK_EXPORT vtkQtChartViewBase : public vtkView
{
public:
  static vtkQtChartViewBase *New();
  vtkTypeRevisionMacro(vtkQtChartViewBase, vtkView);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Updates the view.
  virtual void Update();

  // Description:
  // Updates representations and then repaints the chart widget.
  virtual void Render();

  // Description:
  // Calls show() on the chart widget.
  void Show();

  // Description:
  // A convenience method to add a table to the chart view.
  // This is here for now to work around a python wrapping bug.
  void AddTableToView(vtkTable* table);

  // Description:
  // Set the chart's title.
  void SetTitle(const char* title);

  // Description:
  // Set the chart title's font.
  void SetTitleFont(const char* family, int pointSize, bool bold, bool italic);

  // Description:
  // Set the chart title's color.
  void SetTitleColor(double red, double green, double blue);

  // Description:
  // Set the chart title's alignment.
  void SetTitleAlignment(int alignment);

  // Description:
  // Set the chart axis title for the given index.
  void SetAxisTitle(int index, const char* title);

  // Description:
  // Set the chart axis title's font for the given index.
  void SetAxisTitleFont(int index, const char* family, int pointSize,
    bool bold, bool italic);

  // Description:
  // Set the chart axis title's color for the given index.
  void SetAxisTitleColor(int index, double red, double green, double blue);

  // Description:
  // Set the chart axis title's alignment for the given index.
  void SetAxisTitleAlignment(int index, int alignment);

  // Description:
  // Sets whether or not the chart legend is visible.
  void SetLegendVisibility(bool visible);

  // Description:
  // Sets the legend location.
  void SetLegendLocation(int location);

  // Description:
  // Sets the legend flow.
  void SetLegendFlow(int flow);

  // Description:
  // Sets whether or not the given axis is visible.
  void SetAxisVisibility(int index, bool visible);

  // Description:
  // Sets the color for the given axis.
  void SetAxisColor(int index, double red, double green, double blue);

  // Description:
  // Sets whether or not the grid for the given axis is visible.
  void SetGridVisibility(int index, bool visible);

  // Description:
  // Sets the grid color type for the given axis.
  void SetGridColorType(int index, int gridColorType);

  // Description:
  // Sets the grid color for the given axis.
  void SetGridColor(int index, double red, double green, double blue);

  // Description:
  // Sets whether or not the labels for the given axis are visible.
  void SetAxisLabelVisibility(int index, bool visible);

  // Description:
  // Set the axis label font for the given axis.
  void SetAxisLabelFont(int index, const char* family, int pointSize,
    bool bold, bool italic);

  // Description:
  // Sets the axis label color for the given axis.
  void SetAxisLabelColor(int index, double red, double green, double blue);

  // Description:
  // Sets the axis label notation for the given axis.
  void SetAxisLabelNotation(int index, int notation);

  // Description:
  // Sets the axis label precision for the given axis.
  void SetAxisLabelPrecision(int index, int precision);

  // Description:
  // Sets the scale for the given axis (Linear or Logarithmic).
  void SetAxisScale(int index, int scale);

  // Description:
  // Sets the behavior for the given axis.
  void SetAxisBehavior(int index, int behavior);

  void SetAxisRange(int index, double minimum, double maximum);

  void SetAxisRange(int index, int minimum, int maximum);

  //BTX
  // Description:
  // Adds chart layer selection handlers to the mouse selection.
  virtual void AddChartSelectionHandlers(vtkQtChartMouseSelection* selector);

  // Description:
  // Gets the chart widget, this is the main widget to display.
  vtkQtChartWidget* GetChartWidget();

  // Description:
  // Gets the chart area from the chart widget.  This method is equivalent
  // to GetChartWidget()->getChartArea().
  vtkQtChartArea* GetChartArea();

  // Description:
  // Gets the chart axis for the given index.
  vtkQtChartAxis* GetAxis(int index);

  // Description:
  // Gets the chart series model.
  virtual vtkQtChartSeriesModelCollection* GetChartSeriesModel();

  // Description:
  // Gets the chart legend widget.
  vtkQtChartLegend* GetLegend();
  //ETX
  
protected:
  // Description:
  // Called from Initialize() to setup the default interactor
  virtual void SetupDefaultInteractor();

  // Description:
  // Create a vtkQtChartRepresentation for the given input connection.
  virtual vtkDataRepresentation* CreateDefaultRepresentation(vtkAlgorithmOutput* conn);

  vtkQtChartViewBase();
  ~vtkQtChartViewBase();

private:
  //BTX
  class vtkInternal;
  vtkInternal* Internal;
  //ETX
  
private:
  vtkQtChartViewBase(const vtkQtChartViewBase&);  // Not implemented.
  void operator=(const vtkQtChartViewBase&);  // Not implemented.
};

#endif
