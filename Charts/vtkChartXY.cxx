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
vtkCxxRevisionMacro(vtkChartXY, "1.8");

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
  this->XAxis->SetMaximum(7.0);
  this->XAxis->SetNumberOfTicks(8);
  this->XAxis->SetLabel("X Axis");
  this->YAxis->SetMinimum(1.0);
  this->YAxis->SetMaximum(-1.0);
  this->YAxis->SetNumberOfTicks(5);
  this->YAxis->SetLabel("Y Axis");
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
}

//-----------------------------------------------------------------------------
bool vtkChartXY::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called.");

  // Check whether the geometry has been modified after the axes - update if so
  if (this->MTime > this->XAxis->GetMTime())
    {
    // This is where we set the axes up too
    this->XAxis->SetPoint1(this->Geometry[2], this->Geometry[3]);
    this->XAxis->SetPoint2(this->Geometry[0]-this->Geometry[4],
                           this->Geometry[3]);
    this->YAxis->SetPoint1(this->Geometry[2], this->Geometry[3]);
    this->YAxis->SetPoint2(this->Geometry[2],
                           this->Geometry[1]-this->Geometry[5]);
    }

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

  // This method could be optimized if only certain regions needed painting.

  // Draw a hard wired grid right now - this should be configurable
  painter->GetPen()->SetColorF(0.8, 0.8, 0.8);
  painter->GetPen()->SetWidth(1.0);
  this->Grid->Paint(painter);

  // The origin of the plot area
  float xOrigin = this->Geometry[2];
  float yOrigin = this->Geometry[3];

  // Get the scale for the plot area from the x and y axes
  float *min = this->XAxis->GetPoint1();
  float *max = this->XAxis->GetPoint2();
  float xScale = (this->XAxis->GetMaximum() - this->XAxis->GetMinimum()) /
                 (max[0] - min[0]);
  min = this->YAxis->GetPoint1();
  max = this->YAxis->GetPoint2();
  float yScale = (this->YAxis->GetMaximum() - this->YAxis->GetMinimum()) /
                 (max[1] - min[1]);

  // Clip drawing while plotting
  int clip[4];
  clip[0] = static_cast<int>(xOrigin);
  clip[1] = static_cast<int>(yOrigin);
  clip[2] = this->Geometry[0] - this->Geometry[4] - this->Geometry[2];
  clip[3] = this->Geometry[1] - this->Geometry[5] - this->Geometry[3];
  painter->GetDevice()->SetClipping(&clip[0]);

  vtkTransform2D *transform = vtkTransform2D::New();
  transform->Translate(this->Geometry[2], this->Geometry[3]);
  // Get the scale for the plot area from the x and y axes
  min = this->YAxis->GetPoint1();
  max = this->YAxis->GetPoint2();
  transform->Scale(1.0 / xScale, 1.0 / yScale);
  transform->Translate(-this->XAxis->GetMinimum(), -this->YAxis->GetMinimum());

  // Pop the matrix and use the transform we just calculated
  painter->GetDevice()->PushMatrix();
  //painter->SetTransform(transform);
  painter->GetDevice()->SetMatrix(transform->GetMatrix());
  transform->GetMatrix()->Print(cout);

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

  // Set the color and width, draw the axes, color and width push to axis props
  painter->GetPen()->SetColorF(0.0, 0.0, 0.0, 1.0);
  painter->GetPen()->SetWidth(1.0);
  this->XAxis->Paint(painter);
  this->YAxis->Paint(painter);

  return true;
}

//-----------------------------------------------------------------------------
void vtkChartXY::RenderPlots(vtkContext2D *painter)
{
  // This function ensures that the correct view transforms are in place for
  // the plot functions before calling paint on each one.
  float x[] = { -0.1, -0.1, 10.0, 2.1 };
  painter->GetDevice()->SetViewExtents(&x[0]);
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

      // Set up the scaling for the plot area
      line->Translate(0.0, 0.0);
      line->GetTransform()->Translate(this->Geometry[2],
                                      this->Geometry[3]);
      // Get the scale for the plot area from the x and y axes
      float *min = this->XAxis->GetPoint1();
      float *max = this->XAxis->GetPoint2();
      float xScale = (this->XAxis->GetMaximum() - this->XAxis->GetMinimum()) /
                     (max[0] - min[0]);
      min = this->YAxis->GetPoint1();
      max = this->YAxis->GetPoint2();
      float yScale = (this->YAxis->GetMaximum() - this->YAxis->GetMinimum()) /
                     (max[1] - min[1]);
      line->GetTransform()->Scale(1.0 / xScale, 1.0 / yScale);
      line->GetTransform()->Translate(- this->XAxis->GetMinimum(),
                                      - this->YAxis->GetMinimum());

      plot = line;
      break;
      }
    case POINTS:
      {
      vtkPlotPoints *points = vtkPlotPoints::New();
      this->ChartPrivate->plots.push_back(points);

      // Set up the scaling for the plot area
      points->Translate(0.0, 0.0);
      points->GetTransform()->Translate(this->Geometry[2],
                                        this->Geometry[3]);
      // Get the scale for the plot area from the x and y axes
      float *min = this->XAxis->GetPoint1();
      float *max = this->XAxis->GetPoint2();
      float xScale = (this->XAxis->GetMaximum() - this->XAxis->GetMinimum()) /
                     (max[0] - min[0]);
      min = this->YAxis->GetPoint1();
      max = this->YAxis->GetPoint2();
      float yScale = (this->YAxis->GetMaximum() - this->YAxis->GetMinimum()) /
                     (max[1] - min[1]);
      points->GetTransform()->Scale(1.0 / xScale, 1.0 / yScale);
      points->GetTransform()->Translate(- this->XAxis->GetMinimum(),
                                        - this->YAxis->GetMinimum());

      plot = points;
      break;
      }
    default:
      plot = NULL;
    }
    return plot;
}

//-----------------------------------------------------------------------------
vtkIdType vtkChartXY::GetNumberPlots()
{
  return this->ChartPrivate->plots.size();
}

//-----------------------------------------------------------------------------
void vtkChartXY::ProcessSelectionEvent(vtkObject* caller, void* callData)
{
  cout << "ProcessSelectionEvent called in XY! " << caller << "\t" << callData << endl;
  unsigned int *rect = reinterpret_cast<unsigned int *>(callData);

  // The origin of the plot area
  float xOrigin = this->Geometry[2];
  float yOrigin = this->Geometry[3];

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
