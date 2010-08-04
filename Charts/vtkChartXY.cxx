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
#include "vtkContextDevice2D.h"
#include "vtkTransform2D.h"
#include "vtkContextScene.h"
#include "vtkPoints2D.h"
#include "vtkVector.h"

#include "vtkPlot.h"
#include "vtkPlotBar.h"
#include "vtkPlotLine.h"
#include "vtkPlotPoints.h"
#include "vtkContextMapper2D.h"

#include "vtkAxis.h"
#include "vtkPlotGrid.h"
#include "vtkChartLegend.h"
#include "vtkTooltipItem.h"

#include "vtkTable.h"
#include "vtkAbstractArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkIdTypeArray.h"

#include "vtkAnnotationLink.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"

#include "vtkObjectFactory.h"
#include "vtkCommand.h"

#include "vtkStdString.h"
#include "vtkTextProperty.h"

#include "vtksys/ios/sstream"

// My STL containers
#include <vtkstd/vector>

//-----------------------------------------------------------------------------
class vtkChartXYPrivate
{
public:
  vtkChartXYPrivate()
    {
    this->Colors = vtkSmartPointer<vtkColorSeries>::New();
    this->PlotCorners.resize(4);
    this->PlotTransforms.resize(4);
    this->PlotTransforms[0] = vtkSmartPointer<vtkTransform2D>::New();
    this->Borders[0] = 60;
    this->Borders[1] = 50;
    this->Borders[2] = 20;
    this->Borders[3] = 20;
    }

  vtkstd::vector<vtkPlot *> plots; // Charts can contain multiple plots of data
  vtkstd::vector< vtkstd::vector<vtkPlot *> > PlotCorners; // Stored by corner...
  vtkstd::vector< vtkSmartPointer<vtkTransform2D> > PlotTransforms; // Transforms
  vtkstd::vector<vtkAxis *> axes; // Charts can contain multiple axes
  vtkSmartPointer<vtkColorSeries> Colors; // Colors in the chart
  int Borders[4];
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkChartXY);

//-----------------------------------------------------------------------------
vtkChartXY::vtkChartXY()
{
  this->Legend = vtkChartLegend::New();
  this->Legend->SetChart(this);
  this->ChartPrivate = new vtkChartXYPrivate;

  this->AutoAxes = true;

  for (int i = 0; i < 4; ++i)
    {
    this->ChartPrivate->axes.push_back(vtkAxis::New());
    // By default just show the left and bottom axes
    this->ChartPrivate->axes.back()->SetVisible(i < 2 ? true : false);
    }
  this->ChartPrivate->axes[vtkAxis::LEFT]->SetPosition(vtkAxis::LEFT);
  this->ChartPrivate->axes[vtkAxis::BOTTOM]->SetPosition(vtkAxis::BOTTOM);
  this->ChartPrivate->axes[vtkAxis::RIGHT]->SetPosition(vtkAxis::RIGHT);
  this->ChartPrivate->axes[vtkAxis::TOP]->SetPosition(vtkAxis::TOP);

  // Set up the x and y axes - should be congigured based on data
  this->ChartPrivate->axes[vtkAxis::LEFT]->SetTitle("Y Axis");
  this->ChartPrivate->axes[vtkAxis::BOTTOM]->SetTitle("X Axis");

  this->Grid = vtkPlotGrid::New();
  this->Grid->SetXAxis(this->ChartPrivate->axes[vtkAxis::BOTTOM]);
  this->Grid->SetYAxis(this->ChartPrivate->axes[vtkAxis::LEFT]);
  this->Grid2 = vtkPlotGrid::New();
  this->Grid2->SetXAxis(this->ChartPrivate->axes[vtkAxis::TOP]);
  this->Grid2->SetYAxis(this->ChartPrivate->axes[vtkAxis::RIGHT]);

  this->PlotTransformValid = false;

  this->BoxOrigin[0] = this->BoxOrigin[1] = 0.0f;
  this->BoxGeometry[0] = this->BoxGeometry[1] = 0.0f;
  this->DrawBox = false;
  this->DrawNearestPoint = false;
  this->DrawAxesAtOrigin = false;
  this->BarWidthFraction = 0.8f;

  this->Tooltip = vtkTooltipItem::New();
  this->Tooltip->SetVisible(false);
  this->LayoutChanged = true;
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

  this->Grid->Delete();
  this->Grid2->Delete();

  this->Legend->Delete();
  this->Legend = 0;

  this->Tooltip->Delete();
  this->Tooltip = 0;
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
  if (this->ShowLegend)
    {
    this->Legend->Update();
    }

  if (this->AutoAxes)
    {
    for (int i = 0; i < 4; ++i)
      {
      this->ChartPrivate->axes[i]->SetVisible(false);
      }
    for (int i = 0; i < 4; ++i)
      {
      int visible = 0;
      for (unsigned int j = 0;
           j < this->ChartPrivate->PlotCorners[i].size(); ++j)
        {
        if (this->ChartPrivate->PlotCorners[i][j]->GetVisible())
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
    for (int i = 0; i < 4; ++i)
      {
      int border = 20;
      if (this->ChartPrivate->axes[i]->GetVisible())
        {
        if (i == 1 || i == 3)
          {
          border = 50;
          }
        else
          {
          border = 60;
          }
        }
      if (this->ChartPrivate->Borders[i] != border)
        {
        this->ChartPrivate->Borders[i] = border;
        this->LayoutChanged = true;
        }
      }
    }

}

//-----------------------------------------------------------------------------
bool vtkChartXY::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called.");

  int geometry[] = { this->GetScene()->GetSceneWidth(),
                     this->GetScene()->GetSceneHeight() };
  if (geometry[0] == 0 || geometry[1] == 0 || !this->Visible)
    {
    // The geometry of the chart must be valid before anything can be drawn
    return false;
    }

  int visiblePlots = 0;
  for (size_t i = 0; i < this->ChartPrivate->plots.size(); ++i)
    {
    if (this->ChartPrivate->plots[i]->GetVisible())
      {
      ++visiblePlots;
      }
    }

  if (visiblePlots == 0)
    {
    // Nothing to plot, so don't draw anything.
    return false;
    }

  this->Update();

  bool recalculateTransform = false;
  this->CalculateBarPlots();

  if (geometry[0] != this->Geometry[0] || geometry[1] != this->Geometry[1] ||
      this->MTime > this->ChartPrivate->axes[0]->GetMTime() ||
      this->LayoutChanged)
    {
    // Take up the entire window right now, this could be made configurable
    this->SetGeometry(geometry);
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
    // Y axis (right)
    this->ChartPrivate->axes[2]->SetPoint1(this->Point2[0], this->Point1[1]);
    this->ChartPrivate->axes[2]->SetPoint2(this->Point2[0], this->Point2[1]);
    // X axis (top)
    this->ChartPrivate->axes[3]->SetPoint1(this->Point1[0], this->Point2[1]);
    this->ChartPrivate->axes[3]->SetPoint2(this->Point2[0], this->Point2[1]);

    // Put the legend in the top corner of the chart
    this->Legend->SetPoint(this->Point2[0], this->Point2[1]);
    // Cause the plot transform to be recalculated if necessary
    recalculateTransform = true;
    this->LayoutChanged = false;
    }

  if (this->ChartPrivate->plots[0]->GetData()->GetInput()->GetMTime() > this->MTime)
    {
    this->RecalculateBounds();
    }

  // Recalculate the plot transform, min and max values if necessary
  if (!this->PlotTransformValid)
    {
    this->RecalculatePlotBounds();
    this->RecalculatePlotTransforms();
    }
  else if (recalculateTransform)
    {
    this->RecalculatePlotTransforms();
    }

  // Update the axes in the chart
  for (int i = 0; i < 4; ++i)
    {
    this->ChartPrivate->axes[i]->Update();
    }

  // Draw the grid - the axes take care of its color and visibility
  this->Grid->Paint(painter);
  this->Grid2->Paint(painter);

  // Plot the series of the chart
  this->RenderPlots(painter);

  // Set the color and width, draw the axes, color and width push to axis props
  painter->GetPen()->SetColorF(0.0, 0.0, 0.0, 1.0);
  painter->GetPen()->SetWidth(1.0);

  // Paint the axes in the chart
  for (int i = 0; i < 4; ++i)
    {
    this->ChartPrivate->axes[i]->Paint(painter);
    }

  if (this->ShowLegend)
    {
    this->Legend->Paint(painter);
    }

  // Draw the selection box if necessary
  if (this->DrawBox)
    {
    painter->GetBrush()->SetColor(255, 255, 255, 0);
    painter->GetPen()->SetColor(0, 0, 0, 255);
    painter->GetPen()->SetWidth(1.0);
    painter->DrawRect(this->BoxOrigin[0], this->BoxOrigin[1],
                      this->BoxGeometry[0], this->BoxGeometry[1]);
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

  // Draw in the current mouse location...
  this->Tooltip->Paint(painter);

  return true;
}

//-----------------------------------------------------------------------------
void vtkChartXY::RenderPlots(vtkContext2D *painter)
{
  vtkIdTypeArray *idArray = 0;
  if (this->AnnotationLink)
    {
    this->AnnotationLink->Update();
    vtkSelection *selection =
        vtkSelection::SafeDownCast(this->AnnotationLink->GetOutputDataObject(2));
    if (selection->GetNumberOfNodes())
      {
      vtkSelectionNode *node = selection->GetNode(0);
      idArray = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
      }
    }
  else
    {
    vtkDebugMacro("No annotation link set.");
    }

  // Clip drawing while plotting
  float clip[] = { this->Point1[0], this->Point1[1],
                 this->Point2[0]-this->Point1[0],
                 this->Point2[1]-this->Point1[1] };
  // Check whether the scene has a transform - use it if so
  if (this->Scene->HasTransform())
    {
    this->Scene->GetTransform()->InverseTransformPoints(clip, clip, 2);
    }
  int clipi[] = { static_cast<int>(clip[0]),
                  static_cast<int>(clip[1]),
                  static_cast<int>(clip[2]),
                  static_cast<int>(clip[3]) };
  painter->GetDevice()->SetClipping(clipi);

  // Push the matrix and use the transform we just calculated
  for (int i = 0; i < 4; ++i)
    {
    if (this->ChartPrivate->PlotCorners[i].size())
      {
      painter->PushMatrix();
      painter->AppendTransform(this->ChartPrivate->PlotTransforms[i]);

      // Now iterate through the plots
      vtkstd::vector<vtkPlot*>::iterator it =
          this->ChartPrivate->PlotCorners[i].begin();
      for ( ; it != this->ChartPrivate->PlotCorners[i].end(); ++it)
        {
        (*it)->SetSelection(idArray);
        (*it)->Paint(painter);
        }
      painter->PopMatrix();
      }
    }

  // Stop clipping of the plot area and reset back to screen coordinates
  painter->GetDevice()->DisableClipping();
}

//-----------------------------------------------------------------------------
void vtkChartXY::CalculateBarPlots()
{
  // Calculate the width, spacing and offsets for the bar plot - they are grouped
  size_t n = this->ChartPrivate->plots.size();
  vtkstd::vector<vtkPlotBar *> bars;
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
    float barWidth = 0.0;
    vtkPlotBar* bar = bars[0];
    if (!bar->GetUseIndexForXSeries())
      {
      vtkTable *table = bar->GetData()->GetInput();
      vtkDataArray* x = bar->GetData()->GetInputArrayToProcess(0, table);
      if (x->GetSize() > 1)
        {
        double x0 = x->GetTuple1(0);
        double x1 = x->GetTuple1(1);
        float width = static_cast<float>((x1 - x0) * this->BarWidthFraction);
        barWidth = width / bars.size();
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
  if (this->ChartPrivate->PlotCorners[0].size())
    {
    this->RecalculatePlotTransform(this->ChartPrivate->axes[vtkAxis::BOTTOM],
                                   this->ChartPrivate->axes[vtkAxis::LEFT],
                                   this->ChartPrivate->PlotTransforms[0]);
    }
  if (this->ChartPrivate->PlotCorners[1].size())
    {
    if (!this->ChartPrivate->PlotTransforms[1])
      {
      this->ChartPrivate->PlotTransforms[1] = vtkSmartPointer<vtkTransform2D>::New();
      }
    this->RecalculatePlotTransform(this->ChartPrivate->axes[vtkAxis::BOTTOM],
                                   this->ChartPrivate->axes[vtkAxis::RIGHT],
                                   this->ChartPrivate->PlotTransforms[1]);
    }
  if (this->ChartPrivate->PlotCorners[2].size())
    {
    if (!this->ChartPrivate->PlotTransforms[2])
      {
      this->ChartPrivate->PlotTransforms[2] = vtkSmartPointer<vtkTransform2D>::New();
      }
    this->RecalculatePlotTransform(this->ChartPrivate->axes[vtkAxis::TOP],
                                   this->ChartPrivate->axes[vtkAxis::RIGHT],
                                   this->ChartPrivate->PlotTransforms[2]);
    }
  if (this->ChartPrivate->PlotCorners[3].size())
    {
    if (!this->ChartPrivate->PlotTransforms[3])
      {
      this->ChartPrivate->PlotTransforms[3] = vtkSmartPointer<vtkTransform2D>::New();
      }
    this->RecalculatePlotTransform(this->ChartPrivate->axes[vtkAxis::TOP],
                                   this->ChartPrivate->axes[vtkAxis::LEFT],
                                   this->ChartPrivate->PlotTransforms[3]);
    }
}

//-----------------------------------------------------------------------------
void vtkChartXY::RecalculatePlotTransform(vtkAxis *x, vtkAxis *y,
                                          vtkTransform2D *transform)
{
  // Get the scale for the plot area from the x and y axes
  float *min = x->GetPoint1();
  float *max = x->GetPoint2();
  if (fabs(max[0] - min[0]) == 0.0f)
    {
    return;
    }
  float xScale = (x->GetMaximum() - x->GetMinimum()) / (max[0] - min[0]);

  // Now the y axis
  min = y->GetPoint1();
  max = y->GetPoint2();
  if (fabs(max[1] - min[1]) == 0.0f)
    {
    return;
    }
  float yScale = (y->GetMaximum() - y->GetMinimum()) / (max[1] - min[1]);

  transform->Identity();
  transform->Translate(this->Point1[0], this->Point1[1]);
  // Get the scale for the plot area from the x and y axes
  transform->Scale(1.0 / xScale, 1.0 / yScale);
  transform->Translate(-x->GetMinimum(), -y->GetMinimum());

  // Move the axes if necessary and if the draw axes at origin ivar is true.
  if (this->DrawAxesAtOrigin && x == this->ChartPrivate->axes[vtkAxis::BOTTOM] &&
      y == this->ChartPrivate->axes[vtkAxis::LEFT])
    {
    // Get the screen coordinates for the origin, and move the axes there.
    float origin[2] = { 0.0, 0.0 };
    transform->TransformPoints(origin, origin, 1);
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

    this->ChartPrivate->axes[vtkAxis::BOTTOM]->SetPoint1(this->Point1[0], origin[1]);
    this->ChartPrivate->axes[vtkAxis::BOTTOM]->SetPoint2(this->Point2[0], origin[1]);
    this->ChartPrivate->axes[vtkAxis::LEFT]->SetPoint1(origin[0], this->Point1[1]);
    this->ChartPrivate->axes[vtkAxis::LEFT]->SetPoint2(origin[0], this->Point2[1]);
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
  if (!this->RemovePlotFromCorners(plot))
    {
    vtkWarningMacro("Error removing plot from corners.");
    }
  this->ChartPrivate->PlotCorners[corner].push_back(plot);
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

  vtkstd::vector<vtkPlot*>::iterator it;
  double bounds[4] = { 0.0, 0.0, 0.0, 0.0 };
  for (it = this->ChartPrivate->plots.begin();
       it != this->ChartPrivate->plots.end(); ++it)
    {
    if ((*it)->GetVisible() == false)
      {
      continue;
      }
    (*it)->GetBounds(bounds);
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

    if (axis->GetBehavior() == 0 && initialized[i])
      {
      axis->SetRange(range[0], range[1]);
      axis->AutoScale();
      }
    }

  this->Modified();
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
    default:
      plot = NULL;
    }
  // Add the plot to the default corner
  plot->SetXAxis(this->ChartPrivate->axes[vtkAxis::BOTTOM]);
  plot->SetYAxis(this->ChartPrivate->axes[vtkAxis::LEFT]);
  this->ChartPrivate->plots.push_back(plot);
  this->ChartPrivate->PlotCorners[0].push_back(plot);
  // Ensure that the bounds are recalculated
  this->PlotTransformValid = false;
  // Mark the scene as dirty
  this->Scene->SetDirty(true);
  return plot;
}

//-----------------------------------------------------------------------------
bool vtkChartXY::RemovePlot(vtkIdType index)
{
  if (static_cast<vtkIdType>(this->ChartPrivate->plots.size()) > index)
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
void vtkChartXY::SetScene(vtkContextScene *scene)
{
  this->vtkContextItem::SetScene(scene);
  this->Tooltip->SetScene(scene);
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
  if (mouse.Button == vtkContextMouseEvent::LEFT_BUTTON)
    {
    // Figure out how much the mouse has moved by in plot coordinates - pan
    double screenPos[2] = { mouse.ScreenPos[0], mouse.ScreenPos[1] };
    double lastScreenPos[2] = { mouse.LastScreenPos[0], mouse.LastScreenPos[1] };
    double pos[2] = { 0.0, 0.0 };
    double last[2] = { 0.0, 0.0 };

    // Go from screen to scene coordinates to work out the delta
    this->ChartPrivate->PlotTransforms[0]
        ->InverseTransformPoints(screenPos, pos, 1);
    this->ChartPrivate->PlotTransforms[0]
        ->InverseTransformPoints(lastScreenPos, last, 1);
    double delta[] = { last[0] - pos[0], last[1] - pos[1] };

    // Now move the axes and recalculate the transform
    vtkAxis* xAxis = this->ChartPrivate->axes[vtkAxis::BOTTOM];
    vtkAxis* yAxis = this->ChartPrivate->axes[vtkAxis::LEFT];
    xAxis->SetMinimum(xAxis->GetMinimum() + delta[0]);
    xAxis->SetMaximum(xAxis->GetMaximum() + delta[0]);
    yAxis->SetMinimum(yAxis->GetMinimum() + delta[1]);
    yAxis->SetMaximum(yAxis->GetMaximum() + delta[1]);

    // Same again for the axes in the top right
    if (this->ChartPrivate->PlotTransforms[2])
    {
      // Go from screen to scene coordinates to work out the delta
      this->ChartPrivate->PlotTransforms[2]
          ->InverseTransformPoints(screenPos, pos, 1);
      this->ChartPrivate->PlotTransforms[2]
          ->InverseTransformPoints(lastScreenPos, last, 1);
      delta[0] = last[0] - pos[0];
      delta[1] = last[1] - pos[1];

      // Now move the axes and recalculate the transform
      xAxis = this->ChartPrivate->axes[vtkAxis::TOP];
      yAxis = this->ChartPrivate->axes[vtkAxis::RIGHT];
      xAxis->SetMinimum(xAxis->GetMinimum() + delta[0]);
      xAxis->SetMaximum(xAxis->GetMaximum() + delta[0]);
      yAxis->SetMinimum(yAxis->GetMinimum() + delta[1]);
      yAxis->SetMaximum(yAxis->GetMaximum() + delta[1]);
    }

    this->RecalculatePlotTransforms();
    // Mark the scene as dirty
    this->Scene->SetDirty(true);
    }
  else if (mouse.Button == vtkContextMouseEvent::MIDDLE_BUTTON)
    {
    this->BoxGeometry[0] = mouse.Pos[0] - this->BoxOrigin[0];
    this->BoxGeometry[1] = mouse.Pos[1] - this->BoxOrigin[1];
    // Mark the scene as dirty
    this->Scene->SetDirty(true);
    }
  else if (mouse.Button == vtkContextMouseEvent::RIGHT_BUTTON)
    {
    this->BoxGeometry[0] = mouse.Pos[0] - this->BoxOrigin[0];
    this->BoxGeometry[1] = mouse.Pos[1] - this->BoxOrigin[1];
    // Mark the scene as dirty
    this->Scene->SetDirty(true);
    }
  else if (mouse.Button < 0)
    {
    this->Scene->SetDirty(true);
    this->Tooltip->SetVisible(this->LocatePointInPlots(mouse));
    }

  return true;
}

//-----------------------------------------------------------------------------
bool vtkChartXY::LocatePointInPlots(const vtkContextMouseEvent &mouse)
{
  size_t n = this->ChartPrivate->plots.size();
  if (mouse.ScreenPos[0] > this->Point1[0] &&
      mouse.ScreenPos[0] < this->Point2[0] &&
      mouse.ScreenPos[1] > this->Point1[1] &&
      mouse.ScreenPos[1] < this->Point2[1] && n)
    {
    // Iterate through each corner, and check for a nearby point
    for (int i = 0; i < 4; ++i)
      {
      if (this->ChartPrivate->PlotCorners[i].size())
        {
        vtkVector2f plotPos, position;
        vtkTransform2D* transform = this->ChartPrivate->PlotTransforms[i];
        transform->InverseTransformPoints(mouse.Pos.GetData(),
                                          position.GetData(), 1);
        // Use a tolerance of +/- 5 pixels
        vtkVector2f tolerance(5*(1.0/transform->GetMatrix()->GetElement(0, 0)),
                              5*(1.0/transform->GetMatrix()->GetElement(1, 1)));
        // Iterate through the visible plots and return on the first hit
        for (int j = static_cast<int>(this->ChartPrivate->PlotCorners[i].size()-1);
             j >= 0; --j)
          {
          vtkPlot* plot = this->ChartPrivate->PlotCorners[i][j];
          if (plot->GetVisible())
            {
            bool found = plot->GetNearestPoint(position, tolerance, &plotPos);
            if (found)
              {
              // We found a point, set up the tooltip and return
              vtksys_ios::ostringstream ostr;
              ostr << plot->GetLabel() << ": " << plotPos.X() << ", " << plotPos.Y();
              this->Tooltip->SetText(ostr.str().c_str());
              this->Tooltip->SetPosition(mouse.ScreenPos[0]+2, mouse.ScreenPos[1]+2);
              return true;
              }
            }
          }
        }
      }
    }
  return false;
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
  if (mouse.Button == vtkContextMouseEvent::LEFT_BUTTON)
    {
    // The mouse panning action.
    return true;
    }
  else if (mouse.Button == vtkContextMouseEvent::MIDDLE_BUTTON)
    {
    // Selection, for now at least...
    this->BoxOrigin[0] = mouse.Pos[0];
    this->BoxOrigin[1] = mouse.Pos[1];
    this->BoxGeometry[0] = this->BoxGeometry[1] = 0.0f;
    this->DrawBox = true;
    return true;
    }
  else if (mouse.Button == vtkContextMouseEvent::RIGHT_BUTTON)
    {
    // Right mouse button - zoom box
    this->BoxOrigin[0] = mouse.Pos[0];
    this->BoxOrigin[1] = mouse.Pos[1];
    this->BoxGeometry[0] = this->BoxGeometry[1] = 0.0f;
    this->DrawBox = true;
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
  if (mouse.Button == vtkContextMouseEvent::MIDDLE_BUTTON)
    {
    // Check whether a valid selection box was drawn
    this->BoxGeometry[0] = mouse.Pos[0] - this->BoxOrigin[0];
    this->BoxGeometry[1] = mouse.Pos[1] - this->BoxOrigin[1];
    if (fabs(this->BoxGeometry[0]) < 0.5 || fabs(this->BoxGeometry[1]) < 0.5)
      {
      // Invalid box size - do nothing
      this->BoxGeometry[0] = this->BoxGeometry[1] = 0.0f;
      this->DrawBox = false;
      return true;
      }

    // Iterate through the plots and build a selection
    for (int i = 0; i < 4; ++i)
    {
      if (this->ChartPrivate->PlotCorners[i].size())
      {
        vtkTransform2D *transform = this->ChartPrivate->PlotTransforms[i];
        transform->InverseTransformPoints(this->BoxOrigin, this->BoxOrigin, 1);
        float point2[] = { mouse.Pos[0], mouse.Pos[1] };
        transform->InverseTransformPoints(point2, point2, 1);

        vtkVector2f min(this->BoxOrigin);
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

        vtkstd::vector<vtkPlot*>::iterator it =
            this->ChartPrivate->PlotCorners[i].begin();
        for ( ; it != this->ChartPrivate->PlotCorners[i].end(); ++it)
          {
          vtkPlot* plot = *it;
          if (plot->SelectPoints(min, max))
            {
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
    this->BoxGeometry[0] = this->BoxGeometry[1] = 0.0f;
    this->DrawBox = false;
    // Mark the scene as dirty
    this->Scene->SetDirty(true);
    return true;
    }
  if (mouse.Button == vtkContextMouseEvent::RIGHT_BUTTON)
    {
    // Check whether a valid zoom box was drawn
    this->BoxGeometry[0] = mouse.Pos[0] - this->BoxOrigin[0];
    this->BoxGeometry[1] = mouse.Pos[1] - this->BoxOrigin[1];
    if (fabs(this->BoxGeometry[0]) < 0.5 || fabs(this->BoxGeometry[1]) < 0.5)
      {
      // Invalid box size - do nothing
      this->BoxGeometry[0] = this->BoxGeometry[1] = 0.0f;
      this->DrawBox = false;
      return true;
      }

    // Zoom into the chart by the specified amount, and recalculate the bounds
    float point2[] = { mouse.Pos[0], mouse.Pos[1] };

    this->ZoomInAxes(this->ChartPrivate->axes[vtkAxis::BOTTOM],
                     this->ChartPrivate->axes[vtkAxis::LEFT],
                     this->BoxOrigin, point2);
    this->ZoomInAxes(this->ChartPrivate->axes[vtkAxis::TOP],
                     this->ChartPrivate->axes[vtkAxis::RIGHT],
                     this->BoxOrigin, point2);

    this->RecalculatePlotTransforms();
    this->BoxGeometry[0] = this->BoxGeometry[1] = 0.0f;
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
  this->RecalculatePlotTransform(x, y, transform);
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
void vtkChartXY::ProcessSelectionEvent(vtkObject* , void* )
{
}

//-----------------------------------------------------------------------------
bool vtkChartXY::RemovePlotFromCorners(vtkPlot *plot)
{
  // We know the plot will only ever be in one of the corners
  for (int i = 0; i < 4; ++i)
    {
    vtkstd::vector<vtkPlot*>::iterator it =
        this->ChartPrivate->PlotCorners[i].begin();
    for ( ; it !=this->ChartPrivate->PlotCorners[i].end(); ++it)
      {
      if ((*it) == plot)
        {
        this->ChartPrivate->PlotCorners[i].erase(it);
        return true;
        }
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
