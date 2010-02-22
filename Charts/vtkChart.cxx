/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChart.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkChart.h"

#include "vtkAnnotationLink.h"
#include "vtkContextScene.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkChart, "1.11");
vtkCxxSetObjectMacro(vtkChart, AnnotationLink, vtkAnnotationLink);

//-----------------------------------------------------------------------------
vtkChart::vtkChart()
{
  this->Geometry[0] = 0;
  this->Geometry[1] = 0;
  this->Point1[0] = 0;
  this->Point1[1] = 0;
  this->Point2[0] = 0;
  this->Point2[1] = 0;
  this->ShowLegend = false;
  this->AnnotationLink = NULL;
}

//-----------------------------------------------------------------------------
vtkChart::~vtkChart()
{
}

//-----------------------------------------------------------------------------
vtkPlot * vtkChart::AddPlot(int)
{
  return NULL;
}

//-----------------------------------------------------------------------------
bool vtkChart::RemovePlot(vtkIdType)
{
  return false;
}

//-----------------------------------------------------------------------------
void vtkChart::ClearPlots()
{
}

//-----------------------------------------------------------------------------
vtkPlot* vtkChart::GetPlot(vtkIdType)
{
  return NULL;
}

//-----------------------------------------------------------------------------
vtkIdType vtkChart::GetNumberPlots()
{
  return 0;
}

//-----------------------------------------------------------------------------
vtkAxis* vtkChart::GetAxis(int)
{
  return NULL;
}

//-----------------------------------------------------------------------------
void vtkChart::RecalculateBounds()
{
  return;
}

//-----------------------------------------------------------------------------
void vtkChart::SetBottomBorder(int border)
{
  this->Point1[1] = border >= 0 ? border : 0;
}

//-----------------------------------------------------------------------------
void vtkChart::SetTopBorder(int border)
{
 this->Point2[1] = border >=0 ?
                   this->Geometry[1] - border :
                   this->Geometry[1];
}

//-----------------------------------------------------------------------------
void vtkChart::SetLeftBorder(int border)
{
  this->Point1[0] = border >= 0 ? border : 0;
}

//-----------------------------------------------------------------------------
void vtkChart::SetRightBorder(int border)
{
  this->Point2[0] = border >=0 ?
                    this->Geometry[0] - border :
                    this->Geometry[0];
}

//-----------------------------------------------------------------------------
void vtkChart::SetBorders(int left, int right, int top, int bottom)
{
  this->SetLeftBorder(left);
  this->SetRightBorder(right);
  this->SetTopBorder(top);
  this->SetBottomBorder(bottom);
}

//-----------------------------------------------------------------------------
void vtkChart::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  // Print out the chart's geometry if it has been set
  os << indent << "Point1: " << this->Point1[0] << "\t" << this->Point1[1]
     << endl;
  os << indent << "Point2: " << this->Point2[0] << "\t" << this->Point2[1]
     << endl;
  os << indent << "Width: " << this->Geometry[0] << endl
     << indent << "Height: " << this->Geometry[1] << endl;
}
