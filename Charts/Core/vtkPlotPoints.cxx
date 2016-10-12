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

#include "vtkNew.h"
#include "vtkContext2D.h"
#include "vtkPen.h"
#include "vtkBrush.h"
#include "vtkAxis.h"
#include "vtkContextMapper2D.h"
#include "vtkPoints2D.h"
#include "vtkTable.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkCharArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkLookupTable.h"

#include <vector>
#include <algorithm>
#include <limits>
#include <set>

// PIMPL for STL vector...
struct vtkIndexedVector2f
{
  size_t index;
  vtkVector2f pos;
};

class vtkPlotPoints::VectorPIMPL : public std::vector<vtkIndexedVector2f>
{
public:
  VectorPIMPL(vtkVector2f* array, size_t n)
    : std::vector<vtkIndexedVector2f>()
  {
    this->reserve(n);
    for (size_t i = 0; i < n; ++i)
    {
      vtkIndexedVector2f tmp;
      tmp.index = i;
      tmp.pos = array[i];
      this->push_back(tmp);
    }
  }
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotPoints)

//-----------------------------------------------------------------------------
vtkPlotPoints::vtkPlotPoints()
{
  this->Points = NULL;
  this->Sorted = NULL;
  this->BadPoints = NULL;
  this->ValidPointMask = NULL;
  this->MarkerStyle = vtkPlotPoints::CIRCLE;
  this->MarkerSize = -1.0;
  this->LogX = false;
  this->LogY = false;

  this->LookupTable = 0;
  this->Colors = 0;
  this->ScalarVisibility = 0;

  this->UnscaledInputBounds[0] = this->UnscaledInputBounds[2] = vtkMath::Inf();
  this->UnscaledInputBounds[1] = this->UnscaledInputBounds[3] = -vtkMath::Inf();
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
  if (this->LookupTable)
  {
    this->LookupTable->UnRegister(this);
  }
  if ( this->Colors != 0 )
  {
    this->Colors->UnRegister(this);
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

  if (table && !this->ValidPointMaskName.empty() &&
      table->GetColumnByName(this->ValidPointMaskName))
  {
    this->ValidPointMask = vtkArrayDownCast<vtkCharArray>(
      table->GetColumnByName(this->ValidPointMaskName));
  }
  else
  {
    this->ValidPointMask = 0;
  }

  if (!table)
  {
    vtkDebugMacro(<< "Update event called with no input table set.");
    return;
  }
  else if(this->Data->GetMTime() > this->BuildTime ||
          table->GetMTime() > this->BuildTime ||
          (this->LookupTable && this->LookupTable->GetMTime() > this->BuildTime) ||
          this->MTime > this->BuildTime)
  {
    vtkDebugMacro(<< "Updating cached values.");
    this->UpdateTableCache(table);
  }
  else if (this->XAxis && this->YAxis &&
           ((this->XAxis->GetMTime() > this->BuildTime) ||
            (this->YAxis->GetMTime() > this->BuildTime)))
  {
    if ((this->LogX != this->XAxis->GetLogScale()) ||
        (this->LogY != this->YAxis->GetLogScale()))
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

  if (!this->Visible || !this->Points || this->Points->GetNumberOfPoints() == 0)
  {
    return false;
  }

  // Maintain legacy behavior (using pen width) if MarkerSize was not set
  float width = this->MarkerSize;
  if (width < 0.0f)
  {
    width = this->Pen->GetWidth() * 2.3;
    if (width < 8.0)
    {
      width = 8.0;
    }
  }

  // If there is a marker style, then draw the marker for each point too
  if (this->MarkerStyle != VTK_MARKER_NONE)
  {
    painter->ApplyPen(this->Pen);
    painter->ApplyBrush(this->Brush);
    painter->GetPen()->SetWidth(width);

    float *points = static_cast<float *>(this->Points->GetVoidPointer(0));
    unsigned char *colors = 0;
    int nColorComponents = 0;
    if (this->ScalarVisibility && this->Colors)
    {
      colors = this->Colors->GetPointer(0);
      nColorComponents = static_cast<int>(this->Colors->GetNumberOfComponents());
    }

    if (this->BadPoints && this->BadPoints->GetNumberOfTuples() > 0)
    {
      vtkIdType lastGood = 0;

      for (vtkIdType i = 0; i < this->BadPoints->GetNumberOfTuples(); i++)
      {
        vtkIdType id = this->BadPoints->GetValue(i);

        // render from last good point to one before this bad point
        if (id - lastGood > 2)
        {
          painter->DrawMarkers(this->MarkerStyle, false,
                               points + 2 * (lastGood + 1),
                               id - lastGood - 1,
                               colors ? colors + 4 * (lastGood + 1) : 0,
                               nColorComponents);
        }

        lastGood = id;
      }

        // render any trailing good points
        if (this->Points->GetNumberOfPoints() - lastGood > 2)
        {
          painter->DrawMarkers(this->MarkerStyle, false,
                               points + 2 * (lastGood + 1),
                               this->Points->GetNumberOfPoints() - lastGood - 1,
                               colors ? colors + 4 * (lastGood + 1) : 0,
                               nColorComponents);
        }
    }
    else
    {
      // draw all of the points
      painter->DrawMarkers(this->MarkerStyle, false,
                           points, this->Points->GetNumberOfPoints(),
                           colors, nColorComponents);
    }
  }

  // Now add some decorations for our selected points...
  if (this->Selection && this->Selection->GetNumberOfTuples())
  {
    if (this->Selection->GetMTime() > this->SelectedPoints->GetMTime() ||
        this->GetMTime() > this->SelectedPoints->GetMTime())
    {
      float *f = vtkArrayDownCast<vtkFloatArray>(
            this->Points->GetData())->GetPointer(0);
      int nSelected(static_cast<int>(this->Selection->GetNumberOfTuples()));
      this->SelectedPoints->SetNumberOfComponents(2);
      this->SelectedPoints->SetNumberOfTuples(nSelected);
      float *selectedPtr = static_cast<float *>(this->SelectedPoints->GetVoidPointer(0));
      for (int i = 0; i < nSelected; ++i)
      {
        *(selectedPtr++) = f[2 * this->Selection->GetValue(i)];
        *(selectedPtr++) = f[2 * this->Selection->GetValue(i) + 1];
      }
    }
    vtkDebugMacro(<<"Selection set " << this->Selection->GetNumberOfTuples());
    painter->GetPen()->SetColor(this->SelectionPen->GetColor());
    painter->GetPen()->SetOpacity(this->SelectionPen->GetOpacity());
    painter->GetPen()->SetWidth(width + 2.7);

    if (this->MarkerStyle == VTK_MARKER_NONE)
    {
      painter->DrawMarkers(VTK_MARKER_PLUS, false,
                           static_cast<float *>(
                             this->SelectedPoints->GetVoidPointer(0)),
                           this->SelectedPoints->GetNumberOfTuples());
    }
    else
    {
      painter->DrawMarkers(this->MarkerStyle, true,
                           static_cast<float *>(
                             this->SelectedPoints->GetVoidPointer(0)),
                           this->SelectedPoints->GetNumberOfTuples());
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
bool vtkPlotPoints::PaintLegend(vtkContext2D *painter, const vtkRectf& rect,
                                int)
{
  if (this->MarkerStyle)
  {
    float width = this->Pen->GetWidth() * 2.3;
    if (width < 8.0)
    {
      width = 8.0;
    }
    painter->ApplyPen(this->Pen);
    painter->ApplyBrush(this->Brush);
    painter->GetPen()->SetWidth(width);

    float point[] = { rect[0]+ 0.5f * rect[2], rect[1] + 0.5f * rect[3] };
    painter->DrawMarkers(this->MarkerStyle, false, point, 1);
  }
  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotPoints::GetBounds(double bounds[4])
{
  if (this->Points)
  {
    // There are bad points in the series - need to do this ourselves.
    this->CalculateBounds(bounds);
  }
  vtkDebugMacro(<< "Bounds: " << bounds[0] << "\t" << bounds[1] << "\t"
                << bounds[2] << "\t" << bounds[3]);
}

//-----------------------------------------------------------------------------
void vtkPlotPoints::GetUnscaledInputBounds(double bounds[4])
{
  this->CalculateUnscaledInputBounds();
  for (int i = 0; i < 4; ++i)
  {
    bounds[i] = this->UnscaledInputBounds[i];
  }
  vtkDebugMacro(
    << "Bounds: " << bounds[0] << "\t" << bounds[1] << "\t"
    << bounds[2] << "\t" << bounds[3]);
}

namespace
{

bool compVector3fX(const vtkIndexedVector2f& v1,
                   const vtkIndexedVector2f& v2)
{
  if (v1.pos.GetX() < v2.pos.GetX())
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
  if (current.GetX() > point.GetX() - tol.GetX() && current.GetX() < point.GetX() + tol.GetX() &&
      current.GetY() > point.GetY() - tol.GetY() && current.GetY() < point.GetY() + tol.GetY())
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
void vtkPlotPoints::CreateSortedPoints()
{
  // Sort the data if it has not been done already...
  if (!this->Sorted)
  {
    vtkIdType n = this->Points->GetNumberOfPoints();
    vtkVector2f* data =
        static_cast<vtkVector2f*>(this->Points->GetVoidPointer(0));
    this->Sorted = new VectorPIMPL(data, n);
    std::sort(this->Sorted->begin(), this->Sorted->end(), compVector3fX);
  }
}

//-----------------------------------------------------------------------------
vtkIdType vtkPlotPoints::GetNearestPoint(const vtkVector2f& point,
                                         const vtkVector2f& tol,
                                         vtkVector2f* location)
{
  // Right now doing a simple bisector search of the array.
  if (!this->Points)
  {
    return -1;
  }
  this->CreateSortedPoints();

  // Set up our search array, use the STL lower_bound algorithm
  VectorPIMPL::iterator low;
  VectorPIMPL &v = *this->Sorted;

  // Get the lowest point we might hit within the supplied tolerance
  vtkIndexedVector2f lowPoint;
  lowPoint.index = 0;
  lowPoint.pos = vtkVector2f(point.GetX()-tol.GetX(), 0.0f);
  low = std::lower_bound(v.begin(), v.end(), lowPoint, compVector3fX);

  // Now consider the y axis
  float highX = point.GetX() + tol.GetX();
  while (low != v.end())
  {
    if (inRange(point, tol, (*low).pos))
    {
      *location = (*low).pos;
      return static_cast<int>((*low).index);
    }
    else if (low->pos.GetX() > highX)
    {
      break;
    }
    ++low;
  }
  return -1;
}

//-----------------------------------------------------------------------------
bool vtkPlotPoints::SelectPoints(const vtkVector2f& min, const vtkVector2f& max)
{
  if (!this->Points)
  {
    return false;
  }
  this->CreateSortedPoints();

  if (!this->Selection)
  {
    this->Selection = vtkIdTypeArray::New();
  }
  this->Selection->SetNumberOfTuples(0);

  // Set up our search array, use the STL lower_bound algorithm
  VectorPIMPL::iterator low;
  VectorPIMPL &v = *this->Sorted;

  // Get the lowest point we might hit within the supplied tolerance
  vtkIndexedVector2f lowPoint;
  lowPoint.index = 0;
  lowPoint.pos = min;
  low = std::lower_bound(v.begin(), v.end(), lowPoint, compVector3fX);

  // Output a sorted selection list too.
  std::vector<vtkIdType> selected;
  // Iterate until we are out of range in X
  while (low != v.end())
  {
      if (low->pos.GetX() >= min.GetX() && low->pos.GetX() <= max.GetX() &&
          low->pos.GetY() >= min.GetY() && low->pos.GetY() <= max.GetY())
      {
        selected.push_back(low->index);
      }
      else if (low->pos.GetX() > max.GetX())
      {
        break;
      }
      ++low;
  }
  this->Selection->SetNumberOfTuples(selected.size());
  vtkIdType *ptr = static_cast<vtkIdType *>(this->Selection->GetVoidPointer(0));
  for (size_t i = 0; i < selected.size(); ++i)
  {
    ptr[i] = selected[i];
  }
  std::sort(ptr, ptr + selected.size());
  this->Selection->Modified();
  return this->Selection->GetNumberOfTuples() > 0;
}

//-----------------------------------------------------------------------------
bool vtkPlotPoints::SelectPointsInPolygon(const vtkContextPolygon &polygon)
{
  if (!this->Points)
  {
    // nothing to select
    return false;
  }

  if (!this->Selection)
  {
    // create selection object
    this->Selection = vtkIdTypeArray::New();
  }
  else
  {
    // clear previous selection
    this->Selection->SetNumberOfValues(0);
  }

  for(vtkIdType pointId = 0;
      pointId < this->Points->GetNumberOfPoints();
      pointId++)
  {
    // get point location
    double point[3];
    this->Points->GetPoint(pointId, point);

    if (polygon.Contains(vtkVector2f(point[0], point[1])))
    {
      this->Selection->InsertNextValue(pointId);
    }
  }

  // return true if we selected any points
  return this->Selection->GetNumberOfTuples() > 0;
}

//-----------------------------------------------------------------------------
namespace {

// Find any bad points in the supplied array.
template<typename T>
void SetBadPoints(T *data, vtkIdType n, std::set<vtkIdType> &bad)
{
  for (vtkIdType i = 0; i < n; ++i)
  {
    if (vtkMath::IsInf(data[i]) || vtkMath::IsNan(data[i]))
    {
      bad.insert(i);
    }
  }
}

// Calculate the bounds from the original data.
template<typename A>
void ComputeBounds(A *a, int n, double bounds[2])
{
  bounds[0] =  std::numeric_limits<double>::max();
  bounds[1] = -std::numeric_limits<double>::max();
  for (int i = 0; i < n; ++a, ++i)
  {
    bounds[0] = bounds[0] < *a ? bounds[0] : *a;
    bounds[1] = bounds[1] > *a ? bounds[1] : *a;
  }
}

template<typename A>
void ComputeBounds(A *a, int n, vtkIdTypeArray *bad, double bounds[2])
{
  // If possible, use the simpler code without any bad points.
  if (!bad || bad->GetNumberOfTuples() == 0)
  {
    ComputeBounds(a, n, bounds);
    return;
  }

  // Initialize the first range of points.
  vtkIdType start = 0;
  vtkIdType end = 0;
  vtkIdType i = 0;
  vtkIdType nBad = bad->GetNumberOfTuples();
  if (bad->GetValue(i) == 0)
  {
    while (i < nBad && i == bad->GetValue(i))
    {
      start = bad->GetValue(i++) + 1;
    }
    if (start < n)
    {
      end = n;
    }
    else
    {
      // They are all bad points, return early.
      return;
    }
  }
  if (i < nBad)
  {
    end = bad->GetValue(i++);
  }
  else
  {
    end = n;
  }

  bounds[0] =  std::numeric_limits<double>::max();
  bounds[1] = -std::numeric_limits<double>::max();
  while (start < n)
  {
    // Calculate the min/max in this range.
    while (start < end)
    {
      bounds[0] = bounds[0] < a[start] ? bounds[0] : a[start];
      bounds[1] = bounds[1] > a[start] ? bounds[1] : a[start];
      ++start;
    }
    // Now figure out the next range to be evaluated.
    start = end + 1;
    while (i < nBad && start == bad->GetValue(i))
    {
      start = bad->GetValue(i++) + 1;
    }
    if (i < nBad)
    {
      end = bad->GetValue(i++);
    }
    else
    {
      end = n;
    }
  }
}

// Dispatch this call off to the right function.
template<typename A>
void ComputeBounds(A *a, vtkDataArray *b, int n, vtkIdTypeArray *bad,
                   double bounds[4])
{
  ComputeBounds(a, n, bad, bounds);
  switch(b->GetDataType())
  {
    vtkTemplateMacro(
      ComputeBounds(static_cast<VTK_TT*>(b->GetVoidPointer(0)), n, bad,
                    &bounds[2]));
  }
}

// Copy the two arrays into the points array
template<typename A, typename B>
void CopyToPoints(vtkPoints2D *points, A *a, B *b, int n, const vtkRectd &ss)
{
  points->SetNumberOfPoints(n);
  float* data = static_cast<float*>(points->GetVoidPointer(0));
  for (int i = 0; i < n; ++i)
  {
    data[2 * i]     = static_cast<float>((a[i] + ss[0]) * ss[2]);
    data[2 * i + 1] = static_cast<float>((b[i] + ss[1]) * ss[3]);
  }
}

// Copy one array into the points array, use the index of that array as x
template<typename A>
void CopyToPoints(vtkPoints2D *points, A *a, int n, const vtkRectd &ss)
{
  points->SetNumberOfPoints(n);
  float* data = static_cast<float*>(points->GetVoidPointer(0));
  for (int i = 0; i < n; ++i)
  {
    data[2 * i]     = static_cast<float>((  i  + ss[0]) * ss[2]);
    data[2 * i + 1] = static_cast<float>((a[i] + ss[1]) * ss[3]);
  }
}

// Copy the two arrays into the points array
template<typename A>
void CopyToPointsSwitch(vtkPoints2D *points, A *a, vtkDataArray *b, int n,
                        const vtkRectd &ss)
{
  switch(b->GetDataType())
  {
    vtkTemplateMacro(
      CopyToPoints(
        points, a, static_cast<VTK_TT*>(b->GetVoidPointer(0)), n, ss));
  }
}

}

//-----------------------------------------------------------------------------
bool vtkPlotPoints::GetDataArrays(vtkTable *table, vtkDataArray *array[2])
{
  if (!table)
  {
    return false;
  }

  // Get the x and y arrays (index 0 and 1 respectively)
  array[0] = this->UseIndexForXSeries ?
        0 : this->Data->GetInputArrayToProcess(0, table);
  array[1] = this->Data->GetInputArrayToProcess(1, table);

  if (!array[0] && !this->UseIndexForXSeries)
  {
    vtkErrorMacro(<< "No X column is set (index 0).");
    return false;
  }
  else if (!array[1])
  {
    vtkErrorMacro(<< "No Y column is set (index 1).");
    return false;
  }
  else if (!this->UseIndexForXSeries &&
           array[0]->GetNumberOfTuples() != array[1]->GetNumberOfTuples())
  {
    vtkErrorMacro("The x and y columns must have the same number of elements. "
                  << array[0]->GetNumberOfTuples() << ", "
                  << array[1]->GetNumberOfTuples());
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
bool vtkPlotPoints::UpdateTableCache(vtkTable *table)
{
  vtkDataArray *array[2] = { 0, 0 };
  if (!this->GetDataArrays(table, array))
  {
    this->BuildTime.Modified();
    return false;
  }

  if (!this->Points)
  {
    this->Points = vtkPoints2D::New();
  }
  vtkDataArray *x = array[0];
  vtkDataArray *y = array[1];

  // Now copy the components into their new columns
  if (this->UseIndexForXSeries)
  {
    switch(y->GetDataType())
    {
      vtkTemplateMacro(
        CopyToPoints(
          this->Points, static_cast<VTK_TT*>(y->GetVoidPointer(0)),
          y->GetNumberOfTuples(), this->ShiftScale));
    }
  }
  else
  {
    switch(x->GetDataType())
    {
      vtkTemplateMacro(
        CopyToPointsSwitch(
          this->Points, static_cast<VTK_TT*>(x->GetVoidPointer(0)),
          y, x->GetNumberOfTuples(), this->ShiftScale));
    }
  }
  this->CalculateLogSeries();
  this->FindBadPoints();
  this->Points->Modified();
  delete this->Sorted;
  this->Sorted = 0;

  // Additions for color mapping
  if (this->ScalarVisibility && !this->ColorArrayName.empty())
  {
    vtkDataArray* c =
      vtkArrayDownCast<vtkDataArray>(table->GetColumnByName(this->ColorArrayName));
    // TODO: Should add support for categorical coloring & try enum lookup
    if (c)
    {
      if (!this->LookupTable)
      {
        this->CreateDefaultLookupTable();
      }
      if (this->Colors)
      {
        this->Colors->UnRegister(this);
      }
      this->Colors = this->LookupTable->MapScalars(c, VTK_COLOR_MODE_MAP_SCALARS, -1);
      // Consistent register and unregisters
      this->Colors->Register(this);
      this->Colors->Delete();
    }
    else
    {
      this->Colors->UnRegister(this);
      this->Colors = 0;
    }
  }

  this->BuildTime.Modified();

  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotPoints::CalculateUnscaledInputBounds()
{
  vtkTable *table = this->Data->GetInput();
  vtkDataArray *array[2] = { 0, 0 };
  if (!this->GetDataArrays(table, array))
  {
    return;
  }
  // Now copy the components into their new columns
  if (this->UseIndexForXSeries)
  {
    this->UnscaledInputBounds[0] = 0.0;
    this->UnscaledInputBounds[1] = array[1]->GetNumberOfTuples() - 1;
    switch(array[1]->GetDataType())
    {
      vtkTemplateMacro(
        ComputeBounds(static_cast<VTK_TT*>(array[1]->GetVoidPointer(0)),
            array[1]->GetNumberOfTuples(), this->BadPoints,
            &this->UnscaledInputBounds[2]));
    }
  }
  else
  {
    switch(array[0]->GetDataType())
    {
      vtkTemplateMacro(
        ComputeBounds(static_cast<VTK_TT*>(array[0]->GetVoidPointer(0)),
            array[1], array[0]->GetNumberOfTuples(), this->BadPoints,
            this->UnscaledInputBounds));
    }
  }
}

//-----------------------------------------------------------------------------
void vtkPlotPoints::CalculateLogSeries()
{
  if (!this->XAxis || !this->YAxis)
  {
    return;
  }
  this->LogX = this->XAxis->GetLogScaleActive();
  this->LogY = this->YAxis->GetLogScaleActive();
  float* data = static_cast<float*>(this->Points->GetVoidPointer(0));
  vtkIdType n = this->Points->GetNumberOfPoints();
  if (this->LogX)
  {
    if (this->XAxis->GetUnscaledMinimum() < 0.)
    {
      for (vtkIdType i = 0; i < n; ++i)
      {
        data[2*i] = log10(fabs(data[2*i]));
      }
    }
    else
    {
      for (vtkIdType i = 0; i < n; ++i)
      {
        data[2*i] = log10(data[2*i]);
      }
    }
  }
  if (this->LogY)
  {
    if (this->YAxis->GetUnscaledMinimum() < 0.)
    {
      for (vtkIdType i = 0; i < n; ++i)
      {
        data[2*i+1] = log10(fabs(data[2*i+1]));
      }
    }
    else
    {
      for (vtkIdType i = 0; i < n; ++i)
      {
        data[2*i+1] = log10(data[2*i+1]);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void vtkPlotPoints::FindBadPoints()
{
  // This should be run after CalculateLogSeries as a final step.
  vtkIdType n = this->Points->GetNumberOfPoints();

  // Scan through and find any bad points.
  vtkTable *table = this->Data->GetInput();
  vtkDataArray *array[2] = { 0, 0 };
  if (!this->GetDataArrays(table, array))
  {
    return;
  }
  std::set<vtkIdType> bad;
  if (!this->UseIndexForXSeries)
  {
    switch(array[0]->GetDataType())
    {
      vtkTemplateMacro(
        SetBadPoints(static_cast<VTK_TT*>(array[0]->GetVoidPointer(0)), n, bad));
    }
  }
  switch(array[1]->GetDataType())
  {
    vtkTemplateMacro(
      SetBadPoints(static_cast<VTK_TT*>(array[1]->GetVoidPointer(0)), n, bad));
  }

  // add points from the ValidPointMask
  if (this->ValidPointMask)
  {
    for (vtkIdType i = 0; i < n; i++)
    {
      if (this->ValidPointMask->GetValue(i) == 0)
      {
        bad.insert(i);
      }
    }
  }

  // If there are bad points copy them, if not ensure the pointer is null.
  if (!bad.empty())
  {
    if (!this->BadPoints)
    {
      this->BadPoints = vtkIdTypeArray::New();
    }
    else
    {
      this->BadPoints->SetNumberOfTuples(0);
    }
    for (std::set<vtkIdType>::const_iterator it = bad.begin();
         it != bad.end(); ++it)
    {
      this->BadPoints->InsertNextValue(*it);
    }
  }
  else if (this->BadPoints)
  {
    this->BadPoints->Delete();
    this->BadPoints = NULL;
  }
}

//-----------------------------------------------------------------------------
void vtkPlotPoints::CalculateBounds(double bounds[4])
{
  // We can use the BadPoints array to skip the bad points
  if (!this->Points)
  {
    return;
  }
  this->CalculateUnscaledInputBounds();
  for (int i = 0; i < 4; ++i)
  {
    bounds[i] = this->UnscaledInputBounds[i];
  }
  if (this->LogX)
  {
    bounds[0] = log10(bounds[0]);
    bounds[1] = log10(bounds[1]);
  }
  if (this->LogY)
  {
    bounds[2] = log10(bounds[2]);
    bounds[3] = log10(bounds[3]);
  }
}

//-----------------------------------------------------------------------------
void vtkPlotPoints::SetLookupTable(vtkScalarsToColors *lut)
{
  if ( this->LookupTable != lut )
  {
    if ( this->LookupTable)
    {
      this->LookupTable->UnRegister(this);
    }
    this->LookupTable = lut;
    if (lut)
    {
      lut->Register(this);
    }
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
vtkScalarsToColors *vtkPlotPoints::GetLookupTable()
{
  if ( this->LookupTable == 0 )
  {
    this->CreateDefaultLookupTable();
  }
  return this->LookupTable;
}

//-----------------------------------------------------------------------------
void vtkPlotPoints::CreateDefaultLookupTable()
{
  if ( this->LookupTable)
  {
    this->LookupTable->UnRegister(this);
  }
  this->LookupTable = vtkLookupTable::New();
  // Consistent Register/UnRegisters.
  this->LookupTable->Register(this);
  this->LookupTable->Delete();
}

//-----------------------------------------------------------------------------
void vtkPlotPoints::SelectColorArray(const vtkStdString& arrayName)
{
  vtkTable *table = this->Data->GetInput();
  if (!table)
  {
    vtkDebugMacro(<< "SelectColorArray called with no input table set.");
    return;
  }
  if (this->ColorArrayName == arrayName)
  {
    return;
  }
  for (vtkIdType c = 0; c < table->GetNumberOfColumns(); ++c)
  {
    if (arrayName == table->GetColumnName(c))
    {
      this->ColorArrayName = arrayName;
      this->Modified();
      return;
    }
  }
  vtkDebugMacro(<< "SelectColorArray called with invalid column name.");
  this->ColorArrayName = "";
  this->Modified();
  return;
}

//-----------------------------------------------------------------------------
void vtkPlotPoints::SelectColorArray(vtkIdType arrayNum)
{
  vtkTable *table = this->Data->GetInput();
  if (!table)
  {
    vtkDebugMacro(<< "SelectColorArray called with no input table set.");
    return;
  }
  vtkDataArray *col = vtkArrayDownCast<vtkDataArray>(table->GetColumn(arrayNum));
  // TODO: Should add support for categorical coloring & try enum lookup
  if (!col)
  {
    vtkDebugMacro(<< "SelectColorArray called with invalid column index");
    return;
  }
  else
  {
    const char *arrayName = table->GetColumnName(arrayNum);
    if (this->ColorArrayName == arrayName || arrayName == 0)
    {
      return;
    }
    else
    {
      this->ColorArrayName = arrayName;
      this->Modified();
    }
  }
}

//-----------------------------------------------------------------------------
vtkStdString vtkPlotPoints::GetColorArrayName()
{
  return this->ColorArrayName;
}

//-----------------------------------------------------------------------------
void vtkPlotPoints::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
