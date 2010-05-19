/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtStackedChartView.cxx

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

#include "vtkQtStackedChartView.h"

#include "vtkObjectFactory.h"
#include "vtkQtChartArea.h"
#include "vtkQtChartHelpFormatter.h"
#include "vtkQtChartMouseSelection.h"
#include "vtkQtChartSeriesModelCollection.h"
#include "vtkQtChartSeriesOptionsModelCollection.h"
#include "vtkQtChartSeriesSelectionHandler.h"
#include "vtkQtChartWidget.h"
#include "vtkQtStackedChart.h"
#include "vtkQtStackedChartOptions.h"


//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkQtStackedChartView);

//-----------------------------------------------------------------------------
vtkQtStackedChartView::vtkQtStackedChartView()
{
  // Get the chart area from the base class.
  vtkQtChartArea* area = this->GetChartArea();

  // Create the bar chart and model. Add them to the chart between the
  // grid and axis layers.
  this->StackedChart = new vtkQtStackedChart();
  this->StackedModel = new vtkQtChartSeriesModelCollection(this->StackedChart);
  this->StackedChart->setModel(this->StackedModel);
  this->StackedChart->setOptionsModel(this->GetChartOptionsModel());
  area->insertLayer(area->getAxisLayerIndex(), this->StackedChart);
}

//-----------------------------------------------------------------------------
vtkQtStackedChartView::~vtkQtStackedChartView()
{
}

//-----------------------------------------------------------------------------
void vtkQtStackedChartView::Update()
{
  this->Superclass::Update();
}

//-----------------------------------------------------------------------------
void vtkQtStackedChartView::SetHelpFormat(const char* format)
{
  this->StackedChart->getOptions()->getHelpFormat()->setFormat(
    QString(format));
}

//-----------------------------------------------------------------------------
void vtkQtStackedChartView::SetSumNormalized(bool normalized)
{
  this->StackedChart->getOptions()->setSumNormalized(normalized);
}

//-----------------------------------------------------------------------------
void vtkQtStackedChartView::SetGradientDisplayed(bool gradient)
{
  this->StackedChart->getOptions()->setGradientDisplayed(gradient);
}

//-----------------------------------------------------------------------------
void vtkQtStackedChartView::AddChartSelectionHandlers(
  vtkQtChartMouseSelection* selector)
{
  vtkQtChartSeriesSelectionHandler* handler =
      new vtkQtChartSeriesSelectionHandler(selector);
  handler->setModeNames("Stacked Chart - Series", "Stacked Chart - Points");
  handler->setMousePressModifiers(Qt::ControlModifier, Qt::ControlModifier);
  handler->setLayer(this->StackedChart);
  selector->addHandler(handler);
  selector->setSelectionMode("Stacked Chart - Series");
}

//-----------------------------------------------------------------------------
vtkQtChartSeriesModelCollection* vtkQtStackedChartView::GetChartSeriesModel()
{
  return this->StackedModel;
}

//-----------------------------------------------------------------------------
vtkQtChartSeriesLayer* vtkQtStackedChartView::GetChartSeriesLayer()
{
  return this->StackedChart;
}

//-----------------------------------------------------------------------------
vtkQtChartSeriesOptions* vtkQtStackedChartView::GetChartSeriesOptions(int series)
{
  return this->StackedChart->getSeriesOptions(series);
}

//-----------------------------------------------------------------------------
void vtkQtStackedChartView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


