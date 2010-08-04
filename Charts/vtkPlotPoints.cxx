/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPlotPoints.h"

#include "vtkContext2D.h"
#include "vtkPen.h"
#include "vtkAxis.h"
#include "vtkContextMapper2D.h"
#include "vtkPoints2D.h"
#include "vtkTable.h"
#include "vtkDataArray.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include "vtkstd/vector"
#include "vtkstd/algorithm"

// PIMPL for STL vector...
class vtkPlotPoints::VectorPIMPL : public vtkstd::vector<vtkVector2f>
{
public:
  VectorPIMPL(vtkVector2f* startPos, vtkVector2f* finishPos)
    : vtkstd::vector<vtkVector2f>(startPos, finishPos)
  {
  }
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotPoints);

//-----------------------------------------------------------------------------
vtkPlotPoints::vtkPlotPoints()
{
  this->Points = NULL;
  this->Sorted = NULL;
  this->BadPoints = NULL;
  this->MarkerStyle = vtkPlotPoints::CIRCLE;
  this->LogX = false;
  this->LogY = false;
  this->Marker = NULL;
  this->HighlightMarker = NULL;
}

//-----------------------------------------------------------------------------
vtkPlotPoints::~vtkPlotPoints()
{
  if (this->Points)
    {
    this->Points->Delete();
    this->Points = NULL;
    }
  delete this->Sorted;
  if (this->BadPoints)
    {
    this->BadPoints->Delete();
    this->BadPoints = NULL;
    }
  if (this->Marker)
    {
    this->Marker->Delete();
    }
  if (this->HighlightMarker)
    {
    this->HighlightMarker->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkPlotPoints::Update()
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
bool vtkPlotPoints::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called in vtkPlotPoints.");

  if (!this->Visible || !this->Points)
    {
    return false;
    }

  float width = this->Pen->GetWidth() * 2.3;
  if (width < 8.0)
    {
    width = 8.0;
    }

  // Now add some decorations for our selected points...
  if (this->Selection)
    {
    vtkDebugMacro(<<"Selection set " << this->Selection->GetNumberOfTuples());
    for (int i = 0; i < this->Selection->GetNumberOfTuples(); ++i)
      {
      this->GeneraterMarker(static_cast<int>(width+2.7), true);

      painter->GetPen()->SetColor(255, 50, 0, 255);
      painter->GetPen()->SetWidth(width+2.7);

      vtkIdType id = 0;
      this->Selection->GetTupleValue(i, &id);
      if (id < this->Points->GetNumberOfPoints())
        {
        double *point = this->Points->GetPoint(id);
        float p[] = { point[0], point[1] };
        painter->DrawPointSprites(this->HighlightMarker, p, 1);
        }
      }
    }
  else
    {
    vtkDebugMacro("No selection set.");
    }

  // If there is a marker style, then draw the marker for each point too
  if (this->MarkerStyle)
    {
    this->GeneraterMarker(static_cast<int>(width));
    painter->ApplyPen(this->Pen);
    painter->ApplyBrush(this->Brush);
    painter->GetPen()->SetWidth(width);
    painter->DrawPointSprites(this->Marker, this->Points);
    }

  return true;
}

//-----------------------------------------------------------------------------
bool vtkPlotPoints::PaintLegend(vtkContext2D *painter, float rect[4])
{
  if (this->MarkerStyle)
    {
    float width = this->Pen->GetWidth() * 2.3;
    if (width < 8.0)
      {
      width = 8.0;
      }
    this->GeneraterMarker(static_cast<int>(width));
    painter->ApplyPen(this->Pen);
    painter->ApplyBrush(this->Brush);
    painter->GetPen()->SetWidth(width);

    float point[] = { rect[0]+0.5*rect[2], rect[1]+0.5*rect[3] };
    painter->DrawPointSprites(this->Marker, point, 1);
    }
  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotPoints::GeneraterMarker(int width, bool highlight)
{
  // Set up the image data, if highlight then the mark shape is different
  vtkImageData *data = 0;

  if (!highlight)
    {
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
    data = this->Marker;
    }
  else
    {
    if (!this->HighlightMarker)
      {
      this->HighlightMarker = vtkImageData::New();
      this->HighlightMarker->SetScalarTypeToUnsignedChar();
      this->HighlightMarker->SetNumberOfScalarComponents(4);
      data = this->HighlightMarker;
      }
    else
      {
      if (this->HighlightMarker->GetMTime() >= this->GetMTime() &&
          this->HighlightMarker->GetMTime() >= this->Pen->GetMTime())
        {
        // Marker already generated, no need to do this again.
        return;
        }
      }
    data = this->HighlightMarker;
    }

  data->SetExtent(0, width-1, 0, width-1, 0, 0);
  data->AllocateScalars();
  unsigned char* image =
      static_cast<unsigned char*>(data->GetScalarPointer());

  // Generate the marker image at the required size
  switch (this->MarkerStyle)
    {
    case vtkPlotPoints::CROSS:
      {
      for (int i = 0; i < width; ++i)
        {
        for (int j = 0; j < width; ++j)
          {
          unsigned char color = 0;

          if (highlight)
            {
            if ((i >= j-1 && i <= j+1) || (i >= width-j-1 && i <= width-j+1))
              {
              color = 255;
              }
            }
          else
            {
            if (i == j || i == width-j)
              {
              color = 255;
              }
            }
          image[4*width*i + 4*j] = image[4*width*i + 4*j + 1] =
                                   image[4*width*i + 4*j + 2] = color;
          image[4*width*i + 4*j + 3] = color;
          }
        }
      break;
      }
    case vtkPlotPoints::PLUS:
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
          if (highlight)
            {
            if (i == x-1 || i == x+1 || j == y-1 || j == y+1)
              {
              color = 255;
              }
            }
          image[4*width*i + 4*j] = image[4*width*i + 4*j + 1] =
                                   image[4*width*i + 4*j + 2] = color;
          image[4*width*i + 4*j + 3] = color;
          }
        }
      break;
      }
    case vtkPlotPoints::SQUARE:
      {
      for (int i = 0; i < width; ++i)
        {
        for (int j = 0; j < width; ++j)
          {
          unsigned char color = 255;
          image[4*width*i + 4*j] = image[4*width*i + 4*j + 1] =
                                   image[4*width*i + 4*j + 2] = color;
          image[4*width*i + 4*j + 3] = color;
          }
        }
      break;
      }
    case vtkPlotPoints::CIRCLE:
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
    case vtkPlotPoints::DIAMOND:
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
    default:
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
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPlotPoints::GetBounds(double bounds[4])
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
bool vtkPlotPoints::GetNearestPoint(const vtkVector2f& point,
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

  // Sort the data if it has not been done already...
  if (!this->Sorted)
    {
    vtkVector2f* data =
        static_cast<vtkVector2f*>(this->Points->GetVoidPointer(0));
    this->Sorted = new VectorPIMPL(data, data+n);
    vtkstd::sort(this->Sorted->begin(), this->Sorted->end(), compVector2fX);
    }

  // Set up our search array, use the STL lower_bound algorithm
  VectorPIMPL::iterator low;
  VectorPIMPL &v = *this->Sorted;

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
bool vtkPlotPoints::SelectPoints(const vtkVector2f& min, const vtkVector2f& max)
{
  if (!this->Points)
    {
    return false;
    }

  if (!this->Selection)
    {
    this->Selection = vtkIdTypeArray::New();
    }
  this->Selection->SetNumberOfTuples(0);

  // Iterate through all points and check whether any are in range
  vtkVector2f* data = static_cast<vtkVector2f*>(this->Points->GetVoidPointer(0));
  vtkIdType n = this->Points->GetNumberOfPoints();

  for (vtkIdType i = 0; i < n; ++i)
    {
    if (data[i].X() >= min.X() && data[i].X() <= max.X() &&
        data[i].Y() >= min.Y() && data[i].Y() <= max.Y())
      {
      this->Selection->InsertNextValue(i);
      }
    }
  return this->Selection->GetNumberOfTuples() > 0;
}

//-----------------------------------------------------------------------------
namespace {

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
bool vtkPlotPoints::UpdateTableCache(vtkTable *table)
{
  // Get the x and y arrays (index 0 and 1 respectively)
  vtkDataArray* x = this->UseIndexForXSeries ?
                    0 : this->Data->GetInputArrayToProcess(0, table);
  vtkDataArray* y = this->Data->GetInputArrayToProcess(1, table);
  if (!x && !this->UseIndexForXSeries)
    {
    vtkErrorMacro(<< "No X column is set (index 0).");
    this->BuildTime.Modified();
    return false;
    }
  else if (!y)
    {
    vtkErrorMacro(<< "No Y column is set (index 1).");
    this->BuildTime.Modified();
    return false;
    }
  else if (!this->UseIndexForXSeries &&
           x->GetNumberOfTuples() != y->GetNumberOfTuples())
    {
    vtkErrorMacro("The x and y columns must have the same number of elements. "
                  << x->GetNumberOfTuples() << ", " << y->GetNumberOfTuples());
    this->BuildTime.Modified();
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
    delete this->Sorted;
    this->Sorted = 0;
    }
  this->BuildTime.Modified();
  return true;
}

//-----------------------------------------------------------------------------
inline void vtkPlotPoints::CalculateLogSeries()
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
inline void vtkPlotPoints::FindBadPoints()
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
inline void vtkPlotPoints::CalculateBounds(double bounds[4])
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
void vtkPlotPoints::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
