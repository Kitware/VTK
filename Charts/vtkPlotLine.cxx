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
#include "vtkImageData.h"
#include "vtkExecutive.h"
#include "vtkTimeStamp.h"
#include "vtkInformation.h"
#include "vtkMath.h"

#include "vtkObjectFactory.h"

#include "vtkstd/vector"
#include "vtkstd/algorithm"

vtkCxxRevisionMacro(vtkPlotLine, "1.27");

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotLine);

//-----------------------------------------------------------------------------
vtkPlotLine::vtkPlotLine()
{
  this->Points = NULL;
  this->Sorted = NULL;
  this->BadPoints = NULL;
  this->MarkerStyle = vtkPlotLine::NONE;
  this->LogX = false;
  this->LogY = false;
  this->Marker = NULL;
}

//-----------------------------------------------------------------------------
vtkPlotLine::~vtkPlotLine()
{
  if (this->Points)
    {
    this->Points->Delete();
    this->Points = NULL;
    }
  if (this->Sorted)
    {
    this->Sorted->Delete();
    this->Sorted = NULL;
    }
  if (this->BadPoints)
    {
    this->BadPoints->Delete();
    this->BadPoints = NULL;
    }
  if (this->Marker)
    {
    this->Marker->Delete();
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
    float width = this->Pen->GetWidth() * 2.3;
    if (width < 8.0)
      {
      width = 8.0;
      }
    this->GeneraterMarker(static_cast<int>(width));
    painter->ApplyBrush(this->Brush);
    painter->GetPen()->SetWidth(width);
    painter->AddPointSprite(this->Marker);
    painter->DrawPoints(this->Points);
    painter->AddPointSprite(0);
    }

  return true;
}

void vtkPlotLine::GeneraterMarker(int width)
{
  // Set up the image data
  if (!this->Marker)
    {
    this->Marker = vtkImageData::New();
    this->Marker->SetScalarTypeToUnsignedChar();
    this->Marker->SetNumberOfScalarComponents(4);
    }
  else
    {
    if (this->Marker->GetMTime() >= this->GetMTime() &&
        this->Marker->GetMTime() >= this->Pen->GetMTime())
      {
      // Marker already generated, no need to do this again.
      return;
      }
    }
  this->Marker->SetExtent(0, width-1, 0, width-1, 0, 0);
  this->Marker->AllocateScalars();
  unsigned char* image =
      static_cast<unsigned char*>(this->Marker->GetScalarPointer());

  // Generate the marker image at the required size
  switch (this->MarkerStyle)
    {
    case vtkPlotLine::CROSS:
      {
      for (int i = 0; i < width; ++i)
        {
        for (int j = 0; j < width; ++j)
          {
          unsigned char color = 0;
          if (i == j || i == width-j)
            {
            color = 255;
            }
          image[4*width*i + 4*j] = image[4*width*i + 4*j + 1] =
                                   image[4*width*i + 4*j + 2] = color;
          image[4*width*i + 4*j + 3] = color;
          }
        }
      break;
      }
    case vtkPlotLine::PLUS:
      {
      int x = width / 2;
      int y = width / 2;
      for (int i = 0; i < width; ++i)
        {
        for (int j = 0; j < width; ++j)
          {
          unsigned char color = 0;
          if (i == x || j == y)
            {
            color = 255;
            }
          image[4*width*i + 4*j] = image[4*width*i + 4*j + 1] =
                                   image[4*width*i + 4*j + 2] = color;
          image[4*width*i + 4*j + 3] = color;
          }
        }
      break;
      }
    case vtkPlotLine::SQUARE:
      {
      for (int i = 0; i < width; ++i)
        {
        for (int j = 0; j < width; ++j)
          {
          unsigned char color = 255;
          image[4*width*i + 4*j] = image[4*width*i + 4*j + 1] =
                                   image[4*width*i + 4*j + 2] = color;
          image[4*width*i + 4*j] = 50;
          image[4*width*i + 4*j + 3] = color;
          }
        }
      break;
      }
    case vtkPlotLine::CIRCLE:
      {
      double c = width/2.0;
      for (int i = 0; i < width; ++i)
        {
        double dx2 = (i - c)*(i-c);
        for (int j = 0; j < width; ++j)
          {
          double dy2 = (j - c)*(j - c);
          unsigned char color = 0;
          if (sqrt(dx2 + dy2) < c)
            {
            color = 255;
            }
          image[4*width*i + 4*j] = image[4*width*i + 4*j + 1] =
                                   image[4*width*i + 4*j + 2] = color;
          image[4*width*i + 4*j + 3] = color;
          }
        }
      break;
      }
    case vtkPlotLine::DIAMOND:
      {
      int c = width/2;
      for (int i = 0; i < width; ++i)
        {
        int dx = i-c > 0 ? i-c : c-i;
        for (int j = 0; j < width; ++j)
          {
          int dy = j-c > 0 ? j-c : c-j;
          unsigned char color = 0;
          if (c-dx >= dy)
            {
            color = 255;
            }
          image[4*width*i + 4*j] = image[4*width*i + 4*j + 1] =
                                   image[4*width*i + 4*j + 2] = color;
          image[4*width*i + 4*j + 3] = color;
          }
        }
      break;
      }
    }
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
  if (this->Points)
    {
    if (!this->BadPoints)
      {
      this->Points->GetBounds(bounds);
      }
    else
      {
      // There are bad points in the series - need to do this ourselves.
      this->CalculateBounds(bounds);
      }
    }
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

  if (!this->Sorted)
    {
    this->Sorted = vtkPoints2D::New();
    }

  // Sort the data if necessary
  if (this->Sorted->GetNumberOfPoints() == 0)
    {
    this->Sorted->DeepCopy(this->Points);
    vtkVector2f* data =
        static_cast<vtkVector2f*>(this->Sorted->GetVoidPointer(0));
    vtkstd::vector<vtkVector2f> v(data, data+n);
    vtkstd::sort(v.begin(), v.end(), compVector2fX);
    }

  // Set up our search array, use the STL lower_bound algorithm
  vtkVector2f* data =
      static_cast<vtkVector2f*>(this->Sorted->GetVoidPointer(0));
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
  else if (!this->UseIndexForXSeries &&
           x->GetNumberOfTuples() != y->GetNumberOfTuples())
    {
    vtkErrorMacro("The x and y columns must have the same number of elements. "
                  << x->GetNumberOfTuples() << ", " << y->GetNumberOfTuples());
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
  this->CalculateLogSeries();
  this->FindBadPoints();
  this->Points->Modified();
  if (this->Sorted)
    {
    this->Sorted->SetNumberOfPoints(0);
    }
  this->BuildTime.Modified();
  return true;
}

//-----------------------------------------------------------------------------
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
inline void vtkPlotLine::FindBadPoints()
{
  // This should be run after CalculateLogSeries as a final step.
  float* data = static_cast<float*>(this->Points->GetVoidPointer(0));
  vtkIdType n = this->Points->GetNumberOfPoints();
  if (!this->BadPoints)
    {
    this->BadPoints = vtkIdTypeArray::New();
    }
  else
    {
    this->BadPoints->SetNumberOfTuples(0);
    }

  // Scan through and find any bad points.
  for (vtkIdType i = 0; i < n; ++i)
    {
    vtkIdType p = 2*i;
    if (vtkMath::IsInf(data[p]) || vtkMath::IsInf(data[p+1]) ||
        vtkMath::IsNan(data[p]) || vtkMath::IsNan(data[p+1]))
      {
      this->BadPoints->InsertNextValue(i);
      }
    }

  if (this->BadPoints->GetNumberOfTuples() == 0)
    {
    this->BadPoints->Delete();
    this->BadPoints = NULL;
    }
}

//-----------------------------------------------------------------------------
inline void vtkPlotLine::CalculateBounds(double bounds[4])
{
  // We can use the BadPoints array to skip the bad points
  if (!this->Points || !this->BadPoints)
    {
    return;
    }
  vtkIdType start = 0;
  vtkIdType end = 0;
  vtkIdType i = 0;
  vtkIdType nBad = this->BadPoints->GetNumberOfTuples();
  vtkIdType nPoints = this->Points->GetNumberOfPoints();
  if (this->BadPoints->GetValue(i) == 0)
    {
    while (i < nBad && i == this->BadPoints->GetValue(i))
      {
      start = this->BadPoints->GetValue(i++) + 1;
      }
    if (start < nPoints)
      {
      end = nPoints;
      }
    else
      {
      // They are all bad points
      return;
      }
    }
  if (i < nBad)
    {
    end = this->BadPoints->GetValue(i++);
    }
  else
    {
    end = nPoints;
    }
  vtkVector2f* pts = static_cast<vtkVector2f*>(this->Points->GetVoidPointer(0));

  // Initialize our min/max
  bounds[0] = bounds[1] = pts[start].X();
  bounds[2] = bounds[3] = pts[start++].Y();

  while (start < nPoints)
    {
    // Calculate the min/max in this range
    while (start < end)
      {
      float x = pts[start].X();
      float y = pts[start++].Y();
      if (x < bounds[0])
        {
        bounds[0] = x;
        }
      else if (x > bounds[1])
        {
        bounds[1] = x;
        }
      if (y < bounds[2])
        {
        bounds[2] = y;
        }
      else if (y > bounds[3])
        {
        bounds[3] = y;
        }
      }
    // Now figure out the next range
    start = end + 1;
    if (++i < nBad)
      {
      end = this->BadPoints->GetValue(i);
      }
    else
      {
      end = nPoints;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPlotLine::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
