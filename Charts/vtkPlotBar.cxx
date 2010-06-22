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
#include "vtkSmartPointer.h"
#include "vtkColorSeries.h"
#include "vtkStringArray.h"

#include "vtkObjectFactory.h"

#include "vtkstd/vector"
#include "vtkstd/algorithm"
#include "vtkstd/map"

//-----------------------------------------------------------------------------
namespace {

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

// Copy the two arrays into the points array
template<class A>
void CopyToPointsSwitch(vtkPoints2D *points, vtkPoints2D *previous_points, A *a, vtkDataArray *b, int n)
{
  switch(b->GetDataType())
    {
    vtkTemplateMacro(
        CopyToPoints(points,previous_points, a, static_cast<VTK_TT*>(b->GetVoidPointer(0)), n));
    }
}

// Copy the two arrays into the points array
template<class A, class B>
void CopyToPoints(vtkPoints2D *points, vtkPoints2D *previous_points, A *a, B *b, int n)
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

} // namespace

//-----------------------------------------------------------------------------

class vtkPlotBarSegment : public vtkObject {
  public:
    vtkTypeMacro(vtkPlotBarSegment,vtkObject);
    static vtkPlotBarSegment *New();

    vtkPlotBarSegment()
      {
      this->Bar = 0;
      this->Points = 0;
      this->Sorted = false;
      this->Previous = 0;
      }

    void Configure(vtkPlotBar *bar,vtkDataArray *x_array, vtkDataArray *y_array,vtkPlotBarSegment *prev) 
      {
      this->Bar = bar;
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

    void Paint(vtkContext2D *painter, vtkPen *pen, vtkBrush *brush, float width, float offset)
      {
      painter->ApplyPen(pen);
      painter->ApplyBrush(brush);
      int n = this->Points->GetNumberOfPoints();
      float *f = vtkFloatArray::SafeDownCast(this->Points->GetData())->GetPointer(0);
      float *p = 0;
      if (this->Previous)
        p = vtkFloatArray::SafeDownCast(this->Previous->Points->GetData())->GetPointer(0);

      for (int i = 0; i < n; ++i)
        {
        if (p)
          painter->DrawRect(f[2*i]-(width/2)-offset, p[2*i+1], width, f[2*i+1] - p[2*i+1]);
        else
          painter->DrawRect(f[2*i]-(width/2)-offset, 0.0, width, f[2*i+1]);
        }
      }

    bool GetNearestPoint(const vtkVector2f& point, 
                                     vtkVector2f* location,
                                     float width,
                                     float offset)
      {
      if (!this->Points)
        {
        return false;
        }
      vtkIdType n = this->Points->GetNumberOfPoints();
      if (n < 2)
        {
        return false;
        }

      // Right now doing a simple bisector search of the array. This should be
      // revisited. Assumes the x axis is sorted, which should always be true for
      // bar plots.
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
      float halfWidth = width / 2.0;

      // Set up our search array, use the STL lower_bound algorithm
      // When searching, invert the behavior of the offset and
      // compensate for the half width overlap.
      vtkstd::vector<vtkVector2f>::iterator low;
      vtkVector2f lowPoint(point.X()-(offset * -1)-halfWidth, 0.0f);
      low = vtkstd::lower_bound(v.begin(), v.end(), lowPoint, compVector2fX);

      while (low != v.end())
        {
        // Is the left side of the bar beyond the point?
        if (low->X()-offset-halfWidth > point.X())
          {
          break;
          }
        // Does the bar surround the point?
        else if (low->X()-halfWidth-offset < point.X() &&
                 low->X()+halfWidth-offset > point.X())
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

    vtkSmartPointer<vtkPlotBarSegment>Previous;
    vtkSmartPointer<vtkPoints2D> Points;
    vtkPlotBar *Bar;
    bool Sorted;
};

vtkStandardNewMacro(vtkPlotBarSegment);

class vtkPlotBarPrivate {
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
                       vtkPen *pen, vtkBrush *brush, float width, float offset)
      {
      int colorInSeries = 0;
      bool useColorSeries = this->Segments.size() > 1;
      for (vtkstd::vector<vtkSmartPointer<vtkPlotBarSegment> >::iterator it = 
              this->Segments.begin();
           it != this->Segments.end(); ++it)
        {
        if (useColorSeries)
          brush->SetColor(colorSeries->GetColorRepeating(colorInSeries++).GetData());
        (*it)->Paint(painter,pen,brush,width,offset);
        }
      }


    int GetNearestPoint(const vtkVector2f& point, 
                                     vtkVector2f* location,
                                     float width,
                                     float offset)
      {
      int index = 0;
      for (vtkstd::vector<vtkSmartPointer<vtkPlotBarSegment> >::iterator it = 
              this->Segments.begin();
           it != this->Segments.end(); ++it)
        {
        if ((*it)->GetNearestPoint(point,location,width,offset))
          {
          return index;
          }
        ++index;
        }
      return -1;
      }

    vtkstd::vector<vtkSmartPointer<vtkPlotBarSegment> > Segments;
    vtkPlotBar *Bar;
    vtkstd::map<int,vtkstd::string> AdditionalSeries;
};


//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotBar);

//-----------------------------------------------------------------------------
vtkPlotBar::vtkPlotBar()
{
  this->Private = new vtkPlotBarPrivate(this);
  this->Points = 0;
  this->Sorted = false;
  this->Labels = 0;
  this->AutoLabels = 0;
  this->Width = 1.0;
  this->Pen->SetWidth(1.0);
  this->Offset = 1.0;
  this->ColorSeries = 0;
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

  this->Private->PaintSegments(painter,this->ColorSeries,this->Pen,this->Brush,
                                this->Width,this->Offset);

  return true;
}

//-----------------------------------------------------------------------------
bool vtkPlotBar::PaintLegend(vtkContext2D *painter, float rect[4],int legendIndex)
{
  if (this->ColorSeries)
    this->Brush->SetColor(this->ColorSeries->GetColorRepeating(legendIndex).GetData());

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
    }
  else if (x && y)
    {
    x->GetRange(&bounds[0]);
    // We surround our point by Width/2 on either side
    bounds[0] -= this->Width / 2;
    bounds[1] += this->Width / 2;
    }

  y->GetRange(&bounds[2]);

  double y_range[2];
  vtkstd::map< int, vtkstd::string >::iterator it;
  for ( it = this->Private->AdditionalSeries.begin(); it != 
                  this->Private->AdditionalSeries.end(); ++it )
    {
    y = vtkDataArray::SafeDownCast(table->GetColumnByName((*it).second.c_str()));
    y->GetRange(y_range);
    bounds[3] += y_range[1];
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

//-----------------------------------------------------------------------------
int vtkPlotBar::GetNearestPoint(const vtkVector2f& point,
                                  const vtkVector2f&,
                                  vtkVector2f* location)
{
  return this->Private->GetNearestPoint(point,location,this->Width,this->Offset);
}

//-----------------------------------------------------------------------------
vtkStringArray *vtkPlotBar::GetLabels()
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

    vtkstd::map< int, vtkstd::string >::iterator it;
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

  vtkstd::map< int, vtkstd::string >::iterator it;

  for ( it = this->Private->AdditionalSeries.begin(); 
        it != this->Private->AdditionalSeries.end(); ++it )
    {
    y = vtkDataArray::SafeDownCast(table->GetColumnByName((*it).second.c_str()));
    prev = this->Private->AddSegment(x,y,prev);
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

//-----------------------------------------------------------------------------

void vtkPlotBar::SetInputArray(int index, const char *name)
{
  if (index == 0 || index == 1)
    {
    vtkPlot::SetInputArray(index,name);
    }
  else
    {
    this->Private->AdditionalSeries[index] = vtkstd::string(name);
    }
  this->AutoLabels = 0; // No longer valid
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
