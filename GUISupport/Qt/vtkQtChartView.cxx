/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartView.cxx

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

#include "vtkQtChartView.h"

#include "vtkQtChartArea.h"
#include "vtkObjectFactory.h"

#include <QGraphicsScene>
#include <QGraphicsView>

#include "vtkQtChartRepresentation.h"

vtkCxxRevisionMacro(vtkQtChartView, "1.1");
vtkStandardNewMacro(vtkQtChartView);
//----------------------------------------------------------------------------
vtkQtChartView::vtkQtChartView()
{
  // Create an internal view
  // This can be overwritten 
  // through the API.
  this->ChartView = new vtkQtChartArea();
  this->ChartView->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);  
  this->IOwnChartView = true;  
}

//----------------------------------------------------------------------------
vtkQtChartView::~vtkQtChartView()
{
  if (this->IOwnChartView)
    {
    delete this->ChartView;
    }
}

//----------------------------------------------------------------------------
void vtkQtChartView::Update()
{
  for (int i = 0; i < this->GetNumberOfRepresentations(); ++i)
    {
    vtkQtChartRepresentation* rep = vtkQtChartRepresentation::SafeDownCast(this->GetRepresentation(i));
    if (rep)
      {
      rep->Update();
      }
    }
  
  this->ChartView->update();
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetChartView(vtkQtChartArea *qgv)
{
  // Transfer the scene
//  QGraphicsScene* scene = this->ChartView->scene();
//  qgv->setScene(scene);

  // Delete my internal one
  if (this->IOwnChartView)
    {
    delete this->ChartView;
    this->IOwnChartView = false;
    }
  
  // Set up my internals to point to the new view
  this->ChartView = qgv;
}

//----------------------------------------------------------------------------
void vtkQtChartView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

