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
#include "vtkUnsignedCharArray.h"
#include "vtkLookupTable.h"

#include <vector>
#include <algorithm>

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
  this->MarkerStyle = vtkPlotPoints::CIRCLE;
  this->MarkerSize = -1.0;
  this->LogX = false;
  this->LogY = false;

  this->LookupTable = 0;
  this->Colors = 0;
  this->ScalarVisibility = 0;
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
    if (this->ScalarVisibility && this->Colors)
      {
      painter->DrawMarkers(this->MarkerStyle, false,
                           this->Points, this->Colors);
      }
    else
      {
      painter->DrawMarkers(this->MarkerStyle, false, this->Points);
      }
    }

  // Now add some decorations for our selected points...
  if (this->Selection && this->Selection->GetNumberOfTuples())
    {
    if (this->Selection->GetMTime() > this->SelectedPoints->GetMTime() ||
        this->GetMTime() > this->SelectedPoints->GetMTime())
      {
      float *f = vtkFloatArray::SafeDownCast(
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
    painter->GetPen()->SetColor(255, 50, 0, 150);
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
  std::sort(selected.begin(), selected.end());
  this->Selection->SetNumberOfTuples(selected.size());
  vtkIdType *ptr = static_cast<vtkIdType *>(this->Selection->GetVoidPointer(0));
  for (size_t i = 0; i < selected.size(); ++i)
    {
    ptr[i] = selected[i];
    }
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

  // Additions for color mapping
  if (this->ScalarVisibility && !this->ColorArrayName.empty())
    {
    vtkDataArray* c =
      vtkDataArray::SafeDownCast(table->GetColumnByName(this->ColorArrayName));
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
void vtkPlotPoints::CalculateLogSeries()
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
void vtkPlotPoints::FindBadPoints()
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
void vtkPlotPoints::CalculateBounds(double bounds[4])
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
  bounds[0] = bounds[1] = pts[start].GetX();
  bounds[2] = bounds[3] = pts[start++].GetY();

  while (start < nPoints)
    {
    // Calculate the min/max in this range
    while (start < end)
      {
      float x = pts[start].GetX();
      float y = pts[start++].GetY();
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
  vtkDataArray *col = vtkDataArray::SafeDownCast(table->GetColumn(arrayNum));
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
