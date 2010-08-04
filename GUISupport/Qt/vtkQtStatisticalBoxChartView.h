/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtStatisticalBoxChartView.h

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

#ifndef __vtkQtStatisticalBoxChartView_h
#define __vtkQtStatisticalBoxChartView_h

#include "QVTKWin32Header.h"
#include "vtkQtChartView.h"
#include <QPointer>

class vtkQtStatisticalBoxChart;
class vtkQtChartSeriesModelCollection;
class vtkQtChartSeriesOptions;

class QVTK_EXPORT vtkQtStatisticalBoxChartView : public vtkQtChartView
{
Q_OBJECT

public:
  static vtkQtStatisticalBoxChartView *New();
  vtkTypeMacro(vtkQtStatisticalBoxChartView, vtkQtChartView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Updates the view.
  virtual void Update();

  // Description:
  // Sets the box help format.
  void SetHelpFormat(const char* format);

  // Description:
  // Sets the outlier help format.
  void SetOutlierFormat(const char* format);

  // Description:
  // Sets the box outline style.
  void SetOutlineStyle(int outline);

  // Description:
  // Sets the box width fraction.
  void SetBoxWidthFraction(float fraction);

  // Description:
  // Adds box chart selection handlers to the mouse selection.
  virtual void AddChartSelectionHandlers(vtkQtChartMouseSelection* selector);

  // Description:
  // Gets the statistical box chart series model.
  virtual vtkQtChartSeriesModelCollection* GetChartSeriesModel();

  // Description:
  // Gets the chart series layer
  virtual vtkQtChartSeriesLayer* GetChartSeriesLayer();

  // Description:
  // Gets the statistical box chart series options.
  virtual vtkQtChartSeriesOptions* GetChartSeriesOptions(int series);

protected:
  vtkQtStatisticalBoxChartView();
  ~vtkQtStatisticalBoxChartView();

protected:
  vtkQtStatisticalBoxChart *BoxChart;
  vtkQtChartSeriesModelCollection *BoxModel;

private:
  vtkQtStatisticalBoxChartView(const vtkQtStatisticalBoxChartView&);  // Not implemented.
  void operator=(const vtkQtStatisticalBoxChartView&);  // Not implemented.
};

#endif
