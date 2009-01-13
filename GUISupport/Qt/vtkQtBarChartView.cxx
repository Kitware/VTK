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

#include "vtkQtBarChart.h"
#include "vtkQtBarChartView.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkQtBarChartView, "1.1");
vtkStandardNewMacro(vtkQtBarChartView);

//----------------------------------------------------------------------------
vtkQtBarChartView::vtkQtBarChartView()
{
  // Set the chart layer to a bar chart layer.
  // Note, after this call the ownership of the chart layer
  // is transferred to the chart widget's vtkQtChartArea.
  vtkQtChartSeriesLayer* barChartLayer = new vtkQtBarChart;
  this->SetChartLayer(barChartLayer);
  this->Initialize();
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
void vtkQtBarChartView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
