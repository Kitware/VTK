/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtStackedChartView.h

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

#ifndef _vtkQtStackedChartView_h
#define _vtkQtStackedChartView_h

#include "QVTKWin32Header.h"
#include "vtkQtChartView.h"
#include <QPointer>

class vtkQtStackedChart;
class vtkQtChartSeriesModelCollection;
class vtkQtChartSeriesOptions;

class QVTK_EXPORT vtkQtStackedChartView : public vtkQtChartView
{
Q_OBJECT

public:
  static vtkQtStackedChartView *New();
  vtkTypeMacro(vtkQtStackedChartView, vtkQtChartView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Updates the view.
  virtual void Update();

  // Description:
  // Sets the stacked chart help format.
  void SetHelpFormat(const char* format);

  // Description:
  // Sets whether or not the stacked chart sumation is normalized.
  void SetSumNormalized(bool normalized);

  // Description:
  // Sets whether or not the stacked chart is drawn with a gradient.
  void SetGradientDisplayed(bool gradient);

  //BTX
  // Description:
  // Adds stacked chart selection handlers to the mouse selection.
  virtual void AddChartSelectionHandlers(vtkQtChartMouseSelection* selector);

  // Description:
  // Gets the chart series layer
  virtual vtkQtChartSeriesLayer* GetChartSeriesLayer();

  // Description:
  // Gets the stacked chart series model.
  virtual vtkQtChartSeriesModelCollection* GetChartSeriesModel();

  // Description:
  // Gets the stacked chart series options.
  virtual vtkQtChartSeriesOptions* GetChartSeriesOptions(int series);
  //ETX

protected:
  vtkQtStackedChartView();
  ~vtkQtStackedChartView();

protected:
  vtkQtStackedChart *StackedChart;
  vtkQtChartSeriesModelCollection *StackedModel;

private:
  vtkQtStackedChartView(const vtkQtStackedChartView&);  // Not implemented.
  void operator=(const vtkQtStackedChartView&);  // Not implemented.
};

#endif
