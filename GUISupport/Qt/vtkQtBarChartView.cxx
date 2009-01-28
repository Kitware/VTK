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
#include "vtkQtChartArea.h"
#include "vtkQtChartMouseSelection.h"
#include "vtkQtChartSeriesModelCollection.h"
#include "vtkQtChartSeriesSelectionHandler.h"
#include "vtkQtChartWidget.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkQtBarChartView, "1.3");
vtkStandardNewMacro(vtkQtBarChartView);

//----------------------------------------------------------------------------
vtkQtBarChartView::vtkQtBarChartView()
{
  // Get the chart widget from the base class.
  vtkQtChartWidget* chart = this->GetChartWidget();
  vtkQtChartArea* area = chart->getChartArea();

  // Create the bar chart and model. Add them to the chart between the
  // grid and axis layers.
  this->BarChart = new vtkQtBarChart();
  this->BarModel = new vtkQtChartSeriesModelCollection(this->BarChart);
  this->BarChart->setModel(this->BarModel);
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
void vtkQtBarChartView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
