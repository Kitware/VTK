/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartView.h

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
// .NAME vtkQtChartView - Wraps a vtkQtChartArea into a VTK view.
//
// .SECTION Description
// vtkQtChartView is a vtkView which wraps an instance of vtkQtChartArea.
// This view expects vtkQtChartRepresentation instances as its representation.
//
// .SECTION See Also
// vtkQtChartRepresentation

#ifndef __vtkQtChartView_h
#define __vtkQtChartView_h

#include "QVTKWin32Header.h"
#include "vtkQtView.h"
#include <QPointer>

class vtkQtChartArea;
class vtkQtChartAxis;
class vtkQtChartLegend;
class vtkQtChartMouseSelection;
class vtkQtChartSeriesLayer;
class vtkQtChartSeriesModelCollection;
class vtkQtChartSeriesOptions;
class vtkQtChartSeriesOptionsModelCollection;
class vtkQtChartWidget;
class vtkTable;

class QVTK_EXPORT vtkQtChartView : public vtkQtView
{
Q_OBJECT

public:
  vtkTypeMacro(vtkQtChartView, vtkQtView);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get the main container of this view (a  QWidget).
  // The application typically places the view with a call
  // to GetWidget(): something like this
  // this->ui->box->layout()->addWidget(this->View->GetWidget());
  virtual QWidget* GetWidget();

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

  // Description:
  // Sets the best fir range for the given axis.
  void SetAxisRange(int index, double minimum, double maximum);

  // Description:
  // Sets the best fir range for the given axis.
  void SetAxisRange(int index, int minimum, int maximum);

  //BTX
  // Description:
  // Adds chart layer selection handlers to the mouse selection.
  virtual void AddChartSelectionHandlers(vtkQtChartMouseSelection* selector);

  // Description:
  // Gets the chart area from the chart widget.  This method is equivalent
  // to GetChartWidget()->getChartArea().
  vtkQtChartArea* GetChartArea();

  // Description:
  // Gets the chart axis for the given index.
  vtkQtChartAxis* GetAxis(int index);

  // Description:
  // Gets the chart series model.
  virtual vtkQtChartSeriesModelCollection* GetChartSeriesModel()=0;

  // Description:
  // Gets the series options model.
  virtual vtkQtChartSeriesOptionsModelCollection* GetChartOptionsModel();

  // Description:
  // Gets the series options.
  virtual vtkQtChartSeriesOptions* GetChartSeriesOptions(int series) = 0;

  // Gets the chart series layer
  virtual vtkQtChartSeriesLayer* GetChartSeriesLayer()=0;

  // Description:
  // Gets the chart legend widget.
  vtkQtChartLegend* GetLegend();
  //ETX

  // Description:
  // Sets up the default interactor.
  virtual void SetupDefaultInteractor();

  // Description:
  // Set color scheme methods
  void SetColorSchemeToSpectrum();
  void SetColorSchemeToWarm();
  void SetColorSchemeToCool();
  void SetColorSchemeToBlues();
  void SetColorSchemeToWildFlower();
  void SetColorSchemeToCitrus();

protected:
  vtkQtChartView();
  ~vtkQtChartView();

  // Description:
  // Create a vtkQtChartRepresentation for the given input connection.
  virtual vtkDataRepresentation* CreateDefaultRepresentation(vtkAlgorithmOutput* conn);

private:
  //BTX
  class vtkInternal;
  vtkInternal* Internal;
  //ETX

private:
  vtkQtChartView(const vtkQtChartView&);  // Not implemented.
  void operator=(const vtkQtChartView&);  // Not implemented.
};

#endif
