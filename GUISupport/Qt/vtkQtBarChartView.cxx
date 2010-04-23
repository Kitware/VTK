/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtBarChartView.cxx

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

#include "vtkQtBarChartView.h"

#include "vtkObjectFactory.h"
#include "vtkQtBarChart.h"
#include "vtkQtBarChartOptions.h"
#include "vtkQtChartArea.h"
#include "vtkQtChartHelpFormatter.h"
#include "vtkQtChartMouseSelection.h"
#include "vtkQtChartSeriesModelCollection.h"
#include "vtkQtChartSeriesOptionsModelCollection.h"
#include "vtkQtChartSeriesSelectionHandler.h"
#include "vtkQtChartWidget.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkQtBarChartView);

//----------------------------------------------------------------------------
vtkQtBarChartView::vtkQtBarChartView()
{
  // Get the chart widget from the base class.
  vtkQtChartWidget* chart = qobject_cast<vtkQtChartWidget*>(this->GetWidget());
  vtkQtChartArea* area = chart->getChartArea();

  // Create the bar chart and model. Add them to the chart between the
  // grid and axis layers.
  this->BarChart = new vtkQtBarChart();
  this->BarModel = new vtkQtChartSeriesModelCollection(this->BarChart);
  this->BarChart->setModel(this->BarModel);
  this->BarChart->setOptionsModel(this->GetChartOptionsModel());
  area->insertLayer(area->getAxisLayerIndex(), this->BarChart);
}

//----------------------------------------------------------------------------
vtkQtBarChartView::~vtkQtBarChartView()
{

}

//----------------------------------------------------------------------------
void vtkQtBarChartView::Update()
{
  this->Superclass::Update();
}

//----------------------------------------------------------------------------
void vtkQtBarChartView::SetHelpFormat(const char* format)
{
  this->BarChart->getOptions()->getHelpFormat()->setFormat(QString(format));
}

//----------------------------------------------------------------------------
void vtkQtBarChartView::SetOutlineStyle(int outline)
{
  this->BarChart->getOptions()->setOutlineStyle(
    (vtkQtBarChartOptions::OutlineStyle)outline);
}

//----------------------------------------------------------------------------
void vtkQtBarChartView::SetBarGroupFraction(float fraction)
{
  this->BarChart->getOptions()->setBarGroupFraction(fraction);
}

//----------------------------------------------------------------------------
void vtkQtBarChartView::SetBarWidthFraction(float fraction)
{
  this->BarChart->getOptions()->setBarWidthFraction(fraction);
}

//----------------------------------------------------------------------------
void vtkQtBarChartView::AddChartSelectionHandlers(
  vtkQtChartMouseSelection* selector)
{
  vtkQtChartSeriesSelectionHandler *handler =
      new vtkQtChartSeriesSelectionHandler(selector);
  handler->setModeNames("Bar Chart - Series", "Bar Chart - Bars");
  handler->setMousePressModifiers(Qt::ControlModifier, Qt::ControlModifier);
  handler->setLayer(this->BarChart);
  selector->addHandler(handler);
  selector->setSelectionMode("Bar Chart - Bars");
}

//----------------------------------------------------------------------------
vtkQtChartSeriesModelCollection* vtkQtBarChartView::GetChartSeriesModel()
{
  return this->BarModel;
}

//----------------------------------------------------------------------------
vtkQtChartSeriesOptions* vtkQtBarChartView::GetChartSeriesOptions(int idx)
{
  return this->BarChart->getSeriesOptions(idx);
}

//----------------------------------------------------------------------------
vtkQtChartSeriesLayer* vtkQtBarChartView::GetChartSeriesLayer()
{
  return this->BarChart;
}

//----------------------------------------------------------------------------
void vtkQtBarChartView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
