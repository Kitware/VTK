/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChart.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkChart.h"
#include "vtkAxis.h"
#include "vtkBrush.h"
#include "vtkTransform2D.h"
#include "vtkContextMouseEvent.h"

#include "vtkAnnotationLink.h"
#include "vtkContextScene.h"
#include "vtkTextProperty.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkChart::MouseActions::MouseActions()
{
  this->Data[0] = vtkContextMouseEvent::LEFT_BUTTON;
  this->Data[1] = vtkContextMouseEvent::MIDDLE_BUTTON;
  this->Data[2] = vtkContextMouseEvent::RIGHT_BUTTON;
  this->Data[3] = -1;
}

//-----------------------------------------------------------------------------
vtkChart::MouseClickActions::MouseClickActions()
{
  this->Data[0] = vtkContextMouseEvent::LEFT_BUTTON;
  this->Data[1] = vtkContextMouseEvent::RIGHT_BUTTON;
}

//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkChart, AnnotationLink, vtkAnnotationLink);

//-----------------------------------------------------------------------------
vtkChart::vtkChart()
{
  this->Geometry[0] = 0;
  this->Geometry[1] = 0;
  this->Point1[0] = 0;
  this->Point1[1] = 0;
  this->Point2[0] = 0;
  this->Point2[1] = 0;
  this->Size.Set(0, 0, 0, 0);
  this->ShowLegend = false;
  this->TitleProperties = vtkTextProperty::New();
  this->TitleProperties->SetJustificationToCentered();
  this->TitleProperties->SetColor(0.0, 0.0, 0.0);
  this->TitleProperties->SetFontSize(12);
  this->TitleProperties->SetFontFamilyToArial();
  this->AnnotationLink = NULL;
  this->LayoutStrategy = vtkChart::FILL_SCENE;
  this->RenderEmpty = false;
  this->BackgroundBrush = vtkSmartPointer<vtkBrush>::New();
  this->BackgroundBrush->SetColorF(1, 1, 1, 0);
  this->SelectionMode = vtkContextScene::SELECTION_NONE;
}

//-----------------------------------------------------------------------------
vtkChart::~vtkChart()
{
  for(int i=0; i < 4; i++)
    {
    if(this->GetAxis(i))
      {
      this->GetAxis(i)->RemoveObservers(vtkChart::UpdateRange);
      }
    }
  this->TitleProperties->Delete();
  if (this->AnnotationLink)
    {
    this->AnnotationLink->Delete();
    }
}

//-----------------------------------------------------------------------------
vtkPlot * vtkChart::AddPlot(int)
{
  return NULL;
}

//-----------------------------------------------------------------------------
vtkIdType vtkChart::AddPlot(vtkPlot*)
{
  return -1;
}

//-----------------------------------------------------------------------------
bool vtkChart::RemovePlot(vtkIdType)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkChart::RemovePlotInstance(vtkPlot* plot)
{
  if (plot)
    {
    vtkIdType numberOfPlots = this->GetNumberOfPlots();
    for (vtkIdType i = 0; i < numberOfPlots; ++i)
      {
      if (this->GetPlot(i) == plot)
        {
        return this->RemovePlot(i);
        }
      }
    }
  return false;
}

//-----------------------------------------------------------------------------
void vtkChart::ClearPlots()
{
}

//-----------------------------------------------------------------------------
vtkPlot* vtkChart::GetPlot(vtkIdType)
{
  return NULL;
}

//-----------------------------------------------------------------------------
vtkIdType vtkChart::GetNumberOfPlots()
{
  return 0;
}

//-----------------------------------------------------------------------------
vtkAxis* vtkChart::GetAxis(int)
{
  return NULL;
}

//-----------------------------------------------------------------------------
vtkIdType vtkChart::GetNumberOfAxes()
{
  return 0;
}

//-----------------------------------------------------------------------------
void vtkChart::RecalculateBounds()
{
  return;
}
//-----------------------------------------------------------------------------
void vtkChart::SetShowLegend(bool visible)
{
  if (this->ShowLegend != visible)
    {
    this->ShowLegend = visible;
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
bool vtkChart::GetShowLegend()
{
  return this->ShowLegend;
}

vtkChartLegend * vtkChart::GetLegend()
{
  return 0;
}

//-----------------------------------------------------------------------------
void vtkChart::SetTitle(const vtkStdString &title)
{
  if (this->Title != title)
    {
    this->Title = title;
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
vtkStdString vtkChart::GetTitle()
{
  return this->Title;
}

//-----------------------------------------------------------------------------
bool vtkChart::CalculatePlotTransform(vtkAxis *x, vtkAxis *y,
                                      vtkTransform2D *transform)
{
  if (!x || !y || !transform)
    {
    vtkWarningMacro("Called with null arguments.");
    return false;
    }
  // Get the scale for the plot area from the x and y axes
  float *min = x->GetPoint1();
  float *max = x->GetPoint2();
  if (fabs(max[0] - min[0]) == 0.0f)
    {
    return false;
    }
  float xScale = (x->GetMaximum() - x->GetMinimum()) / (max[0] - min[0]);

  // Now the y axis
  min = y->GetPoint1();
  max = y->GetPoint2();
  if (fabs(max[1] - min[1]) == 0.0f)
    {
    return false;
    }
  float yScale = (y->GetMaximum() - y->GetMinimum()) / (max[1] - min[1]);

  transform->Identity();
  transform->Translate(this->Point1[0], this->Point1[1]);
  // Get the scale for the plot area from the x and y axes
  transform->Scale(1.0 / xScale, 1.0 / yScale);
  transform->Translate(-x->GetMinimum(), -y->GetMinimum());
  return true;
}

//-----------------------------------------------------------------------------
void vtkChart::SetBottomBorder(int border)
{
  this->Point1[1] = border >= 0 ? border : 0;
  this->Point1[1] += static_cast<int>(this->Size.GetY());
}

//-----------------------------------------------------------------------------
void vtkChart::SetTopBorder(int border)
{
 this->Point2[1] = border >=0 ?
                   this->Geometry[1] - border :
                   this->Geometry[1];
 this->Point2[1] += static_cast<int>(this->Size.GetY());
}

//-----------------------------------------------------------------------------
void vtkChart::SetLeftBorder(int border)
{
  this->Point1[0] = border >= 0 ? border : 0;
  this->Point1[0] += static_cast<int>(this->Size.GetX());
}

//-----------------------------------------------------------------------------
void vtkChart::SetRightBorder(int border)
{
  this->Point2[0] = border >=0 ?
                    this->Geometry[0] - border :
                    this->Geometry[0];
  this->Point2[0] += static_cast<int>(this->Size.GetX());
}

//-----------------------------------------------------------------------------
void vtkChart::SetBorders(int left, int bottom, int right, int top)
{
  this->SetLeftBorder(left);
  this->SetRightBorder(right);
  this->SetTopBorder(top);
  this->SetBottomBorder(bottom);
}

void vtkChart::SetSize(const vtkRectf &rect)
{
  this->Size = rect;
  this->Geometry[0] = static_cast<int>(rect.GetWidth());
  this->Geometry[1] = static_cast<int>(rect.GetHeight());
}

vtkRectf vtkChart::GetSize()
{
  return this->Size;
}

void vtkChart::SetActionToButton(int action, int button)
{
  if (action < -1 || action >= MouseActions::MaxAction)
    {
    vtkErrorMacro("Error, invalid action value supplied: " << action)
    return;
    }
  this->Actions[action] = button;
  for (int i = 0; i < MouseActions::MaxAction; ++i)
    {
    if (this->Actions[i] == button && i != action)
      {
      this->Actions[i] = -1;
      }
    }
}

int vtkChart::GetActionToButton(int action)
{
  return this->Actions[action];
}

void vtkChart::SetClickActionToButton(int action, int button)
{
  if (action < vtkChart::SELECT || action > vtkChart::NOTIFY)
    {
    vtkErrorMacro("Error, invalid action value supplied: " << action)
    return;
    }
  this->Actions[action - 2] = button;
}

int vtkChart::GetClickActionToButton(int action)
{
  return this->Actions[action - 2];
}

//-----------------------------------------------------------------------------
void vtkChart::SetBackgroundBrush(vtkBrush *brush)
{
  if(brush == NULL)
    {
    // set to transparent white if brush is null
    this->BackgroundBrush->SetColorF(1, 1, 1, 0);
    }
  else
    {
    this->BackgroundBrush = brush;
    }

  this->Modified();
}

//-----------------------------------------------------------------------------
vtkBrush* vtkChart::GetBackgroundBrush()
{
  return this->BackgroundBrush.GetPointer();
}

//-----------------------------------------------------------------------------
void vtkChart::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  // Print out the chart's geometry if it has been set
  os << indent << "Point1: " << this->Point1[0] << "\t" << this->Point1[1]
     << endl;
  os << indent << "Point2: " << this->Point2[0] << "\t" << this->Point2[1]
     << endl;
  os << indent << "Width: " << this->Geometry[0] << endl
     << indent << "Height: " << this->Geometry[1] << endl;
  os << indent << "SelectionMode: " << this->SelectionMode << endl;
}
//-----------------------------------------------------------------------------
void vtkChart::AttachAxisRangeListener(vtkAxis* axis)
{
  axis->AddObserver(vtkChart::UpdateRange, this, &vtkChart::AxisRangeForwarderCallback);
}

//-----------------------------------------------------------------------------
void vtkChart::AxisRangeForwarderCallback(vtkObject*, unsigned long, void*)
{
  double fullAxisRange[8];
  for(int i=0; i < 4; i++)
    {
    this->GetAxis(i)->GetRange(&fullAxisRange[i*2]);
    }
  this->InvokeEvent(vtkChart::UpdateRange, fullAxisRange);
}

//-----------------------------------------------------------------------------
void vtkChart::SetSelectionMode(int selMode)
  {
  if (this->SelectionMode == selMode ||
    selMode < vtkContextScene::SELECTION_NONE ||
    selMode > vtkContextScene::SELECTION_TOGGLE)
    {
    return;
    }
  this->SelectionMode = selMode;
  this->Modified();
}
