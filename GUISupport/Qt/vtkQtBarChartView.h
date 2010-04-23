/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtBarChartView.h

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
// .NAME vtkQtBarChartView - Wraps a vtkQtChartArea into a VTK view.
//
// .SECTION Description
// vtkQtBarChartView is a type vtkQtChartView designed for line charts.
//
// .SECTION See Also
// vtkQtChartView

#ifndef __vtkQtBarChartView_h
#define __vtkQtBarChartView_h

#include "QVTKWin32Header.h"
#include "vtkQtChartView.h"

class vtkQtBarChart;
class vtkQtChartSeriesModelCollection;
class vtkQtChartSeriesOptions;

class QVTK_EXPORT vtkQtBarChartView : public vtkQtChartView
{
Q_OBJECT

public:
  static vtkQtBarChartView *New();
  vtkTypeMacro(vtkQtBarChartView, vtkQtChartView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Updates the view.
  virtual void Update();

  // Description:
  // Sets the bar chart help format.
  void SetHelpFormat(const char* format);

  // Description:
  // Sets the bar outline style.
  void SetOutlineStyle(int outline);

  // Description:
  // Sets the bar group width fraction.
  void SetBarGroupFraction(float fraction);

  // Description:
  // Sets the bar width fraction.
  void SetBarWidthFraction(float fraction);

  //BTX
  // Description:
  // Adds bar chart selection handlers to the mouse selection.
  virtual void AddChartSelectionHandlers(vtkQtChartMouseSelection* selector);

  // Description:
  // Gets the bar chart series model.
  virtual vtkQtChartSeriesModelCollection* GetChartSeriesModel();

  // Description:
  // Gets the chart series layer
  virtual vtkQtChartSeriesLayer* GetChartSeriesLayer();
  //ETX

  // Description:
  // Gets the series options.
  virtual vtkQtChartSeriesOptions* GetChartSeriesOptions(int series);

protected:
  vtkQtBarChartView();
  ~vtkQtBarChartView();

  vtkQtBarChart *BarChart;
  vtkQtChartSeriesModelCollection *BarModel;

private:
  vtkQtBarChartView(const vtkQtBarChartView&);  // Not implemented.
  void operator=(const vtkQtBarChartView&);  // Not implemented.
};

#endif
