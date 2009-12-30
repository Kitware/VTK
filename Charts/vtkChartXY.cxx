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
#include "vtkContextDevice2D.h"
#include "vtkTransform2D.h"
#include "vtkContextScene.h"
#include "vtkPoints2D.h"

#include "vtkPlot.h"
#include "vtkPlotLine.h"
#include "vtkPlotPoints.h"

#include "vtkAxis.h"
#include "vtkPlotGrid.h"

#include "vtkTable.h"
#include "vtkAbstractArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkIdTypeArray.h"

#include "vtkAnnotationLink.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"

#include "vtkObjectFactory.h"

#include "vtkStdString.h"
#include "vtkTextProperty.h"

// My STL containers
#include <vtkstd/vector>

//-----------------------------------------------------------------------------
class vtkChartXYPrivate
{
  public:
    vtkstd::vector<vtkPlot *> plots; // Charts can contain multiple plots of data
};

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkChartXY, "1.19");

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkChartXY);

//-----------------------------------------------------------------------------
vtkChartXY::vtkChartXY()
{
  this->XAxis = vtkAxis::New();
  this->YAxis = vtkAxis::New();
  this->Grid = vtkPlotGrid::New();
  this->Grid->SetXAxis(this->XAxis);
  this->Grid->SetYAxis(this->YAxis);
  this->ChartPrivate = new vtkChartXYPrivate;

  // Set up the x and y axes - should be congigured based on data
  this->XAxis->SetMinimum(0.0);
  this->XAxis->SetMaximum(40.0);
  this->XAxis->SetNumberOfTicks(8);
  this->XAxis->SetTitle("X Axis");
  this->YAxis->SetMinimum(0.0);
  this->YAxis->SetMaximum(275.0);
  this->YAxis->SetNumberOfTicks(5);
  this->YAxis->SetTitle("Y Axis");

  this->PlotTransform = vtkTransform2D::New();
  this->PlotTransformValid = false;

  this->BoxOrigin[0] = this->BoxOrigin[1] = 0.0f;
  this->BoxGeometry[0] = this->BoxGeometry[1] = 0.0f;
  this->DrawBox = false;
  this->DrawNearestPoint = false;
}

//-----------------------------------------------------------------------------
vtkChartXY::~vtkChartXY()
{
  for (unsigned int i = 0; i < this->ChartPrivate->plots.size(); ++i)
    {
    this->ChartPrivate->plots[i]->Delete();
    }
  delete this->ChartPrivate;
  this->ChartPrivate = 0;

  this->XAxis->Delete();
  this->XAxis = 0;
  this->YAxis->Delete();
  this->YAxis = 0;
  this->Grid->Delete();
  this->Grid = 0;

  if (this->PlotTransform)
    {
    this->PlotTransform->Delete();
    this->PlotTransform = NULL;
    }
}

//-----------------------------------------------------------------------------
bool vtkChartXY::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called.");

  int geometry[] = { this->GetScene()->GetViewWidth(),
                     this->GetScene()->GetViewHeight() };
  if (geometry[0] != this->Geometry[0] || geometry[0] != this->Geometry[0])
    {
    this->SetGeometry(geometry);
    this->SetBorders(60, 20, 20, 50);
    }

  // Check whether the geometry has been modified after the axes - update if so
  if (this->MTime > this->XAxis->GetMTime())
    {
    // This is where we set the axes up too
    this->XAxis->SetPoint1(this->Point1[0], this->Point1[1]);
    this->XAxis->SetPoint2(this->Point2[0], this->Point1[1]);
    this->YAxis->SetPoint1(this->Point1[0], this->Point1[1]);
    this->YAxis->SetPoint2(this->Point1[0], this->Point2[1]);
    this->RecalculatePlotTransform();
    }

  // Recalculate the plot transform, min and max values if necessary
  if (!this->PlotTransformValid)
    {
    this->RecalculatePlotBounds();
    this->RecalculatePlotTransform();
    }

  // Draw a hard wired grid right now - this should be configurable
  painter->GetPen()->SetColorF(0.95, 0.95, 0.95);
  painter->GetPen()->SetWidth(1.0);
  this->Grid->Paint(painter);

  // Plot the series of the chart
  this->RenderPlots(painter);

  // Set the color and width, draw the axes, color and width push to axis props
  painter->GetPen()->SetColorF(0.0, 0.0, 0.0, 1.0);
  painter->GetPen()->SetWidth(1.0);
  this->XAxis->Paint(painter);
  this->YAxis->Paint(painter);

  // Draw the selection box if necessary
  if (this->DrawBox)
    {
    painter->GetBrush()->SetColor(255, 255, 255, 0);
    painter->GetPen()->SetColor(0, 0, 0, 255);
    painter->GetPen()->SetWidth(1.0);
    painter->DrawRect(this->BoxOrigin[0], this->BoxOrigin[1],
                      this->BoxGeometry[0], this->BoxGeometry[1]);
    }

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
  painter->GetDevice()->PushMatrix();
  painter->GetDevice()->SetMatrix(this->PlotTransform->GetMatrix());

  // Now iterate through the plots
  size_t n = this->ChartPrivate->plots.size();
  for (size_t i = 0; i < n; ++i)
    {
    this->ChartPrivate->plots[i]->SetSelection(idArray);
    this->ChartPrivate->plots[i]->Paint(painter);
    }

  // Stop clipping of the plot area and reset back to screen coordinates
  painter->GetDevice()->DisableClipping();
  painter->GetDevice()->PopMatrix();
}

//-----------------------------------------------------------------------------
void vtkChartXY::RecalculatePlotTransform()
{
  // Get the scale for the plot area from the x and y axes
  float *min = this->XAxis->GetPoint1();
  float *max = this->XAxis->GetPoint2();
  float xScale = (this->XAxis->GetMaximum() - this->XAxis->GetMinimum()) /
                 (max[0] - min[0]);
  min = this->YAxis->GetPoint1();
  max = this->YAxis->GetPoint2();
  float yScale = (this->YAxis->GetMaximum() - this->YAxis->GetMinimum()) /
                 (max[1] - min[1]);

  this->PlotTransform->Identity();
  this->PlotTransform->Translate(this->Point1[0], this->Point1[1]);
  // Get the scale for the plot area from the x and y axes
  min = this->YAxis->GetPoint1();
  max = this->YAxis->GetPoint2();
  this->PlotTransform->Scale(1.0 / xScale, 1.0 / yScale);
  this->PlotTransform->Translate(-this->XAxis->GetMinimum(),
                                 -this->YAxis->GetMinimum());

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
  this->XAxis->SetMinimum(xmin);
  this->XAxis->SetMaximum(xmax);
  this->YAxis->SetMinimum(ymin);
  this->YAxis->SetMaximum(ymax);
  this->XAxis->AutoScale();
  this->YAxis->AutoScale();
}

//-----------------------------------------------------------------------------
vtkPlot * vtkChartXY::AddPlot(vtkChart::Type type)
{
  // Use a variable to return the object created (or NULL), this is necessary
  // as the HP compiler is broken (thinks this function does not return) and
  // the MS compiler generates a warning about unreachable code if a redundant
  // return is added at the end.
  vtkPlot *plot = NULL;
  switch (type)
    {
    case LINE:
      {
      vtkPlotLine *line = vtkPlotLine::New();
      this->ChartPrivate->plots.push_back(line);
      plot = line;
      break;
      }
    case POINTS:
      {
      vtkPlotPoints *points = vtkPlotPoints::New();
      this->ChartPrivate->plots.push_back(points);
      plot = points;
      break;
      }
    default:
      plot = NULL;
    }
    // Ensure that the bounds are recalculated
    this->PlotTransformValid = false;
    return plot;
}

//-----------------------------------------------------------------------------
vtkIdType vtkChartXY::GetNumberPlots()
{
  return this->ChartPrivate->plots.size();
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
    this->XAxis->SetMinimum(this->XAxis->GetMinimum() + delta[0]);
    this->XAxis->SetMaximum(this->XAxis->GetMaximum() + delta[0]);
    this->YAxis->SetMinimum(this->YAxis->GetMinimum() + delta[1]);
    this->YAxis->SetMaximum(this->YAxis->GetMaximum() + delta[1]);

    this->RecalculatePlotTransform();
    }
  else if (mouse.Button == 2)
    {
    this->BoxGeometry[0] = mouse.Pos[0] - this->BoxOrigin[0];
    this->BoxGeometry[1] = mouse.Pos[1] - this->BoxOrigin[1];
    }

  return true;
}

//-----------------------------------------------------------------------------
bool vtkChartXY::MouseLeaveEvent(const vtkContextMouseEvent &)
{
  this->DrawNearestPoint = false;
  return true;
}

//-----------------------------------------------------------------------------
bool vtkChartXY::MouseButtonPressEvent(const vtkContextMouseEvent &mouse)
{
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
    // Zoom into the chart by the specified amount, and recalculate the bounds
    this->PlotTransform->InverseTransformPoints(this->BoxOrigin,
                                                this->BoxOrigin, 1);
    float point2[] = { mouse.Pos[0], mouse.Pos[1] };
    this->PlotTransform->InverseTransformPoints(point2, point2, 1);

    // Ensure we preserve the directionality of the axes
    if (this->XAxis->GetMaximum() > this->XAxis->GetMinimum())
      {
      this->XAxis->SetMaximum(this->BoxOrigin[0] > point2[0] ?
                              this->BoxOrigin[0] : point2[0]);
      this->XAxis->SetMinimum(this->BoxOrigin[0] < point2[0] ?
                              this->BoxOrigin[0] : point2[0]);
      }
    else
      {
      this->XAxis->SetMaximum(this->BoxOrigin[0] < point2[0] ?
                              this->BoxOrigin[0] : point2[0]);
      this->XAxis->SetMinimum(this->BoxOrigin[0] > point2[0] ?
                              this->BoxOrigin[0] : point2[0]);
      }
    if (this->YAxis->GetMaximum() > this->YAxis->GetMinimum())
      {
      this->YAxis->SetMaximum(this->BoxOrigin[1] > point2[1] ?
                              this->BoxOrigin[1] : point2[1]);
      this->YAxis->SetMinimum(this->BoxOrigin[1] < point2[1] ?
                              this->BoxOrigin[1] : point2[1]);
      }
    else
      {
      this->YAxis->SetMaximum(this->BoxOrigin[1] < point2[1] ?
                              this->BoxOrigin[1] : point2[1]);
      this->YAxis->SetMinimum(this->BoxOrigin[1] > point2[1] ?
                              this->BoxOrigin[1] : point2[1]);
      }

    this->RecalculatePlotTransform();
    this->BoxGeometry[0] = this->BoxGeometry[1] = 0.0f;
    this->DrawBox = false;
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkChartXY::MouseWheelEvent(const vtkContextMouseEvent &, int delta)
{
  // Get the bounds of each plot.
  float xmin = this->XAxis->GetMinimum();
  float xmax = this->XAxis->GetMaximum();
  float deltax = xmax - xmin;
  float ymin = this->YAxis->GetMinimum();
  float ymax = this->YAxis->GetMaximum();
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
  this->XAxis->SetMinimum(xmin);
  this->XAxis->SetMaximum(xmax);
  this->YAxis->SetMinimum(ymin);
  this->YAxis->SetMaximum(ymax);

  this->RecalculatePlotTransform();

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

  // Get the scale for the plot area from the x and y axes
  float *min = this->XAxis->GetPoint1();
  float *max = this->XAxis->GetPoint2();
  double xScale = (this->XAxis->GetMaximum() - this->XAxis->GetMinimum()) /
                 (max[0] - min[0]);
  min = this->YAxis->GetPoint1();
  max = this->YAxis->GetPoint2();
  double yScale = (this->YAxis->GetMaximum() - this->YAxis->GetMinimum()) /
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
  this->XAxis->SetMinimum(tRect[0]);
  this->XAxis->SetMaximum(tRect[2]);
  this->YAxis->SetMinimum(tRect[1]);
  this->YAxis->SetMaximum(tRect[3]);
}


//-----------------------------------------------------------------------------
void vtkChartXY::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "X Axis: ";
  if (this->XAxis)
    {
    os << endl;
    this->XAxis->PrintSelf(os, indent.GetNextIndent());
    }
    else
    {
    os << "(none)" << endl;
    }
  os << indent << "Y Axis: ";
  if (this->YAxis)
    {
    os << endl;
    this->YAxis->PrintSelf(os, indent.GetNextIndent());
    }
    else
    {
    os << "(none)" << endl;
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
