/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotBar.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPlotBar.h"

#include "vtkContext2D.h"
#include "vtkPen.h"
#include "vtkBrush.h"
#include "vtkContextDevice2D.h"
#include "vtkContextMapper2D.h"
#include "vtkPoints2D.h"
#include "vtkTable.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkExecutive.h"
#include "vtkTimeStamp.h"
#include "vtkInformation.h"

#include "vtkObjectFactory.h"

#include "vtkstd/vector"
#include "vtkstd/algorithm"


//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotBar);

//-----------------------------------------------------------------------------
vtkPlotBar::vtkPlotBar()
{
  this->Points = 0;
  this->Sorted = false;
  this->Label = 0;
  this->Width = 1.0;
  this->Pen->SetWidth(1.0);
  this->Offset = 1.0;
}

//-----------------------------------------------------------------------------
vtkPlotBar::~vtkPlotBar()
{
  if (this->Points)
    {
    this->Points->Delete();
    this->Points = NULL;
    }
}

//-----------------------------------------------------------------------------
bool vtkPlotBar::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called in vtkPlotBar.");

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
    }
  else
    {
    vtkDebugMacro("No selection set.");
    }

  // Now to plot the points
  if (this->Points)
    {
    painter->ApplyPen(this->Pen);
    painter->ApplyBrush(this->Brush);
    int n = this->Points->GetNumberOfPoints();
    float *f = vtkFloatArray::SafeDownCast(this->Points->GetData())->GetPointer(0);

    for (int i = 0; i < n; ++i)
      {
      painter->DrawRect(f[2*i]-(this->Width/2)-this->Offset, 0.0,
                        this->Width, f[2*i+1]);
      }
    }

  return true;
}

//-----------------------------------------------------------------------------
bool vtkPlotBar::PaintLegend(vtkContext2D *painter, float rect[4])
{
  painter->ApplyPen(this->Pen);
  painter->ApplyBrush(this->Brush);
  painter->DrawRect(rect[0], rect[1], rect[2], rect[3]);
  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotBar::GetBounds(double bounds[4])
{
  // Get the x and y arrays (index 0 and 1 respectively)
  vtkTable *table = this->Data->GetInput();
  vtkDataArray* x = this->UseIndexForXSeries ?
                    0 : this->Data->GetInputArrayToProcess(0, table);
  vtkDataArray *y = this->Data->GetInputArrayToProcess(1, table);

  if (this->UseIndexForXSeries && y)
    {
    bounds[0] = 0 - (this->Width / 2 );
    bounds[1] = y->GetNumberOfTuples() + (this->Width/2);
    y->GetRange(&bounds[2]);
    }
  else if (x && y)
    {
    x->GetRange(&bounds[0]);
    // We surround our point by Width/2 on either side
    bounds[0] -= this->Width / 2;
    bounds[1] += this->Width / 2;
    y->GetRange(&bounds[2]);
    }
  // Bar plots always have one of the y bounds at the orgin
  if (bounds[2] > 0.0f)
    {
    bounds[2] = 0.0;
    }
  else if (bounds[3] < 0.0f)
    {
    bounds[3] = 0.0;
    }
  vtkDebugMacro(<< "Bounds: " << bounds[0] << "\t" << bounds[1] << "\t"
                << bounds[2] << "\t" << bounds[3]);
}

//-----------------------------------------------------------------------------
void vtkPlotBar::SetWidth(float width)
{
  this->Width = width;
}

//-----------------------------------------------------------------------------
float vtkPlotBar::GetWidth()
{
  return this->Width;
}

//-----------------------------------------------------------------------------
void vtkPlotBar::SetColor(unsigned char r, unsigned char g, unsigned char b,
                       unsigned char a)
{
  this->Brush->SetColor(r, g, b, a);
}

//-----------------------------------------------------------------------------
void vtkPlotBar::SetColor(double r, double g, double b)
{
  this->Brush->SetColorF(r, g, b);
}

//-----------------------------------------------------------------------------
void vtkPlotBar::GetColor(double rgb[3])
{
  this->Brush->GetColorF(rgb);
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

}

//-----------------------------------------------------------------------------
bool vtkPlotBar::GetNearestPoint(const vtkVector2f& point,
                                  const vtkVector2f&,
                                  vtkVector2f* location)
{
  // Right now doing a simple bisector search of the array. This should be
  // revisited. Assumes the x axis is sorted, which should always be true for
  // bar plots.
  if (!this->Points)
    {
    return false;
    }
  vtkIdType n = this->Points->GetNumberOfPoints();
  if (n < 2)
    {
    return false;
    }

  vtkVector2f* data =
      static_cast<vtkVector2f*>(this->Points->GetVoidPointer(0));
  vtkstd::vector<vtkVector2f> v(data, data+n);

  // Sort if necessary - in the case of bar plots render order does not matter
  if (!this->Sorted)
    {
    vtkstd::sort(v.begin(), v.end(), compVector2fX);
    this->Sorted = true;
    }

  // The extent of any given bar is half a width on either
  // side of the point with which it is associated.
  float halfWidth = this->Width / 2.0;


  // Set up our search array, use the STL lower_bound algorithm
  // When searching, invert the behavior of the offset and
  // compensate for the half width overlap.
  vtkstd::vector<vtkVector2f>::iterator low;
  vtkVector2f lowPoint(point.X()-(this->Offset * -1)-halfWidth, 0.0f);
  low = vtkstd::lower_bound(v.begin(), v.end(), lowPoint, compVector2fX);

  while (low != v.end())
    {
    // Is the left side of the bar beyond the point?
    if (low->X()-this->Offset-halfWidth > point.X())
      {
      break;
      }
    // Does the bar surround the point?
    else if (low->X()-halfWidth-this->Offset < point.X() &&
             low->X()+halfWidth-this->Offset > point.X())
      {
      // Is the point within the vertical extent of the bar?
      if ((point.Y() >= 0 && point.Y() < low->Y()) ||
          (point.Y() < 0 && point.Y() > low->Y()))
        {
        *location = *low;
        return true;
        }
      }
    ++low;
    }

  return false;
}

//-----------------------------------------------------------------------------
namespace {

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

}

//-----------------------------------------------------------------------------
bool vtkPlotBar::UpdateTableCache(vtkTable *table)
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
  else if (!this->UseIndexForXSeries &&
           x->GetNumberOfTuples() != y->GetNumberOfTuples())
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
                         y->GetNumberOfTuples()));
      }
    }
  else
    {
    switch(x->GetDataType())
      {
      vtkTemplateMacro(
          CopyToPointsSwitch(this->Points,
                             static_cast<VTK_TT*>(x->GetVoidPointer(0)),
                             y, x->GetNumberOfTuples()));
      }
    }
  this->Sorted = false;
  this->BuildTime.Modified();
  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotBar::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
