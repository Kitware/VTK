/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChartParallelCoordinates.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkChartParallelCoordinates.h"

#include "vtkContext2D.h"
#include "vtkBrush.h"
#include "vtkPen.h"
#include "vtkContextScene.h"
#include "vtkTextProperty.h"
#include "vtkAxis.h"
#include "vtkPlotParallelCoordinates.h"
#include "vtkContextMapper2D.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkDataArray.h"
#include "vtkIdTypeArray.h"
#include "vtkTransform2D.h"
#include "vtkObjectFactory.h"

#include "vtkstd/vector"
#include "vtkstd/algorithm"

// Minimal storage class for STL containers etc.
class vtkChartParallelCoordinates::Private
{
public:
  Private()
    {
    this->Plot = vtkSmartPointer<vtkPlotParallelCoordinates>::New();
    this->Transform = vtkSmartPointer<vtkTransform2D>::New();
    this->CurrentAxis = -1;
    }
  ~Private()
    {
    for (vtkstd::vector<vtkAxis *>::iterator it = this->Axes.begin();
         it != this->Axes.end(); ++it)
      {
      (*it)->Delete();
      }
    }
  vtkSmartPointer<vtkPlotParallelCoordinates> Plot;
  vtkSmartPointer<vtkTransform2D> Transform;
  vtkstd::vector<vtkAxis *> Axes;
  vtkstd::vector<vtkRectf> AxesSelections;
  int CurrentAxis;
};

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkChartParallelCoordinates, "1.4");

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkChartParallelCoordinates);

//-----------------------------------------------------------------------------
vtkChartParallelCoordinates::vtkChartParallelCoordinates()
{
  this->Storage = new vtkChartParallelCoordinates::Private;
  this->Storage->Plot->SetParent(this);
  this->GeometryValid = false;
  this->Selection = vtkIdTypeArray::New();
  this->Storage->Plot->SetSelection(this->Selection);
}

//-----------------------------------------------------------------------------
vtkChartParallelCoordinates::~vtkChartParallelCoordinates()
{
  this->Storage->Plot->SetSelection(NULL);
  delete this->Storage;
  this->Selection->Delete();
}

//-----------------------------------------------------------------------------
void vtkChartParallelCoordinates::Update()
{
  vtkTable* table = this->Storage->Plot->GetData()->GetInput();
  if (!table)
    {
    return;
    }

  if (table->GetMTime() < this->BuildTime)
  {
    return;
  }

  // Now we have a table, set up the axes accordingly, clear and build.
  if (static_cast<int>(this->Storage->Axes.size()) != table->GetNumberOfColumns())
    {
    for (vtkstd::vector<vtkAxis *>::iterator it = this->Storage->Axes.begin();
         it != this->Storage->Axes.end(); ++it)
      {
      (*it)->Delete();
      }
    this->Storage->Axes.clear();

    for (int i = 0; i < table->GetNumberOfColumns(); ++i)
      {
      vtkAxis* axis = vtkAxis::New();
      axis->SetPosition(vtkAxis::PARALLEL);
      this->Storage->Axes.push_back(axis);
      }
    }

  // Now set up their ranges and locations
  for (int i = 0; i < table->GetNumberOfColumns(); ++i)
    {
    double range[2];
    vtkDataArray* array = vtkDataArray::SafeDownCast(table->GetColumn(i));
    if (array)
      {
      array->GetRange(range);
      }
    vtkAxis* axis = this->Storage->Axes[i];
    axis->SetMinimum(range[0]);
    axis->SetMaximum(range[1]);
    axis->SetTitle(table->GetColumnName(i));
    }
  this->Storage->AxesSelections.clear();

  this->Storage->AxesSelections.resize(this->Storage->Axes.size());
  this->GeometryValid = false;
  this->BuildTime.Modified();
}

//-----------------------------------------------------------------------------
bool vtkChartParallelCoordinates::Paint(vtkContext2D *painter)
{
  if (this->GetScene()->GetViewWidth() == 0 ||
      this->GetScene()->GetViewHeight() == 0 ||
      !this->Visible || !this->Storage->Plot->GetVisible())
    {
    // The geometry of the chart must be valid before anything can be drawn
    return false;
    }

  this->Update();
  this->UpdateGeometry();

  painter->PushMatrix();
  painter->SetTransform(this->Storage->Transform);
  this->Storage->Plot->Paint(painter);
  painter->PopMatrix();

  // Now we have a table, set up the axes accordingly, clear and build.
  for (vtkstd::vector<vtkAxis *>::iterator it = this->Storage->Axes.begin();
       it != this->Storage->Axes.end(); ++it)
    {
    (*it)->Paint(painter);
    }

  // If there is a selected axis, draw the highlight
  if (this->Storage->CurrentAxis >= 0)
    {
    painter->GetBrush()->SetColor(200, 200, 200, 200);
    vtkAxis* axis = this->Storage->Axes[this->Storage->CurrentAxis];
    painter->DrawRect(axis->GetPoint1()[0]-10, this->Point1[1],
                      20, this->Point2[1]-this->Point1[1]);
    }

  // Now draw our active selections
  for (size_t i = 0; i < this->Storage->AxesSelections.size(); ++i)
    {
    vtkRectf &rect = this->Storage->AxesSelections[i];
    if (rect.Height() != 0.0f)
      {
      painter->GetBrush()->SetColor(200, 20, 20, 220);
      painter->DrawRect(rect.X(), rect.Y(), rect.Width(), rect.Height());
      }
    }

  return true;
}

//-----------------------------------------------------------------------------
vtkPlot * vtkChartParallelCoordinates::AddPlot(int)
{
  return NULL;
}

//-----------------------------------------------------------------------------
bool vtkChartParallelCoordinates::RemovePlot(vtkIdType)
{
  return false;
}

//-----------------------------------------------------------------------------
void vtkChartParallelCoordinates::ClearPlots()
{
}

//-----------------------------------------------------------------------------
vtkPlot* vtkChartParallelCoordinates::GetPlot(vtkIdType)
{
  return this->Storage->Plot;
}

//-----------------------------------------------------------------------------
vtkIdType vtkChartParallelCoordinates::GetNumberOfPlots()
{
  return 1;
}

//-----------------------------------------------------------------------------
vtkAxis* vtkChartParallelCoordinates::GetAxis(int index)
{
  if (index < this->GetNumberOfAxes())
    {
    return this->Storage->Axes[index];
    }
  else
    {
    return NULL;
    }
}

//-----------------------------------------------------------------------------
vtkIdType vtkChartParallelCoordinates::GetNumberOfAxes()
{
  return this->Storage->Axes.size();
}

//-----------------------------------------------------------------------------
void vtkChartParallelCoordinates::UpdateGeometry()
{
  int geometry[] = { this->GetScene()->GetViewWidth(),
                     this->GetScene()->GetViewHeight() };

  if (geometry[0] != this->Geometry[0] || geometry[1] != this->Geometry[1] ||
      !this->GeometryValid)
    {
    // Take up the entire window right now, this could be made configurable
    this->SetGeometry(geometry);
    this->SetBorders(60, 20, 20, 50);

    // Iterate through the axes and set them up to span the chart area.
    int xStep = (this->Point2[0] - this->Point1[0]) /
                (static_cast<int>(this->Storage->Axes.size())-1);
    int x =  this->Point1[0];

    for (size_t i = 0; i < this->Storage->Axes.size(); ++i)
      {
      vtkAxis* axis = this->Storage->Axes[i];
      axis->SetPoint1(x, this->Point1[1]);
      axis->SetPoint2(x, this->Point2[1]);
      axis->AutoScale();
      axis->Update();
      x += xStep;
      }

    this->GeometryValid = true;
    // Cause the plot transform to be recalculated if necessary
    this->CalculatePlotTransform();
    this->Storage->Plot->Update();
    }
}

//-----------------------------------------------------------------------------
void vtkChartParallelCoordinates::CalculatePlotTransform()
{
  // In the case of parallel coordinates everything is plotted in a normalized
  // system, where the range is from 0.0 to 1.0 in the y axis, and in screen
  // coordinates along the x axis.
  if (!this->Storage->Axes.size())
    {
    return;
    }

  vtkAxis* axis = this->Storage->Axes[0];
  float *min = axis->GetPoint1();
  float *max = axis->GetPoint2();
  float yScale = 1.0f / (max[1] - min[1]);

  this->Storage->Transform->Identity();
  this->Storage->Transform->Translate(0, axis->GetPoint1()[1]);
  // Get the scale for the plot area from the x and y axes
  this->Storage->Transform->Scale(1.0, 1.0 / yScale);
}

//-----------------------------------------------------------------------------
void vtkChartParallelCoordinates::RecalculateBounds()
{
  return;
}

//-----------------------------------------------------------------------------
bool vtkChartParallelCoordinates::Hit(const vtkContextMouseEvent &mouse)
{
  if (mouse.ScreenPos[0] > this->Point1[0]-10 &&
      mouse.ScreenPos[0] < this->Point2[0]+10 &&
      mouse.ScreenPos[1] > this->Point1[1] &&
      mouse.ScreenPos[1] < this->Point2[1])
    {
    return true;
    }
  else
    {
    return false;
    }
}

//-----------------------------------------------------------------------------
bool vtkChartParallelCoordinates::MouseEnterEvent(const vtkContextMouseEvent &)
{
  return true;
}

//-----------------------------------------------------------------------------
bool vtkChartParallelCoordinates::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  if (mouse.Button == 0)
    {
    // If an axis is selected, then lets try to narrow down a selection...
    if (this->Storage->CurrentAxis >= 0)
      {
      vtkAxis* axis = this->Storage->Axes[this->Storage->CurrentAxis];
      vtkRectf &rect = this->Storage->AxesSelections[this->Storage->CurrentAxis];
      if (mouse.ScenePos[1] > axis->GetPoint2()[1])
        {
        rect.SetHeight(axis->GetPoint2()[1] - rect.Y());
        }
      else if (mouse.ScenePos[1] < axis->GetPoint1()[1])
        {
        rect.SetHeight(axis->GetPoint1()[1] - rect.Y());
        }
      else
        {
        rect.SetHeight(mouse.ScenePos[1] - rect.Y());
        }
      }
    this->Scene->SetDirty(true);

    }
  else if (mouse.Button == 2)
    {
    }
  else if (mouse.Button < 0)
    {

    }

  return true;
}

//-----------------------------------------------------------------------------
bool vtkChartParallelCoordinates::MouseLeaveEvent(const vtkContextMouseEvent &)
{
  return true;
}

//-----------------------------------------------------------------------------
bool vtkChartParallelCoordinates::MouseButtonPressEvent(const vtkContextMouseEvent
                                                        &mouse)
{
  if (mouse.Button == 0)
    {
    // Select an axis if we are within range
    if (mouse.ScenePos[1] > this->Point1[1] &&
        mouse.ScenePos[1] < this->Point2[1])
      {
      // Iterate over the axes, see if we are within 10 pixels of an axis
      for (size_t i = 0; i < this->Storage->Axes.size(); ++i)
        {
        vtkAxis* axis = this->Storage->Axes[i];
        if (axis->GetPoint1()[0]-10 < mouse.ScenePos[0] &&
            axis->GetPoint1()[0]+10 > mouse.ScenePos[0])
          {
          this->Storage->CurrentAxis = static_cast<int>(i);
          this->Scene->SetDirty(true);
          this->Storage->AxesSelections[i].Set(axis->GetPoint1()[0]-5,
                                               mouse.ScenePos[1],
                                               10, 0);
          return true;
          }
        }
      }
      this->Storage->CurrentAxis = -1;
      this->Scene->SetDirty(true);
      return false;
    return true;
    }
  else if (mouse.Button == 2)
    {
    // Right mouse button - zoom box

    return true;
    }
  else
    {
    return false;
    }
}

//-----------------------------------------------------------------------------
bool vtkChartParallelCoordinates::MouseButtonReleaseEvent(const vtkContextMouseEvent
                                                          &mouse)
{
  if (mouse.Button == 0)
    {
    if (this->Storage->CurrentAxis >= 0)
      {
      vtkAxis* axis = this->Storage->Axes[this->Storage->CurrentAxis];
      vtkRectf &rect = this->Storage->AxesSelections[this->Storage->CurrentAxis];

      // Set the final mouse position
      if (mouse.ScenePos[1] > axis->GetPoint2()[1])
        {
        rect.SetHeight(axis->GetPoint2()[1] - rect.Y());
        }
      else if (mouse.ScenePos[1] < axis->GetPoint1()[1])
        {
        rect.SetHeight(axis->GetPoint1()[1] - rect.Y());
        }
      else
        {
        rect.SetHeight(mouse.ScenePos[1] - rect.Y());
        }

      if (rect.Height() == 0.0f)
        {
        // Reset the axes.
        this->Storage->Plot->ResetSelectionRange();

        // Now set the remaining selections that were kept
        float low = 0.0;
        float high = 0.0;
        for (size_t i = 0; i < this->Storage->AxesSelections.size(); ++i)
          {
          vtkRectf &rect2 = this->Storage->AxesSelections[i];
          if (rect2.Height() != 0.0f)
            {
            if (rect2.Height() > 0.0f)
              {
              low = rect2.Y();
              high = rect2.Y() + rect2.Height();
              }
            else
              {
              low = rect2.Y() + rect2.Height();
              high = rect2.Y();
              }
            low -= this->Storage->Transform->GetMatrix()->GetElement(1, 2);
            low /= this->Storage->Transform->GetMatrix()->GetElement(1, 1);
            high -= this->Storage->Transform->GetMatrix()->GetElement(1, 2);
            high /= this->Storage->Transform->GetMatrix()->GetElement(1, 1);

            // Process the selected range and display this
            this->Storage->Plot->SetSelectionRange(static_cast<int>(i),
                                                   low, high);
            }
          }
        }
      else
        {
        float low = 0.0;
        float high = 0.0;
        if (rect.Height() > 0.0f)
          {
          low = rect.Y();
          high = rect.Y() + rect.Height();
          }
        else
          {
          low = rect.Y() + rect.Height();
          high = rect.Y();
          }
        low -= this->Storage->Transform->GetMatrix()->GetElement(1, 2);
        low /= this->Storage->Transform->GetMatrix()->GetElement(1, 1);
        high -= this->Storage->Transform->GetMatrix()->GetElement(1, 2);
        high /= this->Storage->Transform->GetMatrix()->GetElement(1, 1);
        // Process the selected range and display this
        this->Storage->Plot->SetSelectionRange(this->Storage->CurrentAxis,
                                               low, high);
        }

      this->Scene->SetDirty(true);
      }
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkChartParallelCoordinates::MouseWheelEvent(const vtkContextMouseEvent &,
                                                  int)
{
  return true;
}

//-----------------------------------------------------------------------------
void vtkChartParallelCoordinates::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
