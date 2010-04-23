/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtLineChartView.h

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
// .NAME vtkQtLineChartView - Wraps a vtkQtChartArea into a VTK view.
//
// .SECTION Description
// vtkQtLineChartView is a type vtkQtChartView designed for line charts.
//
// .SECTION See Also
// vtkQtChartView

#ifndef __vtkQtLineChartView_h
#define __vtkQtLineChartView_h

#include "QVTKWin32Header.h"
#include "vtkQtChartView.h"

class vtkQtLineChart;
class vtkQtChartSeriesModelCollection;
class vtkQtChartSeriesOptions;

class QVTK_EXPORT vtkQtLineChartView : public vtkQtChartView
{
Q_OBJECT

public:
  static vtkQtLineChartView *New();
  vtkTypeMacro(vtkQtLineChartView, vtkQtChartView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Updates the view.
  virtual void Update();

  // Description:
  // Sets the bar chart help format.
  void SetHelpFormat(const char* format);

  //BTX
  // Description:
  // Adds line chart selection handlers to the mouse selection.
  virtual void AddChartSelectionHandlers(vtkQtChartMouseSelection* selector);

  // Description:
  // Gets the line chart series model.
  virtual vtkQtChartSeriesModelCollection* GetChartSeriesModel();

  // Description:
  // Gets the chart series layer
  virtual vtkQtChartSeriesLayer* GetChartSeriesLayer();
  //ETX

  // Description:
  // Gets the series options.
  virtual vtkQtChartSeriesOptions* GetChartSeriesOptions(int series);

protected:
  vtkQtLineChartView();
  ~vtkQtLineChartView();

  vtkQtLineChart *LineChart;
  vtkQtChartSeriesModelCollection *LineModel;

private:
  vtkQtLineChartView(const vtkQtLineChartView&);  // Not implemented.
  void operator=(const vtkQtLineChartView&);  // Not implemented.
};

#endif
