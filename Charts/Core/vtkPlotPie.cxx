/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotPie.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPlotPie.h"

#include "vtkContext2D.h"
#include "vtkPen.h"
#include "vtkBrush.h"
#include "vtkColorSeries.h"
#include "vtkPoints2D.h"
#include "vtkContextMapper2D.h"
#include "vtkTable.h"
#include "vtkMath.h"
#include "vtkRect.h"

#include "vtkObjectFactory.h"

#include <algorithm>

namespace {

template<class A>
A SumData(A *a, int n)
{
  A sum = 0;
  for (int i = 0; i < n; ++i)
    {
    sum += a[i];
    }
  return sum;
}

template<class A>
void CopyToPoints(vtkPoints2D *points, A *a, int n)
{
  points->SetNumberOfPoints(n);

  A sum = SumData(a,n);
  float* data = static_cast<float*>(points->GetVoidPointer(0));
  float startAngle = 0.0;

  for (int i = 0; i < n; ++i)
    {
    data[2*i] = startAngle;
    data[2*i+1] = startAngle + ((static_cast<float>(a[i]) / sum) * 360.0);
    startAngle = data[2*i+1];
    }
}
}

class vtkPlotPiePrivate
{
  public:
    vtkPlotPiePrivate()
      {
      this->CenterX = 0;
      this->CenterY = 0;
      this->Radius  = 0;
      }

  float CenterX;
  float CenterY;
  float Radius;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotPie);

//-----------------------------------------------------------------------------
vtkPlotPie::vtkPlotPie()
{
  this->ColorSeries = vtkSmartPointer<vtkColorSeries>::New();
  this->Points = 0;
  this->Private = new vtkPlotPiePrivate();
  this->Dimensions[0] = this->Dimensions[1] = this->Dimensions[2] =
    this->Dimensions[3] = 0;
}

//-----------------------------------------------------------------------------
vtkPlotPie::~vtkPlotPie()
{
  delete this->Private;
  if (this->Points)
    {
    this->Points->Delete();
    this->Points = 0;
    }
  this->Private = 0;
}

//-----------------------------------------------------------------------------
bool vtkPlotPie::Paint(vtkContext2D *painter)
{
  if (!this->Visible)
    {
    return false;
    }

  // First check if we have an input
  vtkTable *table = this->Data->GetInput();
  if (!table)
    {
    vtkDebugMacro(<< "Paint event called with no input table set.");
    return false;
    }
  else if(this->Data->GetMTime() > this->BuildTime ||
          table->GetMTime() > this->BuildTime ||
          this->MTime > this->BuildTime)
    {
    vtkDebugMacro(<< "Paint event called with outdated table cache. Updating.");
    this->UpdateTableCache(table);
    }

  float* data = static_cast<float*>(this->Points->GetVoidPointer(0));

  for (int i = 0; i < this->Points->GetNumberOfPoints(); ++i)
    {
    painter->GetBrush()
        ->SetColor(this->ColorSeries->GetColorRepeating(i).GetData());

    painter->DrawEllipseWedge(this->Private->CenterX, this->Private->CenterY,
                              this->Private->Radius, this->Private->Radius,
                              0.0, 0.0,
                              data[2*i], data[2*i+1]
                              );
    }

  this->PaintChildren(painter);
  return true;
}

//-----------------------------------------------------------------------------

bool vtkPlotPie::PaintLegend(vtkContext2D *painter, const vtkRectf& rect,
                             int legendIndex)
{
  if (this->ColorSeries)
    this->Brush
      ->SetColor(this->ColorSeries->GetColorRepeating(legendIndex).GetData());

  painter->ApplyPen(this->Pen);
  painter->ApplyBrush(this->Brush);
  painter->DrawRect(rect[0], rect[1], rect[2], rect[3]);
  return true;
}

//-----------------------------------------------------------------------------

void vtkPlotPie::SetDimensions(int arg1, int arg2, int arg3, int arg4)
{
  if (arg1 != this->Dimensions[0] || arg2 != this->Dimensions[1] ||
      arg3 != this->Dimensions[2] || arg4 != this->Dimensions[3])
    {
    this->Dimensions[0] = arg1;
    this->Dimensions[1] = arg2;
    this->Dimensions[2] = arg3;
    this->Dimensions[3] = arg4;

    this->Private->CenterX = this->Dimensions[0] + 0.5 * this->Dimensions[2];
    this->Private->CenterY = this->Dimensions[1] + 0.5 * this->Dimensions[3];
    this->Private->Radius  = this->Dimensions[2] < this->Dimensions[3]
        ? 0.5 * this->Dimensions[2] : 0.5 * this->Dimensions[3];
    this->Modified();
    }
}

void vtkPlotPie::SetDimensions(int arg[4])
{
  this->SetDimensions(arg[0],arg[1],arg[2],arg[3]);
}


//-----------------------------------------------------------------------------
void vtkPlotPie::SetColorSeries(vtkColorSeries *colorSeries)
{
  if (this->ColorSeries == colorSeries)
    {
    return;
    }
  this->ColorSeries = colorSeries;
  this->Modified();
}

//-----------------------------------------------------------------------------
vtkColorSeries *vtkPlotPie::GetColorSeries()
{
  return this->ColorSeries;
}

//-----------------------------------------------------------------------------
vtkIdType vtkPlotPie::GetNearestPoint(const vtkVector2f& point,
                                      const vtkVector2f&,
                                      vtkVector2f* value)
{
  float x = point.GetX() - this->Private->CenterX;
  float y = point.GetY() - this->Private->CenterY;

  if (sqrt((x*x) + (y*y)) <= this->Private->Radius)
    {
    float *angles = static_cast<float *>(this->Points->GetVoidPointer(0));
    float pointAngle = vtkMath::DegreesFromRadians(atan2(y,x));
    if (pointAngle < 0)
      {
      pointAngle = 180.0 + (180.0 + pointAngle);
      }
    float *lbound = std::lower_bound(angles,
                                     angles + (this->Points->GetNumberOfPoints() * 2),
                                     pointAngle);
    // Location in the array
    int ret = lbound - angles;
    // There are two of each angle in the array (start,end for each point)
    ret = ret / 2;

    vtkTable *table = this->Data->GetInput();
    vtkDataArray* data = this->Data->GetInputArrayToProcess(0, table);
    value->SetX(ret);
    value->SetY(data->GetTuple1(ret));
    return ret;
    }

  return -1;
}

//-----------------------------------------------------------------------------
void vtkPlotPie::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
bool vtkPlotPie::UpdateTableCache(vtkTable *table)
{
  // Get the x and y arrays (index 0 and 1 respectively)
  vtkDataArray* data = this->Data->GetInputArrayToProcess(0, table);

  if (!data)
    {
    vtkErrorMacro(<< "No data set (index 0).");
    return false;
    }

  if (!this->Points)
    {
    this->Points = vtkPoints2D::New();
    }


  switch (data->GetDataType())
    {
    vtkTemplateMacro(
      CopyToPoints(this->Points,
                   static_cast<VTK_TT*>(data->GetVoidPointer(0)),
                   data->GetNumberOfTuples()));
    }

  this->BuildTime.Modified();
  return true;
}
