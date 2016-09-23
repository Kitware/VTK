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

#include "vtkAxis.h"
#include "vtkContext2D.h"
#include "vtkRect.h"
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
#include "vtkSmartPointer.h"
#include "vtkColorSeries.h"
#include "vtkStringArray.h"
#include "vtkNew.h"
#include "vtkLookupTable.h"

#include "vtkObjectFactory.h"

#include <vector>
#include <algorithm>
#include <map>
#include <set>

//-----------------------------------------------------------------------------
namespace {

// Copy the two arrays into the points array
template<class A, class B>
void CopyToPoints(vtkPoints2D *points, vtkPoints2D *previousPoints, A *a, B *b,
                  int n, int logScale, const vtkRectd &ss)
{
  points->SetNumberOfPoints(n);
  float* data = static_cast<float*>(points->GetVoidPointer(0));
  float* prevData = NULL;
  if (previousPoints && static_cast<int>(previousPoints->GetNumberOfPoints()) == n)
  {
    prevData = static_cast<float*>(previousPoints->GetVoidPointer(0));
  }
  float prev = 0.0;
  for (int i = 0; i < n; ++i)
  {
    if (prevData)
    {
      prev = prevData[2 * i + 1];
    }
    A tmpA(static_cast<A>((a[i] + ss[0]) * ss[2]));
    B tmpB(static_cast<B>((b[i] + ss[1]) * ss[3]));
    data[2 * i]     = static_cast<float>((logScale & 1) ?
                                         log10(static_cast<double>(tmpA))
                                         : tmpA);
    data[2 * i + 1] = static_cast<float>((logScale & 2) ?
                                         log10(static_cast<double>(tmpB + prev))
                                         : (tmpB + prev));
  }
}

// Copy one array into the points array, use the index of that array as x
template<class A>
void CopyToPoints(vtkPoints2D *points, vtkPoints2D *previousPoints, A *a, int n,
                  int logScale, const vtkRectd &ss)
{
  points->SetNumberOfPoints(n);
  float* data = static_cast<float*>(points->GetVoidPointer(0));
  float* prevData = NULL;
  if (previousPoints && static_cast<int>(previousPoints->GetNumberOfPoints()) == n)
  {
    prevData = static_cast<float*>(previousPoints->GetVoidPointer(0));
  }
  float prev = 0.0;
  for (int i = 0; i < n; ++i)
  {
    if (prevData)
    {
      prev = prevData[2 * i + 1];
    }
    A tmpA(static_cast<A>((a[i] + ss[1]) * ss[3]));
    data[2 * i]     = static_cast<float>((logScale & 1) ?
                                         log10(static_cast<double>(i + 1.0))
                                         : i);
    data[2 * i + 1] = static_cast<float>((logScale & 2) ?
                                         log10(static_cast<double>(tmpA + prev))
                                         : (tmpA + prev));
  }
}

// Copy the two arrays into the points array
template<class A>
void CopyToPointsSwitch(vtkPoints2D *points, vtkPoints2D *previousPoints, A *a,
                        vtkDataArray *b, int n, int logScale,
                        const vtkRectd &ss)
{
  switch(b->GetDataType())
  {
    vtkTemplateMacro(
        CopyToPoints(points,previousPoints, a,
                     static_cast<VTK_TT*>(b->GetVoidPointer(0)), n, logScale,
                     ss));
  }
}

} // namespace

//-----------------------------------------------------------------------------

class vtkPlotBarSegment : public vtkObject {
  public:
    vtkTypeMacro(vtkPlotBarSegment, vtkObject);
    static vtkPlotBarSegment *New();

    vtkPlotBarSegment()
    {
      this->Bar = NULL;
      this->Points = NULL;
      this->Sorted = NULL;
      this->Previous = NULL;
      this->Colors = NULL;
    }

    ~vtkPlotBarSegment() VTK_OVERRIDE
    {
      delete this->Sorted;
    }

    void Configure(vtkPlotBar* bar, vtkDataArray* xArray, vtkDataArray* yArray,
                   vtkAxis* xAxis, vtkAxis* yAxis, vtkPlotBarSegment* prev)
    {
      this->Bar = bar;
      this->Previous = prev;
      if (!this->Points)
      {
        this->Points = vtkSmartPointer<vtkPoints2D>::New();
      }
      // For the atypical case that Configure is called on a non-fresh "this"
      delete this->Sorted;

      int logScale = (xAxis->GetLogScaleActive() ? 1 : 0) +
          (yAxis->GetLogScaleActive() ? 2 : 0);
      if (xArray)
      {
        switch (xArray->GetDataType())
        {
            vtkTemplateMacro(
              CopyToPointsSwitch(this->Points,this->Previous ? this->Previous->Points : 0,
                                 static_cast<VTK_TT*>(xArray->GetVoidPointer(0)),
                                 yArray, xArray->GetNumberOfTuples(), logScale,
                                 this->Bar->GetShiftScale()));
        }
      }
      else
      { // Using Index for X Series
        switch (yArray->GetDataType())
        {
          vtkTemplateMacro(
            CopyToPoints(this->Points, this->Previous ? this->Previous->Points : 0,
                         static_cast<VTK_TT*>(yArray->GetVoidPointer(0)),
                         yArray->GetNumberOfTuples(), logScale,
                         this->Bar->GetShiftScale()));
        }
      }
    }

    void Paint(vtkContext2D *painter, vtkPen *pen, vtkBrush *brush,
               float width, float offset, int orientation)
    {
      painter->ApplyPen(pen);
      painter->ApplyBrush(brush);
      int n = this->Points->GetNumberOfPoints();
      float *f =
          vtkArrayDownCast<vtkFloatArray>(this->Points->GetData())->GetPointer(0);
      float *p = NULL;
      if (this->Previous)
      {
        p = vtkArrayDownCast<vtkFloatArray>(
              this->Previous->Points->GetData())->GetPointer(0);
      }

      for (int i = 0; i < n; ++i)
      {
        if (this->Colors)
        {
          painter->GetBrush()->SetColor(vtkColor4ub(this->Colors->GetPointer(i * 4)));
        }
        if (orientation == vtkPlotBar::VERTICAL)
        {
          if (p)
          {
            painter->DrawRect(f[2 * i] - (width / 2) - offset, p[2 * i + 1],
                              width, f[2 * i + 1] - p[2 * i + 1]);
          }
          else
          {
            painter->DrawRect(f[2 * i] - (width / 2) - offset, 0.0,
                              width, f[2 * i + 1]);
          }
        }
        else // HORIZONTAL orientation
        {
          if (p)
          {
            painter->DrawRect(p[2 * i + 1], f[2 * i] - (width / 2) - offset,
                              f[2 * i + 1] - p[2 * i + 1], width);
          }
          else
          {
            painter->DrawRect(0.0, f[2 * i] - (width / 2) - offset,
                              f[2 * i + 1], width);
          }
        }
      }
      // Paint selections if there are any.
      vtkIdTypeArray *selection = this->Bar->GetSelection();
      if (!selection)
      {
        return;
      }
      painter->ApplyBrush(this->Bar->GetSelectionBrush());
      for (vtkIdType j = 0; j < selection->GetNumberOfTuples(); ++j)
      {
        int i = selection->GetValue(j);
        if (orientation == vtkPlotBar::VERTICAL)
        {
          if (p)
          {
            painter->DrawRect(f[2 * i] - (width / 2) - offset, p[2 * i + 1],
                              width, f[2 * i + 1] - p[2 * i + 1]);
          }
          else
          {
            painter->DrawRect(f[2 * i] - (width / 2) - offset, 0.0,
                              width, f[2 * i + 1]);
          }
        }
        else // HORIZONTAL orientation
        {
          if (p)
          {
            painter->DrawRect(p[2 * i + 1], f[2 * i] - (width / 2) - offset,
                              f[2 * i + 1] - p[2 * i + 1], width);
          }
          else
          {
            painter->DrawRect(0.0, f[2 * i] - (width / 2) - offset,
                              f[2 * i + 1], width);
          }
        }
      }
    }

    vtkIdType GetNearestPoint(const vtkVector2f& point, vtkVector2f* location,
                              float width, float offset, int orientation)
    {
      if (!this->Points && this->Points->GetNumberOfPoints())
      {
        return -1;
      }

      // The extent of any given bar is half a width on either
      // side of the point with which it is associated.
      float halfWidth = width / 2.0;

      // If orientation is VERTICAL, search normally. For HORIZONTAL,
      // simply transpose the X and Y coordinates of the target, as the rest
      // of the search uses the assumption that X = bar position, Y = bar
      // value; swapping the target X and Y is simpler than swapping the
      // X and Y of all the other references to the bar data.
      vtkVector2f targetPoint(point);
      if (orientation == vtkPlotBar::HORIZONTAL)
      {
        targetPoint.Set(point.GetY(), point.GetX()); // Swap x and y
      }

      this->CreateSortedPoints();

      // Get the left-most bar we might hit
      vtkIndexedVector2f lowPoint;
      lowPoint.index = 0;
      lowPoint.pos = vtkVector2f(targetPoint.GetX()-(offset * -1)-halfWidth, 0.0f);

      // Set up our search array, use the STL lower_bound algorithm
      VectorPIMPL::iterator low;
      VectorPIMPL &v = *this->Sorted;
      low = std::lower_bound(v.begin(), v.end(), lowPoint);

      while (low != v.end())
      {
        // Does the bar surround the point?
        if (low->pos.GetX()-halfWidth-offset < targetPoint.GetX() &&
            low->pos.GetX()+halfWidth-offset > targetPoint.GetX())
        {
          // Is the point within the vertical extent of the bar?
          if ((targetPoint.GetY() >= 0 && targetPoint.GetY() < low->pos.GetY()) ||
              (targetPoint.GetY() < 0 && targetPoint.GetY() > low->pos.GetY()))
          {
            *location = low->pos;
            return low->index;
          }
        }
        // Is the left side of the bar beyond the point?
        if (low->pos.GetX()-offset-halfWidth > targetPoint.GetX())
        {
          break;
        }
        ++low;
      }
      return -1;
    }

    void CreateSortedPoints()
    {
      // Sorted points, used when searching for the nearest point.
      if (!this->Sorted)
      {
        vtkIdType n = this->Points->GetNumberOfPoints();
        vtkVector2f* data =
            static_cast<vtkVector2f*>(this->Points->GetVoidPointer(0));
        this->Sorted = new VectorPIMPL(data, n);
        std::sort(this->Sorted->begin(), this->Sorted->end());
      }
    }

    bool SelectPoints(const vtkVector2f& min, const vtkVector2f& max,
                      float width, float offset, int orientation)
    {
      if (!this->Points)
      {
        return false;
      }

      this->CreateSortedPoints();

      // If orientation is VERTICAL, search normally. For HORIZONTAL,
      // transpose the selection box.
      vtkVector2f targetMin(min);
      vtkVector2f targetMax(max);
      if (orientation == vtkPlotBar::HORIZONTAL)
      {
        targetMin.Set(min.GetY(), min.GetX());
        targetMax.Set(max.GetY(), max.GetX());
      }

      // The extent of any given bar is half a width on either
      // side of the point with which it is associated.
      float halfWidth = width / 2.0;

      // Get the lowest X coordinate we might hit
      vtkIndexedVector2f lowPoint;
      lowPoint.index = 0;
      lowPoint.pos = vtkVector2f(targetMin.GetX()-(offset * -1)-halfWidth, 0.0f);

      // Set up our search array, use the STL lower_bound algorithm
      VectorPIMPL::iterator low;
      VectorPIMPL &v = *this->Sorted;
      low = std::lower_bound(v.begin(), v.end(), lowPoint);

      std::vector<vtkIdType> selected;

      while (low != v.end())
      {
        // Is the bar's X coordinates at least partially within the box?
        if (low->pos.GetX()+halfWidth-offset > targetMin.GetX() &&
            low->pos.GetX()-halfWidth-offset < targetMax.GetX())
        {
          // Is the bar within the vertical extent of the box?
          if ((targetMin.GetY() > 0 && low->pos.GetY() >= targetMin.GetY()) ||
              (targetMax.GetY() < 0 && low->pos.GetY() <= targetMax.GetY()) ||
              (targetMin.GetY() < 0 && targetMax.GetY() > 0))
          {
            selected.push_back(low->index);
          }
        }
        // Is the left side of the bar beyond the box?
        if (low->pos.GetX()-offset-halfWidth > targetMax.GetX())
        {
          break;
        }
        ++low;
      }

      if (selected.empty())
      {
        return false;
      }
      else
      {
        this->Bar->GetSelection()->SetNumberOfTuples(selected.size());
        vtkIdType *ptr =
            static_cast<vtkIdType *>(this->Bar->GetSelection()->GetVoidPointer(0));
        for (size_t i = 0; i < selected.size(); ++i)
        {
          ptr[i] = selected[i];
        }
        this->Bar->GetSelection()->Modified();
        return true;
      }
    }

    // Indexed vector for sorting
    struct vtkIndexedVector2f
    {
      size_t index;
      vtkVector2f pos;

      // Compare two vtkIndexedVector2f, in X component only
      bool operator<(const vtkIndexedVector2f& v2) const
      {
        return (this->pos.GetX() < v2.pos.GetX());
      }
    };

    class VectorPIMPL : public std::vector<vtkIndexedVector2f>
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

    vtkSmartPointer<vtkPlotBarSegment> Previous;
    vtkSmartPointer<vtkPoints2D> Points;
    vtkPlotBar *Bar;
    VectorPIMPL* Sorted;
    vtkVector2d ScalingFactor;
    vtkUnsignedCharArray *Colors;
};

vtkStandardNewMacro(vtkPlotBarSegment);

class vtkPlotBarPrivate
{
public:
  vtkPlotBarPrivate(vtkPlotBar *bar) : Bar(bar) {}

  void Update()
  {
    this->Segments.clear();
  }

  vtkPlotBarSegment* AddSegment(
    vtkDataArray *xArray, vtkDataArray *yArray,
    vtkAxis* xAxis, vtkAxis* yAxis, vtkPlotBarSegment *prev = 0)
  {
    vtkNew<vtkPlotBarSegment> segment;
    segment->Configure(this->Bar, xArray, yArray, xAxis, yAxis, prev);
    this->Segments.push_back(segment.GetPointer());
    return segment.GetPointer();
  }

  void PaintSegments(vtkContext2D *painter, vtkColorSeries *colorSeries,
                     vtkPen *pen, vtkBrush *brush, float width, float offset,
                     int orientation)
  {
    int colorInSeries = 0;
    bool useColorSeries = this->Segments.size() > 1;
    for (std::vector<vtkSmartPointer<vtkPlotBarSegment> >::iterator it =
         this->Segments.begin(); it != this->Segments.end(); ++it)
    {
      if (useColorSeries && colorSeries)
      {
        brush->SetColor(colorSeries->GetColorRepeating(colorInSeries++).GetData());
      }
      (*it)->Paint(painter, pen, brush, width, offset, orientation);
    }
  }


  vtkIdType GetNearestPoint(const vtkVector2f& point, vtkVector2f* location,
                            float width, float offset, int orientation,
                            vtkIdType* segmentIndex)
  {
    vtkIdType segmentIndexCtr = 0;
    for (std::vector<vtkSmartPointer<vtkPlotBarSegment> >::iterator it =
           this->Segments.begin(); it != this->Segments.end(); ++it)
    {
      int barIndex = (*it)->GetNearestPoint(point,location,width,offset,orientation);
      if (barIndex != -1)
      {
        if (segmentIndex)
        {
          *segmentIndex = segmentIndexCtr;
        }
        return barIndex;
      }
      ++segmentIndexCtr;
    }
    if (segmentIndex)
    {
      *segmentIndex = -1;
    }
    return -1;
  }

  bool SelectPoints(const vtkVector2f& min, const vtkVector2f& max,
                    float width, float offset, int orientation)
  {
    // Selection functionality not supported for stacked plots (yet)...
    if (this->Segments.size() != 1)
    {
      return false;
    }

    return this->Segments[0]->SelectPoints(min, max, width, offset, orientation);
  }

  std::vector<vtkSmartPointer<vtkPlotBarSegment> > Segments;
  vtkPlotBar *Bar;
  std::map<int,std::string> AdditionalSeries;
  vtkStdString GroupName;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotBar);

//-----------------------------------------------------------------------------
vtkPlotBar::vtkPlotBar()
{
  this->Private = new vtkPlotBarPrivate(this);
  this->Points = NULL;
  this->AutoLabels = NULL;
  this->Width = 1.0;
  this->Pen->SetWidth(1.0);
  this->Offset = 1.0;
  this->ColorSeries = NULL;
  this->Orientation = vtkPlotBar::VERTICAL;
  this->ScalarVisibility = false;
  this->LogX = false;
  this->LogY = false;
}

//-----------------------------------------------------------------------------
vtkPlotBar::~vtkPlotBar()
{
  if (this->Points)
  {
    this->Points->Delete();
    this->Points = NULL;
  }
  delete this->Private;
}

//-----------------------------------------------------------------------------
void vtkPlotBar::Update()
{
  if (!this->Visible)
  {
    return;
  }
  // First check if we have an input
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
  else if ((this->XAxis->GetMTime() > this->BuildTime) ||
           (this->YAxis->GetMTime() > this->BuildTime))
  {
    if ((this->LogX != this->XAxis->GetLogScale()) ||
        (this->LogY != this->YAxis->GetLogScale()))
    {
      this->LogX = this->XAxis->GetLogScale();
      this->LogY = this->YAxis->GetLogScale();
      this->UpdateTableCache(table);
    }
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

  this->Private->PaintSegments(painter,this->ColorSeries, this->Pen,this->Brush,
                               this->Width, this->Offset, this->Orientation);

  return true;
}

//-----------------------------------------------------------------------------
bool vtkPlotBar::PaintLegend(vtkContext2D *painter, const vtkRectf& rect,
                             int legendIndex)
{
  if (this->ColorSeries)
  {
    this->Brush->SetColor(
          this->ColorSeries->GetColorRepeating(legendIndex).GetData());
  }

  painter->ApplyPen(this->Pen);
  painter->ApplyBrush(this->Brush);
  painter->DrawRect(rect[0], rect[1], rect[2], rect[3]);
  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotBar::GetBounds(double bounds[4], bool unscaled)
{
  int seriesLow, seriesHigh, valuesLow, valuesHigh;
  // Don't re-orient the axes for vertical plots or unscaled bounds:
  if (this->Orientation ==  vtkPlotBar::VERTICAL || unscaled)
  {
    seriesLow = 0; // Xmin
    seriesHigh = 1; // Xmax
    valuesLow = 2; // Ymin
    valuesHigh = 3; // Ymax
  }
  else // HORIZONTAL orientation
  {
    seriesLow = 2; // Ymin
    seriesHigh = 3; // Ymax
    valuesLow = 0; // Xmin
    valuesHigh = 1; // Xmax
  }

  // Get the x and y arrays (index 0 and 1 respectively)
  vtkTable *table = this->Data->GetInput();
  vtkDataArray* x = this->UseIndexForXSeries ?
                    0 : this->Data->GetInputArrayToProcess(0, table);
  vtkDataArray *y = this->Data->GetInputArrayToProcess(1, table);
  if (!y)
  {
    return;
  }

  if (this->UseIndexForXSeries)
  {
    bounds[seriesLow] = 0 - (this->Width / 2 );
    bounds[seriesHigh] = y->GetNumberOfTuples() + (this->Width/2);
  }
  else if (x)
  {
    x->GetRange(&bounds[seriesLow]);
    // We surround our point by Width/2 on either side
    bounds[seriesLow] -= this->Width / 2.0 + this->Offset;
    bounds[seriesHigh] += this->Width / 2.0 - this->Offset;
  }
  else
  {
    return;
  }

  y->GetRange(&bounds[valuesLow]);

  double yRange[2];
  std::map< int, std::string >::iterator it;
  for ( it = this->Private->AdditionalSeries.begin(); it !=
                  this->Private->AdditionalSeries.end(); ++it )
  {
    y = vtkArrayDownCast<vtkDataArray>(table->GetColumnByName((*it).second.c_str()));
    y->GetRange(yRange);
    bounds[valuesHigh] += yRange[1];
  }

  // Bar plots always have one of the value bounds at the origin
  if (bounds[valuesLow] > 0.0f)
  {
    bounds[valuesLow] = 0.0;
  }
  else if (bounds[valuesHigh] < 0.0f)
  {
    bounds[valuesHigh] = 0.0;
  }

  if (unscaled)
  {
    vtkAxis* axes[2];
    axes[seriesLow / 2] = this->GetXAxis();
    axes[valuesLow / 2] = this->GetYAxis();
    if (axes[0]->GetLogScaleActive())
    {
      bounds[0] = log10(fabs(bounds[0]));
      bounds[1] = log10(fabs(bounds[1]));
    }
    if (axes[1]->GetLogScaleActive())
    {
      bounds[2] = log10(fabs(bounds[2]));
      bounds[3] = log10(fabs(bounds[3]));
    }
  }
  vtkDebugMacro(<< "Bounds: " << bounds[0] << "\t" << bounds[1] << "\t"
                << bounds[2] << "\t" << bounds[3]);
}

//-----------------------------------------------------------------------------
void vtkPlotBar::GetBounds(double bounds[4])
{
  this->GetBounds(bounds, false);
}

//-----------------------------------------------------------------------------
void vtkPlotBar::GetUnscaledInputBounds(double bounds[4])
{
  this->GetBounds(bounds, true);
}

//-----------------------------------------------------------------------------
void vtkPlotBar::SetOrientation(int orientation)
{
  if (orientation < 0 || orientation > 1)
  {
    vtkErrorMacro("Error, invalid orientation value supplied: " << orientation)
    return;
  }
  this->Orientation = orientation;
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
  double rgba[4];
  this->Brush->GetColorF(rgba);
  rgb[0] = rgba[0];
  rgb[1] = rgba[1];
  rgb[2] = rgba[2];
}

//-----------------------------------------------------------------------------
vtkIdType vtkPlotBar::GetNearestPoint(const vtkVector2f& point,
                                      const vtkVector2f&,
                                      vtkVector2f* location)
{
  return this->Private->GetNearestPoint(point, location, this->Width,
                                        this->Offset, this->Orientation, 0);
}

//-----------------------------------------------------------------------------
vtkIdType vtkPlotBar::GetNearestPoint(const vtkVector2f& point,
                                      const vtkVector2f&,
                                      vtkVector2f* location,
                                      vtkIdType* segmentIndex)
{
  return this->Private->GetNearestPoint(point, location, this->Width,
                                        this->Offset, this->Orientation,
                                        segmentIndex);
}

//-----------------------------------------------------------------------------
vtkStringArray * vtkPlotBar::GetLabels()
{
  // If the label string is empty, return the y column name
  if (this->Labels)
  {
    return this->Labels;
  }
  else if (this->AutoLabels)
  {
    return this->AutoLabels;
  }
  else if (this->Data->GetInput() &&
           this->Data->GetInputArrayToProcess(1, this->Data->GetInput()))
  {
    this->AutoLabels = vtkSmartPointer<vtkStringArray>::New();
    this->AutoLabels->InsertNextValue(this->Data->GetInputArrayToProcess(1,
                                      this->Data->GetInput())->GetName());

    std::map< int, std::string >::iterator it;
    for ( it = this->Private->AdditionalSeries.begin();
          it != this->Private->AdditionalSeries.end(); ++it )
    {
      this->AutoLabels->InsertNextValue((*it).second);
    }
    return this->AutoLabels;
  }
  else
  {
    return NULL;
  }
}

void vtkPlotBar::SetGroupName(const vtkStdString &name)
{
  if (this->Private->GroupName != name)
  {
    this->Private->GroupName = name;
    this->Modified();
  }
}

vtkStdString vtkPlotBar::GetGroupName()
{
  return this->Private->GroupName;
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

  this->Private->Update();

  vtkPlotBarSegment *prev = this->Private->AddSegment(x, y, this->GetXAxis(),
                                                      this->GetYAxis());

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
      this->Colors = this->LookupTable->MapScalars(c,
                                                   VTK_COLOR_MODE_MAP_SCALARS,
                                                   -1);
      prev->Colors = this->Colors;
      this->Colors->Delete();
    }
    else
    {
      this->Colors = NULL;
      prev->Colors = NULL;
    }
  }

  std::map< int, std::string >::iterator it;

  for ( it = this->Private->AdditionalSeries.begin();
        it != this->Private->AdditionalSeries.end(); ++it )
  {
    y = vtkArrayDownCast<vtkDataArray>(table->GetColumnByName((*it).second.c_str()));
    prev = this->Private->AddSegment(x,y, this->GetXAxis(), this->GetYAxis(),prev);
  }

  this->TooltipDefaultLabelFormat.clear();
  // Set the default tooltip according to the segments
  if (this->Private->Segments.size() > 1)
  {
    this->TooltipDefaultLabelFormat = "%s: ";
  }
  if (this->IndexedLabels)
  {
    this->TooltipDefaultLabelFormat += "%i: ";
  }
  this->TooltipDefaultLabelFormat += "%x,  %y";

  this->BuildTime.Modified();
  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotBar::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------

void vtkPlotBar::SetInputArray(int index, const vtkStdString &name)
{
  if (index == 0 || index == 1)
  {
    vtkPlot::SetInputArray(index, name);
  }
  else
  {
    this->Private->AdditionalSeries[index] = name;
  }
  this->AutoLabels = NULL; // No longer valid
}

//-----------------------------------------------------------------------------
void vtkPlotBar::SetColorSeries(vtkColorSeries *colorSeries)
{
  if (this->ColorSeries == colorSeries)
  {
    return;
  }
  this->ColorSeries = colorSeries;
  this->Modified();
}


//-----------------------------------------------------------------------------
vtkColorSeries *vtkPlotBar::GetColorSeries()
{
  return this->ColorSeries;
}

//-----------------------------------------------------------------------------
void vtkPlotBar::SetLookupTable(vtkScalarsToColors *lut)
{
  if (this->LookupTable != lut)
  {
    this->LookupTable = lut;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
vtkScalarsToColors *vtkPlotBar::GetLookupTable()
{
  if (!this->LookupTable)
  {
    this->CreateDefaultLookupTable();
  }
  return this->LookupTable.Get();
}

//-----------------------------------------------------------------------------
void vtkPlotBar::CreateDefaultLookupTable()
{
  vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
  // rainbow - blue to red
  lut->SetHueRange(0.6667, 0.0);
  lut->Build();
  double bounds[4];
  this->GetBounds(bounds);
  lut->SetRange(bounds[0], bounds[1]);
  this->LookupTable = lut;
}

//-----------------------------------------------------------------------------
void vtkPlotBar::SelectColorArray(const vtkStdString& arrayName)
{
  if (this->ColorArrayName == arrayName)
  {
    return;
  }
  vtkTable *table = this->Data->GetInput();
  if (!table)
  {
    vtkWarningMacro(<< "SelectColorArray called with no input table set.");
    return;
  }
  for (vtkIdType i = 0; i < table->GetNumberOfColumns(); ++i)
  {
    if (arrayName == table->GetColumnName(i))
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
void vtkPlotBar::SelectColorArray(vtkIdType arrayNum)
{
  vtkTable *table = this->Data->GetInput();
  if (!table)
  {
    vtkWarningMacro(<< "SelectColorArray called with no input table set.");
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
vtkStdString vtkPlotBar::GetColorArrayName()
{
  return this->ColorArrayName;
}

//-----------------------------------------------------------------------------
bool vtkPlotBar::SelectPoints(const vtkVector2f& min, const vtkVector2f& max)
{
  if (!this->Selection)
  {
    this->Selection = vtkIdTypeArray::New();
  }
  this->Selection->SetNumberOfTuples(0);

  return this->Private->SelectPoints(min, max, this->Width, this->Offset,
                                     this->Orientation);
}

//-----------------------------------------------------------------------------
vtkStdString vtkPlotBar::GetTooltipLabel(const vtkVector2d &plotPos,
                                         vtkIdType seriesIndex,
                                         vtkIdType segmentIndex)
{
  vtkStdString baseLabel = Superclass::GetTooltipLabel(plotPos, seriesIndex,
                                                       segmentIndex);
  vtkStdString tooltipLabel;
  bool escapeNext = false;
  for (size_t i = 0; i < baseLabel.length(); ++i)
  {
    if (escapeNext)
    {
      switch (baseLabel[i])
      {
        case 's':
          if (segmentIndex >= 0 && this->GetLabels() &&
              segmentIndex < this->GetLabels()->GetNumberOfTuples())
          {
            tooltipLabel += this->GetLabels()->GetValue(segmentIndex);
          }
          break;
        default: // If no match, insert the entire format tag
          tooltipLabel += "%";
          tooltipLabel += baseLabel[i];
          break;
      }
      escapeNext = false;
    }
    else
    {
      if (baseLabel[i] == '%')
      {
        escapeNext = true;
      }
      else
      {
        tooltipLabel += baseLabel[i];
      }
    }
  }
  return tooltipLabel;
}

//-----------------------------------------------------------------------------
int vtkPlotBar::GetBarsCount()
{
  vtkTable *table = this->Data->GetInput();
  if (!table)
  {
    vtkWarningMacro(<< "GetBarsCount called with no input table set.");
    return 0;
  }
  vtkDataArray* x = this->Data->GetInputArrayToProcess(0, table);
  return x ? x->GetNumberOfTuples() : 0;
}

//-----------------------------------------------------------------------------
void vtkPlotBar::GetDataBounds(double bounds[2])
{
  assert(bounds);
  // Get the x and y arrays (index 0 and 1 respectively)
  vtkTable *table = this->Data->GetInput();
  if (!table)
  {
    vtkWarningMacro(<< "GetDataBounds called with no input table set.");
    bounds[0] = VTK_DOUBLE_MAX;
    bounds[1] = VTK_DOUBLE_MIN;
    return;
  }
  vtkDataArray* x = this->Data->GetInputArrayToProcess(0, table);
  if (x)
  {
    x->GetRange(bounds);
  }
}
