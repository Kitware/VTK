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

#include "vtkObjectFactory.h"

#include <vector>
#include <algorithm>
#include <map>
#include <set>

//-----------------------------------------------------------------------------
namespace {

// Copy the two arrays into the points array
template<class A, class B>
void CopyToPoints(vtkPoints2D *points, vtkPoints2D *previous_points, A *a, B *b,
                  int n)
{
  points->SetNumberOfPoints(n);
  for (int i = 0; i < n; ++i)
    {
    double prev[] = {0.0,0.0};
    if (previous_points)
      previous_points->GetPoint(i,prev);
    points->SetPoint(i, a[i], b[i] + prev[1]);
    }
}

// Copy one array into the points array, use the index of that array as x
template<class A>
void CopyToPoints(vtkPoints2D *points, vtkPoints2D *previous_points, A *a, int n)
{
  points->SetNumberOfPoints(n);
  for (int i = 0; i < n; ++i)
    {
    double prev[] = {0.0,0.0};
    if (previous_points)
      previous_points->GetPoint(i,prev);
    points->SetPoint(i, i, a[i] + prev[1]);
    }
}

// Copy the two arrays into the points array
template<class A>
void CopyToPointsSwitch(vtkPoints2D *points, vtkPoints2D *previous_points, A *a,
                        vtkDataArray *b, int n)
{
  switch(b->GetDataType())
    {
    vtkTemplateMacro(
        CopyToPoints(points,previous_points, a,
                     static_cast<VTK_TT*>(b->GetVoidPointer(0)), n));
    }
}

// Indexed vector for sorting
struct vtkIndexedVector2f
{
  size_t index;
  vtkVector2f pos;
};

// Compare two vtkIndexedVector2f, in X component only
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

} // namespace

//-----------------------------------------------------------------------------

class vtkPlotBarSegment : public vtkObject {
  public:
    vtkTypeMacro(vtkPlotBarSegment,vtkObject);
    static vtkPlotBarSegment *New();

    vtkPlotBarSegment()
      {
      this->Bar = NULL;
      this->Points = NULL;
      this->Sorted = NULL;
      this->Previous = NULL;
      this->SelectionSet = NULL;
      }

    ~vtkPlotBarSegment()
      {
      delete this->Sorted;
      delete this->SelectionSet;
      }

    void Configure(vtkPlotBar *bar, vtkDataArray *x_array,
                   vtkDataArray *y_array, vtkPlotBarSegment *prev)
      {
      this->Bar = bar;
      this->Previous = prev;
      if (!this->Points)
        {
        this->Points = vtkSmartPointer<vtkPoints2D>::New();
        }
      // For the atypical case that Configure is called on a non-fresh "this"
      delete this->Sorted;
      delete this->SelectionSet;

      if (x_array)
        {
        switch (x_array->GetDataType())
          {
            vtkTemplateMacro(
              CopyToPointsSwitch(this->Points,this->Previous ? this->Previous->Points : 0,
                                 static_cast<VTK_TT*>(x_array->GetVoidPointer(0)),
                                 y_array,x_array->GetNumberOfTuples()));
          }
        }
      else
        { // Using Index for X Series
        switch (y_array->GetDataType())
          {
          vtkTemplateMacro(
            CopyToPoints(this->Points, this->Previous ? this->Previous->Points : 0,
                         static_cast<VTK_TT*>(y_array->GetVoidPointer(0)),
                         y_array->GetNumberOfTuples()));
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
          vtkFloatArray::SafeDownCast(this->Points->GetData())->GetPointer(0);
      float *p = NULL;
      if (this->Previous)
        p = vtkFloatArray::SafeDownCast(
              this->Previous->Points->GetData())->GetPointer(0);

      for (int i = 0; i < n; ++i)
        {
        if (this->SelectionSet &&
            this->SelectionSet->find(static_cast<vtkIdType>(i)) !=
            this->SelectionSet->end())
          {
          painter->GetBrush()->SetColor(255, 50, 0, 150);
          }
        else
          {
          painter->GetBrush()->SetColor(brush->GetColorObject());
          }
        if (orientation == vtkPlotBar::VERTICAL)
          {
          if (p)
            painter->DrawRect(f[2*i]-(width/2)-offset, p[2*i+1],
                              width, f[2*i+1] - p[2*i+1]);
          else
            painter->DrawRect(f[2*i]-(width/2)-offset, 0.0,
                              width, f[2*i+1]);
          }
        else // HORIZONTAL orientation
          {
          if (p)
            painter->DrawRect(p[2*i+1], f[2*i]-(width/2)-offset,
                              f[2*i+1] - p[2*i+1], width);
          else
            painter->DrawRect(0.0, f[2*i]-(width/2)-offset,
                              f[2*i+1], width);
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
      low = std::lower_bound(v.begin(), v.end(), lowPoint, compVector3fX);

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
        std::sort(this->Sorted->begin(), this->Sorted->end(), compVector3fX);
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

      if (!this->SelectionSet)
        {
        // Use a Set for faster lookup during paint
        this->SelectionSet = new std::set<vtkIdType>();
        }
      this->SelectionSet->clear();

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
      low = std::lower_bound(v.begin(), v.end(), lowPoint, compVector3fX);

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
            this->SelectionSet->insert(low->index);
            }
          }
        // Is the left side of the bar beyond the box?
        if (low->pos.GetX()-offset-halfWidth > targetMax.GetX())
          {
          break;
          }
        ++low;
        }

      if (this->SelectionSet->empty())
        {
        return false;
        }
      else
        {
        return true;
        }
      }

    vtkSmartPointer<vtkPlotBarSegment> Previous;
    vtkSmartPointer<vtkPoints2D> Points;
    vtkPlotBar *Bar;
    VectorPIMPL* Sorted;
    std::set<vtkIdType>* SelectionSet;
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

  vtkPlotBarSegment *AddSegment(vtkDataArray *x_array, vtkDataArray *y_array,
                                vtkPlotBarSegment *prev=0)
    {
    vtkSmartPointer<vtkPlotBarSegment> segment =
        vtkSmartPointer<vtkPlotBarSegment>::New();
    segment->Configure(this->Bar,x_array,y_array,prev);
    this->Segments.push_back(segment);
    return segment;
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
                    float width, float offset, int orientation,
                    vtkIdTypeArray* selection)
  {
    // Selection functionality not supported for stacked plots (yet)
    if (this->Segments.size() != 1)
      {
      return false;
      }

    // This has the side effect of generating SelectionSet
    if (this->Segments[0]->SelectPoints(min, max, width, offset, orientation))
      {
      for(std::set<vtkIdType>::const_iterator itr =
            this->Segments[0]->SelectionSet->begin();
          itr != this->Segments[0]->SelectionSet->end();
          itr++)
        {
        selection->InsertNextValue(*itr);
        }
      return true;
      }
    else
      {
      return false;
      }
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
void vtkPlotBar::GetBounds(double bounds[4])
{
  int seriesLow, seriesHigh, valuesLow, valuesHigh;
  if (this->Orientation ==  vtkPlotBar::VERTICAL)
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

  double y_range[2];
  std::map< int, std::string >::iterator it;
  for ( it = this->Private->AdditionalSeries.begin(); it !=
                  this->Private->AdditionalSeries.end(); ++it )
    {
    y = vtkDataArray::SafeDownCast(table->GetColumnByName((*it).second.c_str()));
    y->GetRange(y_range);
    bounds[valuesHigh] += y_range[1];
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
  vtkDebugMacro(<< "Bounds: " << bounds[0] << "\t" << bounds[1] << "\t"
                << bounds[2] << "\t" << bounds[3]);
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
  this->Brush->GetColorF(rgb);
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

  vtkPlotBarSegment *prev = this->Private->AddSegment(x,y);

  std::map< int, std::string >::iterator it;

  for ( it = this->Private->AdditionalSeries.begin();
        it != this->Private->AdditionalSeries.end(); ++it )
    {
    y = vtkDataArray::SafeDownCast(table->GetColumnByName((*it).second.c_str()));
    prev = this->Private->AddSegment(x,y,prev);
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
bool vtkPlotBar::SelectPoints(const vtkVector2f& min, const vtkVector2f& max)
{
  if (!this->Selection)
    {
    this->Selection = vtkIdTypeArray::New();
    }
  this->Selection->SetNumberOfTuples(0);

  return this->Private->SelectPoints(min, max, this->Width, this->Offset,
                                                this->Orientation,
                                                this->Selection);
}

//-----------------------------------------------------------------------------
vtkStdString vtkPlotBar::GetTooltipLabel(const vtkVector2f &plotPos,
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
