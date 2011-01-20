/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChart2DHistogram.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkChartHistogram2D.h"

#include "vtkContext2D.h"
#include "vtkBrush.h"
#include "vtkPen.h"
#include "vtkContextScene.h"
#include "vtkContextMouseEvent.h"
#include "vtkTextProperty.h"
#include "vtkAxis.h"
#include "vtkPlotHistogram2D.h"
#include "vtkColorLegend.h"
#include "vtkSmartPointer.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkChartHistogram2D);

//-----------------------------------------------------------------------------
vtkChartHistogram2D::vtkChartHistogram2D()
{
  // Now for the 2D histogram
  this->Histogram = vtkSmartPointer<vtkPlotHistogram2D>::New();
  this->AddPlot(this->Histogram);

  this->RemoveItem(this->Legend);
  this->Legend = vtkSmartPointer<vtkColorLegend>::New();
  this->AddItem(this->Legend);
}

//-----------------------------------------------------------------------------
vtkChartHistogram2D::~vtkChartHistogram2D()
{
}

//-----------------------------------------------------------------------------
void vtkChartHistogram2D::Update()
{
  this->Histogram->Update();
  this->Legend->Update();
  this->vtkChartXY::Update();
}

void vtkChartHistogram2D::SetInput(vtkImageData *data, vtkIdType z)
{
  this->Histogram->SetInput(data, z);
}

void vtkChartHistogram2D::SetTransferFunction(vtkScalarsToColors *function)
{
  this->Histogram->SetTransferFunction(function);
  vtkColorLegend *legend = vtkColorLegend::SafeDownCast(this->Legend);
  if (legend)
    {
    legend->SetTransferFunction(function);
    }
}

//-----------------------------------------------------------------------------
bool vtkChartHistogram2D::UpdateLayout(vtkContext2D *painter)
{
  this->vtkChartXY::UpdateLayout(painter);
  vtkColorLegend *legend = vtkColorLegend::SafeDownCast(this->Legend);
  if (legend)
    {
    legend->SetPosition(vtkRectf(this->Point2[0] + 5, this->Point1[1],
                                 this->Legend->GetSymbolWidth(),
                                 this->Point2[1] - this->Point1[1]));
    }
  this->Legend->Update();
  return true;
}

//-----------------------------------------------------------------------------
bool vtkChartHistogram2D::Hit(const vtkContextMouseEvent &mouse)
{
  if (mouse.ScreenPos[0] > this->Point1[0]-10 &&
      mouse.ScreenPos[0] < this->Point2[0]+10 &&
      mouse.ScreenPos[1] > this->Point1[1] &&
      mouse.ScreenPos[1] < this->Point2[1])
    {
    return true;
    }
  else
    {
    return false;
    }
}

//-----------------------------------------------------------------------------
void vtkChartHistogram2D::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
