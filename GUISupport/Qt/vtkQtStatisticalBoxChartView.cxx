/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtStatisticalBoxChartView.cxx

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

#include "vtkQtStatisticalBoxChartView.h"

#include "vtkObjectFactory.h"
#include "vtkQtChartArea.h"
#include "vtkQtChartHelpFormatter.h"
#include "vtkQtChartMouseSelection.h"
#include "vtkQtChartSeriesModelCollection.h"
#include "vtkQtChartSeriesOptionsModelCollection.h"
#include "vtkQtChartSeriesSelectionHandler.h"
#include "vtkQtChartWidget.h"
#include "vtkQtStatisticalBoxChart.h"
#include "vtkQtStatisticalBoxChartOptions.h"


//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkQtStatisticalBoxChartView);

//-----------------------------------------------------------------------------
vtkQtStatisticalBoxChartView::vtkQtStatisticalBoxChartView()
{
  // Get the chart area from the base class.
  vtkQtChartArea* area = this->GetChartArea();

  // Create the bar chart and model. Add them to the chart between the
  // grid and axis layers.
  this->BoxChart = new vtkQtStatisticalBoxChart();
  this->BoxModel = new vtkQtChartSeriesModelCollection(this->BoxChart);
  this->BoxChart->setModel(this->BoxModel);
  this->BoxChart->setOptionsModel(this->GetChartOptionsModel());
  area->insertLayer(area->getAxisLayerIndex(), this->BoxChart);
}

//-----------------------------------------------------------------------------
vtkQtStatisticalBoxChartView::~vtkQtStatisticalBoxChartView()
{
}

//-----------------------------------------------------------------------------
void vtkQtStatisticalBoxChartView::Update()
{
  this->Superclass::Update();
}

//-----------------------------------------------------------------------------
void vtkQtStatisticalBoxChartView::SetHelpFormat(const char* format)
{
  this->BoxChart->getOptions()->getHelpFormat()->setFormat(QString(format));
}

//-----------------------------------------------------------------------------
void vtkQtStatisticalBoxChartView::SetOutlierFormat(const char* format)
{
  this->BoxChart->getOptions()->getOutlierFormat()->setFormat(QString(format));
}

//-----------------------------------------------------------------------------
void vtkQtStatisticalBoxChartView::SetOutlineStyle(int outline)
{
  this->BoxChart->getOptions()->setOutlineStyle(
    (vtkQtStatisticalBoxChartOptions::OutlineStyle)outline);
}

//-----------------------------------------------------------------------------
void vtkQtStatisticalBoxChartView::SetBoxWidthFraction(float fraction)
{
  this->BoxChart->getOptions()->setBoxWidthFraction(fraction);
}

//-----------------------------------------------------------------------------
void vtkQtStatisticalBoxChartView::AddChartSelectionHandlers(
  vtkQtChartMouseSelection* selector)
{
  vtkQtChartSeriesSelectionHandler *handler =
      new vtkQtChartSeriesSelectionHandler(selector);
  handler->setModeNames("Box Chart - Series", "Box Chart - Outliers");
  handler->setMousePressModifiers(Qt::ControlModifier, Qt::ControlModifier);
  handler->setLayer(this->BoxChart);
  selector->addHandler(handler);
  selector->setSelectionMode("Box Chart - Series");
}

//-----------------------------------------------------------------------------
vtkQtChartSeriesModelCollection*
vtkQtStatisticalBoxChartView::GetChartSeriesModel()
{
  return this->BoxModel;
}

//-----------------------------------------------------------------------------
vtkQtChartSeriesLayer* vtkQtStatisticalBoxChartView::GetChartSeriesLayer()
{
  return this->BoxChart;
}

//-----------------------------------------------------------------------------
vtkQtChartSeriesOptions* vtkQtStatisticalBoxChartView::GetChartSeriesOptions(
  int series)
{
  return this->BoxChart->getSeriesOptions(series);
}

//-----------------------------------------------------------------------------
void vtkQtStatisticalBoxChartView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
