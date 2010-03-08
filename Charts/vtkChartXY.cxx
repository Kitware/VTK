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
    }

  vtkstd::vector<vtkPlot *> plots; // Charts can contain multiple plots of data
  vtkstd::vector<vtkAxis *> axes; // Charts can contain multiple axes
  vtkSmartPointer<vtkColorSeries> Colors; // Colors in the chart
};

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkChartXY, "1.43");

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkChartXY);

//-----------------------------------------------------------------------------
vtkChartXY::vtkChartXY()
{
  this->Legend = vtkChartLegend::New();
  this->Legend->SetChart(this);
  this->ChartPrivate = new vtkChartXYPrivate;
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
  this->Grid->SetXAxis(this->ChartPrivate->axes[1]);
  this->Grid->SetYAxis(this->ChartPrivate->axes[0]);

  this->PlotTransform = vtkTransform2D::New();
  this->PlotTransformValid = false;

  this->BoxOrigin[0] = this->BoxOrigin[1] = 0.0f;
  this->BoxGeometry[0] = this->BoxGeometry[1] = 0.0f;
  this->DrawBox = false;
  this->DrawNearestPoint = false;
  this->DrawAxesAtOrigin = false;
  this->BarWidthFraction = 0.8;

  this->Tooltip = vtkTooltipItem::New();
  this->Tooltip->SetVisible(false);
}

//-----------------------------------------------------------------------------
vtkChartXY::~vtkChartXY()
{
  for (unsigned int i = 0; i < this->ChartPrivate->plots.size(); ++i)
    {
    this->ChartPrivate->plots[i]->Delete();
    }
  for (int i = 0; i < 4; ++i)
    {
    this->ChartPrivate->axes[i]->Delete();
    }
  delete this->ChartPrivate;
  this->ChartPrivate = 0;

  this->Grid->Delete();
  this->Grid = 0;
  this->Legend->Delete();
  this->Legend = 0;

  if (this->PlotTransform)
    {
    this->PlotTransform->Delete();
    this->PlotTransform = NULL;
    }
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
}

//-----------------------------------------------------------------------------
bool vtkChartXY::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called.");

  int geometry[] = { this->GetScene()->GetViewWidth(),
                     this->GetScene()->GetViewHeight() };
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
      this->MTime > this->ChartPrivate->axes[0]->GetMTime())
    {
    // Take up the entire window right now, this could be made configurable
    this->SetGeometry(geometry);
    this->SetBorders(60, 20, 20, 50);
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
    }

  // Recalculate the plot transform, min and max values if necessary
  if (!this->PlotTransformValid)
    {
    this->RecalculatePlotBounds();
    this->RecalculatePlotTransform();
    }
  else if (recalculateTransform)
    {
    this->RecalculatePlotTransform();
    }

  // Update the axes in the chart
  for (int i = 0; i < 4; ++i)
    {
    this->ChartPrivate->axes[i]->Update();
    }

  // Draw the grid - the axes take care of its color and visibility
  this->Grid->Paint(painter);

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
  int clip[] = { this->Point1[0], this->Point1[1],
                 this->Point2[0]-this->Point1[0],
                 this->Point2[1]-this->Point1[1] };
  painter->GetDevice()->SetClipping(&clip[0]);

  // Push the matrix and use the transform we just calculated
  painter->PushMatrix();
  painter->SetTransform(this->PlotTransform);

  // Now iterate through the plots
  size_t n = this->ChartPrivate->plots.size();
  for (size_t i = 0; i < n; ++i)
    {
    this->ChartPrivate->plots[i]->SetSelection(idArray);
    this->ChartPrivate->plots[i]->Paint(painter);
    }

  // Stop clipping of the plot area and reset back to screen coordinates
  painter->GetDevice()->DisableClipping();
  painter->PopMatrix();
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
    for (size_t i = 0; i < bars.size(); ++i)
      {
      bars[i]->SetWidth(barWidth);
      bars[i]->SetOffset(float(bars.size()-i-1)*barWidth);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkChartXY::RecalculatePlotTransform()
{
  // Get the scale for the plot area from the x and y axes

  // First the bottom axis (x)
  vtkAxis* axis = this->ChartPrivate->axes[vtkAxis::BOTTOM];
  float *min = axis->GetPoint1();
  float *max = axis->GetPoint2();
  if (fabs(max[0] - min[0]) == 0.0f)
    {
    return;
    }
  float xScale = (axis->GetMaximum() - axis->GetMinimum()) / (max[0] - min[0]);

  // Now the left axis (y)
  axis = this->ChartPrivate->axes[vtkAxis::LEFT];
  min = axis->GetPoint1();
  max = axis->GetPoint2();
  if (fabs(max[1] - min[1]) == 0.0f)
    {
    return;
    }
  float yScale = (axis->GetMaximum() - axis->GetMinimum()) / (max[1] - min[1]);

  this->PlotTransform->Identity();
  this->PlotTransform->Translate(this->Point1[0], this->Point1[1]);
  // Get the scale for the plot area from the x and y axes
  this->PlotTransform->Scale(1.0 / xScale, 1.0 / yScale);
  this->PlotTransform->Translate(
      -this->ChartPrivate->axes[vtkAxis::BOTTOM]->GetMinimum(),
      -this->ChartPrivate->axes[vtkAxis::LEFT]->GetMinimum());

  // Move the axes if necessary and if the draw axes at origin ivar is true.
  if (this->DrawAxesAtOrigin)
    {
    // Get the screen coordinates for the origin, and move the axes there.
    float origin[2] = { 0.0, 0.0 };
    this->PlotTransform->TransformPoints(origin, origin, 1);
    // Need to clamp the axes in the plot area.
    if (int(origin[0]) < this->Point1[0]) origin[0] = this->Point1[0];
    if (int(origin[0]) > this->Point2[0]) origin[0] = this->Point2[0];
    if (int(origin[1]) < this->Point1[1]) origin[1] = this->Point1[1];
    if (int(origin[1]) > this->Point2[1]) origin[1] = this->Point2[1];

    this->ChartPrivate->axes[vtkAxis::BOTTOM]->SetPoint1(this->Point1[0], origin[1]);
    this->ChartPrivate->axes[vtkAxis::BOTTOM]->SetPoint2(this->Point2[0], origin[1]);
    this->ChartPrivate->axes[vtkAxis::LEFT]->SetPoint1(origin[0], this->Point1[1]);
    this->ChartPrivate->axes[vtkAxis::LEFT]->SetPoint2(origin[0], this->Point2[1]);
    }

  this->PlotTransformValid = true;
}

//-----------------------------------------------------------------------------
void vtkChartXY::RecalculatePlotBounds()
{
  // Get the bounds of each plot.
  float xmin = 0.0;
  float xmax = 1.0;
  float ymin = 0.0;
  float ymax = 1.0;

  size_t n = this->ChartPrivate->plots.size();
  double bounds[4] = { 0.0, 0.0, 0.0, 0.0 };
  for (size_t i = 0; i < n; ++i)
    {
    if (this->ChartPrivate->plots[i]->GetVisible() == false)
      {
      continue;
      }
    this->ChartPrivate->plots[i]->GetBounds(bounds);
    if (i == 0)
      {
      // Initialize the bounds for the chart
      xmin = float(bounds[0]);
      xmax = float(bounds[1]);
      ymin = float(bounds[2]);
      ymax = float(bounds[3]);
      }
    else
      {
      if (xmin > bounds[0]) xmin = float(bounds[0]);
      if (xmax < bounds[1]) xmax = float(bounds[1]);
      if (ymin > bounds[2]) ymin = float(bounds[2]);
      if (ymax < bounds[3]) ymax = float(bounds[3]);
      }
    }

  // Now set the newly calculated bounds on the axes
  vtkAxis* xAxis = this->ChartPrivate->axes[vtkAxis::BOTTOM];
  vtkAxis* yAxis = this->ChartPrivate->axes[vtkAxis::LEFT];
  if (xAxis->GetBehavior() == 0)
    {
    xAxis->SetMinimum(xmin);
    xAxis->SetMaximum(xmax);
    xAxis->AutoScale();
    }
  if (yAxis->GetBehavior() == 0)
    {
    yAxis->SetMinimum(ymin);
    yAxis->SetMaximum(ymax);
    yAxis->AutoScale();
    }
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
      this->ChartPrivate->plots.push_back(line);
      line->GetPen()->SetColor(color.GetData());
      line->SetXAxis(this->ChartPrivate->axes[vtkAxis::BOTTOM]);
      line->SetYAxis(this->ChartPrivate->axes[vtkAxis::LEFT]);
      plot = line;
      break;
      }
    case POINTS:
      {
      vtkPlotPoints *points = vtkPlotPoints::New();
      this->ChartPrivate->plots.push_back(points);
      points->GetPen()->SetColor(color.GetData());
      plot = points;
      break;
      }
    case BAR:
      {
      vtkPlotBar *bar = vtkPlotBar::New();
      this->ChartPrivate->plots.push_back(bar);
      bar->GetBrush()->SetColor(color.GetData());
      plot = bar;
      break;
      }
    default:
      plot = NULL;
    }
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
  if (mouse.Button == 0)
    {
    // Figure out how much the mouse has moved by in plot coordinates - pan
    double pos[] = { mouse.ScreenPos[0], mouse.ScreenPos[1] };
    double last[] = { mouse.LastScreenPos[0], mouse.LastScreenPos[1] };

    // Go from screen to scene coordinates to work out the delta
    this->PlotTransform->InverseTransformPoints(pos, pos, 1);
    this->PlotTransform->InverseTransformPoints(last, last, 1);
    double delta[] = { last[0] - pos[0], last[1] - pos[1] };

    // Now move the axes and recalculate the transform
    vtkAxis* xAxis = this->ChartPrivate->axes[vtkAxis::BOTTOM];
    vtkAxis* yAxis = this->ChartPrivate->axes[vtkAxis::LEFT];
    xAxis->SetMinimum(xAxis->GetMinimum() + delta[0]);
    xAxis->SetMaximum(xAxis->GetMaximum() + delta[0]);
    yAxis->SetMinimum(yAxis->GetMinimum() + delta[1]);
    yAxis->SetMaximum(yAxis->GetMaximum() + delta[1]);

    this->RecalculatePlotTransform();
    // Mark the scene as dirty
    this->Scene->SetDirty(true);
    }
  else if (mouse.Button == 2)
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
  if (mouse.ScreenPos[0] > this->Point1[0] && mouse.ScreenPos[0] < this->Point2[0] &&
      mouse.ScreenPos[1] > this->Point1[1] && mouse.ScreenPos[1] < this->Point2[1] &&
      this->PlotTransform && n)
    {
    vtkVector2f plotPos, position;
    this->PlotTransform->InverseTransformPoints(mouse.Pos, position.GetData(),
                                                1);
    // Use a tolerance of +/- 5 pixels
    vtkVector2f tolerance(5*(1.0/this->PlotTransform->GetMatrix()->GetElement(0, 0)),
                          5*(1.0/this->PlotTransform->GetMatrix()->GetElement(1, 1)));
    // Iterate through the visible plots and return on the first hit
    for (int i = static_cast<int>(--n); i >= 0; --i)
      {
      vtkPlot* plot = this->ChartPrivate->plots[i];
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
  if (mouse.Button == 0)
    {
    // The mouse panning action.
    return true;
    }
  else if (mouse.Button == 2)
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
  if (mouse.Button == 2)
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
    this->PlotTransform->InverseTransformPoints(this->BoxOrigin,
                                                this->BoxOrigin, 1);
    float point2[] = { mouse.Pos[0], mouse.Pos[1] };
    this->PlotTransform->InverseTransformPoints(point2, point2, 1);

    // Ensure we preserve the directionality of the axes
    vtkAxis* xAxis = this->ChartPrivate->axes[vtkAxis::BOTTOM];
    vtkAxis* yAxis = this->ChartPrivate->axes[vtkAxis::LEFT];
    if (xAxis->GetMaximum() > xAxis->GetMinimum())
      {
      xAxis->SetMaximum(this->BoxOrigin[0] > point2[0] ?
                              this->BoxOrigin[0] : point2[0]);
      xAxis->SetMinimum(this->BoxOrigin[0] < point2[0] ?
                              this->BoxOrigin[0] : point2[0]);
      }
    else
      {
      xAxis->SetMaximum(this->BoxOrigin[0] < point2[0] ?
                              this->BoxOrigin[0] : point2[0]);
      xAxis->SetMinimum(this->BoxOrigin[0] > point2[0] ?
                              this->BoxOrigin[0] : point2[0]);
      }
    if (yAxis->GetMaximum() > yAxis->GetMinimum())
      {
      yAxis->SetMaximum(this->BoxOrigin[1] > point2[1] ?
                              this->BoxOrigin[1] : point2[1]);
      yAxis->SetMinimum(this->BoxOrigin[1] < point2[1] ?
                              this->BoxOrigin[1] : point2[1]);
      }
    else
      {
      yAxis->SetMaximum(this->BoxOrigin[1] < point2[1] ?
                              this->BoxOrigin[1] : point2[1]);
      yAxis->SetMinimum(this->BoxOrigin[1] > point2[1] ?
                              this->BoxOrigin[1] : point2[1]);
      }

    this->RecalculatePlotTransform();
    xAxis->RecalculateTickSpacing();
    yAxis->RecalculateTickSpacing();
    this->BoxGeometry[0] = this->BoxGeometry[1] = 0.0f;
    this->DrawBox = false;
    // Mark the scene as dirty
    this->Scene->SetDirty(true);
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkChartXY::MouseWheelEvent(const vtkContextMouseEvent &, int delta)
{
  this->Tooltip->SetVisible(false);
  // Get the bounds of each plot.
  vtkAxis* xAxis = this->ChartPrivate->axes[vtkAxis::BOTTOM];
  vtkAxis* yAxis = this->ChartPrivate->axes[vtkAxis::LEFT];
  float xmin = xAxis->GetMinimum();
  float xmax = xAxis->GetMaximum();
  float deltax = xmax - xmin;
  float ymin = yAxis->GetMinimum();
  float ymax = yAxis->GetMaximum();
  float deltay = ymax - ymin;

  if (delta > 0)
    {
    xmin += 0.1 * deltax;
    xmax -= 0.1 * deltax;
    ymin += 0.1 * deltay;
    ymax -= 0.1 * deltay;
    }
  else
    {
    xmin -= 0.1 * deltax;
    xmax += 0.1 * deltax;
    ymin -= 0.1 * deltay;
    ymax += 0.1 * deltay;
    }
  // Now set the newly calculated bounds on the axes
  xAxis->SetMinimum(xmin);
  xAxis->SetMaximum(xmax);
  yAxis->SetMinimum(ymin);
  yAxis->SetMaximum(ymax);

  this->RecalculatePlotTransform();
  xAxis->RecalculateTickSpacing();
  yAxis->RecalculateTickSpacing();

  // Mark the scene as dirty
  this->Scene->SetDirty(true);

  return true;
}

//-----------------------------------------------------------------------------
void vtkChartXY::ProcessSelectionEvent(vtkObject* caller, void* callData)
{
  cout << "ProcessSelectionEvent called in XY! " << caller << "\t" << callData << endl;
  unsigned int *rect = reinterpret_cast<unsigned int *>(callData);

  // The origin of the plot area
  float xOrigin = this->Point1[0];
  float yOrigin = this->Point1[1];

  vtkAxis* xAxis = this->ChartPrivate->axes[vtkAxis::BOTTOM];
  vtkAxis* yAxis = this->ChartPrivate->axes[vtkAxis::LEFT];

  // Get the scale for the plot area from the x and y axes
  float *min = xAxis->GetPoint1();
  float *max = xAxis->GetPoint2();
  double xScale = (xAxis->GetMaximum() - xAxis->GetMinimum()) /
                 (max[0] - min[0]);
  min = yAxis->GetPoint1();
  max = yAxis->GetPoint2();
  double yScale = (yAxis->GetMaximum() - yAxis->GetMinimum()) /
                 (max[1] - min[1]);

  double matrix[3][3];
  matrix[0][0] = xScale;
  matrix[0][1] = 0;
  matrix[0][2] = -1.0 * xOrigin*xScale;

  matrix[1][0] = yScale;
  matrix[1][1] = 0;
  matrix[1][2] = -1.0 * yOrigin*yScale;

  matrix[2][0] = 0;
  matrix[2][1] = 0;
  matrix[2][2] = 1;

  double tRect[4];

  tRect[0] = matrix[0][0]*rect[0] + matrix[0][2];
  tRect[1] = matrix[1][0]*rect[1] + matrix[1][2];

  tRect[2] = matrix[0][0]*rect[2] + matrix[0][2];
  tRect[3] = matrix[1][0]*rect[3] + matrix[1][2];

  // As an example - handle zooming using the rubber band...
  if (tRect[0] > tRect[2])
    {
    double tmp = tRect[0];
    tRect[0] = tRect[2];
    tRect[2] = tmp;
    }
  if (tRect[1] > tRect[3])
    {
    double tmp = tRect[1];
    tRect[1] = tRect[3];
    tRect[3] = tmp;
    }
  // Now set the values of the axes
  xAxis->SetMinimum(tRect[0]);
  xAxis->SetMaximum(tRect[2]);
  yAxis->SetMinimum(tRect[1]);
  yAxis->SetMaximum(tRect[3]);
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
