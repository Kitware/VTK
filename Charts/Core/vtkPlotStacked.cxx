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
#include "vtkFloatArray.h"
#include "vtkStringArray.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkColorSeries.h"
#include "vtkSmartPointer.h"
#include "vtkNew.h"

#include <vector>
#include <algorithm>
#include <map>

//-----------------------------------------------------------------------------
namespace {

// Compare the two vectors, in X component only
bool compVector2fX(const vtkVector2f& v1, const vtkVector2f& v2)
{
  if (v1.GetX() < v2.GetX())
    {
    return true;
    }
  else
    {
    return false;
    }
}

// Copy the two arrays into the points array
template<class A, class B>
void CopyToPoints(vtkPoints2D *points, vtkPoints2D *previous_points, A *a, B *b,
                  int n, double bds[4])
{
  points->SetNumberOfPoints(n);
  for (int i = 0; i < n; ++i)
    {
    double prev[] = {0.0,0.0};
    if (previous_points)
      previous_points->GetPoint(i,prev);
    double yi = b[i] + prev[1];
    points->SetPoint(i, a[i], yi);

    bds[0] = bds[0] < a[i] ? bds[0] : a[i];
    bds[1] = bds[1] > a[i] ? bds[1] : a[i];

    bds[2] = bds[2] < yi ? bds[2] : yi;
    bds[3] = bds[3] > yi ? bds[3] : yi;
    }
}

// Copy one array into the points array, use the index of that array as x
template<class A>
void CopyToPoints(
  vtkPoints2D *points, vtkPoints2D *previous_points, A *a, int n, double bds[4])
{
  bds[0] = 0.;
  bds[1] = n - 1.;
  points->SetNumberOfPoints(n);
  for (int i = 0; i < n; ++i)
    {
    double prev[] = {0.0,0.0};
    if (previous_points)
      previous_points->GetPoint(i,prev);
    double yi = a[i] + prev[1];
    points->SetPoint(i, i, yi);

    bds[2] = bds[2] < yi ? bds[2] : yi;
    bds[3] = bds[3] > yi ? bds[3] : yi;
    }
}

// Copy the two arrays into the points array
template<class A>
void CopyToPointsSwitch(vtkPoints2D *points, vtkPoints2D *previous_points, A *a,
                        vtkDataArray *b, int n, double bds[4])
{
  switch(b->GetDataType())
    {
    vtkTemplateMacro(
        CopyToPoints(points,previous_points, a,
                     static_cast<VTK_TT*>(b->GetVoidPointer(0)), n, bds));
    }
}

} // namespace

class vtkPlotStackedSegment : public vtkObject {
  public:
    vtkTypeMacro(vtkPlotStackedSegment,vtkObject);
    static vtkPlotStackedSegment *New();

    vtkPlotStackedSegment()
      {
      this->Stacked = 0;
      this->Points = 0;
      this->BadPoints = 0;
      this->Previous = 0;
      this->Sorted = false;
      }

    void Configure(
      vtkPlotStacked *stacked, vtkDataArray *x_array,
      vtkDataArray *y_array,vtkPlotStackedSegment *prev,
      double bds[4])
      {
      this->Stacked = stacked;
      this->Sorted = false;
      this->Previous = prev;

      if (!this->Points)
        {
        this->Points = vtkSmartPointer<vtkPoints2D>::New();
        }

      if (x_array)
        {
        switch (x_array->GetDataType())
          {
            vtkTemplateMacro(
              CopyToPointsSwitch(this->Points,this->Previous ? this->Previous->Points : 0,
                                 static_cast<VTK_TT*>(x_array->GetVoidPointer(0)),
                                 y_array,x_array->GetNumberOfTuples(), bds));
          }
        }
      else
        { // Using Index for X Series
        switch (y_array->GetDataType())
          {
          vtkTemplateMacro(
            CopyToPoints(this->Points, this->Previous ? this->Previous->Points : 0,
                         static_cast<VTK_TT*>(y_array->GetVoidPointer(0)),
                         y_array->GetNumberOfTuples(), bds));
          }
        }

      // Nothing works if we're not sorted on the X access
      vtkIdType n = this->Points->GetNumberOfPoints();
      vtkVector2f* data =
          static_cast<vtkVector2f*>(this->Points->GetVoidPointer(0));
      std::vector<vtkVector2f> v(data, data+n);
      std::sort(v.begin(), v.end(), compVector2fX);

      this->CalculateLogSeries();
      this->FindBadPoints();
      }

    void CalculateLogSeries()
      {
      vtkAxis *xAxis = this->Stacked->GetXAxis();
      vtkAxis *yAxis = this->Stacked->GetYAxis();

      if (!xAxis || !yAxis)
        {
        return;
        }

      bool logX = xAxis->GetLogScaleActive();
      bool logY = yAxis->GetLogScaleActive();

      float* data = static_cast<float*>(this->Points->GetVoidPointer(0));

      vtkIdType n = this->Points->GetNumberOfPoints();
      if (logX)
        {
        for (vtkIdType i = 0; i < n; ++i)
          {
          data[2*i] = log10(data[2*i]);
          }
        }
      if (logY)
        {
        for (vtkIdType i = 0; i < n; ++i)
          {
          data[2*i+1] = log10(data[2*i+1]);
          }
        }
      }

    void FindBadPoints()
      {
      // This should be run after CalculateLogSeries as a final step.
      float* data = static_cast<float*>(this->Points->GetVoidPointer(0));
      vtkIdType n = this->Points->GetNumberOfPoints();
      if (!this->BadPoints)
        {
        this->BadPoints = vtkSmartPointer<vtkIdTypeArray>::New();
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
        this->BadPoints = 0;
        }
      }

      void GetBounds(double bounds[4])
        {
        bounds[0] = bounds[1] = bounds[2] = bounds[3] = 0.0;
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
        }


      void CalculateBounds(double bounds[4])
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


    void Paint(vtkContext2D *painter, vtkPen *pen, vtkBrush *brush)
      {
      painter->ApplyPen(pen);
      painter->ApplyBrush(brush);
      int n = this->Points->GetNumberOfPoints();
      float *data_extent = vtkFloatArray::SafeDownCast(this->Points->GetData())->GetPointer(0);
      float *data_base = 0;
      if (this->Previous)
        data_base = vtkFloatArray::SafeDownCast(this->Previous->Points->GetData())->GetPointer(0);

      if (n >= 2)
        {
        float poly_points[8];

        for (int i = 0; i < (n - 1); ++i)
          {
          if (data_base)
            {
            poly_points[0] = data_base[2*i];
            poly_points[1] = data_base[2*i+1];
            poly_points[2] = data_base[2*i+2];
            poly_points[3] = data_base[2*i+3];
            }
          else
            {
            poly_points[0] = data_extent[2*i];  // Use the same X as extent
            poly_points[1] = 0.0;
            poly_points[2] = data_extent[2*i+2]; // Use the same X as extent
            poly_points[3] = 0.0;
            }
          poly_points[4] = data_extent[2*i+2];
          poly_points[5] = data_extent[2*i+3];
          poly_points[6] = data_extent[2*i];
          poly_points[7] = data_extent[2*i+1];

          painter->DrawQuad(poly_points);
          }
        }
      }

    bool GetNearestPoint(const vtkVector2f& point,
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
      // When searching, invert the behavior of the offset and
      // compensate for the half width overlap.
      std::vector<vtkVector2f>::iterator low;
      vtkVector2f lowPoint(point.GetX()-tol.GetX(), 0.0f);

      vtkVector2f* data =
          static_cast<vtkVector2f*>(this->Points->GetVoidPointer(0));
      std::vector<vtkVector2f> v(data, data+n);

      low = std::lower_bound(v.begin(), v.end(), lowPoint, compVector2fX);

      // Now consider the y axis.  We only worry about our extent
      // to the base because each segment is called in order and the
      // first positive wins.
      while (low != v.end())
        {
        if (low->GetX() - tol.GetX() > point.GetX())
          {
          break;
          }
        else if (low->GetX()-tol.GetX() < point.GetX() &&
                 low->GetX()+tol.GetX() > point.GetX())
          {
          if ((point.GetY() >= 0 && point.GetY() < low->GetY()) ||
              (point.GetY() < 0 && point.GetY() > low->GetY()))
              {
              *location = *low;
              return true;
              }
          }
        ++low;
        }
      return false;
      }

    void SelectPoints(const vtkVector2f& min,
                      const vtkVector2f& max,
                      vtkIdTypeArray *selection)
      {
      if (!this->Points)
        {
        return;
        }

      // Iterate through all points and check whether any are in range
      vtkVector2f* data = static_cast<vtkVector2f*>(this->Points->GetVoidPointer(0));
      vtkIdType n = this->Points->GetNumberOfPoints();

      for (vtkIdType i = 0; i < n; ++i)
        {
        if (data[i].GetX() >= min.GetX() && data[i].GetX() <= max.GetX() &&
            data[i].GetY() >= min.GetY() && data[i].GetY() <= max.GetY())
          {
          selection->InsertNextValue(i);
          }
        }
      }

    vtkSmartPointer<vtkPlotStackedSegment> Previous;
    vtkSmartPointer<vtkPoints2D> Points;
    vtkSmartPointer<vtkIdTypeArray> BadPoints;
    vtkPlotStacked *Stacked;
    bool Sorted;
};

vtkStandardNewMacro(vtkPlotStackedSegment);

//-----------------------------------------------------------------------------

class vtkPlotStackedPrivate {
  public:
    vtkPlotStackedPrivate(vtkPlotStacked *stacked) : Stacked(stacked) {}
    void Update()
      {
      this->Segments.clear();
      this->UnscaledInputBounds[0] = this->UnscaledInputBounds[2] = vtkMath::Inf();
      this->UnscaledInputBounds[1] = this->UnscaledInputBounds[3] = -vtkMath::Inf();
      }

    vtkPlotStackedSegment *AddSegment(vtkDataArray *x_array, vtkDataArray *y_array, vtkPlotStackedSegment *prev=0)
      {
      vtkSmartPointer<vtkPlotStackedSegment> segment = vtkSmartPointer<vtkPlotStackedSegment>::New();
      segment->Configure(this->Stacked,x_array,y_array,prev,this->UnscaledInputBounds);
      this->Segments.push_back(segment);
      return segment;
      }

    void PaintSegments(vtkContext2D *painter, vtkColorSeries *colorSeries,
                       vtkPen *pen, vtkBrush *brush)
      {
      int colorInSeries = 0;
      bool useColorSeries = this->Segments.size() > 1;
      for (std::vector<vtkSmartPointer<vtkPlotStackedSegment> >::iterator it = this->Segments.begin();
           it != this->Segments.end(); ++it)
        {
        if (useColorSeries && colorSeries)
          brush->SetColor(colorSeries->GetColorRepeating(colorInSeries++).GetData());
        (*it)->Paint(painter,pen,brush);
        }
      }


    vtkIdType GetNearestPoint(const vtkVector2f& point,
                              const vtkVector2f& tol,
                              vtkVector2f* location)
      {
      // Depends on the fact that we check the segments in order. Each
      // Segment only worrys about its own total extent from the base.
      int index = 0;
      for (std::vector<vtkSmartPointer<vtkPlotStackedSegment> >::iterator it = this->Segments.begin();
           it != this->Segments.end(); ++it)
        {
        if ((*it)->GetNearestPoint(point,tol,location))
          {
          return index;
          }
        ++index;
        }
      return -1;
      }

    void GetBounds(double bounds[4])
      {
      // Depends on the fact that we check the segments in order. Each
      // Segment only worrys about its own total extent from the base.
      double segment_bounds[4];
      for (std::vector<vtkSmartPointer<vtkPlotStackedSegment> >::iterator it = this->Segments.begin();
           it != this->Segments.end(); ++it)
        {
        (*it)->GetBounds(segment_bounds);
        if (segment_bounds[0] < bounds[0])
          {
          bounds[0] = segment_bounds[0];
          }
        if (segment_bounds[1] > bounds[1])
          {
          bounds[1] = segment_bounds[1];
          }
        if (segment_bounds[2] < bounds[2])
          {
          bounds[2] = segment_bounds[2];
          }
        if (segment_bounds[3] > bounds[3])
          {
          bounds[3] = segment_bounds[3];
          }
        }
      }


    void SelectPoints(const vtkVector2f& min,
                      const vtkVector2f& max,
                      vtkIdTypeArray *selection)
      {
      for (std::vector<vtkSmartPointer<vtkPlotStackedSegment> >::iterator it = this->Segments.begin();
           it != this->Segments.end(); ++it)
        {
        (*it)->SelectPoints(min,max,selection);
        }
      }

    std::vector<vtkSmartPointer<vtkPlotStackedSegment> > Segments;
    vtkPlotStacked *Stacked;
    std::map<int,std::string> AdditionalSeries;
    double UnscaledInputBounds[4];
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotStacked);

//-----------------------------------------------------------------------------
vtkPlotStacked::vtkPlotStacked()
{
  this->Private = new vtkPlotStackedPrivate(this);
  this->BaseBadPoints = NULL;
  this->ExtentBadPoints = NULL;
  this->AutoLabels = NULL;
  this->Pen->SetColor(0,0,0,0);
  this->LogX = false;
  this->LogY = false;
}

//-----------------------------------------------------------------------------
vtkPlotStacked::~vtkPlotStacked()
{
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

  delete this->Private;
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
          this->MTime > this->BuildTime)
    {
    vtkDebugMacro(<< "Updating cached values.");
    this->UpdateTableCache(table);
    }
  else if ((this->XAxis->GetMTime() > this->BuildTime) ||
           (this->YAxis->GetMTime() > this->BuildTime))
    {
    if (this->LogX != this->XAxis->GetLogScaleActive() ||
        this->LogY != this->YAxis->GetLogScaleActive())
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

  if (!this->Visible)
    {
    return false;
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

  this->Private->PaintSegments(painter,this->ColorSeries,this->Pen,this->Brush);

  return true;
}

//-----------------------------------------------------------------------------
bool vtkPlotStacked::PaintLegend(vtkContext2D *painter, const vtkRectf& rect,
                                 int legendIndex)
{
  if (this->ColorSeries)
    {
    vtkNew<vtkPen> pen;
    vtkNew<vtkBrush> brush;
    pen->SetColor(this->ColorSeries->GetColorRepeating(legendIndex).GetData());
    brush->SetColor(pen->GetColor());
    painter->ApplyPen(pen.GetPointer());
    painter->ApplyBrush(brush.GetPointer());
    }
  else
    {
    painter->ApplyPen(this->Pen);
    painter->ApplyBrush(this->Brush);
    }
  painter->DrawRect(rect[0], rect[1], rect[2], rect[3]);
  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotStacked::GetBounds(double bounds[4])
{
  this->Private->GetBounds(bounds);
}

//-----------------------------------------------------------------------------
void vtkPlotStacked::GetUnscaledInputBounds(double bounds[4])
{
  for (int i = 0; i < 4; ++i)
    {
    bounds[i] = this->Private->UnscaledInputBounds[i];
    }
}

//-----------------------------------------------------------------------------
vtkIdType vtkPlotStacked::GetNearestPoint(const vtkVector2f& point,
                                    const vtkVector2f& tol,
                                    vtkVector2f* location)
{
  return this->Private->GetNearestPoint(point,tol,location);
}

//-----------------------------------------------------------------------------
bool vtkPlotStacked::SelectPoints(const vtkVector2f& min, const vtkVector2f& max)
{
  if (!this->Selection)
    {
    this->Selection = vtkIdTypeArray::New();
    }
  this->Selection->SetNumberOfTuples(0);

  this->Private->SelectPoints(min,max,this->Selection);

  return this->Selection->GetNumberOfTuples() > 0;
}

//-----------------------------------------------------------------------------
vtkStringArray *vtkPlotStacked::GetLabels()
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
    this->AutoLabels->InsertNextValue(this->Data->GetInputArrayToProcess(1, this->Data->GetInput())->GetName());

    std::map< int, std::string >::iterator it;
    for ( it = this->Private->AdditionalSeries.begin(); it != this->Private->AdditionalSeries.end(); ++it )
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
  this->Private->Update();

  vtkPlotStackedSegment *prev = this->Private->AddSegment(x,y);

  std::map< int, std::string >::iterator it;

  for ( it = this->Private->AdditionalSeries.begin(); it != this->Private->AdditionalSeries.end(); ++it )
    {
    y = vtkDataArray::SafeDownCast(table->GetColumnByName((*it).second.c_str()));
    prev = this->Private->AddSegment(x,y,prev);
    }

  // Record if this update was done with Log scale.
  this->LogX = this->XAxis ? this->XAxis->GetLogScaleActive(): false;
  this->LogY = this->YAxis ? this->YAxis->GetLogScaleActive(): false;

  this->BuildTime.Modified();
  return true;
}

//-----------------------------------------------------------------------------
void vtkPlotStacked::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------

void vtkPlotStacked::SetInputArray(int index, const vtkStdString &name)
{
  if (index == 0 || index == 1)
    {
    vtkPlot::SetInputArray(index,name);
    }
  else
    {
    this->Private->AdditionalSeries[index] = name;
    }
  this->AutoLabels = 0; // No longer valid
}

//-----------------------------------------------------------------------------
void vtkPlotStacked::SetColorSeries(vtkColorSeries *colorSeries)
{
  if (this->ColorSeries == colorSeries)
    {
    return;
    }
  this->ColorSeries = colorSeries;
  this->Modified();
}

//-----------------------------------------------------------------------------
vtkColorSeries *vtkPlotStacked::GetColorSeries()
{
  return this->ColorSeries;
}
