/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChartXY.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkChartXY.h"

#include "vtkContext2D.h"
#include "vtkPen.h"
#include "vtkBrush.h"
#include "vtkColorSeries.h"

#include "vtkTransform2D.h"
#include "vtkContextScene.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextTransform.h"
#include "vtkContextClip.h"
#include "vtkPoints2D.h"
#include "vtkVector.h"

#include "vtkPlotBar.h"
#include "vtkPlotStacked.h"
#include "vtkPlotLine.h"
#include "vtkPlotPoints.h"
#include "vtkContextMapper2D.h"

#include "vtkAxis.h"
#include "vtkPlotGrid.h"
#include "vtkChartLegend.h"
#include "vtkTooltipItem.h"

#include "vtkTable.h"
#include "vtkIdTypeArray.h"

#include "vtkAnnotationLink.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"

#include "vtkObjectFactory.h"
#include "vtkCommand.h"

#include "vtkStdString.h"
#include "vtkTextProperty.h"

#include "vtkDataArray.h"
#include "vtkStringArray.h"

// My STL containers
#include <vector>

//-----------------------------------------------------------------------------
class vtkChartXYPrivate
{
public:
  vtkChartXYPrivate()
    {
    this->Colors = vtkSmartPointer<vtkColorSeries>::New();
    this->Clip = vtkSmartPointer<vtkContextClip>::New();
    this->Borders[0] = 60;
    this->Borders[1] = 50;
    this->Borders[2] = 20;
    this->Borders[3] = 20;
    }

  std::vector<vtkPlot *> plots; // Charts can contain multiple plots of data
  std::vector<vtkContextTransform *> PlotCorners; // Stored by corner...
  std::vector<vtkAxis *> axes; // Charts can contain multiple axes
  vtkSmartPointer<vtkColorSeries> Colors; // Colors in the chart
  vtkSmartPointer<vtkContextClip> Clip; // Colors in the chart
  int Borders[4];
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkChartXY);

//-----------------------------------------------------------------------------
vtkChartXY::vtkChartXY()
{
  this->ChartPrivate = new vtkChartXYPrivate;

  this->AutoAxes = true;
  this->HiddenAxisBorder = 20;

  // The grid is drawn first.
  vtkPlotGrid *grid1 = vtkPlotGrid::New();
  this->AddItem(grid1);
  grid1->Delete();

  // The second grid for the far side/top axis
  vtkPlotGrid *grid2 = vtkPlotGrid::New();
  this->AddItem(grid2);
  grid2->Delete();

  // The plots are drawn on top of the grid, in a clipped, transformed area.
  this->AddItem(this->ChartPrivate->Clip);
  // Set up the bottom-left transform, the rest are often not required (set up
  // on demand if used later). Add it as a child item, rendered automatically.
  vtkSmartPointer<vtkContextTransform> corner =
      vtkSmartPointer<vtkContextTransform>::New();
  this->ChartPrivate->PlotCorners.push_back(corner);
  this->ChartPrivate->Clip->AddItem(corner); // Child list maintains ownership.

  // Next is the axes
  for (int i = 0; i < 4; ++i)
    {
    this->ChartPrivate->axes.push_back(vtkAxis::New());
    // By default just show the left and bottom axes
    this->ChartPrivate->axes.back()->SetVisible(i < 2 ? true : false);
    this->AddItem(this->ChartPrivate->axes.back());
    }
  this->ChartPrivate->axes[vtkAxis::LEFT]->SetPosition(vtkAxis::LEFT);
  this->ChartPrivate->axes[vtkAxis::BOTTOM]->SetPosition(vtkAxis::BOTTOM);
  this->ChartPrivate->axes[vtkAxis::RIGHT]->SetPosition(vtkAxis::RIGHT);
  this->ChartPrivate->axes[vtkAxis::TOP]->SetPosition(vtkAxis::TOP);

  // Set up the x and y axes - should be configured based on data
  this->ChartPrivate->axes[vtkAxis::LEFT]->SetTitle("Y Axis");
  this->ChartPrivate->axes[vtkAxis::BOTTOM]->SetTitle("X Axis");

  grid1->SetXAxis(this->ChartPrivate->axes[vtkAxis::BOTTOM]);
  grid1->SetYAxis(this->ChartPrivate->axes[vtkAxis::LEFT]);
  grid2->SetXAxis(this->ChartPrivate->axes[vtkAxis::TOP]);
  grid2->SetYAxis(this->ChartPrivate->axes[vtkAxis::RIGHT]);

  // Then the legend is drawn
  this->Legend = vtkSmartPointer<vtkChartLegend>::New();
  this->Legend->SetChart(this);
  this->Legend->SetVisible(false);
  this->AddItem(this->Legend);

  this->PlotTransformValid = false;

  this->DrawBox = false;
  this->DrawNearestPoint = false;
  this->DrawAxesAtOrigin = false;
  this->BarWidthFraction = 0.8f;

  this->Tooltip = vtkSmartPointer<vtkTooltipItem>::New();
  this->Tooltip->SetVisible(false);
  this->AddItem(this->Tooltip);
  this->LayoutChanged = true;

  this->ForceAxesToBounds = false;
}

//-----------------------------------------------------------------------------
vtkChartXY::~vtkChartXY()
{
  for (unsigned int i = 0; i < this->ChartPrivate->plots.size(); ++i)
    {
    this->ChartPrivate->plots[i]->Delete();
    }
  for (size_t i = 0; i < 4; ++i)
    {
    this->ChartPrivate->axes[i]->Delete();
    }
  delete this->ChartPrivate;
  this->ChartPrivate = 0;
}

//-----------------------------------------------------------------------------
void vtkChartXY::Update()
{
  // Perform any necessary updates that are not graphical
  // Update the plots if necessary
  for (size_t i = 0; i < this->ChartPrivate->plots.size(); ++i)
    {
    this->ChartPrivate->plots[i]->Update();
    }
  this->Legend->Update();

  // Update the selections if necessary.
  if (this->AnnotationLink)
    {
    this->AnnotationLink->Update();
    vtkSelection *selection =
        vtkSelection::SafeDownCast(this->AnnotationLink->GetOutputDataObject(2));
    if (selection->GetNumberOfNodes())
      {
      vtkSelectionNode *node = selection->GetNode(0);
      vtkIdTypeArray *idArray =
          vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
      // Now iterate through the plots to update selection data
      std::vector<vtkPlot*>::iterator it =
          this->ChartPrivate->plots.begin();
      for ( ; it != this->ChartPrivate->plots.end(); ++it)
        {
        (*it)->SetSelection(idArray);
        }
      }
    }
  else
    {
    vtkDebugMacro("No annotation link set.");
    }

  this->CalculateBarPlots();

  if (this->AutoAxes)
    {
    for (int i = 0; i < 4; ++i)
      {
      this->ChartPrivate->axes[i]->SetVisible(false);
      }
    for (size_t i = 0; i < this->ChartPrivate->PlotCorners.size(); ++i)
      {
      int visible = 0;
      for (unsigned int j = 0;
           j < this->ChartPrivate->PlotCorners[i]->GetNumberOfItems(); ++j)
        {
        if (vtkPlot::SafeDownCast(this->ChartPrivate->PlotCorners[i]
                                  ->GetItem(j))->GetVisible())
          {
          ++visible;
          }
        }
      if (visible)
        {
        if (i < 3)
          {
          this->ChartPrivate->axes[i]->SetVisible(true);
          this->ChartPrivate->axes[i+1]->SetVisible(true);
          }
        else
          {
          this->ChartPrivate->axes[0]->SetVisible(true);
          this->ChartPrivate->axes[3]->SetVisible(true);
          }
        }
      }
    }
}

//-----------------------------------------------------------------------------
bool vtkChartXY::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called.");
  if (!this->Visible)
    {
    // The geometry of the chart must be valid before anything can be drawn
    return false;
    }

  vtkVector2i geometry;
  bool recalculateTransform = false;
  if (this->AutoSize)
    {
    geometry = vtkVector2i(this->GetScene()->GetSceneWidth(),
                           this->GetScene()->GetSceneHeight());
    if (geometry.X() != this->Geometry[0] || geometry.Y() != this->Geometry[1])
      {
      recalculateTransform = true;
      this->LayoutChanged = true;
      }
    this->SetSize(vtkRectf(0.0, 0.0, geometry.X(), geometry.Y()));
    }

  int visiblePlots = 0;
  for (size_t i = 0; i < this->ChartPrivate->plots.size(); ++i)
    {
    if (this->ChartPrivate->plots[i]->GetVisible())
      {
      ++visiblePlots;
      }
    }
  if (visiblePlots == 0 && !this->RenderEmpty)
    {
    // Nothing to plot, so don't draw anything.
    return false;
    }

  this->Update();

  if (this->MTime < this->ChartPrivate->axes[0]->GetMTime())
    {
    // Cause the plot transform to be recalculated if necessary
    recalculateTransform = true;
    this->LayoutChanged = true;
    }

  this->UpdateLayout(painter);
  // Recalculate the plot transform, min and max values if necessary
  if (!this->PlotTransformValid)
    {
    this->RecalculatePlotBounds();
    recalculateTransform = true;
    }
  if (this->UpdateLayout(painter) || recalculateTransform)
    {
    this->RecalculatePlotTransforms();
    }

  // Update the clipping if necessary
  this->ChartPrivate->Clip->SetClip(this->Point1[0], this->Point1[1],
                                    this->Point2[0]-this->Point1[0],
                                    this->Point2[1]-this->Point1[1]);

  // Use the scene to render most of the chart.
  this->PaintChildren(painter);

  // Draw the selection box if necessary
  if (this->DrawBox)
    {
    painter->GetBrush()->SetColor(255, 255, 255, 0);
    painter->GetPen()->SetColor(0, 0, 0, 255);
    painter->GetPen()->SetWidth(1.0);
    painter->DrawRect(this->MouseBox.X(), this->MouseBox.Y(),
                      this->MouseBox.Width(), this->MouseBox.Height());
    }

  if (this->Title)
    {
    vtkPoints2D *rect = vtkPoints2D::New();
    rect->InsertNextPoint(this->Point1[0], this->Point2[1]);
    rect->InsertNextPoint(this->Point2[0]-this->Point1[0], 10);
    painter->ApplyTextProp(this->TitleProperties);
    painter->DrawStringRect(rect, this->Title);
    rect->Delete();
    }

  return true;
}

//-----------------------------------------------------------------------------
void vtkChartXY::CalculateBarPlots()
{
  // Calculate the width, spacing and offsets for the bar plot - they are grouped
  size_t n = this->ChartPrivate->plots.size();
  std::vector<vtkPlotBar *> bars;
  for (size_t i = 0; i < n; ++i)
    {
    vtkPlotBar* bar = vtkPlotBar::SafeDownCast(this->ChartPrivate->plots[i]);
    if (bar && bar->GetVisible())
      {
      bars.push_back(bar);
      }
    }
  if (bars.size())
    {
    // We have some bar plots - work out offsets etc.
    float barWidth = 0.1;
    vtkPlotBar* bar = bars[0];
    if (!bar->GetUseIndexForXSeries())
      {
      vtkTable *table = bar->GetData()->GetInput();
      if (table)
        {
        vtkDataArray* x = bar->GetData()->GetInputArrayToProcess(0, table);
        if (x && x->GetNumberOfTuples() > 1)
          {
          double x0 = x->GetTuple1(0);
          double x1 = x->GetTuple1(1);
          float width = static_cast<float>(fabs(x1 - x0) * this->BarWidthFraction);
          barWidth = width / bars.size();
          }
        }
      }
    else
      {
      barWidth = 1.0f / bars.size() * this->BarWidthFraction;
      }

    // Now set the offsets and widths on each bar
    // The offsetIndex deals with the fact that half the bars
    // must shift to the left of the point and half to the right
    int offsetIndex = static_cast<int>(bars.size() - 1);
    for (size_t i = 0; i < bars.size(); ++i)
      {
      bars[i]->SetWidth(barWidth);
      bars[i]->SetOffset(offsetIndex * (barWidth / 2));
      // Increment by two since we need to shift by half widths
      // but make room for entire bars. Increment backwards because
      // offsets are always subtracted and Positive offsets move
      // the bar leftwards.  Negative offsets will shift the bar
      // to the right.
      offsetIndex -= 2;
      //bars[i]->SetOffset(float(bars.size()-i-1)*(barWidth/2));
      }
    }
}

//-----------------------------------------------------------------------------
void vtkChartXY::RecalculatePlotTransforms()
{
  for (int i = 0; i < int(this->ChartPrivate->PlotCorners.size()); ++i)
    {
    if (this->ChartPrivate->PlotCorners[i]->GetNumberOfItems())
      {
      vtkAxis *xAxis = 0;
      vtkAxis *yAxis = 0;
      // Get the appropriate axes, and recalculate the transform.
      switch (i)
        {
        case 0:
          {
          xAxis = this->ChartPrivate->axes[vtkAxis::BOTTOM];
          yAxis = this->ChartPrivate->axes[vtkAxis::LEFT];
          break;
          }
        case 1:
          xAxis = this->ChartPrivate->axes[vtkAxis::BOTTOM];
          yAxis = this->ChartPrivate->axes[vtkAxis::RIGHT];
          break;
        case 2:
          xAxis = this->ChartPrivate->axes[vtkAxis::TOP];
          yAxis = this->ChartPrivate->axes[vtkAxis::RIGHT];
          break;
        case 3:
          xAxis = this->ChartPrivate->axes[vtkAxis::TOP];
          yAxis = this->ChartPrivate->axes[vtkAxis::LEFT];
          break;
        default:
          vtkWarningMacro("Error: default case in recalculate plot transforms.");
        }
      this->CalculatePlotTransform(xAxis, yAxis,
                                   this->ChartPrivate
                                   ->PlotCorners[i]->GetTransform());
      }
    }
  this->PlotTransformValid = true;
}

//-----------------------------------------------------------------------------
int vtkChartXY::GetPlotCorner(vtkPlot *plot)
{
  vtkAxis *x = plot->GetXAxis();
  vtkAxis *y = plot->GetYAxis();
  if (x == this->ChartPrivate->axes[vtkAxis::BOTTOM] &&
      y == this->ChartPrivate->axes[vtkAxis::LEFT])
    {
    return 0;
    }
  else if (x == this->ChartPrivate->axes[vtkAxis::BOTTOM] &&
           y == this->ChartPrivate->axes[vtkAxis::RIGHT])
    {
    return 1;
    }
  else if (x == this->ChartPrivate->axes[vtkAxis::TOP] &&
           y == this->ChartPrivate->axes[vtkAxis::RIGHT])
    {
    return 2;
    }
  else if (x == this->ChartPrivate->axes[vtkAxis::TOP] &&
           y == this->ChartPrivate->axes[vtkAxis::LEFT])
    {
    return 3;
    }
  else
    {
    // Should never happen.
    return 4;
    }
}

//-----------------------------------------------------------------------------
void vtkChartXY::SetPlotCorner(vtkPlot *plot, int corner)
{
  if (corner < 0 || corner > 3)
    {
    vtkWarningMacro("Invalid corner specified, should be between 0 and 3: "
                    << corner);
    return;
    }
  this->RemovePlotFromCorners(plot);
  // Grow the plot corners if necessary
  if (int(this->ChartPrivate->PlotCorners.size()) <= corner)
    {
    while (int(this->ChartPrivate->PlotCorners.size()) <= corner)
      {
      vtkSmartPointer<vtkContextTransform> transform =
        vtkSmartPointer<vtkContextTransform>::New();
      this->ChartPrivate->PlotCorners.push_back(transform);
      this->ChartPrivate->Clip->AddItem(transform); // Clip maintains ownership.
      }
    }
  this->ChartPrivate->PlotCorners[corner]->AddItem(plot);
  if (corner == 0)
    {
    plot->SetXAxis(this->ChartPrivate->axes[vtkAxis::BOTTOM]);
    plot->SetYAxis(this->ChartPrivate->axes[vtkAxis::LEFT]);
    }
  else if (corner == 1)
    {
    plot->SetXAxis(this->ChartPrivate->axes[vtkAxis::BOTTOM]);
    plot->SetYAxis(this->ChartPrivate->axes[vtkAxis::RIGHT]);
    }
  else if (corner == 2)
    {
    plot->SetXAxis(this->ChartPrivate->axes[vtkAxis::TOP]);
    plot->SetYAxis(this->ChartPrivate->axes[vtkAxis::RIGHT]);
    }
  else if (corner == 3)
    {
    plot->SetXAxis(this->ChartPrivate->axes[vtkAxis::TOP]);
    plot->SetYAxis(this->ChartPrivate->axes[vtkAxis::LEFT]);
    }
  this->PlotTransformValid = false;
}

//-----------------------------------------------------------------------------
void vtkChartXY::RecalculatePlotBounds()
{
  // Get the bounds of each plot, and each axis  - ordering as laid out below
  double y1[] = { 0.0, 0.0 }; // left -> 0
  double x1[] = { 0.0, 0.0 }; // bottom -> 1
  double y2[] = { 0.0, 0.0 }; // right -> 2
  double x2[] = { 0.0, 0.0 }; // top -> 3
  // Store whether the ranges have been initialized - follows same order
  bool initialized[] = { false, false, false, false };

  std::vector<vtkPlot*>::iterator it;
  double bounds[4] = { 0.0, 0.0, 0.0, 0.0 };
  for (it = this->ChartPrivate->plots.begin();
       it != this->ChartPrivate->plots.end(); ++it)
    {
    if ((*it)->GetVisible() == false)
      {
      continue;
      }
    (*it)->GetBounds(bounds);
    if (bounds[1]-bounds[0] < 0.0)
      {
      // skip uninitialized bounds.
      continue;
      }
    int corner = this->GetPlotCorner(*it);

    // Initialize the appropriate ranges, or push out the ranges
    if ((corner == 0 || corner == 3)) // left
      {
      if (!initialized[0])
        {
        y1[0] = bounds[2];
        y1[1] = bounds[3];
        initialized[0] = true;
        }
      else
        {
        if (y1[0] > bounds[2]) // min
          {
          y1[0] = bounds[2];
          }
        if (y1[1] < bounds[3]) // max
          {
          y1[1] = bounds[3];
          }
        }
      }
    if ((corner == 0 || corner == 1)) // bottom
      {
      if (!initialized[1])
        {
        x1[0] = bounds[0];
        x1[1] = bounds[1];
        initialized[1] = true;
        }
      else
        {
        if (x1[0] > bounds[0]) // min
          {
          x1[0] = bounds[0];
          }
        if (x1[1] < bounds[1]) // max
          {
          x1[1] = bounds[1];
          }
        }
      }
    if ((corner == 1 || corner == 2)) // right
      {
      if (!initialized[2])
        {
        y2[0] = bounds[2];
        y2[1] = bounds[3];
        initialized[2] = true;
        }
      else
        {
        if (y2[0] > bounds[2]) // min
          {
          y2[0] = bounds[2];
          }
        if (y2[1] < bounds[3]) // max
          {
          y2[1] = bounds[3];
          }
        }
      }
    if ((corner == 2 || corner == 3)) // top
      {
      if (!initialized[3])
        {
        x2[0] = bounds[0];
        x2[1] = bounds[1];
        initialized[3] = true;
        }
      else
        {
        if (x2[0] > bounds[0]) // min
          {
          x2[0] = bounds[0];
          }
        if (x2[1] < bounds[1]) // max
          {
          x2[1] = bounds[1];
          }
        }
      }
    }

  // Now set the newly calculated bounds on the axes
  for (int i = 0; i < 4; ++i)
    {
    vtkAxis *axis = this->ChartPrivate->axes[i];
    double *range = 0;
    switch (i)
      {
      case 0:
        range = y1;
        break;
      case 1:
        range = x1;
        break;
      case 2:
        range = y2;
        break;
      case 3:
        range = x2;
        break;
      default:
        return;
      }

    if (this->ForceAxesToBounds)
      {
      axis->SetMinimumLimit(range[0]);
      axis->SetMaximumLimit(range[1]);
      }
    if (axis->GetBehavior() == vtkAxis::AUTO && initialized[i])
      {
      axis->SetRange(range[0], range[1]);
      axis->AutoScale();
      }
    }

  this->Modified();
}

//-----------------------------------------------------------------------------
bool vtkChartXY::UpdateLayout(vtkContext2D* painter)
{
  // The main use of this method is currently to query the visible axes for
  // their bounds, and to update the chart in response to that.
  bool changed = false;

  // Axes
  for (int i = 0; i < 4; ++i)
    {
    int border = 0;
    vtkAxis* axis = this->ChartPrivate->axes[i];
    axis->Update();
    if (axis->GetVisible())
      {
      vtkRectf bounds = axis->GetBoundingRect(painter);
      if (i == 1 || i == 3)
        {// Horizontal axes
        border = int(bounds.GetHeight());
        }
      else
        {// Vertical axes
        border = int(bounds.GetWidth());
        }
      }
    border += this->GetLegendBorder(painter, i);
    border = border < this->HiddenAxisBorder ? this->HiddenAxisBorder : border;
    if (this->ChartPrivate->Borders[i] != border)
      {
      this->ChartPrivate->Borders[i] = border;
      changed = true;
      }
    }

  if (this->LayoutChanged || changed)
    {
    if (this->DrawAxesAtOrigin)
      {
      this->SetBorders(this->HiddenAxisBorder,
                       this->HiddenAxisBorder,
                       this->ChartPrivate->Borders[2],
                       this->ChartPrivate->Borders[3]);
      // Get the screen coordinates for the origin, and move the axes there.
      vtkVector2f origin;
      vtkTransform2D* transform =
          this->ChartPrivate->PlotCorners[0]->GetTransform();
      transform->TransformPoints(origin.GetData(), origin.GetData(), 1);
      // Need to clamp the axes in the plot area.
      if (int(origin[0]) < this->Point1[0])
        {
        origin[0] = this->Point1[0];
        }
      if (int(origin[0]) > this->Point2[0])
        {
        origin[0] = this->Point2[0];
        }
      if (int(origin[1]) < this->Point1[1])
        {
        origin[1] = this->Point1[1];
        }
      if (int(origin[1]) > this->Point2[1])
        {
        origin[1] = this->Point2[1];
        }

      this->ChartPrivate->axes[vtkAxis::BOTTOM]
          ->SetPoint1(this->Point1[0], origin[1]);
      this->ChartPrivate->axes[vtkAxis::BOTTOM]
          ->SetPoint2(this->Point2[0], origin[1]);
      this->ChartPrivate->axes[vtkAxis::LEFT]
          ->SetPoint1(origin[0], this->Point1[1]);
      this->ChartPrivate->axes[vtkAxis::LEFT]
          ->SetPoint2(origin[0], this->Point2[1]);
      }
    else
      {
      this->SetBorders(this->ChartPrivate->Borders[0],
                       this->ChartPrivate->Borders[1],
                       this->ChartPrivate->Borders[2],
                       this->ChartPrivate->Borders[3]);
      // This is where we set the axes up too
      // Y axis (left)
      this->ChartPrivate->axes[0]->SetPoint1(this->Point1[0], this->Point1[1]);
      this->ChartPrivate->axes[0]->SetPoint2(this->Point1[0], this->Point2[1]);
      // X axis (bottom)
      this->ChartPrivate->axes[1]->SetPoint1(this->Point1[0], this->Point1[1]);
      this->ChartPrivate->axes[1]->SetPoint2(this->Point2[0], this->Point1[1]);
      }
    // Y axis (right)
    this->ChartPrivate->axes[2]->SetPoint1(this->Point2[0], this->Point1[1]);
    this->ChartPrivate->axes[2]->SetPoint2(this->Point2[0], this->Point2[1]);
    // X axis (top)
    this->ChartPrivate->axes[3]->SetPoint1(this->Point1[0], this->Point2[1]);
    this->ChartPrivate->axes[3]->SetPoint2(this->Point2[0], this->Point2[1]);

    for (int i = 0; i < 4; ++i)
      {
      this->ChartPrivate->axes[i]->Update();
      }
    }
  this->SetLegendPosition(this->Legend->GetBoundingRect(painter));

  return changed;
}

//-----------------------------------------------------------------------------
int vtkChartXY::GetLegendBorder(vtkContext2D* painter, int axisPosition)
{
  if (!this->Legend->GetVisible() || this->Legend->GetInline())
    {
    return 0;
    }

  int padding = 10;
  vtkVector2i legendSize;
  vtkVector2i legendAlignment(this->Legend->GetHorizontalAlignment(),
                              this->Legend->GetVerticalAlignment());
  this->Legend->Update();
  vtkRectf rect = this->Legend->GetBoundingRect(painter);
  legendSize.Set(static_cast<int>(rect.Width()),
                 static_cast<int>(rect.Height()));

  // Figure out the correct place and alignment based on the legend layout.
  if (axisPosition == vtkAxis::LEFT &&
      legendAlignment.X() == vtkChartLegend::LEFT)
    {
    return legendSize.X() + padding;
    }
  else if (axisPosition == vtkAxis::RIGHT &&
           legendAlignment.X() == vtkChartLegend::RIGHT)
    {
    return legendSize.X() + padding;
    }
  else if ((axisPosition == vtkAxis::TOP || axisPosition == vtkAxis::BOTTOM) &&
           (legendAlignment.X() == vtkChartLegend::LEFT ||
            legendAlignment.X() == vtkChartLegend::RIGHT))
    {
    return 0;
    }
  else if (axisPosition == vtkAxis::TOP &&
           legendAlignment.Y() == vtkChartLegend::TOP)
    {
    return legendSize.Y() + padding;
    }
  else if (axisPosition == vtkAxis::BOTTOM &&
           legendAlignment.Y() == vtkChartLegend::BOTTOM)
    {
    return legendSize.Y() + padding;
    }
  else
    {
    return 0;
    }
}

//-----------------------------------------------------------------------------
void vtkChartXY::SetLegendPosition(const vtkRectf& rect)
{
  // Put the legend in the top corner of the chart
  vtkVector2f pos;
  int padding = 5;
  vtkVector2i legendAlignment(this->Legend->GetHorizontalAlignment(),
                              this->Legend->GetVerticalAlignment());

  if (legendAlignment[0] == vtkChartLegend::CUSTOM ||
      legendAlignment[1] == vtkChartLegend::CUSTOM)
    {
    return;
    }

  if (this->Legend->GetInline())
    {
    switch (this->Legend->GetHorizontalAlignment())
      {
      case vtkChartLegend::LEFT:
        pos.SetX(this->Point1[0]);
        break;
      case vtkChartLegend::CENTER:
        pos.SetX(((this->Point2[0] - this->Point1[0]) / 2.0)
                 - rect.Width() / 2.0 + this->Point1[0]);
        break;
      case vtkChartLegend::RIGHT:
      default:
        pos.SetX(this->Point2[0] - rect.Width());
      }
    switch (this->Legend->GetVerticalAlignment())
      {
      case vtkChartLegend::TOP:
        pos.SetY(this->Point2[1] - rect.Height());
        break;
      case vtkChartLegend::CENTER:
        pos.SetY((this->Point2[1] - this->Point1[1]) / 2.0
                 - rect.Height() / 2.0 + this->Point1[1]);
        break;
      case vtkChartLegend::BOTTOM:
      default:
        pos.SetY(this->Point1[1]);
      }
    }
  else
    {
    // Non-inline legends.
    if (legendAlignment.X() == vtkChartLegend::LEFT)
      {
      pos.SetX(this->Point1[0] - this->ChartPrivate->Borders[vtkAxis::LEFT]
               + padding);
      }
    else if (legendAlignment.X() == vtkChartLegend::RIGHT)
      {
      pos.SetX(this->Point2[0] + this->ChartPrivate->Borders[vtkAxis::RIGHT]
               - rect.Width() - padding);
      }
    else if (legendAlignment.X() == vtkChartLegend::CENTER)
      {
      pos.SetX(((this->Point2[0] - this->Point1[0]) / 2.0)
               - (rect.Width() / 2.0) + this->Point1[0]);
      // Check for the special case where the legend is on the top or bottom
      if (legendAlignment.Y() == vtkChartLegend::TOP)
        {
        pos.SetY(this->Point2[1] + this->ChartPrivate->Borders[vtkAxis::TOP]
                 - rect.Height() - padding);
        }
      else if (legendAlignment.Y() == vtkChartLegend::BOTTOM)
        {
        pos.SetY(this->Point1[1] - this->ChartPrivate->Borders[vtkAxis::BOTTOM]
                 + padding);
        }
      }
    // Vertical alignment
    if (legendAlignment.X() != vtkChartLegend::CENTER)
      {
      if (legendAlignment.Y() == vtkChartLegend::TOP)
        {
        pos.SetY(this->Point2[1] - rect.Height());
        }
      else if (legendAlignment.Y() == vtkChartLegend::BOTTOM)
        {
        pos.SetY(this->Point1[1]);
        }
      }
    if (legendAlignment.Y() == vtkChartLegend::CENTER)
      {
      pos.SetY(((this->Point2[1] - this->Point1[1]) / 2.0)
               - (rect.Height() / 2.0) + this->Point1[1]);
      }
    }

  this->Legend->SetPoint(pos);
}

//-----------------------------------------------------------------------------
vtkPlot * vtkChartXY::AddPlot(int type)
{
  // Use a variable to return the object created (or NULL), this is necessary
  // as the HP compiler is broken (thinks this function does not return) and
  // the MS compiler generates a warning about unreachable code if a redundant
  // return is added at the end.
  vtkPlot *plot = NULL;
  vtkColor3ub color = this->ChartPrivate->Colors->GetColorRepeating(
      static_cast<int>(this->ChartPrivate->plots.size()));
  switch (type)
    {
    case LINE:
      {
      vtkPlotLine *line = vtkPlotLine::New();
      line->GetPen()->SetColor(color.GetData());
      plot = line;
      break;
      }
    case POINTS:
      {
      vtkPlotPoints *points = vtkPlotPoints::New();
      points->GetPen()->SetColor(color.GetData());
      plot = points;
      break;
      }
    case BAR:
      {
      vtkPlotBar *bar = vtkPlotBar::New();
      bar->GetBrush()->SetColor(color.GetData());
      plot = bar;
      break;
      }
    case STACKED:
      {
      vtkPlotStacked *stacked = vtkPlotStacked::New();
      stacked->SetParent(this);
      stacked->GetBrush()->SetColor(color.GetData());
      plot = stacked;
      break;
      }
    default:
      plot = NULL;
    }
  if (plot)
    {
    this->AddPlot(plot);
    plot->Delete();
    }
  return plot;
}

//-----------------------------------------------------------------------------
vtkIdType vtkChartXY::AddPlot(vtkPlot * plot)
{
  if (plot == NULL)
    {
    return -1;
    }
  plot->Register(this);
  this->ChartPrivate->plots.push_back(plot);
  vtkIdType plotIndex = this->ChartPrivate->plots.size() - 1;
  this->SetPlotCorner(plot, 0);
  // Ensure that the bounds are recalculated
  this->PlotTransformValid = false;
  // Mark the scene as dirty
  if (this->Scene)
    {
    this->Scene->SetDirty(true);
    }
  return plotIndex;
}

//-----------------------------------------------------------------------------
bool vtkChartXY::RemovePlot(vtkIdType index)
{
  if (index < static_cast<vtkIdType>(this->ChartPrivate->plots.size()))
    {
    this->RemovePlotFromCorners(this->ChartPrivate->plots[index]);
    this->ChartPrivate->plots[index]->Delete();
    this->ChartPrivate->plots.erase(this->ChartPrivate->plots.begin()+index);

    // Ensure that the bounds are recalculated
    this->PlotTransformValid = false;
    // Mark the scene as dirty
    this->Scene->SetDirty(true);
    return true;
    }
  else
    {
    return false;
    }
}

//-----------------------------------------------------------------------------
void vtkChartXY::ClearPlots()
{
  for (unsigned int i = 0; i < this->ChartPrivate->plots.size(); ++i)
    {
    this->ChartPrivate->plots[i]->Delete();
    }
  this->ChartPrivate->plots.clear();
  // Clear the corners too
  for (int i = 0; i < int(this->ChartPrivate->PlotCorners.size()); ++i)
    {
    this->ChartPrivate->PlotCorners[i]->ClearItems();
    }

  // Ensure that the bounds are recalculated
  this->PlotTransformValid = false;
  // Mark the scene as dirty
  this->Scene->SetDirty(true);
}

//-----------------------------------------------------------------------------
vtkPlot* vtkChartXY::GetPlot(vtkIdType index)
{
  if (static_cast<vtkIdType>(this->ChartPrivate->plots.size()) > index)
    {
    return this->ChartPrivate->plots[index];
    }
  else
    {
    return NULL;
    }
}

//-----------------------------------------------------------------------------
void vtkChartXY::SetShowLegend(bool visible)
{
  this->vtkChart::SetShowLegend(visible);
  this->Legend->SetVisible(visible);
}

//-----------------------------------------------------------------------------
vtkChartLegend* vtkChartXY::GetLegend()
{
  return this->Legend;
}

//-----------------------------------------------------------------------------
vtkTooltipItem* vtkChartXY::GetTooltip()
{
  return this->Tooltip;
}

//-----------------------------------------------------------------------------
vtkIdType vtkChartXY::GetNumberOfPlots()
{
  return this->ChartPrivate->plots.size();
}

//-----------------------------------------------------------------------------
vtkAxis* vtkChartXY::GetAxis(int axisIndex)
{
  if (axisIndex < 4)
    {
    return this->ChartPrivate->axes[axisIndex];
    }
  else
    {
    return NULL;
    }
}

//-----------------------------------------------------------------------------
vtkIdType vtkChartXY::GetNumberOfAxes()
{
  return 4;
}


//-----------------------------------------------------------------------------
void vtkChartXY::RecalculateBounds()
{
  // Ensure that the bounds are recalculated
  this->PlotTransformValid = false;
  // Mark the scene as dirty
  this->Scene->SetDirty(true);
}

//-----------------------------------------------------------------------------
bool vtkChartXY::Hit(const vtkContextMouseEvent &mouse)
{
  if (mouse.ScreenPos[0] > this->Point1[0] &&
      mouse.ScreenPos[0] < this->Point2[0] &&
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
bool vtkChartXY::MouseEnterEvent(const vtkContextMouseEvent &)
{
  // Find the nearest point on the curves and snap to it
  this->DrawNearestPoint = true;
  return true;
}

//-----------------------------------------------------------------------------
bool vtkChartXY::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  // Iterate through each corner, and check for a nearby point
  for (size_t i = 0; i < this->ChartPrivate->PlotCorners.size(); ++i)
    {
    if (this->ChartPrivate->PlotCorners[i]->MouseMoveEvent(mouse))
      {
      return true;
      }
    }

  if (mouse.Button == this->Actions.Pan())
    {
    // Figure out how much the mouse has moved by in plot coordinates - pan
    double screenPos[2] = { mouse.ScreenPos[0], mouse.ScreenPos[1] };
    double lastScreenPos[2] = { mouse.LastScreenPos[0], mouse.LastScreenPos[1] };
    double pos[2] = { 0.0, 0.0 };
    double last[2] = { 0.0, 0.0 };

    // Go from screen to scene coordinates to work out the delta
    vtkTransform2D *transform =
        this->ChartPrivate->PlotCorners[0]->GetTransform();
    transform->InverseTransformPoints(screenPos, pos, 1);
    transform->InverseTransformPoints(lastScreenPos, last, 1);
    double delta[] = { last[0] - pos[0], last[1] - pos[1] };

    // Now move the axes and recalculate the transform
    vtkAxis* xAxis = this->ChartPrivate->axes[vtkAxis::BOTTOM];
    vtkAxis* yAxis = this->ChartPrivate->axes[vtkAxis::LEFT];
    delta[0] = delta[0] > 0 ?
      std::min(delta[0], xAxis->GetMaximumLimit() - xAxis->GetMaximum()) :
      std::max(delta[0], xAxis->GetMinimumLimit() - xAxis->GetMinimum());
    delta[1] = delta[1] > 0 ?
      std::min(delta[1], yAxis->GetMaximumLimit() - yAxis->GetMaximum()) :
      std::max(delta[1], yAxis->GetMinimumLimit() - yAxis->GetMinimum());
    xAxis->SetMinimum(xAxis->GetMinimum() + delta[0]);
    xAxis->SetMaximum(xAxis->GetMaximum() + delta[0]);
    yAxis->SetMinimum(yAxis->GetMinimum() + delta[1]);
    yAxis->SetMaximum(yAxis->GetMaximum() + delta[1]);

    // Same again for the axes in the top right
    if (this->ChartPrivate->PlotCorners.size() > 2)
      {
      // Go from screen to scene coordinates to work out the delta
      transform = this->ChartPrivate->PlotCorners[2]->GetTransform();
      transform->InverseTransformPoints(screenPos, pos, 1);
      transform->InverseTransformPoints(lastScreenPos, last, 1);
      delta[0] = last[0] - pos[0];
      delta[1] = last[1] - pos[1];

      // Now move the axes and recalculate the transform
      xAxis = this->ChartPrivate->axes[vtkAxis::TOP];
      yAxis = this->ChartPrivate->axes[vtkAxis::RIGHT];
      delta[0] = delta[0] > 0 ?
        std::min(delta[0], xAxis->GetMaximumLimit() - xAxis->GetMaximum()) :
        std::max(delta[0], xAxis->GetMinimumLimit() - xAxis->GetMinimum());
      delta[1] = delta[1] > 0 ?
        std::min(delta[1], yAxis->GetMaximumLimit() - yAxis->GetMaximum()) :
        std::max(delta[1], yAxis->GetMinimumLimit() - yAxis->GetMinimum());
      xAxis->SetMinimum(xAxis->GetMinimum() + delta[0]);
      xAxis->SetMaximum(xAxis->GetMaximum() + delta[0]);
      yAxis->SetMinimum(yAxis->GetMinimum() + delta[1]);
      yAxis->SetMaximum(yAxis->GetMaximum() + delta[1]);
      }

    this->RecalculatePlotTransforms();
    // Mark the scene as dirty
    this->Scene->SetDirty(true);
    }
  else if (mouse.Button == this->Actions.Zoom() ||
           mouse.Button == this->Actions.Select())
    {
    this->MouseBox.SetWidth(mouse.Pos.X() - this->MouseBox.X());
    this->MouseBox.SetHeight(mouse.Pos.Y() - this->MouseBox.Y());
    // Mark the scene as dirty
    this->Scene->SetDirty(true);
    }
  else if (mouse.Button == vtkContextMouseEvent::NO_BUTTON)
    {
    this->Scene->SetDirty(true);
    this->Tooltip->SetVisible(this->LocatePointInPlots(mouse));
    }

  return true;
}

//-----------------------------------------------------------------------------
bool vtkChartXY::LocatePointInPlots(const vtkContextMouseEvent &mouse,
                                    int invokeEvent)
{
  size_t n = this->ChartPrivate->plots.size();
  if (mouse.ScreenPos[0] > this->Point1[0] &&
      mouse.ScreenPos[0] < this->Point2[0] &&
      mouse.ScreenPos[1] > this->Point1[1] &&
      mouse.ScreenPos[1] < this->Point2[1] && n)
    {
    // Iterate through each corner, and check for a nearby point
    for (size_t i = 0; i < this->ChartPrivate->PlotCorners.size(); ++i)
      {
      int items = static_cast<int>(this->ChartPrivate->PlotCorners[i]
                                   ->GetNumberOfItems());
      if (items)
        {
        vtkVector2f plotPos, position;
        vtkTransform2D* transform =
            this->ChartPrivate->PlotCorners[i]->GetTransform();
        transform->InverseTransformPoints(mouse.Pos.GetData(),
                                          position.GetData(), 1);
        // Use a tolerance of +/- 5 pixels
        vtkVector2f tolerance(5*(1.0/transform->GetMatrix()->GetElement(0, 0)),
                              5*(1.0/transform->GetMatrix()->GetElement(1, 1)));
        // Iterate through the visible plots and return on the first hit
        for (int j = items-1; j >= 0; --j)
          {
          vtkPlot* plot = vtkPlot::SafeDownCast(this->ChartPrivate->
                                                PlotCorners[i]->GetItem(j));
          if (plot && plot->GetVisible())
            {
            int seriesIndex;
            vtkIdType segmentIndex = -1;
            vtkPlotBar* plotBar = vtkPlotBar::SafeDownCast(plot);
            if (plotBar)
              {
              // If the plot is a vtkPlotBar, get the segment index too
              seriesIndex = plotBar->GetNearestPoint(position, tolerance,
                                                     &plotPos, &segmentIndex);
              }
            else
              {
              seriesIndex = plot->GetNearestPoint(position, tolerance,
                                                  &plotPos);
              }
            if (seriesIndex >= 0)
              {
              // We found a point, set up the tooltip and return
              this->SetTooltipInfo(mouse, plotPos, seriesIndex, plot,
                                   segmentIndex);
              if (invokeEvent >= 0)
                {
                vtkChartPlotData plotIndex;
                plotIndex.SeriesName = plot->GetLabel();
                plotIndex.Position = plotPos;
                plotIndex.ScreenPosition = mouse.ScreenPos;
                plotIndex.Index = seriesIndex;
                // Invoke an event, with the client data supplied
                this->InvokeEvent(invokeEvent, static_cast<void*>(&plotIndex));

                if (invokeEvent == vtkCommand::SelectionChangedEvent)
                  {
                  vtkNew<vtkIdTypeArray> selectionIds;
                  selectionIds->InsertNextValue(seriesIndex);
                  plot->SetSelection(selectionIds.GetPointer());

                  if (this->AnnotationLink)
                    {
                    vtkNew<vtkSelection> selection;
                    vtkNew<vtkSelectionNode> node;
                    selection->AddNode(node.GetPointer());
                    node->SetContentType(vtkSelectionNode::INDICES);
                    node->SetFieldType(vtkSelectionNode::POINT);
                    node->SetSelectionList(selectionIds.GetPointer());
                    this->AnnotationLink
                        ->SetCurrentSelection(selection.GetPointer());
                    }
                  }
                }
              return true;
              }
            }
          }
        }
      }
    }
  return false;
}

void vtkChartXY::SetTooltipInfo(const vtkContextMouseEvent& mouse,
                                const vtkVector2f &plotPos,
                                vtkIdType seriesIndex, vtkPlot* plot,
                                vtkIdType segmentIndex)
{
  // Have the plot generate its tooltip label
  vtkStdString tooltipLabel = plot->GetTooltipLabel(plotPos, seriesIndex,
                                                    segmentIndex);

  // Set the tooltip
  this->Tooltip->SetText(tooltipLabel);
  this->Tooltip->SetPosition(mouse.ScreenPos[0]+2,
                             mouse.ScreenPos[1]+2);
}

//-----------------------------------------------------------------------------
bool vtkChartXY::MouseLeaveEvent(const vtkContextMouseEvent &)
{
  this->DrawNearestPoint = false;
  this->Tooltip->SetVisible(false);
  return true;
}

//-----------------------------------------------------------------------------
bool vtkChartXY::MouseButtonPressEvent(const vtkContextMouseEvent &mouse)
{
  this->Tooltip->SetVisible(false);
  // Iterate through each corner, and check for a nearby point
  for (size_t i = 0; i < this->ChartPrivate->PlotCorners.size(); ++i)
    {
    if (this->ChartPrivate->PlotCorners[i]->MouseButtonPressEvent(mouse))
      {
      return true;
      }
    }
  if (mouse.Button == this->Actions.Pan())
    {
    // The mouse panning action.
    this->MouseBox.Set(mouse.Pos.X(), mouse.Pos.Y(), 0.0, 0.0);
    this->DrawBox = false;
    return true;
    }
  else if (mouse.Button == this->Actions.Zoom() ||
           mouse.Button == this->Actions.Select())
    {
    // Selection, for now at least...
    this->MouseBox.Set(mouse.Pos.X(), mouse.Pos.Y(), 0.0, 0.0);
    this->DrawBox = true;
    return true;
    }
  else if (mouse.Button == this->ActionsClick.Select() ||
           mouse.Button == this->ActionsClick.Notify())
    {
    return true;
    }
  else
    {
    return false;
    }
}

//-----------------------------------------------------------------------------
bool vtkChartXY::MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse)
{
  // Iterate through each corner, and check for a nearby point
  for (size_t i = 0; i < this->ChartPrivate->PlotCorners.size(); ++i)
    {
    if (this->ChartPrivate->PlotCorners[i]->MouseButtonReleaseEvent(mouse))
      {
      return true;
      }
    }
  if (mouse.Button > vtkContextMouseEvent::NO_BUTTON &&
      mouse.Button <= vtkContextMouseEvent::RIGHT_BUTTON)
    {
    this->MouseBox.SetWidth(mouse.Pos.X() - this->MouseBox.X());
    this->MouseBox.SetHeight(mouse.Pos.Y() - this->MouseBox.Y());
    if ((fabs(this->MouseBox.Width()) < 0.5 && fabs(this->MouseBox.Height()) < 0.5)
        && (mouse.Button == this->Actions.Select() ||
            mouse.Button == this->Actions.Pan()))
      {
      // Invalid box size - treat as a single clicke event
      this->MouseBox.SetWidth(0.0);
      this->MouseBox.SetHeight(0.0);
      this->DrawBox = false;
      if (mouse.Button == this->ActionsClick.Notify())
        {
        this->LocatePointInPlots(mouse, vtkCommand::InteractionEvent);
        return true;
        }
      else if (mouse.Button == this->ActionsClick.Select())
        {
        this->LocatePointInPlots(mouse, vtkCommand::SelectionChangedEvent);
        return true;
        }
      else
        {
        return false;
        }
      }
    }
  if (mouse.Button == this->Actions.Select())
    {
    if (fabs(this->MouseBox.Width()) < 0.5 || fabs(this->MouseBox.Height()) < 0.5)
      {
      // Invalid box size - do nothing
      this->MouseBox.SetWidth(0.0);
      this->MouseBox.SetHeight(0.0);
      this->DrawBox = false;
      return true;
      }
    // Iterate through the plots and build a selection
    for (size_t i = 0; i < this->ChartPrivate->PlotCorners.size(); ++i)
      {
      int items = static_cast<int>(this->ChartPrivate->PlotCorners[i]
                                   ->GetNumberOfItems());
      if (items)
        {
        vtkTransform2D *transform =
            this->ChartPrivate->PlotCorners[i]->GetTransform();
        transform->InverseTransformPoints(this->MouseBox.GetData(),
                                          this->MouseBox.GetData(), 1);
        float point2[] = { mouse.Pos[0], mouse.Pos[1] };
        transform->InverseTransformPoints(point2, point2, 1);

        vtkVector2f min(this->MouseBox.GetData());
        vtkVector2f max(point2);
        if (min.X() > max.X())
          {
          float tmp = min.X();
          min.SetX(max.X());
          max.SetX(tmp);
          }
        if (min.Y() > max.Y())
          {
          float tmp = min.Y();
          min.SetY(max.Y());
          max.SetY(tmp);
          }

        for (int j = 0; j < items; ++j)
          {
          vtkPlot* plot = vtkPlot::SafeDownCast(this->ChartPrivate->
                                                PlotCorners[i]->GetItem(j));
          if (plot && plot->GetVisible())
            {
            /*
             * Populate the internal selection.  This will be referenced later
             * to subsequently populate the selection inside the annotation link.
             */
            plot->SelectPoints(min, max);

            if (this->AnnotationLink)
              {
              // FIXME: Build up a selection from each plot?
              vtkSelection* selection = vtkSelection::New();
              vtkSelectionNode* node = vtkSelectionNode::New();
              selection->AddNode(node);
              node->SetContentType(vtkSelectionNode::INDICES);
              node->SetFieldType(vtkSelectionNode::POINT);
              node->SetSelectionList(plot->GetSelection());
              this->AnnotationLink->SetCurrentSelection(selection);
              node->Delete();
              selection->Delete();
              }
            }
          }
        }
      }

    this->InvokeEvent(vtkCommand::SelectionChangedEvent);
    this->MouseBox.SetWidth(0.0);
    this->MouseBox.SetHeight(0.0);
    this->DrawBox = false;
    // Mark the scene as dirty
    this->Scene->SetDirty(true);
    return true;
    }
  else if (mouse.Button == this->Actions.Zoom())
    {
    // Check whether a valid zoom box was drawn
    if (fabs(this->MouseBox.Width()) < 0.5 || fabs(this->MouseBox.Height()) < 0.5)
      {
      // Invalid box size - do nothing
      this->MouseBox.SetWidth(0.0);
      this->MouseBox.SetHeight(0.0);
      this->DrawBox = false;
      return true;
      }

    // Zoom into the chart by the specified amount, and recalculate the bounds
    float point2[] = { mouse.Pos[0], mouse.Pos[1] };

    this->ZoomInAxes(this->ChartPrivate->axes[vtkAxis::BOTTOM],
                     this->ChartPrivate->axes[vtkAxis::LEFT],
                     this->MouseBox.GetData(), point2);
    this->ZoomInAxes(this->ChartPrivate->axes[vtkAxis::TOP],
                     this->ChartPrivate->axes[vtkAxis::RIGHT],
                     this->MouseBox.GetData(), point2);

    this->RecalculatePlotTransforms();
    this->MouseBox.SetWidth(0.0);
    this->MouseBox.SetHeight(0.0);
    this->DrawBox = false;
    // Mark the scene as dirty
    this->Scene->SetDirty(true);
    return true;
    }
  return false;
}

void vtkChartXY::ZoomInAxes(vtkAxis *x, vtkAxis *y, float *origin, float *max)
{
  vtkTransform2D *transform = vtkTransform2D::New();
  this->CalculatePlotTransform(x, y, transform);
  float torigin[2];
  transform->InverseTransformPoints(origin, torigin, 1);
  float tmax[2];
  transform->InverseTransformPoints(max, tmax, 1);

  // Ensure we preserve the directionality of the axes
  if (x->GetMaximum() > x->GetMinimum())
    {
    x->SetMaximum(torigin[0] > tmax[0] ? torigin[0] : tmax[0]);
    x->SetMinimum(torigin[0] < tmax[0] ? torigin[0] : tmax[0]);
    }
  else
    {
    x->SetMaximum(torigin[0] < tmax[0] ? torigin[0] : tmax[0]);
    x->SetMinimum(torigin[0] > tmax[0] ? torigin[0] : tmax[0]);
    }
  if (y->GetMaximum() > y->GetMinimum())
    {
    y->SetMaximum(torigin[1] > tmax[1] ? torigin[1] : tmax[1]);
    y->SetMinimum(torigin[1] < tmax[1] ? torigin[1] : tmax[1]);
    }
  else
    {
    y->SetMaximum(torigin[1] < tmax[1] ? torigin[1] : tmax[1]);
    y->SetMinimum(torigin[1] > tmax[1] ? torigin[1] : tmax[1]);
    }
  x->RecalculateTickSpacing();
  y->RecalculateTickSpacing();
  transform->Delete();
}

//-----------------------------------------------------------------------------
bool vtkChartXY::MouseWheelEvent(const vtkContextMouseEvent &, int delta)
{
  this->Tooltip->SetVisible(false);
  // Get the bounds of each plot.
  for (int i = 0; i < 4; ++i)
    {
    vtkAxis *axis = this->ChartPrivate->axes[i];
    double min = axis->GetMinimum();
    double max = axis->GetMaximum();
    double frac = (max - min) * 0.1;
    if (frac > 0.0)
      {
      min += delta*frac;
      max -= delta*frac;
      }
    else
      {
      min -= delta*frac;
      max += delta*frac;
      }
    axis->SetMinimum(min);
    axis->SetMaximum(max);
    axis->RecalculateTickSpacing();
    }

  this->RecalculatePlotTransforms();

  // Mark the scene as dirty
  this->Scene->SetDirty(true);

  return true;
}

//-----------------------------------------------------------------------------
bool vtkChartXY::RemovePlotFromCorners(vtkPlot *plot)
{
  // We know the plot will only ever be in one of the corners
  for (size_t i = 0; i < this->ChartPrivate->PlotCorners.size(); ++i)
    {
    if(this->ChartPrivate->PlotCorners[i]->RemoveItem(plot))
      {
      return true;
      }
    }
  return false;
}

//-----------------------------------------------------------------------------
void vtkChartXY::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Axes: " << endl;
  for (int i = 0; i < 4; ++i)
    {
    this->ChartPrivate->axes[i]->PrintSelf(os, indent.GetNextIndent());
    }
  if (this->ChartPrivate)
    {
    os << indent << "Number of plots: " << this->ChartPrivate->plots.size()
       << endl;
    for (unsigned int i = 0; i < this->ChartPrivate->plots.size(); ++i)
      {
      os << indent << "Plot " << i << ":" << endl;
      this->ChartPrivate->plots[i]->PrintSelf(os, indent.GetNextIndent());
      }
    }

}
