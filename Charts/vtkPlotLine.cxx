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

vtkCxxRevisionMacro(vtkPlotLine, "1.14");

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotLine);

//-----------------------------------------------------------------------------
vtkPlotLine::vtkPlotLine()
{
  this->Points = 0;
  this->MarkerStyle = vtkPlotLine::NONE;
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
bool vtkPlotLine::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called in vtkPlotLine.");

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
  // Get the x and y arrays (index 0 and 1 respectively)
  vtkTable *table = this->Data->GetInput();
  vtkDataArray *x = this->Data->GetInputArrayToProcess(0, table);
  vtkDataArray *y = this->Data->GetInputArrayToProcess(1, table);

  if (this->UseIndexForXSeries && y)
    {
    bounds[0] = 0;
    bounds[1] = y->GetSize();
    y->GetRange(&bounds[2]);
    }
  else if (x && y)
    {
    x->GetRange(&bounds[0]);
    y->GetRange(&bounds[2]);
    }
  vtkDebugMacro(<< "Bounds: " << bounds[0] << "\t" << bounds[1] << "\t"
                << bounds[2] << "\t" << bounds[3]);
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
  for (int i = 0; i < n; ++i)
    {
    points->SetPoint(i, a[i], b[i]);
    }
}

// Copy one array into the points array, use the index of that array as x
template<class A>
void CopyToPoints(vtkPoints2D *points, A *a, int n)
{
  points->SetNumberOfPoints(n);
  for (int i = 0; i < n; ++i)
    {
    points->SetPoint(i, i, a[i]);
    }
}

}

//-----------------------------------------------------------------------------
bool vtkPlotLine::UpdateTableCache(vtkTable *table)
{
  // Get the x and y arrays (index 0 and 1 respectively)
  vtkDataArray* x = this->Data->GetInputArrayToProcess(0, table);
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
  else if (x->GetSize() != y->GetSize() && !this->UseIndexForXSeries)
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
  this->BuildTime.Modified();
  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotLine::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
