/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtLineChartView.cxx

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

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtLineChartView.h"

#include "vtkQtChartArea.h"
#include "vtkQtChartMouseSelection.h"
#include "vtkQtChartSeriesModelCollection.h"
#include "vtkQtChartSeriesSelectionHandler.h"
#include "vtkQtChartWidget.h"
#include "vtkQtLineChart.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkQtLineChartView, "1.4");
vtkStandardNewMacro(vtkQtLineChartView);

//----------------------------------------------------------------------------
vtkQtLineChartView::vtkQtLineChartView()
{
  // Get the chart widget from the base class.
  vtkQtChartWidget* chart = this->GetChartWidget();
  vtkQtChartArea* area = chart->getChartArea();

  // Create the line chart and model. Add the line chart on top of the
  // axis layer.
  this->LineChart = new vtkQtLineChart();
  this->LineModel = new vtkQtChartSeriesModelCollection(this->LineChart);
  this->LineChart->setModel(this->LineModel);
  area->addLayer(this->LineChart);

  // TEMP
  this->SetupDefaultInteractor();
}

//----------------------------------------------------------------------------
vtkQtLineChartView::~vtkQtLineChartView()
{
}

//----------------------------------------------------------------------------
void vtkQtLineChartView::Update()
{
  this->Superclass::Update();
}

//----------------------------------------------------------------------------
void vtkQtLineChartView::AddChartSelectionHandlers(
  vtkQtChartMouseSelection* selector)
{
  vtkQtChartSeriesSelectionHandler *handler =
      new vtkQtChartSeriesSelectionHandler(selector);
  handler->setModeNames("Line Chart - Series", "Line Chart - Points");
  handler->setMousePressModifiers(Qt::ControlModifier, Qt::ControlModifier);
  handler->setLayer(this->LineChart);
  selector->addHandler(handler);
  selector->setSelectionMode("Line Chart - Series");
}

//----------------------------------------------------------------------------
vtkQtChartSeriesModelCollection* vtkQtLineChartView::GetChartSeriesModel()
{
  return this->LineModel;
}

//----------------------------------------------------------------------------
void vtkQtLineChartView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
