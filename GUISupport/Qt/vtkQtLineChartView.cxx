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

#include "vtkQtLineChart.h"
#include "vtkQtLineChartView.h"
#include "vtkQtChartLegendModel.h"
#include "vtkQtChartSeriesOptions.h"
#include "vtkQtChartSeriesModelCollection.h"
#include "vtkObjectFactory.h"

#include <QPen>

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkQtLineChartView, "1.3");
vtkStandardNewMacro(vtkQtLineChartView);

//----------------------------------------------------------------------------
vtkQtLineChartView::vtkQtLineChartView()
{
  // Set the chart layer to a line chart layer.
  // Note, after this call the ownership of the chart layer
  // is transferred to the chart widget's vtkQtChartArea.
  vtkQtChartSeriesLayer* lineChartLayer = new vtkQtLineChart;
  this->SetChartLayer(lineChartLayer);
  this->Initialize();
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
void vtkQtLineChartView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
