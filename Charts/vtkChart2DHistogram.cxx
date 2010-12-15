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

#include "vtkChart2DHistogram.h"

#include "vtkContext2D.h"
#include "vtkBrush.h"
#include "vtkPen.h"
#include "vtkContextScene.h"
#include "vtkContextMouseEvent.h"
#include "vtkTextProperty.h"
#include "vtkAxis.h"
#include "vtk2DHistogramItem.h"
#include "vtkColorLegend.h"
#include "vtkSmartPointer.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkChart2DHistogram);

//-----------------------------------------------------------------------------
vtkChart2DHistogram::vtkChart2DHistogram()
{
  // Now for the 2D histogram
  this->Histogram = vtkSmartPointer<vtk2DHistogramItem>::New();
  this->AddPlot(this->Histogram);

  this->Legend = vtkSmartPointer<vtkColorLegend>::New();
  this->AddItem(this->Legend);
}

//-----------------------------------------------------------------------------
vtkChart2DHistogram::~vtkChart2DHistogram()
{
}

//-----------------------------------------------------------------------------
void vtkChart2DHistogram::Update()
{
  this->Histogram->Update();
  this->Legend->Update();
  this->vtkChartXY::Update();
}

void vtkChart2DHistogram::SetInput(vtkImageData *data, vtkIdType z)
{
  this->Histogram->SetInput(data, z);
}

void vtkChart2DHistogram::SetTransferFunction(vtkScalarsToColors *function)
{
  this->Histogram->SetTransferFunction(function);
  this->Legend->SetTransferFunction(function);
}

//-----------------------------------------------------------------------------
bool vtkChart2DHistogram::UpdateLayout(vtkContext2D *painter)
{
  this->vtkChartXY::UpdateLayout(painter);
  this->Legend->SetPosition(vtkRectf(this->Point2[0], this->Point1[1],
                                     10, this->Point2[1] - this->Point1[1]));
  this->Legend->Update();
  return true;
}

//-----------------------------------------------------------------------------
bool vtkChart2DHistogram::Hit(const vtkContextMouseEvent &mouse)
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
void vtkChart2DHistogram::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
