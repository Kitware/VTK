/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotLine.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPlotLine.h"

#include "vtkContext2D.h"
#include "vtkAxis.h"
#include "vtkPen.h"
#include "vtkFloatArray.h"
#include "vtkVector.h"
#include "vtkTransform2D.h"
#include "vtkContextDevice2D.h"
#include "vtkContextMapper2D.h"
#include "vtkPoints2D.h"
#include "vtkTable.h"
#include "vtkDataArray.h"
#include "vtkIdTypeArray.h"
#include "vtkExecutive.h"
#include "vtkTimeStamp.h"
#include "vtkInformation.h"

#include "vtkObjectFactory.h"

#include "vtkstd/vector"
#include "vtkstd/algorithm"

vtkCxxRevisionMacro(vtkPlotLine, "1.18");

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotLine);

//-----------------------------------------------------------------------------
vtkPlotLine::vtkPlotLine()
{
  this->Points = 0;
  this->MarkerStyle = vtkPlotLine::NONE;
  this->LogX = false;
  this->LogY = false;
}

//-----------------------------------------------------------------------------
vtkPlotLine::~vtkPlotLine()
{
  if (this->Points)
    {
    this->Points->Delete();
    this->Points = NULL;
    }
}

//-----------------------------------------------------------------------------
void vtkPlotLine::Update()
{
  if (!this->Visible)
    {
    return;
    }
  // Check if we have an input
  vtkTable *table = this->Data->GetInput();
  if (!table)
    {
    vtkDebugMacro(<< "Update event called with no input table set.");
    return;
    }
  else if(this->Data->GetMTime() > this->BuildTime ||
          table->GetMTime() > this->BuildTime ||
          this->MTime > this->BuildTime)
    {
    vtkDebugMacro(<< "Updating cached values.");
    this->UpdateTableCache(table);
    }
  else if ((this->XAxis && this->XAxis->GetMTime() > this->BuildTime) ||
           (this->YAxis && this->YAxis->GetMaximum() > this->BuildTime))
    {
    if (this->LogX != this->XAxis->GetLogScale() ||
        this->LogY != this->YAxis->GetLogScale())
      {
      this->UpdateTableCache(table);
      }
    }
}

//-----------------------------------------------------------------------------
bool vtkPlotLine::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called in vtkPlotLine.");

  if (!this->Visible)
    {
    return false;
    }

  // Now add some decorations for our selected points...
  if (this->Selection)
    {
    vtkDebugMacro(<<"Selection set " << this->Selection->GetNumberOfTuples());
    for (int i = 0; i < this->Selection->GetNumberOfTuples(); ++i)
      {
      painter->ApplyPen(this->Pen);
      painter->GetPen()->SetWidth(this->Pen->GetWidth()*15.0);
      vtkIdType id = 0;
      this->Selection->GetTupleValue(i, &id);
      if (id < this->Points->GetNumberOfPoints())
        {
        double *point = this->Points->GetPoint(id);
        painter->DrawPoint(point[0], point[1]);
        }
      }
    }
  else
    {
    vtkDebugMacro("No selection set.");
    }

  // Now to plot the points
  if (this->Points)
    {
    painter->ApplyPen(this->Pen);
    painter->DrawPoly(this->Points);
    painter->GetPen()->SetLineType(vtkPen::SOLID_LINE);
    }
  // If there is a marker style, then draw the marker for each point too
  if (this->MarkerStyle)
    {
    painter->ApplyBrush(this->Brush);
    painter->GetPen()->SetWidth(1.0);
    float radiusX = (this->Pen->GetWidth() / 2.0f) + 4.0;
    float radiusY = radiusX;
    // Figure out what this is in pixel space
    vtkTransform2D *transform = painter->GetTransform();
    if (transform)
      {
      radiusX /= transform->GetMatrix()->GetElement(0, 0);
      radiusY /= transform->GetMatrix()->GetElement(1, 1);
      }
    int n = this->Points->GetNumberOfPoints();
    float *f = vtkFloatArray::SafeDownCast(this->Points->GetData())->GetPointer(0);
    vtkVector2f *pts = reinterpret_cast<vtkVector2f*>(f);
    switch (this->MarkerStyle)
      {
      case vtkPlotLine::CROSS:
        {
        for (int i = 0; i < n; ++i)
          {
          painter->DrawLine(pts[i].X()+radiusX, pts[i].Y()+radiusY,
                            pts[i].X()-radiusX, pts[i].Y()-radiusY);
          painter->DrawLine(pts[i].X()+radiusX, pts[i].Y()-radiusY,
                            pts[i].X()-radiusX, pts[i].Y()+radiusY);
          }
        break;
        }
      case vtkPlotLine::PLUS:
        {
        for (int i = 0; i < n; ++i)
          {
          painter->DrawLine(pts[i].X()-radiusX, pts[i].Y(),
                            pts[i].X()+radiusX, pts[i].Y());
          painter->DrawLine(pts[i].X(), pts[i].Y()+radiusY,
                            pts[i].X(), pts[i].Y()-radiusY);
          }
        break;
        }
      case vtkPlotLine::SQUARE:
        {
        float widthX = 2.0*radiusX;
        float widthY = 2.0*radiusY;
        for (int i = 0; i < n; ++i)
          {
          painter->DrawRect(pts[i].X()-radiusX, pts[i].Y()-radiusY,
                            widthX, widthY);
          }
        break;
        }
      case vtkPlotLine::CIRCLE:
        {
        painter->GetPen()->SetWidth(this->Pen->GetWidth()+5.0);
        painter->DrawPoints(f, n);
        break;
        }
      case vtkPlotLine::DIAMOND:
        {
        for (int i = 0; i < n; ++i)
          {
          painter->DrawQuad(pts[i].X()-radiusX, pts[i].Y(),
                            pts[i].X(), pts[i].Y()+radiusY,
                            pts[i].X()+radiusX, pts[i].Y(),
                            pts[i].X(), pts[i].Y()-radiusY);
          }
        break;
        }
      }
    }

  return true;
}

//-----------------------------------------------------------------------------
bool vtkPlotLine::PaintLegend(vtkContext2D *painter, float rect[4])
{
  painter->ApplyPen(this->Pen);
  painter->DrawLine(rect[0], rect[1]+0.5*rect[3],
                    rect[0]+rect[2], rect[1]+0.5*rect[3]);
  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotLine::GetBounds(double bounds[4])
{
  this->Points->GetBounds(bounds);
  vtkDebugMacro(<< "Bounds: " << bounds[0] << "\t" << bounds[1] << "\t"
                << bounds[2] << "\t" << bounds[3]);
}

namespace
{

// Compare the two vectors, in X component only
bool compVector2fX(const vtkVector2f& v1, const vtkVector2f& v2)
{
  if (v1.X() < v2.X())
    {
    return true;
    }
  else
    {
    return false;
    }
}

// See if the point is within tolerance.
bool inRange(const vtkVector2f& point, const vtkVector2f& tol,
             const vtkVector2f& current)
{
  if (current.X() > point.X() - tol.X() && current.X() < point.X() + tol.X() &&
      current.Y() > point.Y() - tol.Y() && current.Y() < point.Y() + tol.Y())
    {
    return true;
    }
  else
    {
    return false;
    }
}

}

//-----------------------------------------------------------------------------
bool vtkPlotLine::GetNearestPoint(const vtkVector2f& point,
                                  const vtkVector2f& tol,
                                  vtkVector2f* location)
{
  // Right now doing a simple bisector search of the array. This should be
  // revisited. Assumes the x axis is sorted, which should always be true for
  // line plots.
  if (!this->Points)
    {
    return false;
    }
  vtkIdType n = this->Points->GetNumberOfPoints();
  if (n < 2)
    {
    return false;
    }

  // Set up our search array, use the STL lower_bound algorithm
  vtkVector2f* data = static_cast<vtkVector2f*>(this->Points->GetVoidPointer(0));
  vtkstd::vector<vtkVector2f> v(data, data+n);
  vtkstd::vector<vtkVector2f>::iterator low;

  // Get the lowest point we might hit within the supplied tolerance
  vtkVector2f lowPoint(point.X()-tol.X(), 0.0f);
  low = vtkstd::lower_bound(v.begin(), v.end(), lowPoint, compVector2fX);

  // Now consider the y axis
  float highX = point.X() + tol.X();
  while (low != v.end())
    {
    if (inRange(point, tol, *low))
      {
      *location = *low;
      return true;
      }
    else if (low->X() > highX)
      {
      break;
      }
    ++low;
    }
  return false;
}

//-----------------------------------------------------------------------------
namespace {

// Copy the two arrays into the points array
template<class A>
void CopyToPointsSwitch(vtkPoints2D *points, A *a, vtkDataArray *b, int n)
{
  switch(b->GetDataType())
    {
    vtkTemplateMacro(
        CopyToPoints(points, a, static_cast<VTK_TT*>(b->GetVoidPointer(0)), n));
    }
}

// Copy the two arrays into the points array
template<class A, class B>
void CopyToPoints(vtkPoints2D *points, A *a, B *b, int n)
{
  points->SetNumberOfPoints(n);
  float* data = static_cast<float*>(points->GetVoidPointer(0));
  for (int i = 0; i < n; ++i)
    {
    data[2*i] = a[i];
    data[2*i+1] = b[i];
    }
}

// Copy one array into the points array, use the index of that array as x
template<class A>
void CopyToPoints(vtkPoints2D *points, A *a, int n)
{
  points->SetNumberOfPoints(n);
  float* data = static_cast<float*>(points->GetVoidPointer(0));
  for (int i = 0; i < n; ++i)
    {
    data[2*i] = static_cast<float>(i);
    data[2*i+1] = a[i];
    }
}

}

//-----------------------------------------------------------------------------
bool vtkPlotLine::UpdateTableCache(vtkTable *table)
{
  // Get the x and y arrays (index 0 and 1 respectively)
  vtkDataArray* x = this->UseIndexForXSeries ?
                    0 : this->Data->GetInputArrayToProcess(0, table);
  vtkDataArray* y = this->Data->GetInputArrayToProcess(1, table);
  if (!x && !this->UseIndexForXSeries)
    {
    vtkErrorMacro(<< "No X column is set (index 0).");
    return false;
    }
  else if (!y)
    {
    vtkErrorMacro(<< "No Y column is set (index 1).");
    return false;
    }
  else if (!this->UseIndexForXSeries && x->GetSize() != y->GetSize())
    {
    vtkErrorMacro("The x and y columns must have the same number of elements.");
    return false;
    }

  if (!this->Points)
    {
    this->Points = vtkPoints2D::New();
    }

  // Now copy the components into their new columns
  if (this->UseIndexForXSeries)
    {
    switch(y->GetDataType())
      {
        vtkTemplateMacro(
            CopyToPoints(this->Points,
                         static_cast<VTK_TT*>(y->GetVoidPointer(0)),
                         y->GetSize()));
      }
    }
  else
    {
    switch(x->GetDataType())
      {
      vtkTemplateMacro(
          CopyToPointsSwitch(this->Points,
                             static_cast<VTK_TT*>(x->GetVoidPointer(0)),
                             y, x->GetSize()));
      }
    }
  this->CalculateLogSeries();
  this->Points->Modified();
  this->BuildTime.Modified();
  return true;
}

inline void vtkPlotLine::CalculateLogSeries()
{
  if (!this->XAxis || !this->YAxis)
    {
    return;
    }
  this->LogX = this->XAxis->GetLogScale();
  this->LogY = this->YAxis->GetLogScale();
  float* data = static_cast<float*>(this->Points->GetVoidPointer(0));
  vtkIdType n = this->Points->GetNumberOfPoints();
  if (this->LogX)
    {
    for (vtkIdType i = 0; i < n; ++i)
      {
      data[2*i] = log10(data[2*i]);
      }
    }
  if (this->LogY)
  {
  for (vtkIdType i = 0; i < n; ++i)
    {
    data[2*i+1] = log10(data[2*i+1]);
    }
  }
}

//-----------------------------------------------------------------------------
void vtkPlotLine::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
