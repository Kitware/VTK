/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotStacked.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPlotStacked.h"

#include "vtkChartXY.h"
#include "vtkContext2D.h"
#include "vtkPen.h"
#include "vtkBrush.h"
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

#define vtkStackedMIN(x, y) (((x)<(y))?(x):(y))
#define vtkStackedMAX(x, y) (((x)>(y))?(x):(y))

// PIMPL for STL vector...
class vtkPlotStacked::VectorPIMPL : public vtkstd::vector<vtkVector3f>
{
public:
  VectorPIMPL()
    : vtkstd::vector<vtkVector3f>::vector()
  {
  }
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotStacked);

//-----------------------------------------------------------------------------
vtkPlotStacked::vtkPlotStacked()
{
  this->BasePoints = NULL;
  this->ExtentPoints = NULL;
  this->Sorted = NULL;
  this->BaseBadPoints = NULL;
  this->ExtentBadPoints = NULL;
  this->Pen->SetColor(0,0,0,0);
  this->LogX = false;
  this->LogY = false;
}

//-----------------------------------------------------------------------------
vtkPlotStacked::~vtkPlotStacked()
{
  if (this->BasePoints)
    {
    this->BasePoints->Delete();
    this->BasePoints = NULL;
    }
  if (this->ExtentPoints)
    {
    this->ExtentPoints->Delete();
    this->ExtentPoints = NULL;
    }
  if (this->Sorted)
    {
    delete this->Sorted;
    this->Sorted = NULL;
    }
  if (this->BaseBadPoints)
    {
    this->BaseBadPoints->Delete();
    this->BaseBadPoints = NULL;
    }
  if (this->ExtentBadPoints)
    {
    this->ExtentBadPoints->Delete();
    this->ExtentBadPoints = NULL;
    }
}

//-----------------------------------------------------------------------------
void vtkPlotStacked::SetColor(unsigned char r, unsigned char g, unsigned char b,
                       unsigned char a)
{
  this->Brush->SetColor(r, g, b, a);
}

//-----------------------------------------------------------------------------
void vtkPlotStacked::SetColor(double r, double g, double b)
{
  this->Brush->SetColorF(r, g, b);
}

//-----------------------------------------------------------------------------
void vtkPlotStacked::GetColor(double rgb[3])
{
  this->Brush->GetColorF(rgb);
}


//-----------------------------------------------------------------------------
void vtkPlotStacked::Update()
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
          this->Parent->GetStackParticipantsChanged() > this->BuildTime ||
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
bool vtkPlotStacked::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called in vtkPlotStacked.");

  if (!this->Visible || !this->BasePoints)
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
    }
  else
    {
    vtkDebugMacro("No selection set.");
    }

  painter->ApplyPen(this->Pen);
  painter->ApplyBrush(this->Brush);

  float* data_base = static_cast<float*>(this->BasePoints->GetVoidPointer(0));
  float* data_extent = static_cast<float*>(this->ExtentPoints->GetVoidPointer(0));
  vtkIdType n = this->BasePoints->GetNumberOfPoints();
  float poly_points[10];
  if (n >= 2)
    {
    for (vtkIdType i = 0; i < (n-1); ++i)
      {
      poly_points[0] = data_base[2*i];
      poly_points[1] = data_base[2*i+1];
      poly_points[2] = data_base[2*i+2];
      poly_points[3] = data_base[2*i+3];

      poly_points[4] = data_extent[2*i+2];
      poly_points[5] = data_extent[2*i+3];
      poly_points[6] = data_extent[2*i];
      poly_points[7] = data_extent[2*i+1];

      // painter->DrawPoly(poly_points,5);
      painter->DrawQuad(poly_points);
      }
    }

  painter->GetPen()->SetLineType(vtkPen::SOLID_LINE);

  return true;
}

//-----------------------------------------------------------------------------
bool vtkPlotStacked::PaintLegend(vtkContext2D *painter, float rect[4])
{
  painter->ApplyPen(this->Pen);
  painter->ApplyBrush(this->Brush);
  painter->DrawRect(rect[0], rect[1], rect[2], rect[3]);
  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotStacked::GetBounds(double bounds[4])
{
  double base_bounds[4] = {0.0,0.0,0.0,0.0};
  double extent_bounds[4] = {0.0,0.0,0.0,0.0};


  if (this->BasePoints)
    {
    if (!this->BaseBadPoints)
      {
      this->BasePoints->GetBounds(base_bounds);
      }
    else
      {
      // There are bad points in the series - need to do this ourselves.
      this->CalculateBounds(this->BasePoints,this->BaseBadPoints,base_bounds);
      }
    }

  if (this->ExtentPoints)
    {
    if (!this->ExtentBadPoints)
      {
      this->ExtentPoints->GetBounds(extent_bounds);
      }
    else
      {
      this->CalculateBounds(this->ExtentPoints,this->ExtentBadPoints,extent_bounds);
      }
    }

  bounds[0] = vtkStackedMIN(base_bounds[0],extent_bounds[0]);
  bounds[1] = vtkStackedMAX(base_bounds[1],extent_bounds[1]);
  bounds[2] = vtkStackedMIN(base_bounds[2],extent_bounds[2]);
  bounds[3] = vtkStackedMAX(base_bounds[3],extent_bounds[3]);

  vtkDebugMacro(<< "Bounds: " << bounds[0] << "\t" << bounds[1] << "\t"
                << bounds[2] << "\t" << bounds[3]);
}

namespace
{

// Compare the two vectors, in X component only
bool compVector3fX(const vtkVector3f& v1, const vtkVector3f& v2)
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

// See if the point is within tolerance on x and between base and extent on Y.
bool inRange(const vtkVector2f& point, const vtkVector2f& tol,
             const vtkVector3f& current)
{
  if (current.X() > point.X() - tol.X() && current.X() < point.X() + tol.X() &&
      point.Y() > current.Y() && point.Y() < current.Z())
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
bool vtkPlotStacked::GetNearestPoint(const vtkVector2f& point,
                                    const vtkVector2f& tol,
                                    vtkVector2f* location)
{
  // Right now doing a simple bisector search of the array. This should be
  // revisited. Assumes the x axis is sorted, which should always be true for
  // line plots.
  if (!this->BasePoints)
    {
    return false;
    }
  vtkIdType n = this->BasePoints->GetNumberOfPoints();
  if (n < 2)
    {
    return false;
    }

  // Sort the data if it has not been done already.  We need to sort it
  // and collect the base and extent into the same vector since both will
  // get involved in range checking.
  if (!this->Sorted)
    {
    vtkVector2f* data_base =
        static_cast<vtkVector2f*>(this->BasePoints->GetVoidPointer(0));
    vtkVector2f* data_extent =
        static_cast<vtkVector2f*>(this->ExtentPoints->GetVoidPointer(0));
    this->Sorted = new VectorPIMPL();
    for (int i = 0; i < n; i++)
      {
      vtkVector3f combined(data_base[i].X(),data_base[i].Y(),data_extent[i].Y());
      this->Sorted->push_back(combined);
      }
    vtkstd::sort(this->Sorted->begin(), this->Sorted->end(), compVector3fX);
    }

  // Set up our search array, use the STL lower_bound algorithm
  VectorPIMPL::iterator low;
  VectorPIMPL &v = *this->Sorted;

  // Get the lowest point we might hit within the supplied tolerance
  vtkVector3f lowPoint(point.X()-tol.X(), 0.0f, 0.0f);
  low = vtkstd::lower_bound(v.begin(), v.end(), lowPoint, compVector3fX);

  // Now consider the y axis
  float highX = point.X() + tol.X();
  while (low != v.end())
    {
    if (inRange(point, tol, *low))
      {
      // If we're in range, the value that's interesting is the absolute value of 
      // the "wedge" at the closest point, not the base or extent by themselves
      location->SetX(low->X());
      location->SetY(low->Z() - low->Y());
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
bool vtkPlotStacked::SelectPoints(const vtkVector2f& min, const vtkVector2f& max)
{
  if (!this->BasePoints)
    {
    return false;
    }

  if (!this->Selection)
    {
    this->Selection = vtkIdTypeArray::New();
    }
  this->Selection->SetNumberOfTuples(0);

  // Iterate through all points and check whether any are in range
  vtkVector2f* data = static_cast<vtkVector2f*>(this->BasePoints->GetVoidPointer(0));
  vtkIdType n = this->BasePoints->GetNumberOfPoints();

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
void vtkPlotStacked::SetParent(vtkChartXY *parent)
{
  this->Parent = parent;
}

//-----------------------------------------------------------------------------
void vtkPlotStacked::SetVisible(bool visible)
{
  if (this->Visible != visible)
    this->Parent->SetStackPartipantsChanged();

  this->vtkPlot::SetVisible(visible);
}

//-----------------------------------------------------------------------------
namespace {

// Copy the two arrays into the points array
template<class A>
void CopyToPointsSwitch(vtkPoints2D *base_points, vtkPoints2D *extent_points, 
                        A *x_data, vtkDataArray *y_accumulator, vtkDataArray *y, int n)
{
  switch(y_accumulator->GetDataType())
    {
    vtkTemplateMacro(
        CopyToPoints(base_points, extent_points, x_data, static_cast<VTK_TT*>(y_accumulator->GetVoidPointer(0)), 
                     static_cast<VTK_TT*>(y->GetVoidPointer(0)),n));
    }
}

// Copy the two arrays into the points array
template<class A, class B>
void CopyToPoints(vtkPoints2D *base_points, vtkPoints2D *extent_points, A *x_data, B *y_accumulator_data, B *y_data, int n)
{
  base_points->SetNumberOfPoints(n);
  float* data_base = static_cast<float*>(base_points->GetVoidPointer(0));

  extent_points->SetNumberOfPoints(n);
  float* data_extent = static_cast<float*>(extent_points->GetVoidPointer(0));

  for (int i = 0; i < n; ++i)
    {
    data_base[2*i] = x_data[i];
    data_base[2*i+1] = y_accumulator_data[i];

    data_extent[2*i] = x_data[i];
    y_accumulator_data[i]  += y_data[i];
    data_extent[2*i+1] = y_accumulator_data[i];
    }
}

// Copy one array into the points array, use the index of that array as x
template<class A>
void CopyToPoints(vtkPoints2D *base_points, vtkPoints2D *extent_points, A *y_accumulator_data, A *y_data, int n)
{
  base_points->SetNumberOfPoints(n);
  float* data_base = static_cast<float*>(base_points->GetVoidPointer(0));

  extent_points->SetNumberOfPoints(n);
  float* data_extent = static_cast<float*>(extent_points->GetVoidPointer(0));

  for (int i = 0; i < n; ++i)
    {
    data_base[2*i] = static_cast<float>(i);
    data_base[2*i+1] = y_accumulator_data[i];

    data_extent[2*i] = static_cast<float>(i);
    y_accumulator_data[i] += y_data[i];
    data_extent[2*i+1] = y_accumulator_data[i];
    }
}

}

//-----------------------------------------------------------------------------
bool vtkPlotStacked::UpdateTableCache(vtkTable *table)
{
  // Get the x and ybase and yextent arrays (index 0 1 2 respectively)
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
                  << x->GetNumberOfTuples() << ", " << y->GetNumberOfTuples() << ", " <<
                  y->GetNumberOfTuples()
                  );
    this->BuildTime.Modified();
    return false;
    }

  if (!this->BasePoints)
    {
    this->BasePoints = vtkPoints2D::New();
    }
  if (!this->ExtentPoints)
    {
    this->ExtentPoints = vtkPoints2D::New();
    }

  // We use our parent chart's StackedPlot accumulator as our base points
  vtkDataArray *ybase = this->Parent->GetStackedPlotAccumulator(y->GetDataType(),y->GetNumberOfTuples());
  if (!ybase)
    {
    vtkErrorMacro("No accumulator could be obtained from parent");
    return false;
    }

  // Now copy the components into their new columns
  if (this->UseIndexForXSeries)
    {
    switch(ybase->GetDataType())
      {
        vtkTemplateMacro(
            CopyToPoints(this->BasePoints,
                         this->ExtentPoints,
                         static_cast<VTK_TT*>(ybase->GetVoidPointer(0)),
                         static_cast<VTK_TT*>(y->GetVoidPointer(0)),
                         ybase->GetNumberOfTuples()));
      }
    }
  else
    {
    switch(x->GetDataType())
      {
      vtkTemplateMacro(
          CopyToPointsSwitch(this->BasePoints,this->ExtentPoints,
                             static_cast<VTK_TT*>(x->GetVoidPointer(0)),
                             ybase, y, x->GetNumberOfTuples()));
      }
    }
  this->FixExtent();
  this->CalculateLogSeries();
  this->FindBadPoints();
  this->BasePoints->Modified();
  this->ExtentPoints->Modified();
  if (this->Sorted)
    {
    delete this->Sorted;
    this->Sorted = 0;
    }
  this->BuildTime.Modified();
  return true;
}

//-----------------------------------------------------------------------------
inline void vtkPlotStacked::FixExtent()
{
  if (!this->XAxis || !this->YAxis)
    {
    return;
    }
  float* data_base = static_cast<float*>(this->BasePoints->GetVoidPointer(0));
  float* data_extent = static_cast<float*>(this->ExtentPoints->GetVoidPointer(0));
  vtkIdType n = this->BasePoints->GetNumberOfPoints();

  // Extent must be greater than or equal to the base
  for (vtkIdType i = 0; i < n; ++i) 
    {
    if (data_base[2*i+1] > data_extent[2*i+1])
      {
      data_extent[2*i+1] = data_base[2*i+1];
      }
    }
}

//-----------------------------------------------------------------------------
inline void vtkPlotStacked::CalculateLogSeries()
{
  if (!this->XAxis || !this->YAxis)
    {
    return;
    }
  this->LogX = this->XAxis->GetLogScale();
  this->LogY = this->YAxis->GetLogScale();
  float* data_base = static_cast<float*>(this->BasePoints->GetVoidPointer(0));
  float* data_extent = static_cast<float*>(this->ExtentPoints->GetVoidPointer(0));
  vtkIdType n = this->BasePoints->GetNumberOfPoints();
  if (this->LogX)
    {
    for (vtkIdType i = 0; i < n; ++i)
      {
      data_base[2*i] = log10(data_base[2*i]);
      data_extent[2*i] = log10(data_extent[2*i]);
      }
    }
  if (this->LogY)
    {
    for (vtkIdType i = 0; i < n; ++i)
    {
    data_base[2*i+1] = log10(data_base[2*i+1]);
    data_extent[2*i+1] = log10(data_extent[2*i+1]);
    }
  }
}

//-----------------------------------------------------------------------------
inline void vtkPlotStacked::FindBadPoints()
{
  // This should be run after CalculateLogSeries as a final step.
  float* data_base = static_cast<float*>(this->BasePoints->GetVoidPointer(0));
  float* data_extent = static_cast<float*>(this->ExtentPoints->GetVoidPointer(0));
  vtkIdType n = this->BasePoints->GetNumberOfPoints();
  if (!this->BaseBadPoints)
    {
    this->BaseBadPoints = vtkIdTypeArray::New();
    }
  else
    {
    this->BaseBadPoints->SetNumberOfTuples(0);
    }
  if (!this->ExtentBadPoints)
    {
    this->ExtentBadPoints = vtkIdTypeArray::New();
    }
  else
    {
    this->ExtentBadPoints->SetNumberOfTuples(0);
    }

  // Scan through and find any bad points.
  for (vtkIdType i = 0; i < n; ++i)
    {
    vtkIdType p = 2*i;
    if (vtkMath::IsInf(data_base[p]) || vtkMath::IsInf(data_base[p+1]) ||
        vtkMath::IsNan(data_base[p]) || vtkMath::IsNan(data_base[p+1]))
      {
      this->BaseBadPoints->InsertNextValue(i);
      }
    if (vtkMath::IsInf(data_extent[p]) || vtkMath::IsInf(data_extent[p+1]) ||
        vtkMath::IsNan(data_extent[p]) || vtkMath::IsNan(data_extent[p+1]))
      {
      this->ExtentBadPoints->InsertNextValue(i);
      }
    }

  if (this->BaseBadPoints->GetNumberOfTuples() == 0)
    {
    this->BaseBadPoints->Delete();
    this->BaseBadPoints = NULL;
    }
  if (this->ExtentBadPoints->GetNumberOfTuples() == 0)
    {
    this->ExtentBadPoints->Delete();
    this->ExtentBadPoints = NULL;
    }
}

//-----------------------------------------------------------------------------
inline void vtkPlotStacked::CalculateBounds(vtkPoints2D *points, vtkIdTypeArray *badPoints, double bounds[4])
{
  // We can use the BadPoints array to skip the bad points
  if (!points || !badPoints)
    {
    return;
    }
  vtkIdType start = 0;
  vtkIdType end = 0;
  vtkIdType i = 0;
  vtkIdType nBad = badPoints->GetNumberOfTuples();
  vtkIdType nPoints = points->GetNumberOfPoints();
  if (badPoints->GetValue(i) == 0)
    {
    while (i < nBad && i == badPoints->GetValue(i))
      {
      start = badPoints->GetValue(i++) + 1;
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
    end = badPoints->GetValue(i++);
    }
  else
    {
    end = nPoints;
    }
  vtkVector2f* pts = static_cast<vtkVector2f*>(points->GetVoidPointer(0));

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
      end = badPoints->GetValue(i);
      }
    else
      {
      end = nPoints;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPlotStacked::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
